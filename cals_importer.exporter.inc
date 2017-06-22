<?php


/**
 * Simple wrapper so we can invoke from main repo node page for testing purposes
 *
 * @param $node
 *   the node passed to the parser
 */

function _cals_importer_create_marc_stub_node($node) {
  require 'File/MARCXML.php';
  require 'File/MARC/Lint.php';
  //printAndDie($tags);
  //die;
  _cals_importer_create_marc_stub($node);
  drupal_goto("node/{$node->nid}/view-xml-stub");

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
  //printAndDie($wrapper->getPropertyInfo());
  $field = 'id';
  $field = 'field_code';
  $code = $wrapper->{$field}->value();
  $field_code = _cals_importer_exporter_get_normalized_code(
    trim($wrapper->{$field}->value()));
  if($field_code == 'ldr' || $code < 20) return;
  $me['field_code'] = $field_code;
  $me['callback'] = '_cals_importer_exporter_callback_' . $field_code;
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
    'field_indc1',
    'field_ind2',
    'field_include_in_export',
    'field_source_value',
    'field_description',
  );

  foreach($wrapper->field_marc_subfield_collection as $fc_wrapper) {
    //$info = $fc_wrapper->getPropertyInfo();
    //$keys = array_keys($info);
    foreach($keys as $field) {
      $subs[$field] = trim($fc_wrapper->{$field}->value());
    }
    $me['subfields'][] = $subs;
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
function _cals_importer_exporter_filing_characters($value) {
  $value = strtolower($value);
  $arr = array(
    "a ", "a'", "al ", "al- ", "am ", "an ", "an t- ", "ane ", "ang ", "ang mga ",
    "as ", "az ", "bat ", "bir ", "d'", "da ", "das ", "de ", "dei ", "dem ",
    "den ", "der ", "des ", "det ", "di ", "die ", "dos ", "e ", "'e ", "een ",
    "eene ", "egy ", "ei ", "ein ", "eine ", "einem ", "einen ", "einer ",
    "eines ", "eit ", "el ", "el- ", "els ", "en ", "enne ", "et ", "ett ",
    "eyn ", "eyne ", "gl'", "gli ", "ha- ", "hai ", "he ", "hē ", "he- ",
    "heis ", "hen ", "hena ", "henas ", "het ", "hin ", "hina ", "hinar ", "hinir ",
    "hinn ", "hinna ", "hinnar ", "hinni ", "hins ", "hinu ", "hinum ", "hið ",
    "ho ", "hoi ", "i ", "ih'", "il ", "il- ", "in ", "it ",
    "ka ", "ke ", "l'", "l- ", "la ", "las ", "le ", "les ", "lh ", "lhi ",
    "li ", "lis ", "lo ", "los ", "lou ", "lu ", "mga ", "mia ", "'n ", "na ",
    "na h- ", "njē ", "ny ", "'o ", "o ", "os ", "'r ", "'s ", "'t ", "ta ",
    "tais ", "tas ", "tē ", "tēn ", "tēs ", "the ", "to ", "tō ", "tois ",
    "ton ", "tōn ", "tou ", "um ", "uma ", "un ", "un'", "una ", "une ",
    "unei ", "unha ", "uno ", "uns ", "unui ", "us ", "y ", "ye ", "yr ");

  foreach($arr as $char) {
    $len = strlen($char);
    $substr = substr($value, 0, $len);
    if($substr == $char) return $len; //printAndDie("SUBSTRING = $substr | NONFILING = $len");
  }
  return 0;

}
/**
 * Primary title, a bit tricky due to nonfiling characters.
 *
 * @param $node
 * @param $record
 */
function _cals_importer_exporter_callback_245($vars) {
  //if vocab, $term = 'field_subject';
  $node = $vars['node'];
  $tag = $vars['tag'];
  $values = _cals_importer_exporter_field_value($node, 'title_field');
  //field_view_field('node', $node, 'title_field', array('default'));
  $safe_values = array();
  if(is_array($values)) {
    foreach($values['#items'] as $value) {
      $subfields = array();
      $safe_values[] = trim($value['value']);

    }
  }
  $code = 245;
  $subfield = 'a';
  $ind1 = $tag['subfields'][0]['field_indc1'];
  $rec[245]['ind1'] = $tag['subfields'][0]['field_indc1'];
  $rec[245]['ind2'] = $tag['subfields'][0]['field_ind2'];
  foreach($safe_values as $value) {
    $value = trim($value);
    $ind2 = _cals_importer_exporter_filing_characters($value);
    $rec[245]['subfields'][] = array(
      'value' => $value . '.', 'subfield' => 'a',
    );

  }

  return $rec;

}


/**
 * Returns publisher tag: $a = place, $b = pub'r name, $c = date
 * @param $vars
 * @return mixed
 */
function _cals_importer_exporter_callback_264($vars) {
  $node = $vars['node'];
  $tag = $vars['tag'];
  $wrapper = $vars['node_wrapper'];
  $values = _cals_importer_exporter_field_value($node, 'field_publisher');

  $return[264] = array(
    'ind1' => ' ',
    'ind2' => ' ',
  );
  //field_view_field('node', $node, 'title_field', array('default'));
  $safe_values = array();
  if(is_array($values)) {
    foreach($values['#items'] as $value) {
      $safe_values[] = trim($value['value']);
    }
  }
  foreach($safe_values as $k => $v) {
    $s = ($k == 0) ? 'a' : 'b';
    $v = trim($v);
    //$ind2 = _cals_importer_exporter_filing_characters($value);
    $return[264]['subfields'][] = array(
      'value' => $v, 'subfield' => $s,
    );
  }

  $dateset = FALSE;
  foreach($wrapper->field_date as $key => $date) {
    $q = strtolower($date->field_qualifier_date->value());
    if($q == 'created') {
      $d = $date->field_dc_date->value();
      //printPre("Q = $q | DATE = $d");
      $dateset = TRUE;
    }
  }
  if($dateset) {
    $return[264]['subfields'][] = array(
      'value' => $d, 'subfield' => 'c',
    );
  }
  return $return;

}

function _cals_importer_exporter_callback_856($vars) {
  $tag = $vars['tag'];
  $node = $vars['node'];
  //field_publisher
  //printAndDie($tag);
  $nid = $node->nid;
  $arr[856] = array(
    'ind1' => '',
    'ind2' => '',
    '$subfield' => array('value' => 'http://nnels.ca/node/' . $nid, 'subfield' => 'u'),
  );
  return $arr;
}


function normalize_title(&$value) {

  $last = substr($string, -1);
  if($last != '.') $value .= '.';

}


/**
 * Iterate thru taxonomy terms (field_subject, field_genre) and add values to
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
        _cals_importer_exporter_insert_field($record, $field, $key);
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
          printPre("$title | code = $code | export = $export | source = $source");
          $x = $record->deleteFields($code);


          break;
      }

    }
  }
  printAndDie(__FUNCTION__, $tags);
}


function _cals_importer_exporter_normalize_tags($tags) {
  $arr = array();
  foreach($tags as $tag) {
    $code = _cals_importer_exporter_get_normalized_code($tag['field_code']);

    //$tmp = array("code", "export", "source");
    $ex = array('ldr',  245);
    if(!in_array($code, $ex) && !empty($tag['field_subfield_code']) &&
      $tag['field_subfield_code'] != 'n/a') {

      $subcode = trim($tag['field_subfield_code']);
      $subcode = explode(",", $subcode);
      $title = $tag['title'];
      $export = $tag['field_include_in_export'];
      $source = $tag['field_source_value'];
      $ind1 = $tag['field_indc1'];
      $ind2 = $tag['field_ind2'];
      $arr[$code]['title'] = $title;
      $arr[$code]['notes'] = trim($tag['field_description']);
      foreach($subcode as $sb) {
        $sb = trim($sb);
        $arr[$code]['subfieldsxx'][$sb] = array(
          'ind1' => $ind1, 'ind2' => $ind2, 'value' => '',
        );
      }
    }
  }
  return $arr;
}

/**
 * Placeholder function
 * @param $record
 */
function _cals_importer_exporter_iterate_record($node, &$record, $tags) {
  //$tags = _cals_importer_exporter_normalize_tags($tags);
  $node_wrapper = entity_metadata_wrapper('node', $node);
  $datafields = array();
  foreach($tags as $tag) {
    //$code = _cals_importer_exporter_get_normalized_code($tag['field_code']);
    $code = $tag['field_code'];
    $title = $tag['title'];
    $export = $tag['field_include_in_export'];
    $source = $tag['field_source_value'];
    $ind1 = $tag['field_indc1'];
    $ind2 = $tag['field_ind2'];
    $subcode = $tag['field_subfield_code'];
    $callback = $tag['callback'];
    $ex = array('ldr');
    if(!in_array($code, $ex)) {

      switch($export . '::' . $source) {
        //must export, grab value(s) from Drupal
        case "1::1":
          printPre("$title | code = $code | export = $export | source = $source | callback = $callback");
          if(function_exists($callback)) {
            $x = $record->deleteFields($code);
            //e.g., _cals_importer_exporter_callback_245
            //'record' => $record,
            $vars = array('node' => $node, 'tag' => $tag, 'node_wrapper' => $node_wrapper);
            $tmp = $callback($vars);
            $datafields  = $datafields + $tmp;
          }
      }
    }
  }


  _cals_importer_exporter_iterate_new_record_array($record, $datafields);

  //printAndDie($df);
}

function _cals_importer_exporter_iterate_new_record_array(&$record, $datafields) {
  foreach($datafields as $code => $arr) {
    $subfields = $arr['subfields'];
    printPre($code, $arr, $subfields);
    $marc_subfields = _cals_importer_exporter_get_marc_subfield($subfields);
    $field = new File_MARC_Data_Field($code, $marc_subfields, 0, null);
    $field->setIndicator(1, $arr['ind1']);
    $field->setIndicator(2, $arr['ind2']);
    _cals_importer_exporter_insert_field($record, $field, $key);
  }

}
function _cals_importer_exporter_get_marc_subfield($arr) {
  $subfields = array();
  foreach ($arr as $k => $v) {
    $subfields[] = new File_MARC_Subfield($v['subfield'], $v['value']);
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


  /*
  $id = 700;
  $tag = $record->getFields($id);

  // Insert the new field
  if (is_array($tag)) {
    $record->insertField($field, $tag[0], false);
  }
  elseif ($tags) {
    $record->insertField($field, $tag, true);
  }
  */
  //printAndDie(is_array($tag), $record);


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
        //printAndDie($safe_value);
        $subfields = array();
        if($tag != '001') $subfields[] = new File_MARC_Subfield($code, $safe_value);
        $field = new File_MARC_Data_Field($tag, $subfields, 0, null);
        $field->setIndicator(1, $ind1);
        $field->setIndicator(2, $ind2);
        //if($tag == '001') printAndDie($field);
        _cals_importer_exporter_insert_field($record, $field, $key);
      }
    }

  }

  foreach ($record->getFields() as $tag=>$value) {

    if ($value instanceof File_MARC_Control_Field) {
      printPre("TAG = $tag | CODE = $code | VALUE = " . $value->getData());
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


function __245_deprecate() {
  $subfields[] = new File_MARC_Subfield($tag['field_code'], $safe_value);
  $field = new File_MARC_Data_Field($tag, $subfields, 0, null);
  $field->setIndicator(1, $ind1);
  $field->setIndicator(2, $ind2);

  $fieldnames =
    array(
      //'field_system_control_number' => array('001', '', '', '', NULL),
      'field_dc_creator' => array(100, 'a', 1, '', NULL),
      'title_field' => array(245, 'a', 1, 4, 'normalize_title'),
      //'field_publisher' => array(264, 'a', '', 1, NULL),


      'body' => array(520, 'a', '', '', NULL),
      'field_publisher_source' => array(534, 'c', '', '', NULL),

    );
  foreach($fieldnames as $fieldname => $marc) {
    $tag = $marc[0];
    $code = $marc[1];
    $ind1 = $marc[2];
    $ind2 = $marc[3];
    $callback = $marc[4];
    $values = field_view_field('node', $node, $fieldname, array('default'));

    $x = $record->deleteFields($tag);

    if(is_array($values)) {
      foreach($values['#items'] as $value) {
        $subfields = array();
        $safe_value = trim($value['value']);
        if(!empty($callback)) $callback($safe_value);
        //printAndDie($safe_value);
        $subfields = array();
        if($tag != '001') $subfields[] = new File_MARC_Subfield($code, $safe_value);
        $field = new File_MARC_Data_Field($tag, $subfields, 0, null);
        $field->setIndicator(1, $ind1);
        $field->setIndicator(2, $ind2);
        //if($tag == '001') printAndDie($field);
        _cals_importer_exporter_insert_field($record, $field, $key);
      }
    }

  }
}
