<?php

/**
 * Simple wrapper so we can invoke from main repo node page for testing purposes
 *
 * @param $node
 *   the node passed to the parser
 */

function _cals_importer_exporter_build_marcxml_file_node($field_collection_item) {
  _cals_importer_exporter_build_file_resource_snippet($field_collection_item);
  drupal_goto("field-collection/field-file-resource/{$field_collection_item->item_id}/");
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
      _cals_importer_exporter_build_marcxml_file($entity);
    }
    else{
      drupal_set_message(t("Invalid selection: not a repos item"));
    }
  }//end main if

  return "";

}

/**
 * The main workhorse
 *
 * @param \FieldCollectionItemEntity $field_collection_item
 *   the FC entity passed to VBO action
 * @return array $buildable_snippet_array
 */
function _cals_importer_exporter_build_file_resource_snippet
($field_collection_item) {
  //$tags = _cals_importer_get_tag_array(); //tag mappings
  //$lint = new File_MARC_Lint();

  //Wrapper
  $fc_wrapper = _cals_wrap_fc($field_collection_item);

  //Temporary access to stub if needed
  $node = $fc_wrapper->host_entity->value();
  $stubfields = unserialize($node->field_xml_stub[LANGUAGE_NONE][0]['value']);

  //Try _cals_importer_get_tag_array ?
  $per_file_tags = array(
    '008',
    '264',
    '300',
    '306',
    'GMD',
    '511',
  );

  $buildable_snippet_array = array();

  //GMD handles 336-338
  foreach ($per_file_tags as $tag) {
    $callable = '_cals_importer_exporter_snippet_callback_' .
      $tag;

    if ( function_exists($callable) )
      $result = call_user_func($callable,
        $fc_wrapper);

      if ($result == FALSE) {
        drupal_set_message(t("Something went wrong in $callable. Please check its field definitions."), 'error');
        continue;
      }
        //store callbacks in callbacks include eventually
        if ($tag != 'GMD') {
          $buildable_snippet_array['datafields'][$tag] = $result;
        }
        else { //GMD contains multiple
          foreach ($result as $tags => $value) {
            $buildable_snippet_array['datafields'][$tags] = $value;
          }
        }
        unset($result);
  }

  //Serialize and overwrite existing stored snippet
  $serialized_snippet = serialize($buildable_snippet_array);
  $fc_wrapper->field_marc_xml_snippet->set(
    $serialized_snippet);
  $fc_wrapper->save();

  $check_result = $stubfields + $buildable_snippet_array;
  dpm($check_result);
  dpm($buildable_snippet_array);


  //Pass this off to Stub + Snippet record builder
  return $buildable_snippet_array;
}



/**
 * DEPRECATED
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
 * @param $format
 * @return string $byte
 */
function _cals_importer_exporter_snippet_callback_LDR($format) {
  switch($format) {
    case 4:
      //Nonmusical Sound Recording (i) for MP3
      return $byte = 'i';
    default:
      //Language Material (a) for all other formats
      return $byte = 'a';
  }
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

function _cals_importer_exporter_snippet_callback_264($fc_wrapped) {
  //Use new Publisher fields from forthcoming VBO
}

/**
 * @param \EntityDrupalWrapper $fc_wrapped
 * @return array $workable_array
 */
function _cals_importer_exporter_snippet_callback_300($fc_wrapped) {

  $key = $fc_wrapped->field_file_format->value();
  $field = field_info_field('field_file_format');
  $format_label = $field['settings']['allowed_values'][$key];

  $size = format_size($fc_wrapped->field_s3_file_upload->file->size->value());

  $workable_array = array(
      'ind1' => '',
      'ind2' => '',
  );

  $format_size_stanza = sprintf("1 %s file (%s)", $format_label,
    $size );

  $workable_array['subfields'] = array( 'a' => $format_size_stanza);

  return $workable_array;
}

/**
 * @param \EntityDrupalWrapper $fc_wrapped
 * @return array $workable_array
 */
function _cals_importer_exporter_snippet_callback_306($fc_wrapped) {
  $runtime = $fc_wrapped->field_running_time->value();
  $snippet_306 = array(
    'ind1' => '',
    'ind2' => '',
    'subfields' => array(
      'a' => $runtime
    ),
  );

  return $snippet_306;
}

/**
 * Given a collection of formats and corresponding Data Field pattern, create
 * the Data Fields per format and append to a workable array.
 *
 * @param \EntityDrupalWrapper $fc_wrapped
 * @return mixed $workable_array
 */
function _cals_importer_exporter_snippet_callback_GMD($fc_wrapped) {

  $format_keys = array(
    'epub 2' => 'etext',
    'epub 3' => 'etext',
    'pdf' => 'etext',
    'e-text' => 'etext',
    'azw3' => 'etext',
    'mp3' => 'mp3',
    'daisy 202' => 'daisy',
    'daisy 3' => 'daisy',
    'braille' => 'etext'
  );

  /**
   * Electronic text (EPUB, PDF, e-text)
   *  =336 \\$a txt $2 rdacontent
   *  =337 \\$a computer $2 rdamedia
   *  =338 \\$a online resource $2 rdacarrier
   **/
  $etext_pattern = array(
    336 => array(
      0 => array(
        'a' => 'text',
        'b' => 'txt',
        2 => 'rdacontent',
      )
    ),
    337 => array(
      0 => array(
        'a' => 'computer',
        'b' => 'c',
        2 => 'rdamedia',
      )
    ),
    338 => array(
      0 => array(
        'a' => 'online resource',
        'b' => 'cr',
        2 => 'rdacarrier',
        )
      )
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
    336 => array (
      0 => array(
        'a' => 'spoken word',
        'b' => 'spw',
        2 => 'rdacontent',
      ),
    ),
    337 => array(
      0 => array(
        'a' => 'audio',
        'b' => 's',
        2 => 'rdamedia',
      ),
      1 => array(
        'a' => 'computer',
        'b' => 'c',
        2 => 'rdamedia',
      ),
    ),
    338 => array(
      0 => array(
        'a' => 'other',
        'b' => 'sz',
        2 => 'rdacarrier',
      ),
      1 => array(
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
    336 => array (
      0 => array(
        'a' => 'spoken word',
        'b' => 'spw',
        2 => 'rdacontent',
      ),
      1 => array(
        'a' => 'text',
        'b' => 'txt',
        2 => 'rdacontent',
      ),
    ),
    337 => array(
      0 => array(
        'a' => 'audio',
        'b' => 's',
        2 => 'rdamedia',
      ),
      1 => array(
        'a' => 'computer',
        'b' => 'c',
        2 => 'rdamedia',
      ),
    ),
    338 => array(
      0 => array(
        'a' => 'other',
        'b' => 'sz',
        2 => 'rdacarrier',
      ),
      1 => array(
        'a' => 'online-resource',
        'b' => 'cr',
        2 => 'rdacarrier',
      ),
    ),
  );

  $format = $fc_wrapped->field_file_format->value();
  $format_label = strtolower($fc_wrapped->field_file_format->label());

  $pattern = (string) $format_keys[$format_label] . '_pattern';
  $loaded_pattern = $$pattern;

  if (empty($loaded_pattern)) {
    drupal_set_message($loaded_pattern, "No pattern found: ");
    return FALSE;
  }

  $workable_array = array();

  foreach ($loaded_pattern as $tag => $instance) {
      $workable_array[$tag]['ind1'] = '';
      $workable_array[$tag]['ind2'] = '';
      $workable_array[$tag]['subfields'] = $instance;
  }

  //Call small helper function for LDR byte 6
  $workable_array['controlfields']['ldr']['06'] =
    _cals_importer_exporter_snippet_callback_LDR
  ($format);

  return $workable_array;
}

/**
 * @param \EntityDrupalWrapper $fc_wrapped
 * @return array $workable_array
 */
function _cals_importer_exporter_snippet_callback_511($fc_wrapped) {
  $workable_array = array(
    '511' => array(
      'ind1' => 0,
      'ind2' => ''
    )
  );

  $narrator = $fc_wrapped->field_performer[0]->value();

  $workable_array[511]['subfields'] = array( 'a' => "Narrator: $narrator");

  return $workable_array;
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
