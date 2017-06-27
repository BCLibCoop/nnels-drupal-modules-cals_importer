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

function rebuild_marc_tag($entity) {
  $me = array();
  $wrapper = $entity->wrapper();
  $field = 'id';
  $field = 'field_code';
  $code = $wrapper->{$field}->value();
  $field_code = _cals_importer_exporter_get_normalized_code(
    trim($wrapper->{$field}->value()));
  if($field_code == 'ldr' || $code < 20) return;
  $me['field_code'] = $field_code;
  $me['callback'] = '_cals_importer_exporter_callback_' . $field_code;
  $me['callback_exists'] = (function_exists($me['callback'])) ? TRUE : FALSE;
  $fields = array(
    //'id',
    'title',
    'field_include_in_export',

    'field_functions',
    'field_indc1',
    'field_ind2',
    'field_source_value',
    'field_subfield_code',
  );

  foreach($fields as $field) {
    $info = $wrapper->{$field}->info();

    switch($field) {

      case 'title':
        $me[$field] = $wrapper->{$field}->value(array('sanitize' => TRUE));
        break;
      case 'field_description':
        $me[$field] = $wrapper->{$field}->value(array('decode' => TRUE));
        break;

      default:
        $value = $wrapper->{$field}->value();
        if(is_string($value)) $me[$field] = trim($value);
        break;

    }
  }
  $subs = array();
  $keys = array(
    'field_subfield_code',
    //'field_indc1',
    //'field_ind2',
    'field_include_in_export',
    'field_source_value',
    'field_description',
  );

  foreach($wrapper->field_marc_subfield_collection as $fc_wrapper) {
    //$info = $fc_wrapper->getPropertyInfo();
    //$keys = array_keys($info);
    foreach($keys as $field) {
      $subs[$field] = trim($fc_wrapper->{$field}->value());
      $key = $fc_wrapper->field_subfield_code->value();
    }
    $me['subfields'][$key] = $subs;
  }
  $field = 'field_description';
  $me[$field] = trim($wrapper->{$field}->value());

  return $me;

}

function _cals_importer_get_subfield_array() {
  //$rs = db_query("select * from {eck_marc_tag} ");
  $es = entity_load("marc_tag", FALSE);
  $me = array();
  foreach($es as $id => $e) {
    $tagset = rebuild_marc_tag($e);
    if(is_array($tagset)) {
      $key = $tagset['field_code'];
      $me[$key] = $tagset;
    }

  }
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
  //$xml = simplexml_load_string($xml_string);
  $records = new File_MARCXML($xml_string, File_MARC::SOURCE_STRING, "http://www.loc.gov/MARC21/slim");
  //$xml = _cals_get_marc_xml_string($node);

  //printAndDie($xml);

  $n = 0;
  while ($record = $records->next()) {
    $n++;
    //_cals_importer_exporter_marcxml_remove_tags($record, $tags);
    //_cals_importer_exporter_callback_245($node, $record);
    //_cals_importer_exporter_add_taxonomy_terms_marcxml($node, $record);
    //iterate through records


    _cals_importer_exporter_iterate_record($node, $record, $tags);
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
  }

  $node->field_xml_stub[LANGUAGE_NONE][0]['value'] = $new_xml_stub;
  //_cals_importer_update_admin_audit($node,  $action);
  node_save($node);

}
/**
 * Iterate thru taxonomy terms (field_subject, field_genre) and add values to
 * current record
 *
 * @param $node
 * @param $record
 */
function _cals_importer_exporter_add_nnels_values_marcxml($node, &$record) {
  //if vocab, $term = 'field_subject';
  $fieldnames =
    array(
      //'field_system_control_number' => array('001', '', '', '', NULL),
      'field_dc_creator' => array('040', 'a', 1, '', NULL),
      'title_field' => array(245, 'a', 1, 4, 'normalize_title'),

      //'field_access_restrictions' => array(),
    );

  foreach($fieldnames as $fieldname => $marc) {
    $tag = $marc[0];
    $code = $marc[1];
    $ind1 = $marc[2];
    $ind2 = $marc[3];
    $callback = $marc[4];
    $values = field_view_field('node', $node, $fieldname, array('default'));

    if(is_array($values)) {
      foreach($values['#items'] as $value) {
        $subfields = array();
        $safe_value = trim($value['value']);
        if(!empty($callback)) $callback($safe_value);
        $subfields = array();
        if($tag != '001') $subfields[] = new File_MARC_Subfield($code, $safe_value);
        $field = new File_MARC_Data_Field($tag, $subfields, 0, null);
        $field->setIndicator(1, $ind1);
        $field->setIndicator(2, $ind2);
        _cals_importer_exporter_insert_field($record, $field, $key);
      }
    }

  }
  //return new File_MARC_Data_Field('650', $subfields, 0, null);
}

function _cals_importer_exporter_field_value($node, $fieldname) {
  $values = field_view_field('node', $node, $fieldname, array('default'));
  return $values;
}


function normalize_title(&$value) {
  $last = substr($string, -1);

  if($last != '.') $value .= '.';
}


/**
 * Taxonomy terms (field_subject, field_genre) and add values to
 * current record
 *
 * @param $node
 * @param $record
 */
function _cals_importer_exporter_add_taxonomy_terms_marcxml($node, &$record) {
  //if vocab, $term = 'field_subject';
  $fieldnames =
    array(
      'field_subject' => array(650, 'a', 1, 4),
      'field_genre' => array(655, 'a', ' ', 7),
    );

  foreach($fieldnames as $fieldname => $marc) {
    $tag = $marc[0];
    $code = $marc[1];
    $ind1 = $marc[2];
    $ind2 = $marc[3];
    $terms = field_view_field('node', $node, $fieldname, array('default'));
    if(is_array($terms)) {
      foreach($terms["#items"] as $term){
        $subfields = array();
        $subfields[] = new File_MARC_Subfield($code, $term["taxonomy_term"]->name);
        $field = new File_MARC_Data_Field($tag, $subfields, 0, null);
        $field->setIndicator(1, $ind1);
        $field->setIndicator(2, $ind2);
        _cals_importer_exporter_insert_field($record, $field, $tag);
      }
    }
  }
  //return new File_MARC_Data_Field('650', $subfields, 0, null);
}

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
  $tags = array(
    '020', '040', 100, 245, 246,
    300, 336, 337, 338, 344,
    347, 538, 506, 520, 521, 534,
    650, 655);
  foreach($tags as $tag) {
    $x = $record->deleteFields($tag);
  }
}
function _cals_importer_exporter_marcxml_remove_tagsxxxx() {
  foreach($tags as $tag) {
    $code = _cals_importer_exporter_get_normalized_code($tag['field_code']);
    $title = $tag['title'];
    $export = $tag['field_include_in_export'];
    $source = $tag['field_source_value'];

    //$tmp = array("code", "export", "source");
    if($code != 'ldr') {

      switch($export . '::' . $source) {
        case "1::1":
          //printPre("$title | code = $code | export = $export | source = $source");
          $x = $record->deleteFields($code);


          break;
      }

    }
  }
}

function _cals_importer_exporter_get_something($vars) {
  $subfields = $vars['subfields'];
  foreach($subfields as $subcode => $details) {

    printAndDie(__FUNCTION__, "SUBCODE = $subcode", $details, $vars);

    if($key == $tag && $details['field_include_in_export'] == 2) {
      $sub[$code][] = array(
        'value' => $subdata->getData(),
        'subfield' => $code
      );
    }
  }

}

/**
 * Parses the original marc file
 * @param $record
 * @param $tags
 * @param $xml
 */
function _cals_importer_exporter_parse_original_marc($record, $vars, $xml) {
  $tag = $vars['field_code'];
  //printAndDie(__FUNCTION__, $tag, $vars);

  foreach($vars['subfields'] as $code => $value) {
    if(strlen($code) != 1) return;
    $values = _cals_importer_get_xpath_values($tag, NULL, NULL, $code, $xml);
    if(empty($values)) return;

    //else continue

    $ind1 = $vars['field_indc1'];
    $ind2 = $vars['field_ind2'];
    $inds = array();
    if ($ind1 == 'use MarcXML indicators') {
      $inds = _cals_importer_get_marc_indicators($xml, $tag);
      printPre("USE MARC INDICATORS", $inds);
    }
    else {
      $inds[1] = $ind1;
      $inds[2] = $ind2;

    }
    if(count($inds)) {
      $values = _cals_importer_get_xpath_values($tag, $inds[1], $inds[2], $code, $xml);
      if (count($values)) {

        $return = array($tag => array("ind1" => $inds[1], "ind2" => $inds[2]));
        foreach ($values as $value) {
          $return[$tag]['subfield'][$code][] =  $value;
        }
      }
    }
    return $return;

  }


    /*
  printPre($tags[$tag]);
  foreach($record->getFields(240) as $code=>$value) {
    printPre("CODE = $code", $value);
    $ind1 = $value->getIndicator(1);
    $ind2 = $value->getIndicator(2);
    $subdata = $value->getSubfields();
    foreach($subdata as $i => $v) {
      printPre("SUBDATA: $tag = $tag | ind1 = $ind1 | ind2 = $ind2 | code = {$v->getCode()} | value = {$v->getData()}");
    }
    printPre($code, "IND1: $ind1", $value->getSubfields());
  }
  //die;

  //method using xpath:

  */

}

/**
 * Placeholder function
 * @param $record
 */
function _cals_importer_exporter_iterate_record($node, &$record, $tags) {
  //$tags = _cals_importer_exporter_normalize_tags($tags);
  $node_wrapper = entity_metadata_wrapper('node', $node);
  $datafields = array();
  $xml = _cals_get_marc_xml_string($node);
  //loop thru tags

  foreach($tags as $tag) {
    $callback_exists = $tag['callback_exists'];
    $code = $tag['field_code'];
    if($code != 24533333) {
      $export = $tag['field_include_in_export'];
      $source = $tag['field_source_value'];
      $ex_source = $export . '.' . $source;
      $x = $record->deleteFields($code);

      switch ($ex_source) {
        case 1.1:

          break;
        case 2.1:
        case 5.1:
          $vars = array("code" => $code, 'tag' => $tag);
          break;
      }
      if ($callback_exists) {
        $vars = array(
          'node' => $node,
          'tag' => $tag,
          'node_wrapper' => $node_wrapper
        );
        $callback = $tag['callback'];
        $tmp = $callback($vars);
        $datafields = $datafields + $tmp;
      }
      else {
        $tmp = _cals_importer_exporter_parse_original_marc($record, $tag, $xml);
        if(is_array($tmp))
          $datafields = $datafields + $tmp;
        //printAndDie($tags);

      }
    }
  }
  ksort($datafields);

  _cals_importer_exporter_iterate_new_record_array($record, $datafields);

}

function _cals_importer_exporter_iterate_new_record_array(&$record, $datafields) {
  //printAndDie(__FUNCTION__, $datafields);
  //printPre($datafields);
  foreach($datafields as $code => $arr) {
    $subfields = $arr['subfield'];
    //printPre($code, $arr, $subfields);
    $marc_subfields = _cals_importer_exporter_get_marc_subfield($subfields);
    //printAndDie($subfields, $marc_subfields,  __FUNCTION__);

    $field = new File_MARC_Data_Field($code, $marc_subfields, 0, null);
    $field->setIndicator(1, $arr['ind1']);
    $field->setIndicator(2, $arr['ind2']);
    _cals_importer_exporter_insert_field($record, $field, $code);
  }

}
function _cals_importer_exporter_get_marc_subfield($arr) {
  $subfields = array();
  foreach ($arr as $code => $values) {
    foreach($values as $value) {
      $subfields[] = new File_MARC_Subfield($code, $value);
    }
  }
  return $subfields;
}

function _populate_record() {
  printAndDie(__FUNCTION__, $tags);
  printAndDie(__FUNCTION__, $tag, $value, "indicator 2 = $ind2");

  $subfields = array();
  $subfields[] = new File_MARC_Subfield($subfield, $value);
  $field = new File_MARC_Data_Field($tag, $subfields, 0, null);
  $field->setIndicator(1, $ind1);
  $field->setIndicator(2, $ind2);
  _cals_importer_exporter_insert_field($record, $field, $key);
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
  //_cals_importer_exporter_iterate_record($record);
  $record->appendField($field);
}

function _iterate_tags_deprecate() {

  $fieldnames =
    array(
      //'field_system_control_number' => array('001', '', '', '', NULL),
      'field_dc_creator' => array('040', 'a', 1, '', NULL),
      'title_field' => array(245, 'a', 1, 4, 'normalize_title'),

      //'field_access_restrictions' => array(),
    );

  foreach($fieldnames as $fieldname => $marc) {
    $tag = $marc[0];
    $code = $marc[1];
    $ind1 = $marc[2];
    $ind2 = $marc[3];
    $callback = $marc[4];
    $values = field_view_field('node', $node, $fieldname, array('default'));

    if(is_array($values)) {
      foreach($values['#items'] as $value) {
        $subfields = array();
        $safe_value = trim($value['value']);
        if(!empty($callback)) $callback($safe_value);
        $subfields = array();
        if($tag != '001') $subfields[] = new File_MARC_Subfield($code, $safe_value);
        $field = new File_MARC_Data_Field($tag, $subfields, 0, null);
        $field->setIndicator(1, $ind1);
        $field->setIndicator(2, $ind2);
        _cals_importer_exporter_insert_field($record, $field, $key);
      }
    }

  }

  foreach ($record->getFields() as $tag=>$value) {

    if ($value instanceof File_MARC_Control_Field) {
      //printPre("TAG = $tag | CODE = $code | VALUE = " . $value->getData());
    }
    else {

      $sub = array();
      foreach ($value->getSubfields() as $code=>$subdata) {
        //printPre("TAG = $tag | CODE = $code | VALUE = " . $subdata->getData());
        $sub[$code] = array(
          'value' => $subdata->getData(),
        );
      }
      if(!empty($tag)) {
        //$tag2 = str_replace($tag,'', 0, 0);
        if(substr($tag, 0, 1) == '0') {
          $tag = ltrim($tag, '0');

        }
        $df[$tag] = array(
          'ind1' => $value->getIndicator(1),
          'ind2' => $value->getIndicator(2),
          'sub' => $sub,

        );

      }

    }
  }
  ksort($df);
}