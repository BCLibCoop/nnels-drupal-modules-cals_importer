<?php

/**
 * Simple wrapper so we can invoke from main repo node page for testing purposes
 *
 * @param $node
 *   the node passed to the parser
 */

function _cals_importer_exporter_build_marcxml_file_node($field_collection_item) {
  _cals_importer_exporter_build_marcxml_file($field_collection_item);
  //drupal_goto("node/{$field_collection_item->nid}/TBA");
}

/**
 * VBO to add populate the stub marc-xml string
 *
 * @param $entity
 *   the node passed to the parser
 */
function _cals_importer_exporter_build_marcxml_file_vbo(&$entity, $context = array()) {
  if (isset($entity->nid)) {
    $node = node_load($entity->nid);
    //safety check to make sure we're parsing repos items!!
    $content_types = array('repository_item');
    if (in_array($node->type, $content_types)  ) {
      -      _cals_importer_exporter_build_marcxml_file($node);
      +      _cals_importer_exporter_build_marcxml_file($entity);
    }
    else{
      drupal_set_message(t("Invalid selection: not a repos item"));
    }
  }//end main if

  return "";

}

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

    if (in_array($entity->type, $content_types)  ) {
      _cals_importer_exporter_build_marcxml_file($entity);
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
 * Dave's main workhorse
 *
 * @param $field_collection_item
 *   the node passed to the parser
 *
 */
function _cals_importer_exporter_build_marcxml_file($field_collection_item) {
  $tags = _cals_importer_get_tag_array(); //tag mappings
  $lint = new File_MARC_Lint();
  $wrapper = entity_metadata_wrapper('field_collection_item', $field_collection_item);
  //not sure if you need this?
  $node = $wrapper->host_entity->value();
  $xml_string = $node->field_xml_string[LANGUAGE_NONE][0]['value'];
  $xml_string = MARC_HEADER . $xml_string . '</marc:collection>';
  $stubfields = unserialize($node->field_xml_stub[LANGUAGE_NONE][0]['value']);
  printAndDie("HEY, we made it this far", $stubfields);
  /**
  JS' logic to merge in data structure from snippet fields
  e.g., loop thru each field attachment to create file?
   */
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
 * 00-05 (Date entered on file) = system generated;
 * this is the date MARC record was first created; format must be: ddmmyy
 *
 * 35-37 (Language of the item) = use 3-digit MARC Code for Languages;
 * usually will either be eng (for English) or fre (for French) but
 * could be any of the languages listed in Drupal under Language(s)
 *
 * 39 (Cataloging source) = either use the fill character ( | ) or the
 * code in field 040 subfield $a if it exists in the MarcXML.
 *
 * This code should be invoked at the file attachment level and will need to
 * distinguish between MP3 (value = 4) and all other formats recorded in the
 * field_file_format field.
 *
 * @see https://github.com/BCLibCoop/nnels/issues/131
 *
 * @param $node
 * @return bool|string
 */
function _cals_importer_export_callback_008($node, $node_wrapper, $xml) {

  $tmp = $xml->xpath("//marc:datafield[@tag='040']/marc:subfield[@code='a']");
  $source = '|';
  if (count($tmp) >= 1) {
    $source = (string) $tmp[0];
  }
  $val = $node->changed;
  $val = $node->created;
  //yyyymmddhhmmss.f
  //date + 3 blanks at position 14-17
  $val = date('dmy', $val) . '   ';

  //@todo - get audience value
  $audience = '';
  //@todo - get $field_file_format value
  if ($field_file_format == 4) {
    $val .= 'nnnn' . $audience . 'o';
    //008/24-29 = use a blank for each of 6 positions
    $val .= '      ';
    //008/30-31 = use the fill character ( | ) for 2 positions
    $val .= '||';
    //008/32 = use the fill character ( | )
    $val .= '|';
    //008/33 = n
  }
  else {
    $val .= '||||' . $audience . 'o';
    //008/24-27 = |||| (use fill character for all 4 positions)
    $val .= '||||';
    //008/28-32
    $val .= '||||';
    //008/33 = insert appropriate Literary Format code (from Literary Format
    // field in Drupal), if no code then use the fill character ( | ).
    // View codes:
    //http://www.itsmarc.com/crs/mergedprojects/helptop1/helptop1/variable_control_fields/idh_008_33_bk_bib.htm
    $val .= '|'; //using placeholder until we get this nailed down!

  }
  //008/34 = use the fill character ( | )
  $val .= '|';

  //008/35-37 (Language of the item) = use 3-digit MARC Code for Languages;
  $lgs = array();
  //$lgs = $node->field_iso_language[LANGUAGE_NONE];
  $lg = (!empty($node->field_iso_language)) ?
    $node->field_iso_language[LANGUAGE_NONE][0]['value']:
    'eng';
  $val .= $lg;
  //008/38 = use the fill character ( | );
  // 008/39 (Cataloging source) = use the fill character ( | )
  $val .= '||';
  return $val;
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
