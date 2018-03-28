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
 * The main workhorse for building file attachment XML snippet
 *
 * @param \FieldCollectionItemEntity $field_collection_item
 *   the FC entity passed to VBO action
 * @return array $buildable_snippet_array
 */
function _cals_importer_exporter_build_file_resource_snippet
($field_collection_item) {
  //$tags = _cals_importer_get_tag_array_file_attach(); //tag mappings


  //Wrapper
  $fc_wrapper = _cals_wrap_fc($field_collection_item);

  //Temporary access to stub if needed
  $node = $fc_wrapper->host_entity->value();
  $stubfields = unserialize($node->field_xml_stub[LANGUAGE_NONE][0]['value']);

  //Try _cals_importer_get_tag_array ?
  $per_file_tags = array(
    '264',
    '300',
    '306',
    'GMD',
    '008',
    '511',
  );

  $buildable_snippet_array = array();

  //GMD handles 336-338
  foreach ($per_file_tags as $tag) {
    $callable = '_cals_importer_exporter_snippet_callback_' .
      $tag;

    if ( function_exists($callable) )
      $result = call_user_func($callable, $fc_wrapper);

    //@todo log bad callbacks instead
    if ($result == FALSE) {
      drupal_set_message(t("Something went wrong in $callable. Please check its field definitions."), 'error');
      continue;
    }

    if ($tag == '008') {
      $buildable_snippet_array['controlfields'][$tag] = $result;
    }
    elseif ($tag != 'GMD') {
      $buildable_snippet_array['datafields'][$tag] = $result;

      //Filter out empty subfields via FALSE
      $buildable_snippet_array['datafields'][$tag]['subfields'] = _cals_recurse_filter_array
      ($buildable_snippet_array['datafields'][$tag]['subfields']);

      //If we lost all our subfields, remove the tag
      if ( empty($buildable_snippet_array['datafields'][$tag]['subfields']
      )) unset($buildable_snippet_array['datafields'][$tag]);
    }
    else { //GMD contains multiple
      foreach ($result as $tags => $value) {
        $buildable_snippet_array['datafields'][$tags] = $value;
      }
      //Bump up controlfields that got set deeper inside array based on GMD
      if (array_key_exists('controlfields', $buildable_snippet_array['datafields'])) {
        $ctrl = $buildable_snippet_array['datafields']['controlfields'];
        unset($buildable_snippet_array['datafields']['controlfields']);
        $buildable_snippet_array['controlfields'] = $ctrl;
      }
    }
    unset($result); //clear for next tag in case of bad return
  } //end per_file_tags foreach

  //Serialize and overwrite existing stored snippet
  $serialized_snippet = serialize($buildable_snippet_array);
  $fc_wrapper->field_marc_xml_snippet->set(
    $serialized_snippet);
  $fc_wrapper->save();

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
               $collection) {
        //must now use each field_collection found
        $collect_id = $collection->getIdentifier();
        $fc_entity = entity_load('field_collection_item', array($collect_id));
        $snippet = _cals_importer_exporter_build_file_resource_snippet
        ($fc_entity[$collect_id]);

        $record = new File_MARC_Record(NULL);
        _cals_importer_exporter_build_new_record($record, $snippet);
        $output[] = $record->toXML();
      }

      $filename = "public://" . $node->nid . "_file_attach_fields_test.xml";
      $data = '';

      foreach ($output as $file => $rec) {
        $data .= $rec;
      }
      $check = file_unmanaged_save_data($data, $filename);
    $uri = file_create_url($check);

    drupal_set_message(t("File saved: please download via <a href='@url'>@url</a>",
      array("@url" => url($uri) ) ) );

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
 * @param $node_wrapper
 * @return array
 */
function _cals_importer_exporter_snippet_callback_008($fc_wrapped) {

  define("MARC_SPACE", chr(32) );
  define("MARC_FILL", chr(124) );

  $final_008 = '';

  //Get a well-formed XML string from hostEntity
  $node = node_load($fc_wrapped->host_entity->getIdentifier());
  $xml = _cals_get_marc_xml_string($node);

  //Obtain original 008
  if (! empty($xml)) {
    $element_008 = $xml->xpath("//marc:controlfield[@tag='008']");
    if ($element_008) $original_008 = $element_008[0]->__toString();
  } else $original_008 = '';

  //Keep 00-05 if exists
  //yymmdd format

  $date_entered = substr($original_008, 0, 6);
  if (strlen($date_entered) == 6) $final_008 .= $date_entered;
  else $final_008 .= date('ymd'); //Generate

  //Set 06 to p
  $final_008[6] = 'p';

  //07-10 Date Created

  //default to timestamp of file creation
  $created= date("Y", $fc_wrapped->field_s3_file_upload->value()
  ['timestamp']);

  //Loop through all field_date instances and find an Created year
  foreach ($fc_wrapped->host_entity->field_date->value() as $instance =>
           $ent) {
    $date_wrapped = _cals_wrap_fc($ent);

    if ($date_wrapped->field_qualifier_date->value() == 'Created') {
      //Replace default if we found a Created qualifier
      $created = $date_wrapped->field_dc_date->value();
      break;
    }
  }

  $final_008 .= $created;

  //11-14 Date Issued

  //Default value
  $issued = 'uuuu';

  //Loop through all field_date instances and find an Issued year
  foreach ($fc_wrapped->host_entity->field_date->value() as $instance =>
           $ent) {
    $date_wrapped = _cals_wrap_fc($ent);

    if ($date_wrapped->field_qualifier_date->value() == 'Issued') {
      //Replace default if we found an Issued qualifier
      $issued = $date_wrapped->field_dc_date->value();
      break;
    }
  }

  $final_008 .= $issued;

  //15-17 three fill chars
  $final_008 .= 'xx' . MARC_SPACE;

  //Get format, audience, lang and branch
  $format = $fc_wrapped->field_file_format->label();
  $audience = $fc_wrapped->host_entity->field_audience->value();
  $lang = $fc_wrapped->host_entity->field_iso_language->value()[0];

  if ($format == 'MP3') {
    //18-21 nnnn
    foreach (range(18,21) as $key ) {
      $final_008[$key] = 'n';
    }

    //22 audience
    if (strlen($audience) == 1) $final_008[22] = $audience;
    else $final_008[22] = MARC_SPACE;

    $final_008[23] = 's';

    //24-29 six space chars
    foreach (range(24,29) as $key ) {
      $final_008[$key] = MARC_SPACE;
    }

    //30-31 literary format
    $label = strtolower($fc_wrapped->host_entity->field_literary_format
      ->label());
    $lit_forms = array(
      'drama' => 'd',
      'fiction' => 'f',
      'poetry' => 'p',
      'speeches' => 'l',
    );

    if ( $label ) $final_008[30] = $lit_forms[$label];
    else $final_008[30] = 'z';

    $final_008[31] = MARC_SPACE;
    $final_008[32] = MARC_FILL;
    $final_008[33] = 'n';

  } else { //Non-MP3

    //18-21 four fill chars
    foreach (range(18,21) as $key ) {
      $final_008[$key] = MARC_FILL;
    }

    //22 audience
    if (strlen($audience) == 1) $final_008[22] = $audience;
    else $final_008[22] = MARC_SPACE;

    $final_008[23] = 'o';

    //24-32 four fill chars
    foreach (range(24,32) as $key ) {
      $final_008[$key] = MARC_FILL;
    }

    //33 literary format
    $litform = $fc_wrapped->host_entity->field_literary_format->value();
    $final_008[33] = $litform ? $litform : MARC_FILL;
  }

  //34 one fill char
  $final_008[34] = MARC_FILL;

  //35-37 three chars for language of item
  $final_008 .= strlen($lang) == 3 ? $lang : 'eng'; //default eng

  $final_008[38] = MARC_FILL;

  //Cataloguing source
  $final_008[39] = 'd';

  return array( 'value' => $final_008);
}

function _cals_importer_exporter_snippet_callback_264($fc_wrapped) {
  //Machine name is confusing... format_publisher ?
  $pub_form = _cals_get_extant_drupal_fc_val('field_publisher_format',
    $fc_wrapped);
  $pub_place = _cals_get_extant_drupal_fc_val('field_place_of_publication',
    $fc_wrapped);
  $pub_date = _cals_get_extant_drupal_fc_val('field_date_created',
    $fc_wrapped);

  $workable_array = array(
    'ind1' => '',
    'ind2' => '1',
  );

  $workable_array['subfields'][] = array (
    'a' => $pub_place,
    'b' => $pub_form,
    //'c' => $pub_date,
  );

  //Was not available in field collection with Created qualifier
  if ($pub_date == FALSE) {
    foreach ($fc_wrapped->host_entity->field_date->value() as $instance =>
             $ent) {
      //Attempt to grab Issued or Copyrighted year
      $date_wrapped = _cals_wrap_fc($ent);
      $qualifier = $date_wrapped->field_qualifier_date->value();
      if (in_array($qualifier, array('Issued', 'Copyrighted'))) $years[] =
        $date_wrapped->field_dc_date->value();
    }

    //Take earliest year
    sort($years);
    $pub_date = $years[0];
  }
  //We got a value but it was a timestamp, extract year
  elseif ($pub_date) {
    $pub_date = date('Y', $pub_date);
    $workable_array['subfields'][0]['c'] =
      $pub_date;
  }

  return $workable_array;
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

  $workable_array['subfields'][] = array( 'a' => $format_size_stanza);

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
      0 => array(
        'a' => $runtime
      )
    )
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
        'a' => 'online resource',
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
        'a' => 'online resource',
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
  $workable_array['controlfields']['ldr'] = array(
    'value' => array(
        6 => _cals_importer_exporter_snippet_callback_LDR($format),
      ),
    'required' => 1
  );

  return $workable_array;
}

/**
 * Populate Narrator from Performer in Drupal.
 * Multiple values supported.
 * @param \EntityDrupalWrapper $fc_wrapped
 * @return array $workable_array
 */
function _cals_importer_exporter_snippet_callback_511($fc_wrapped) {
  $workable_array = array(
      'ind1' => 0,
      'ind2' => ''
  );

  $narrators = _cals_get_extant_drupal_fc_val('field_performer', $fc_wrapped);

  if ($narrators){
    $narrator = implode(', ' , $narrators);
    $narrators_string = "Narrator: ";
    $workable_array['subfields'][] = array('a' => $narrators_string .
      $narrator . '.');
  }
  //Narrator was not found, will be purged
  else $workable_array['subfields'][] = $narrators;

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
