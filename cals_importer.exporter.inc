<?php
require 'File/MARCXML.php';
require 'File/MARC/Lint.php';

module_load_include('inc', 'cals_importer', 'cals_importer.exporter_callbacks');
module_load_include('inc', 'cals_importer', 'cals_importer.exporter_file_snippets');


/**
 * Simple wrapper so we can invoke from main repo node page for testing purposes
 *
 * @param $node
 *   the node passed to the parser
 */

function _cals_importer_create_marc_stub_node($node) {
  _cals_importer_create_marc_stub($node);
  //drupal_goto("node/" . $node->nid);
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

/**
 * Updates the node with stub marc-xml record
 *
 * @param $node
 *   the node passed to the parser
 *
 */
function _cals_importer_create_marc_stub($node) {
  $tags = _cals_importer_get_tag_array(); //tag mappings
  $lint = new File_MARC_Lint();
  $xml_string = $node->field_xml_string[LANGUAGE_NONE][0]['value'];
  $xml_string = MARC_HEADER . $xml_string . '</marc:collection>';
  $records = new File_MARCXML($xml_string, File_MARC::SOURCE_STRING, "http://www.loc.gov/MARC21/slim");
  //$xml = _cals_get_marc_xml_string($node);
  $n = 0;
  while ($record = $records->next()) {

    //$xml = simplexml_load_string($xml_string);
    _cals_importer_exporter_marcxml_remove_tags($record, $tags);
    //get marc-ready array
    $marcfields = _cals_importer_exporter_get_marc_fields($node, $record, $tags);
    $node->field_xml_stub[LANGUAGE_NONE][0]['value'] = serialize($marcfields);

    if(function_exists('dpm')) {
      //dpm($marcfields['controlfields']);
      //dpm($marcfields['datafields']);
    }
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
    /* */
    //$new_xml_stub = $record->toRaw();

    $node->field_xml_stub_xml[LANGUAGE_NONE][0]['value'] = $new_xml_stub;
    //_cals_importer_update_admin_audit($node,  $action);
    node_save($node);
  }


}

/**
 * Returns a slightly cleaned up array of all tags
 * @param $entity
 * @return array|void
 */

function _cals_importer_exporter_rebuild_array_marc_tag($entity) {

  $me = array();
  $wrapper = $entity->wrapper();
  $tag = trim($wrapper->field_marc_tag->value());
  $tag_type = $wrapper->field_marc_tag_type->value();
  if($tag_type < 2) return;
  $me['tag'] = $tag;
  $me['tag_type'] = $tag_type;
  $me['callback'] = '_cals_importer_exporter_callback_' . $tag;
  $me['callback_exists'] = (function_exists($me['callback'])) ? TRUE : FALSE;

  $me['ind1'] = $wrapper->field_indc1->value(); // $wrapper->field_indc1->value();
  $me['ind2'] = $wrapper->field_ind2->value(); //$wrapper->field_ind2->value();
  $fields = array(
    //'id',
    'title',
    'field_include_in_export',
    'field_functions',
    'field_source_value',
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
  //if tag type = datafield
  if($tag_type == 3) {
    $subs = array();
    $me['subfields'] = _cals_importer_get_subfields_collection_fields($wrapper);
  }
  //$me['description'] = trim($wrapper->field_description->value());
  return $me;

}

/**
 * @param $wrapper
 * @return array
 */
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
        if($str == 'drupal_fieldname') {
          $val = $fc_wrapper->{$fieldname}[0]->value();
        }
        else {
          $val = trim($fc_wrapper->{$fieldname}->value());
        }
        if($str != 'description')
          $subs[$str] = $val;
      }
    }
    $subfields[$code] = $subs;
  }
  return $subfields;
}

/**
 * Returns an array of all tags in structure similar to this:
 * $me['datafields'][245] = array(
 *   'ind1'=> 1, 'ind2 => 2,
 *
 *   'subfields' = array(
 *     'a' => array('value' => 'Value of 245$a tag', 'required' => TRUE|FALSE ),
 *     'b' => array('value' => 'Value of 245$b tag', 'required' => TRUE|FALSE ),
 *   )
 * );
 * @return array
 */
function _cals_importer_get_tag_array() {
  //$rs = db_query("select * from {eck_marc_tag} ");
  $es = entity_load("marc_tag", FALSE);
  $me = array();
  foreach($es as $id => $e) {
    if(!empty($e->field_marc_tag_type) && !empty($e->field_export_type) &&
      $e->field_marc_tag_type[LANGUAGE_NONE][0]['value'] > 1 &&
      $e->field_export_type[LANGUAGE_NONE][0]['value'] == 1) {
      $tagset = _cals_importer_exporter_rebuild_array_marc_tag($e);
      if(is_array($tagset)) {
        $key = $tagset['tag'];
        $type = ($e->field_marc_tag_type[LANGUAGE_NONE][0]['value'] == 3) ? 'datafields' : 'controlfields';
        //printPre("type = " . $type, $tagset['tag'], $e->field_marc_tag_type[LANGUAGE_NONE][0]['value']);
        $me[$type][$key] = $tagset;
      }
    }
  }
  ksort($me['datafields']);
  //printAndDie($me);
  return $me;
}

/**
 * Returns array of values for a given node + fieldname.
 * Used by various callbacks.
 *
 * @param $node
 * @param $fieldname
 * @return mixed
 */
function _cals_importer_exporter_field_value($node, $fieldname) {
  $values = field_view_field('node', $node, $fieldname, array('default'));
  return $values;
}

/**
 * Returns array of values for a given node + fieldname.
 * Used by various callbacks.
 *
 * @param $node
 * @param $fieldname
 * @return mixed
 */
function _cals_importer_exporter_field_value_taxonomy_term($node, $fieldname) {
  printAndDie($values);

  $values = field_view_field('taxonomy_term', $node, $fieldname, array('default'));
  return $values;
}

/**
 * Returns the subfield array when provided w/ a Drupal fieldname + code +
 * wrappper
 *
 * @param $code
 * @param $fieldname
 * @param $wrapper
 * @return array
 */

function _cals_importer_exporter_get_subfield_array_from_drupal_fields($code, $fieldname, $wrapper, $enumerations = NULL) {
  $arr = array();
  if(!empty($wrapper->{$fieldname}->value())) {
    $values = $wrapper->{$fieldname}->value();
    if(is_string($values) && !empty(trim($values))) {
      $arr[][$code] = (is_array($enumerations)) ? $enumerations[$values] : $values;
    }

    if(is_array($values)) {
      foreach($values as $value) {
        $arr[][$code] = (is_array($enumerations)) ? $enumerations[$values] : $value;
      }
    }
  }

  //if($code == 'z' and $fieldname == 'field_isbn') printAndDie($arr);

  return $arr; // = array();
}

/**
 * Returns a subfield array when given a tag and existing subfield array;
 *
 * @param $tag
 * @param $subfields
 * @return array
 *
 */
function _cals_build_marc_subfield_array($tag, $subfields) {
  if(is_array($subfields) && count($subfields)) {
    return array($tag => array(
      'subfields' => $subfields,
    ));
  }
  return '';
}

/**
 * Returns a normalized title field
 * @param $value
 */
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
 * Parses the original marc file to find values associated w/ specific tag / indicators
 * @param $record
 * @param $tags
 * @param $xml
 */
function _cals_importer_exporter_parse_original_marc($tagset, $code, $record, $xml) {
  $ind1 = $tagset['ind1'];
  $ind2 = $tagset['ind2'];
  $tag = $tagset['tag'];
  //$subdata = $tagset['subfields'][$code];
  $values =  _cals_importer_get_xpath_values($tag, NULL, NULL, $code, $xml);
  //if($tag == 720) printAndDie($code, $tagset, $subfields, $values);

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
    $tmp = $function($node, $node_wrapper, $record, $tagset, $xml);
    if(!empty($tmp))
      $fields = $fields + $tmp;
  }
  ksort($fields);
  //printAndDie($fields);
  return $fields;
}

function _cals_importer_exporter_parse_controlfields($node, $node_wrapper, &$record, $tags, $xml) {
  //$xml = _cals_get_marc_xml_string($node);
  foreach($tags as $tag => $arr) {

    switch($tag) {
      case 'leader':
        $val = _cals_importer_exporter_callback_leader($node_wrapper);
        break;
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
        $val = _cals_importer_export_callback_008($node, $node_wrapper, $xml);
        break;

    }
    $tmp['controlfields'][$tag] = array(
      'value' => $val,
      'required' => $arr['include_in_export']
    );


  }
  return $tmp;

}
//_cals_importer_exporter_parse_controlfields($node, $node_wrapper, &$record, $tags) {


function _cals_importer_exporter_parse_datafields($node, $node_wrapper, &$record, $tags, $xml) {
  //$xml = _cals_get_marc_xml_string($node);

  $arr = array();
  $tmp = array();
  foreach($tags as $tag => $tagset) {
    $callback_exists = $tagset['callback_exists'];
    try {
      $subs = $tagset['subfields'];
      //Treat parent tag as a unit if callback exists.
      if ($callback_exists) {
        $vars = array(
          'node' => $node,
          'tag' => $tag,
          'node_wrapper' => $node_wrapper,
          'xml' => $xml,
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
          //if($ind1 = "''") $ind1 = '';
          if($ind2 = "''") $ind2 = '';
          if ($ind1 == 'use MarcXML indicators') {
            $ind1 = _cals_importer_get_marc_indicators($xml, $tag, 1);
          }
          if ($ind2 == 'use MarcXML indicators') {
            $ind2 = _cals_importer_get_marc_indicators($xml, $tag, 2);
          }
          $tmp[$tag]['ind1'] = $ind1;
          $tmp[$tag]['ind2'] = $ind2;
        }
      }


      //include the array even if empty so we can track invalid records
      if(!empty($tmp[$tag]['subfields']) ||
        (isset($tmp[$tag]['required']) && $tmp[$tag]['required'] == 1)) {
        $arr['datafields'][$tag] = $tmp[$tag];
        $arr['datafields'][$tag]['required'] = $tagset['include_in_export'];
      }
    }

//catch exception
    catch(Exception $e) {
      drupal_set_message('Message: issue with  ' . $tag . ": " .$e->getMessage());
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
    //for the rare fields that we pre-assign values to, e.g., 300$a and 300$b
    elseif (!empty($fields['static_value'])) {
      $tmp = $fields['static_value'];
      $subfields[$code][] = $tmp;
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
        foreach($tmp as $k => $v) {
          $subfields[$k][$code] = trim($v);

        }
      }
    }
  }
  $me[$tag]['subfields'] = $subfields;
  return $me;
}

/**
 * Loops thru control and datafields array to return new MARC XML record
 *
 * @param $record
 * @param $marcfields
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
    if($arr['required'] == 1 && empty($data)) {
      drupal_set_message("Missing value for tag $tag. Invalid XML Record Stub.");
      return;
    }
    _cals_importer_exporter_insert_field($record, $field, $tag);
  }
}
/**
 * Iterate thru array of datafields to insert xml datafields
 * @param $record
 * @param $datafields
 */
function _cals_importer_exporter_iterate_datafields(&$record, $datafields) {
  foreach($datafields as $tag => $arr) {
    $subfields = $arr['subfields'];
    if(empty($subfields)) {
      if($arr['required'] == 1)
        //throw error msg
        drupal_set_message("Missing subfields for tag $code. Invalid XML Record Stub.");
    }
    else {
      //if($code == 720) printAndDie($marc_subfields);
      if($tag != 7200) {
        foreach($subfields as $subfield) {

          $marc_subfields = array();

          $marc_subfields = _cals_importer_exporter_get_marc_subfield($subfield);
          $field = new File_MARC_Data_Field($tag, $marc_subfields, 0, null);
          $field->setIndicator(1, "{$arr['ind1']}");
          $field->setIndicator(2, "{$arr['ind2']}");
          _cals_importer_exporter_insert_field($record, $field, $tag);

          //printAndDie($field);

          /*

          foreach($marc_subfields as $marc_subfield) {
            $tmp = array($marc_subfield);
            printAndDie($code, $tmp, $subfields);
            $field = new File_MARC_Data_Field($code, $tmp, 0, null);
            $field->setIndicator(1, "{$arr['ind1']}");
            $field->setIndicator(2, "{$arr['ind2']}");
            _cals_importer_exporter_insert_field($record, $field, $tag);
          }
          */
        }

      }
      else { //old method
        $field = new File_MARC_Data_Field($code, $marc_subfields, 0, null);
        $field->setIndicator(1, "{$arr['ind1']}");
        $field->setIndicator(2, "{$arr['ind2']}");
        _cals_importer_exporter_insert_field($record, $field, $code);
      }
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
  foreach ($arr as $code => $value) {
      $subfields[] = new File_MARC_Subfield($code, $value);
    //foreach($values as $value) { }
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


/**
 * View marc-xml stub for repository item
 * @param $node
 */
function _cals_importer_view_marcxml_stub($node) {

  if (!empty($node->field_xml_stub)) {
    $array = unserialize($node->field_xml_stub[LANGUAGE_NONE][0]['value']);
    $items = array();
    foreach ($array as $type => $tags) {
      foreach ($tags as $tag => $content) {
        if ($type == 'controlfields') {
          $str = $content['value'];
          if($tag == '008') $str = str_replace(' ', '.', $str);;
          $items[] = "$tag = $str";
        }
        else {
          if (!empty($content['subfields'])) {
            foreach ($content['subfields'] as $i => $subfields) {

              $ind1 = (isset($content['ind1'])) ? $content['ind1'] : '';
              $ind2 = (isset($content['ind2'])) ? $content['ind2'] : '';
              if (is_array($subfields)) {
                foreach ($subfields as $subcode => $str) {
                  if (empty($subcode)) {
                    drupal_set_message("Issue: $tag requires a valid code");
                    return '';
                  }
                  $items[] = "$tag\$" . $subcode . "($ind1)($ind2) = $str";
                }
              }
            }
          }
        }
      }
    }
    $vars = array(
      'title' => t('Currently saved array of MARC ready fields'),
      'items' => $items,
      'type' => 'ul',
      'attributes' => array()
    );
    $html = theme('item_list', $vars);
    if(!empty($node->field_xml_stub_xml)) {
      $html .= t('<h2>New MARC XML</h2>') .
        _cals_importer_exporter_formatted_marcxml(
          $node->field_xml_stub_xml[LANGUAGE_NONE][0]['safe_value']
        );
    }
    if(!empty($node->field_xml_string)) {
      $html .= t('<h2>Original MARC XML</h2>') .
        _cals_importer_exporter_formatted_marcxml(
        $node->field_xml_string[LANGUAGE_NONE][0]['safe_value']
      );
    }
    //printAndDie($node);
    return $html;
  }
  else{

    return t('Stub has not been defined. You can click on Create XML Stub entry to generate this');
  }
}

/**
 * Pretties up the marc xml string
 * @param $str
 * @return mixed
 */
function _cals_importer_exporter_formatted_marcxml($str) {
  $str = str_replace("marc:", "", $str);
  $str = str_replace("&lt;leader", "<br/>&nbsp;&nbsp;" . "&lt;leader", $str);
  $str = str_replace("&lt;controlfield", "<br/>&nbsp;&nbsp;" . "&lt;controlfield", $str);
  $str = str_replace("&lt;datafield", "<br/>&nbsp;&nbsp;" . "&lt;datafield", $str);
  $str = str_replace("&lt;/datafield", "<br/>&nbsp;&nbsp;" . "&lt;/datafield", $str);

  $str = str_replace("&lt;subfield", "<br/>&nbsp;&nbsp;&nbsp;&nbsp;" . "&lt;subfield", $str);
  $str = str_replace("&lt;record", "<br/>" . "&lt;record", $str);
  $str = str_replace("&lt;/record", "<br/>" . "&lt;/record", $str);
  return $str;
}

/**
 * @param $node
 * @return mixed
 */
function _cals_importer_exporter_build_marcxml_from_array($node) {
  //$xml_string = MARC_HEADER . '</marc:collection>';
  $record = new File_MARC_Record(NULL);
  $marcfields = unserialize($node->field_xml_stub[LANGUAGE_NONE][0]['value']);
  if(function_exists('dpm')) dpm($marcfields);
  _cals_importer_exporter_array_to_marcxml($record, $marcfields);
  $lint = new File_MARC_Lint();

  $valid = $lint->checkRecord($record);
  if(count($valid)) {
    $vars = array("items" => $valid , "title" => "XML Stub Created with following errors", "type" => "ol", "attributes" => array());
    drupal_set_message(theme_item_list($vars));
  }
  return $record->__toString();
  //drupal_goto("node/{$node->nid}/build-validate-marcxml-output");
}

/**
 * Returns a new record based on datafields
 *
 * @param $record
 * @param $datafields
 */
function _cals_importer_exporter_array_to_marcxml(&$record, $marcfields) {
  $xml = array();
  foreach ($marcfields as $type => $arr) {
    $function = '_cals_importer_exporter_array_to_marcxml_' . $type;
    if ($function) {
      $function($record, $arr);
    }
  }
}

/**
 * Create marcxml controlfields
 * @param $record
 * @param $tags
 */
function _cals_importer_exporter_array_to_marcxml_controlfields(&$record, $tags) {
  $xml = array();
  foreach($tags as $tag => $value) {
    $field = new File_MARC_Control_Field($tag, $value['value'], null, null);
    $record->appendField($field);
  }
}

/**
 * Create marcxml datafields
 * @param $record
 * @param $tags
 */
function _cals_importer_exporter_array_to_marcxml_datafields(&$record, $tags) {
  $xml = array();
  foreach($tags as $tag => $tagset) {
    $subfields = array();
    $ind1 = $tagset['ind1'];
    $ind2 = $tagset['ind2'];
    foreach ($tagset['subfields'] as $code => $values) {
      foreach ($values as $value) {
        $subfields[] = new File_MARC_Subfield($code, $value);
      }
      $field = new File_MARC_Data_Field($tag, $subfields, "$ind1", "$ind2");
      //dpm("tag = $tag, ind1 = $ind1, ind2 = $ind2");
      $record->appendField($field);
    }
  }
}