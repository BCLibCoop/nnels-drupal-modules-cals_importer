<?php

/**
 * Simple wrapper so we can invoke from main repo node page for testing purposes
 *
 * @param $node
 *   the node passed to the parser
 */

//function _cals_importer_exporter_build_marcxml_file_node($node) {
//  _cals_importer_exporter_build_marcxml_file_action($node);
//  drupal_goto("node/{$node->nid}/TBA");
//}

/**
 * Action to add populate the stub marc-xml string
 *
 * @param $node
 *   the node passed to the parser
 */
function _cals_importer_exporter_build_marcxml_file_action(&$entity,
                                                             $context =
array()) {
    $host_node = $entity->hostEntity(); //Repository item
    $export_set = node_load($context['fields']['container_nid']);

    //safety check to make sure we're parsing repos items!!
    $content_types = array('repository_item');
    if (in_array($host_node->type, $content_types)) {
      $record_collection =
        _cals_importer_exporter_generate_record_export_collection
      ($entity, $host_node);

      //Placeholder filename
      $date = new DateTime();
      $filename = $date->getTimestamp() . '_test_marcxml_export.xml';

      //Append raw collection to file temporarily
      file_put_contents($filename, $record_collection, FILE_APPEND |
        LOCK_EX);
    }
    else {
      drupal_set_message(t("Invalid selection: not a repository item"));
    }
  }

  if ($context['progress']['current'] == $context['progress']['total']) {
    $uri = file_create_url($filename);
    //Wrap in marc:collection tag upon retrieval
    $content = MARC_HEADER . "\n" . file_get_contents($filename) .
      "</marc:collection>";

    $xml = simplexml_load_string($content);
    $file = file_save_data($xml->asXml(), $filename, FILE_EXISTS_REPLACE);
    $file->display = 1;
    $export_set->field_marc_xml_output_file[LANGUAGE_NONE][0] = (array)$file;
    node_save($export_set);
    drupal_set_message(t("File saved: please download via <a href='@url'>@url</a>",
          array("@url" => url("node/" . $export_set->nid) ) ) );
  }
}

/**
 * The main workhorse
 *
 * @param $entity
 *   the entity passed to the parser
 * @return $record_collection
 *
 *
 */
function _cals_importer_exporter_generate_record_export_collection($entity,
                                                                   $host_node) {
  $tags = _cals_importer_get_tag_array(); //tag mappings
  $lint = new File_MARC_Lint();

  //Proceed only with a stub existing
  $stubfields = unserialize($host_node->field_xml_stub[LANGUAGE_NONE][0]['value']);
  if ( ! empty($stubfields) && ! empty($host_node->field_nnels_856_tag) ){

    $fc_wrapper = _cals_wrap_fc($entity);

    //Only one per file format
    if ( ! $fc_wrapper->field_marc_xml_snippet[0]->value() ) {
      //Ensure there are snippets waiting
      _cals_importer_create_marcxml_snippet_file_resource($host_node);
    }

    //TODO: This needs to be decoupled from sort and return array
    //Generates one full array record per file entity
    $full_array_record = _cals_get_set_xml_snippet($fc_wrapper,
        $stubfields);

    //TODO: use Dave's sort

  } else {
    //stub didnt exist or didnt have 856 tag - should we ask to generate them
    // or just throw back message?
    return;
  }

  return $full_array_record;
}


/**
 * Calls for each format in field collections.
 * Gets back a File_MARC_Record, converts to XML snippet for storage
 *
 * @param stdClass $node
 *
 */
function _cals_importer_create_marcxml_snippet_file_resource($node) {
  $node_wrapper = entity_metadata_wrapper('node', $node);
  foreach ($node_wrapper->field_file_resource->getIterator() as $delta =>
           $field_collection) {
    //must now use each field_collection found
    $snippet = _cals_importer_exporter_add_gmd_per_format($field_collection);

    //TODO: Pass off to 511 function

    $new_xml_snippet = $snippet->toXML("UTF-8", FALSE, FALSE);
    $dom = new DOMDocument();
    $dom->loadXML($new_xml_snippet);
    $xml_out = $dom->saveXML($dom->documentElement);
    //need to remove prologue here
    $fc_wrapper = entity_metadata_wrapper('field_collection_item',
      $field_collection);
    $fc_wrapper->field_marc_xml_snippet->set( array(
      $new_xml_snippet) );
    $fc_wrapper->save();
  }
  $node_wrapper->save();

  //this should show all of them on one display
  //drupal_goto("node/{$field_collection->id}/view-xml-snippet");
}

/**
 * Given a wrapped FieldCollectionItemEntity, append each tag-value pair
 * from its snippet to its stub, pass through Data_Field sorting
 * function and return an XML string for writing to export file.
 *
 * @param \EntityMetadataWrapper $fc_wrapper
 * @param string $str
 * @return string $records
 */

function _cals_get_set_xml_snippet($fc_wrapper, $str) {

  require_once 'File/MARCXML.php';
  require_once 'File/MARC/Lint.php';

  $records = '';
  $stub_record = new File_MARCXML($str, File_MARC::SOURCE_STRING);
  $record = $stub_record->next(); //File_MARC_Record instance
  $this_snippet = $fc_wrapper->field_marc_xml_snippet[0]->value();

  $snippets = new File_MARCXML($this_snippet, File_MARC::SOURCE_STRING);

  if ($record) {
    //there's only one record snippet per file resource so just advance once
    $snippet_context = $snippets->next();

    foreach ($snippet_context->getFields() as $tag => $value) {
      $record->appendField($value);
    }

    //Adjust byte 6 of leader to reflect added format since ingest
    $ldr_arr = str_split($record->getLeader());
    $format = $fc_wrapper->field_file_format->value();

    switch($format) {
      case 4:
        //Nonmusical Sound Recording (i) for MP3
        $ldr_arr[6] = 'i';
        break;
      default:
        //Language Material (a) for all other formats
        $ldr_arr[6] = 'a';
        break;
    }

    //Restore string and set
    $ldr_arr = implode("", $ldr_arr);
    $record->setLeader($ldr_arr);

    //Stub + new fields to sorter, returns XML string
    //TODO: Sort later
    //$records .= _cals_importer_exporter_sort_record_datafields($record) .
    // "\n";
  }

  return $records;
}

/**
 * Given a collection of formats and corresponding Data Field pattern, create
 * the Data Fields per format and append to the record.
 *
 * @param $node
 * @param $record
 * @return int|void
 */

function _cals_importer_exporter_add_gmd_per_format($field_collection) {

  $format_keys = array(
    'epub 2' => 'etext',
    'epub 3' => 'etext',
    'pdf' => 'etext',
    'e-text' => 'etext',
    'azw3' => 'etext',
    'mp3' => 'mp3',
    'daisy 202' => 'daisy',
    'daisy 3' => 'daisy',
  );

  /**
   * Electronic text (EPUB, PDF, e-text)
   *  =336 \\$a txt $2 rdacontent
   *  =337 \\$a computer $2 rdamedia
   *  =338 \\$a online resource $2 rdacarrier
   **/

  $etext_pattern = array(
    0 => array(
      '336' => array (
        'a' => 'txt',
        2 => 'rdacontent',
      )
    ),
    1 => array(
      '337' => array(
        'a' => 'computer',
        2 => 'rdamedia',
      )
    ),
    2 => array(
      '338' => array(
        'a' => 'online resource',
        2 => 'rdacarrier',
      )
    ),
  );

  /**
   * MP3 books
   * =336  \\$a spoken word $b spw $2 rdacontent
   * =337  \\$a audio $b s $2 rdamedia
   * =337  \\$a computer $b c $2 rdamedia
   * =338  \\$a other $b sz $2 rdacarrier
   * =338  \\$a online resource $b cr $2 rdacarrier
   **/

  $mp3_pattern = array(
    0 => array(
      '336' => array (
        'a' => 'spoken word',
        'b' => 'spw',
        2 => 'rdacontent',
      )
    ),
    1 => array(
      '337' => array(
        'a' => 'audio',
        'b' => 's',
        2 => 'rdamedia',
      )
    ),
    2 => array(
      '337' => array(
        'a' => 'computer',
        'b' => 'c',
        2 => 'rdamedia',
      )
    ),
    3 => array(
      '338' => array(
        'a' => 'other',
        'b' => 'sz',
        2 => 'rdacarrier',
      )
    ),
    4 => array(
      '338' => array(
        'a' => 'online-resource',
        'b' => 'cr',
        2 => 'rdacarrier',
      ),
    ),
  );

  /**
   * DAISY books
   * =336  \\$a spoken word $b spw $2 rdacontent
   * =336  \\$a txt $2 rdacontent
   * =337  \\$a audio $b s $2 rdamedia
   * =337  \\$a computer $b c $2 rdamedia
   * =338  \\$a other $b sz $2 rdacarrier
   * =338  \\$a online resource $b cr $2 rdacarrier
   **/

  $daisy_pattern = array(
    0 => array(
      '336' => array (
        'a' => 'spoken word',
        'b' => 'spw',
        2 => 'rdacontent',
      )
    ),
    1 => array(
      '336' => array(
        'a' => 'txt',
        2 => 'rdacontent',
      )
    ),
    2 => array(
      '337' => array(
        'a' => 'audio',
        'b' => 's',
        2 => 'rdamedia',
      )
    ),
    3 => array(
      '337' => array(
        'a' => 'computer',
        'b' => 'c',
        2 => 'rdamedia',
      )
    ),
    4 => array(
      '338' => array(
        'a' => 'other',
        'b' => 'sz',
        2 => 'rdacarrier',
      ),
    ),
    5 => array(
      '338' => array(
        'a' => 'online-resource',
        'b' => 'cr',
        2 => 'rdacarrier',
      )
    ),
  );

  $format = strtolower($field_collection->field_file_format->label());

  $pattern = (string) $format_keys[$format] . '_pattern';
  $loaded_pattern = $$pattern;

  if (empty($loaded_pattern)) {
    drupal_set_message($loaded_pattern, "No pattern found: ");
    return;
  }

  $subfields = array();
  $snippet = new File_MARC_Record();

  foreach ($loaded_pattern as $instance => $tags) { //0 => array of tags
    foreach ($tags as $tag => $codes) { //336 => array of codes
      foreach ($codes as $key => $value) { //code => entry
        if (in_array($key, array('a', 'b', 1, 2, 3))) {
          $subfields[] = new File_MARC_Subfield($key, $value);
          unset($key);
        }
      }
      //Roll up all subfields into a DataField array for each tag
      $field = new File_MARC_Data_Field($tag, $subfields, 0, NULL);
      //_cals_importer_exporter_insert_field($record, $field, $tag);
      $snippet->appendField($field);
      unset($subfields);
      unset($field);
    }
  }
  return $snippet;
}

/**
 * @param File_MARC_Record $record
 */
function _cals_importer_exporter_sort_record_datafields($stub) {

  $sortlist = new File_MARC_List();
  $fresh_list = new File_MARC_List();
  $unsortedlist = $stub->getFields();
  $leader = $stub->getLeader();
  $warnings = $stub->getWarnings();

  /*
   * Note: For unknown reasons, possibly related to the override of key() in
   * File_MARC_List class, the next()
   * method inherited from the parent class
   * sets current() to null. So while we are calling SplDoublyLinkedList
   * methods, they aren't iterating at all. $index value and checking
   * the offset before doing any adding or comparing is the hack to implement
   * Insert Sort algorithm.
  */

  foreach ($unsortedlist as $item ) {

    $cloned_item = clone $item; //clone field objects or else the
    // deleteFields on the record will wipe them out by reference.

    //Dealing with an empty list, append item and manually advance index
    if ($sortlist->isEmpty() ) {
      $sortlist->appendNode($cloned_item);
    } else { //Non-empty list
      $sortlist->rewind();
      $index = 0;

      $unsorted_tag = $item->getTag();
      $check_sort_tag = $sortlist->offsetGet($index)->getTag();//latest
      settype($unsorted_tag, "integer");
      settype($check_sort_tag, "integer");

      $valid = $sortlist->valid(); //checking valid status
      while($sortlist->valid() && $check_sort_tag <= $unsorted_tag
      ) {
        $sortlist->next();
        $index++;
        //If this position exists, set its tag for next sorting
        if ($sortlist->offsetExists($index) ) {
          $check_sort_tag = $sortlist->offsetGet($index)
            ->getTag(); //get the new
          // item offset tag and re-run the while check.
          settype($check_sort_tag, "integer");
        }
      }
      $sortlist->add($index, $cloned_item);
    }
    //next $item
  }

  //Need a new object because __destruct() sets data to NULL and we cannot
  // build a new one with an existing File_MARC_List object
  $sorted_record = new File_MARC_Record();

  //Append the sorted fields from the linked list
  foreach ($sortlist as $tag => $field) {
    $sorted_record->appendField($field);
  }

  $sorted_record->setLeader($leader);

  //Single record XML string outside of collection, namespace
  $sorted_xml = simplexml_load_string(
    $sorted_record
      ->toXML())
    ->children(NAME_SPACE)
    ->asXML();

  return $sorted_xml;
}
