<?php

module_load_include("inc", "cals_importer", "cals_importer.exporter_callbacks");

/**
 * Simple wrapper so we can invoke from main repo node page for testing purposes
 *
 * @param $node
 *   the node passed to the parser
 */

function _cals_importer_create_marc_stub_node($node) {
  require 'File/MARCXML.php';
  require 'File/MARC/Lint.php';
  _cals_importer_create_marc_stub($node);
  drupal_goto("node/{$node->nid}/view-xml-stub");

}

function _cals_importer_create_marcxml_snippet_file_resource($node) {

  require 'File/MARCXML.php';
  require 'File/MARC/Lint.php';

  //need field_collection->field_file_resource entity
  $node_wrapper = entity_metadata_wrapper('node', $node);

  //Calls for each format in field collections
  //Gets back a File_MARC_Record, converts to XML snippet for storage

  foreach ($node_wrapper->field_file_resource->getIterator() as $delta =>
           $field_collection) {
    //must now use each field_collection found
    $snippet = _cals_importer_exporter_add_gmd_per_format($field_collection);
    $new_xml_snippet = $snippet->toXML("UTF-8", FALSE, FALSE);
    $fc_wrapper = entity_metadata_wrapper('field_collection_item',
      $field_collection);
    $fc_wrapper->field_marc_xml_snippet->set( array(
      $new_xml_snippet) );
    //_cals_importer_update_admin_audit($node,  $action);
    $fc_wrapper->save();
  }
  $node_wrapper->save();

  //this should show all of them on one display
  //drupal_goto("node/{$field_collection->id}/view-xml-snippet");
}

/**
 * VBO to add populate the stub marc-xml string
 *
 * @param $entity
 *   the node passed to the parser
 */
function _cals_importer_create_marc_stub_vbo(&$entity, $context = array()) {
  require 'File/MARCXML.php';
  require 'File/MARC/Lint.php';
  if (isset($entity->nid)) {
    $node = node_load($entity->nid);
    //safety check to make sure we're parsing repos items!!
    $content_types = array('repository_item');
    if (in_array($node->type, $content_types)  ) {
      _cals_importer_create_marc_stub($node);
    }
    else{
      drupal_set_message(t("Invalid selection: not a repos item"));
    }
  }//end main if

  return "";

}


function _cals_importer_normalize_indicators($str) {
  $str = trim($str);
  if($str == '""' || $str = "''") return '';
  return $str;
}

/**
 * @param $entity
 * @return array|void
 */

function _cals_importer_exporter_rebuild_array_marc_tag($entity) {
  $me = array();
  $wrapper = $entity->wrapper();
  $field = 'id';
  $field = 'field_code';
  $code = $wrapper->{$field}->value();
  $field_code = _cals_importer_exporter_get_normalized_code(
    trim($wrapper->{$field}->value()));
  //if($field_code == 'ldr' || $code < 20) return;
  $tag_type = $wrapper->field_marc_tag_type->value();
  if($tag_type < 2) return;
  $me['field_code'] = $field_code;
  $me['tag'] = $field_code;
  $me['tag_type'] = $tag_type;
  $me['callback'] = '_cals_importer_exporter_callback_' . $field_code;
  $me['callback_exists'] = (function_exists($me['callback'])) ? TRUE : FALSE;

  $ind1 = ($wrapper->field_indc1->value() == "''") ? '' : $wrapper->field_indc1->value();
  $ind2 = ($wrapper->field_ind2->value() == "''") ? '' : $wrapper->field_ind2->value();

  $me['ind1'] = $ind1; // $wrapper->field_indc1->value();
  $me['ind2'] = $ind2; //$wrapper->field_ind2->value();

  if($tag_type == 3) {
    //$me['ind1'] = _cals_importer_normalize_indicators($wrapper->field_indc1->value());
    //$me['ind2'] = _cals_importer_normalize_indicators($wrapper->field_ind2->value());
  }

  //if($field_code == 246) printAndDie($me, $wrapper->field_indc1->value(), $wrapper->field_ind2->value());

  //printAndDie(__FUNCTION__,$wrapper->getPropertyInfo());
  $fields = array(
    //'id',
    'title',
    'field_include_in_export',
    'field_functions',
    'field_source_value',
    'field_subfield_code',
    'field_marc_tag_type',
  );

  foreach($fields as $field) {
    $info = $wrapper->{$field}->info();
    $abbrev = str_replace("field_", '', $field);

    switch($field) {

      case 'title':
        $me[$abbrev] = $wrapper->{$field}->value(array('sanitize' => TRUE));
        break;
      case 'field_description':
        $me[$abbrev] = $wrapper->{$field}->value(array('decode' => TRUE));
        break;

      default:
        $value = $wrapper->{$field}->value();
        if(is_string($value)) $me[$abbrev] = trim($value);
        break;
    }
  }
  if($tag_type == 3) {
    $subs = array();
    $me['subfields'] = _cals_importer_get_subfields_collection_fields($wrapper);
  }

  $me['description'] = trim($wrapper->field_description->value());
  return $me;

}
function _cals_importer_get_subfields_collection_fields($wrapper) {
  $subfields = array();
  foreach($wrapper->field_marc_subfield_collection as $fc_wrapper) {
    $info = $fc_wrapper->getPropertyInfo();
    $subs = array();
    $code = $fc_wrapper->field_subfield_code->value();
    foreach($fc_wrapper as $fieldname => $array) {
      if(substr($fieldname, 0, 6) == 'field_' && $fieldname != 'field_name') {
        $str = str_replace("field_", '', $fieldname);
        $fieldtype = trim($info[$fieldname]['type']) . "x"; //for some reason, can't check this.
        if($fieldtype == 'listx') printAndDie(__FUNCTION__,"WTF");
        if($str == 'drupal_fieldname') {
          $val = $fc_wrapper->{$fieldname}[0]->value();
        }
        else {
          $val = trim($fc_wrapper->{$fieldname}->value());
        }
        $subs[$str] = $val;

      }
    }

    $subfields[$code] = $subs;

  }
  return $subfields;
}

function _cals_importer_get_subfield_array() {
  //$rs = db_query("select * from {eck_marc_tag} ");
  $es = entity_load("marc_tag", FALSE);
  $me = array();
  foreach($es as $id => $e) {
    if(!empty($e->field_marc_tag_type) && $e->field_marc_tag_type[LANGUAGE_NONE][0]['value'] > 1) {
      $tagset = _cals_importer_exporter_rebuild_array_marc_tag($e);
      //if($me['field_code'] == 720) printAndDie(__FUNCTION__,$me);
      if(is_array($tagset)) {
        $key = $tagset['field_code'];
        $type = ($e->field_marc_tag_type[LANGUAGE_NONE][0]['value'] == 2) ? 'controlfields' : 'datafields';
        $me[$type][$key] = $tagset;
      }
    }
  }
  ksort($me);
  return $me;
}

/**
 * Updates the node with stub marc-xml record
 *
 * @param $node
 *   the node passed to the parser
 *
 */
function _cals_importer_create_marc_stub($node) {
  $tags = _cals_importer_get_subfield_array(); //tag mappings
  $lint = new File_MARC_Lint();
  $xml_string = $node->field_xml_string[LANGUAGE_NONE][0]['value'];
  $xml_string = MARC_HEADER . $xml_string . '</marc:collection>';

  $records = new File_MARCXML($xml_string, File_MARC::SOURCE_STRING, "http://www.loc.gov/MARC21/slim");
  //$xml = _cals_get_marc_xml_string($node);
  
  $n = 0;
  while ($record = $records->next()) {

    //$xml = simplexml_load_string($xml_string);
    _cals_importer_exporter_marcxml_remove_tags($record, $tags);
    //_cals_importer_exporter_callback_245($node, $record);
    //_cals_importer_exporter_add_taxonomy_terms_marcxml($node, $record);

    $marcfields = _cals_importer_exporter_get_marc_fields($node, $record, $tags);
    $node->field_xml_stub[LANGUAGE_NONE][0]['value'] = serialize($marcfields);

    //with new datafields, generate the new record
    _cals_importer_exporter_build_new_record($record, $marcfields);
    $valid = $lint->checkRecord($record);
    if(count($valid)) {
      $vars = array("items" => $valid , "title" => "XML Stub Created with following errors", "type" => "ol", "attributes" => array());
      drupal_set_message(theme_item_list($vars));
    }
    else {
      drupal_set_message(t("XML Stub Created"));
    }

    $new_xml_stub = $record->toXML("UTF-8", FALSE, FALSE);
    //$new_xml_stub = $record->toRaw();

    //$node->field_xml_stub[LANGUAGE_NONE][0]['value'] = $new_xml_stub;
    //_cals_importer_update_admin_audit($node,  $action);
    node_save($node);
  }


}


function _cals_importer_exporter_field_value($node, $fieldname) {
  $values = field_view_field('node', $node, $fieldname, array('default'));
  return $values;
}


function normalize_title(&$value) {
  $last = substr($value, -1);
  if($last != '.') $value .= '.';
}

/**
 * @param $code
 * @return string
 */
function _cals_importer_exporter_get_normalized_code($code) {
  $code = trim($code);

  if(is_numeric($code)) {
    if($code > 10 && $code < 100 ) return "0" . $code;
    if($code < 10) return "00" . $code;
    return $code;
  }
  return $code;
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
   * =338 \\$a online resource $2 rdacarrier
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
 * Remove tags from record
 * @param $record
 */
function _cals_importer_exporter_marcxml_remove_tags(&$record, $tags) {
  $tags = array();
  //control tags to delete
  $arr = array('001', '005', '008');

  foreach ($record->getFields() as $tag=>$value) {
    if ($value instanceof File_MARC_Data_Field || in_array($tag, $arr) ) {
      $tags[] = $tag;
    }
    else {
      if($tag == 0051111) $tags[] = $tag;
    }
  }
  foreach($tags as $tag) {
    $n = $record->deleteFields($tag);
  }
}
/**
 * Parses the original marc file
 * @param $record
 * @param $tags
 * @param $xml
 */
function _cals_importer_exporter_parse_original_marc($tagset, $code, $record, $xml) {
  $ind1 = $tagset['ind1'];
  $ind2 = $tagset['ind2'];
  $tag = $tagset['tag'];
  $subdata = $tagset['subfields'][$code];
  $values =  _cals_importer_get_xpath_values($tag, NULL, NULL, $code, $xml);
  if(empty($values)) return;
  return $values;
}

/**
 * Placeholder function
 * @param $record
 */
function _cals_importer_exporter_get_marc_fields($node, &$record, $tags) {
  $node_wrapper = entity_metadata_wrapper('node', $node);
  $fields = array();
  $xml = _cals_get_marc_xml_string($node);
  //_cals_importer_exporter_parse_controlfields or
  // _cals_importer_exporter_parse_datafields
  foreach ($tags as $type => $tagset) {
    $function = '_cals_importer_exporter_parse_' . $type;
    $tmp = $function($node, $node_wrapper, $record, $tagset);
    if(!empty($tmp))
      $fields = $fields + $tmp;
  }
  ksort($fields);
  return $fields;
}

function _cals_importer_exporter_parse_controlfields($node, $node_wrapper, &$record, $tags) {
  foreach($tags as $tag => $arr) {

    switch($tag) {
      case '001':
        if(!empty($node->field_system_control_number))
          $val = $node->field_system_control_number[LANGUAGE_NONE][0]['value'];
        else
          $val = $node->field_isbn[LANGUAGE_NONE][0]['value'];
        break;

      case '003':
        $val = 'BVNNELS';
        break;

      case '005':
        $val = $node->changed;
        //yyyymmddhhmmss.f
        $val = date('YmdHis.0', $val);
        break;
      case '008':

        $val = _cals_importer_export_callback_008($node);

        break;

    }
    $tmp['controlfields'][$tag] = array(
      'value' => $val,
      'required' => $arr['include_in_export']
    );


  }
  return $tmp;

}

function _cals_importer_exporter_parse_datafields($node, $node_wrapper, &$record, $tags) {
  $xml = _cals_get_marc_xml_string($node);
  $arr = array();
  $tmp = array();
  foreach($tags as $tag => $tagset) {
    $callback_exists = $tagset['callback_exists'];
    if($tag !== 511111111) {
      $subs = $tagset['subfields'];
      //Treat parent tag as a unit if callback exists.
      if ($callback_exists) {
        $vars = array(
          'node' => $node,
          'tag' => $tag,
          'node_wrapper' => $node_wrapper
        );
        $callback = $tagset['callback'];
        $tmp = $callback($vars);
      }
      else {
        //parse subfields
        $tmp = _cals_importer_exporter_parse_subfields($tagset, $node_wrapper, $record, $xml);
        if (is_array($tmp) && !empty($tmp[$tag]['subfields'])) {
          $ind1 = $tagset['ind1'];
          $ind2 = $tagset['ind2'];
          if ($ind1 == 'use MarcXML indicators') {
            $ind1 = _cals_importer_get_marc_indicators($xml, $tag);
          }
          if ($ind2 == 'use MarcXML indicators') {
            $ind2 = _cals_importer_get_marc_indicators($xml, $tag);
          }
          $tmp[$tag]['ind1'] = $ind1;
          $tmp[$tag]['ind2'] = $ind2;
        }
      }

    }
    //include the array even if empty so we can track invalid records
    if(!empty($tmp[$tag]['subfields']) || (isset($tmp[$tag]['required']) && $tmp[$tag]['required'] == 1)) {
      $arr['datafields'][$tag] = $tmp[$tag];
      $arr['datafields'][$tag]['required'] = $tagset['include_in_export'];
    }
  }
  return $arr;
}

/**
 * @param $tagset
 * @param $wrapper
 * @param $record
 * @param $xml
 * @return array
 */
function _cals_importer_exporter_parse_subfields($tagset, $wrapper, $record, $xml) {
  $tag = $tagset['tag'];
  $datafields = $tagset['subfields'];
  $subfields = array();
  foreach($datafields as $code => $fields) {
    $code = trim($fields['subcode']);
    $cb = "_cals_importer_exporter_callback_" . $tag . "_" . $code;
    //check to see if there is a callback for specific code
    if (function_exists($cb)) {
      //still working on these!!
      $subfields[$code] = array("YO PLACEHOLDER");
    }
    //or do we extract the value directly from Drupal?
    elseif (!empty($fields['drupal_fieldname'])) {
      $tmp = $fields['drupal_fieldname'];
      if (isset($wrapper->{$tmp})) {
        $values = $wrapper->{$tmp}->value();
        $subfields[$code] = $values;
      }
      else {
        drupal_set_message( t("Field not found: please check 'marc-tags-entity-view'
        to ensure there's no mismatch on fieldname {$tmp} entered for tag = $tag
        and subcode = $code") );
      }
    }
    //Otherwise, reparse the original marc record string
    else {
      $tmp = _cals_importer_exporter_parse_original_marc($tagset, $code, $record, $xml);
      if (!empty($tmp)) {
        $subfields[$code] = $tmp;
      }
    }
  }
  $me[$tag]['subfields'] = $subfields;
  return $me;
}

/**
 * Returns a new record based on datafields
 *
 * @param $record
 * @param $datafields
 */
function _cals_importer_exporter_build_new_record(&$record, $marcfields) {
  foreach ($marcfields as $type => $arr) {
    $fields = $marcfields[$type];
    $function = '_cals_importer_exporter_iterate_' . $type;
    $function($record, $fields);
  }
}

/**
 * Iterate thru array of controlfields to insert xml controlfields
 * @param $record
 * @param $controlfields
 */
function _cals_importer_exporter_iterate_controlfields(&$record, $controlfields) {
  foreach($controlfields as $tag => $arr) {
    $data = $arr['value'];
    $field = new File_MARC_Control_Field($tag, $data, null, null);
    if($arr['required'] == 1 && empty($data))
      drupal_set_message("Missing subfields for tag $code. Invalid XML Record Stub.");
    _cals_importer_exporter_insert_field($record, $field, $code);
  }
}
/**
 * Iterate thru array of datafields to insert xml datafields
 * @param $record
 * @param $datafields
 */
function _cals_importer_exporter_iterate_datafields(&$record, $datafields) {
  foreach($datafields as $code => $arr) {

    $subfields = $arr['subfields'];
    if(empty($subfields)) {
      if($arr['required'] == 1)
        //throw error msg
        drupal_set_message("Missing subfields for tag $code. Invalid XML Record Stub.");
    }
    else {
      $marc_subfields = _cals_importer_exporter_get_marc_subfield($subfields);
      $field = new File_MARC_Data_Field($code, $marc_subfields, 0, null);
      $field->setIndicator(1, "{$arr['ind1']}");
      $field->setIndicator(2, "{$arr['ind2']}");
      _cals_importer_exporter_insert_field($record, $field, $code);
    }
  }
}

/**
 * Get MARC formatted subfields (see FILE_MARC)
 * @param $arr
 * @return array
 */
function _cals_importer_exporter_get_marc_subfield($arr) {
  $subfields = array();
  foreach ($arr as $code => $values) {
    foreach($values as $value) {
      $subfields[] = new File_MARC_Subfield($code, $value);
    }
  }
  return $subfields;
}

/**
 * Add field to record structure
 * @param $record
 * @param $field
 * @param $id
 *
 */
function _cals_importer_exporter_insert_field(&$record, $field, $id) {
  //e.g., id = 650
  //_cals_importer_exporter_get_datafields($record);
  $record->appendField($field);
}