<?php
/**
 * @file
 * Manages marc xml ingests
 */
 
/**
 * Simple wrapper to call main parser; if preferred, can invoke batch api instead. 
 */

function _cals_importer_update_repository_item($node) {
  //can also push this thru the batch api instead. To do so, comment out above and uncomment
  //the following function call.
  _cals_parse_record_details($node, $fields=array());

  //return user back to main node page
  drupal_goto("node/" . $node->nid);
}


/**
 * invoked via batch api view
 *
 * @param $entity
 *   the node passed to the parser
 *
 */  
 
function _cals_importer_update_repository_items(&$entity, $context = array()) {
  if (isset($entity->nid)) {
    $node = node_load($entity->nid);
    //safety check to make sure we're parsing repos items!!    
    $content_types = array('repository_item');
    if (in_array($node->type, $content_types)  ) {
      _cals_parse_record_details($node, $fields=array());
      //dpm($node->title);
      //drupal_set_message(t("%t MARC XML file parsed and saved."), array('%t' => $node->title));
    }
    else {
      //dpm(t("Invalid selection: not a repos item"));
    }

  }//end main if
 
  return "";
  
}


/**
 * Returns fields for repository item
 *
 * @return
 *   an array of flattened field defs 
 *
 * @todo
 *   wrap into a static variable?
 *
 */ 
function _cals_get_fields_repository_item($fieldnames) {
  $fields = field_info_instances("node", "repository_item");


  //printAndDie($fields);
  //set title as first array element
//  $map = array(
//   "title" => array(
//     'type' => 'text',
//     'required' => 1,
//     'values' => '',
//     'parser' => array("function" => '_cals_get_title_from_marc'),
//   ),
//  );
  $map = array();
	$tmp = array(
	  "_autocomplete", "_with_summary", "_hidden", "_buttons", "_select", "_textfield", "_embed", "_textarea",
  );
  //create an array of taxonomies to be used in parsing field collections, etc.
  foreach (taxonomy_get_vocabularies() as $k => $v) {
    $vocabularies[$v->machine_name] = $k;
  }
  if (count($fieldnames)) {
    foreach($fields as $k => $v) {
      if (!in_array($k, $fieldnames)) {
        unset($fields[$k]);
      }
    }
  }

  foreach($fields as $k => $v) {
    $parser = _cals_importer_repo_field_callback($k);
    if(!empty($parser)) {
      $type = $v['widget']['type'];
      $label = $v['label'];
      $t2 = $type;
      foreach($tmp as $strip) $t2 = str_replace($strip, '', $t2);
      $req = $v['required'];

      $map[$k] = array(
        'label' => $label,
        'type' => $t2,
        'required' => $req,
        'values' => '',
        'parser' => _cals_importer_repo_field_callback($k),
      );
      if($t2 == 'taxonomy') {
        $info = field_info_field($k);
        $vocab_keys = array();
        foreach ($info['settings']['allowed_values'] as $item) {
          if(!empty($item['vocabulary'])) $map[$k]['vid'] = $vocabularies[$item['vocabulary']];
        }
      }

      //set a default if defined
      $map[$k]['default'] = (isset($req) && isset($v['default_value'][0]['value'])) ? $v['default_value'][0]['value'] : '';
      $types[$t2][] = $k;
    }
  }

  $arr = array("types" => $types, "map" => $map);
  return $arr;

}

/**
 * maps fieldnames to parser functions below
 *
 * @return
 *   an array of fieldname to functions and params
 *
 */

function _cals_importer_repo_field_callback($fieldname) {

  $arr = array(

    "title" => array('function' => '_cals_get_title_from_marc'),

    "title_field" => array('function' => '_cals_get_title_from_marc'),
    "field_title_alternate" => array('function' => '_cals_get_alternate_title_from_marc'),
    "field_uniform_title" => array('function' => '_cals_get_uniform_title_from_marc'),

    'body'  => array('function' => '_cals_importer_get_basic_values_from_marc',
      "tags" => array(
        array('tag' => 520, 'code' => 'a'),
      )
    ),
   	"field_iso_language" => array('function' => '_cals_importer_get_isolanguage_from_marcxml'),
  	"field_date" => array('function' => '_cals_get_date_issued_from_marc'),

    "field_dc_coverage" => array('function' => '_cals_importer_get_coverage_marc'),
    "field_dc_description" => array('function' => '_cals_get_descriptors_from_marc'),
    
    "field_isbn" => array('function' => '_cals_importer_get_isbn_sysnums_from_marc'),
    'field_performer' => array('function' => '_cals_importer_get_performer_from_marcxml'),

    "field_subject" => array("function" => '_cals_get_subject_from_marc'),
    "field_genre" => array("function" => '_cals_get_genre_from_marc'), 

    
    'field_access_rights' => array('function' => '_cals_importer_get_basic_values_from_marc', 
      "tags" => array(
        array('tag' => 506, 'code' => 'a'),
        array('tag' => 506, 'code' => 'd'),
        array('tag' => 540, 'code' => 'a'),
        array('tag' => 540, 'code' => 'd'),
      )
    ),
    'field_publisher' => array('function' => '_cals_importer_get_basic_values_from_marc', 
      "tags" => array(
        array('tag' => 264, 'code' => 'a'),
        array('tag' => 264, 'code' => 'b'),
        array('tag' => 260, 'code' => 'a'),
        array('tag' => 260, 'code' => 'b'),

      )
    ),
    'field_dc_contributor' => array('function' => '_cals_importer_get_basic_values_from_marc', 
      "tags" => array(
        array('tag' => 700, 'code' => 'a'),
      )
    ),
    'field_dc_creator' => array('function' => '_cals_importer_get_basic_values_from_marc', 
      "tags" => array(
        array('tag' => 100, 'code' => 'a'),
      )
    ),
    'field_title_alternative' => array('function' => '_cals_importer_get_basic_values_from_marc', 
      "tags" => array(
        array('tag' => 130, 'code' => 'a'),
        array('tag' => 730, 'code' => 'a'),
      )
    ),
    'field_uri' => array('function' => '_cals_importer_get_basic_values_from_marc', 
      "tags" => array(
        array('tag' => 856, 'code' => 'u'),
      )
    ),
    
    //relations
    
    'field_dc_relation' => array('function' => '_cals_importer_get_dc_relations_from_marc', 
      "tags" => array(
        array('tag' => 250, 'code' => 'a', 'qualifier' => "type", 'enum' => "isVersionOf"),
        array('tag' => 800, 'code' => 'a', 'qualifier' => "type", 'enum' => "IsPartOf"),
				array('tag' => 830, 'code' => 'a', 'qualifier' => "type", 'enum' => "IsPartOf"),
				array('tag' => 760, 'code' => 'a', 'qualifier' => "type", 'enum' => "IsPartOf"),
				array('tag' => 534, 'code' => 'e', 'qualifier' => "type", 'enum' => "IsFormatOf"),
      )
    ),
 
    'field_access_rights' => array('function' => '_cals_importer_get_basic_values_from_marc',
      "tags" => array(
        array('tag' => 540, 'code' => 'a',),
        array('tag' => 540, 'code' => 'd',),
        array('tag' => 506, 'code' => 'a',),
        array('tag' => 506, 'code' => 'd',),
      )
    
    ),
    
    'field_rights_holder' => array('function' => '_cals_importer_get_basic_values_from_marc', 
      "tags" => array(
        array('tag' => 542, 'code' => 'd'),
      )
    ),
    
    'field_access_restrictions' => array('function' => '_cals_importer_get_access_restrictions'),
  
		'field_oclc_number' => array('function' => '_cals_importer_get_system_numbers_from_marc'),  
		'field_system_control_number' => array('function' => '_cals_importer_get_system_numbers_from_marc'),
		
		'field_cataloging_agency' => array('function' => "_cals_importer_get_cataloging_agency_from_marc"),  
  
  
    
  );
  
  if(array_key_exists($fieldname, $arr)) return $arr[$fieldname];
  
  return '';  


}

/**
 * Normalizes title, creator, contributor and other fields
 *
 * @param $node
 *   the node passed to the parser
 * @param $field
 *   the $node->{field} acted on
 *
 */ 
function _cals_importer_normalize_node_field(&$node, $field) {
  //drupal_set_message(__FUNCTION__ . ": " . $field);
	switch($field) {
	  case "title":
    case "title_field":
    case "field_title_uniform":
    case "field_title_alternate":
		  _cals_importer_strip_square_brackets($node, $field);
      _cals_importer_strip_trailing_periods($node, $field);
			//_cals_importer_normalize_title_fields($node, $field); to deprecate
      break;
	  case "field_dc_creator":
		case "field_dc_contributor":	
			_cals_importer_strip_square_brackets($node, $field);
			_cals_importer_normalize_creators($node, $field);
			//if($field == 'field_dc_creator') printAndDie($node->field_dc_creator, $map);
	  	break;
	  case "field_publisher":
			//_cals_importer_strip_square_brackets($node, "title");
	  	_cals_importer_normalize_publisher($node);
	  	break;
	}
}

function _cals_reparse_title_edition($node) {
  $fields = array("title", "title_field", "field_uniform_title", "field_title_alternate", "field_dc_relation");
  _cals_parse_record_details($node, $fields);
    //return user back to main node page
  drupal_goto("node/" . $node->nid);
}

/**
 * The main MARC parser
 *
 * @param $node
 *   the node passed to the parser
 *
 * @return
 *   an array.
 *
 */
function _cals_parse_record_details($node, $fields) {
  $xml = _cals_get_marc_xml_string($node);
  if($xml === 0) return;

  $arr_fields = _cals_get_fields_repository_item($fields);
  $map = $arr_fields['map'];

  //loop thru map to populate node
  _cals_importer_iterate_field_map($node, $xml, $map, "parse");
  drupal_set_message("Updated: " . $node->title);
  if(count($fields)) {
    //populate audit trail
    $action['description'] = "";
    $action['activity'] = 6;
    _cals_importer_update_admin_audit($node,  $action);

  }
  else {
    //populate audit trail
    $action['description'] = "";
    $action['activity'] = 2;
    _cals_importer_update_admin_audit($node,  $action);
    $action['description'] = "";
    $action['activity'] = 3;
    _cals_importer_update_admin_audit($node,  $action);

  }

}



function _cals_reparse_record_details_form($settings, &$form_state) {
  $form = array();
  $fields = array();
  $arr = _cals_get_fields_repository_item();
  //$fields = field_info_instances("node", "repository_item");
  $map = $arr['map'];
  foreach($map as $k => $v) $options[$k] = $v['label'];

  $form['fields_to_parse'] = array(
    '#type' => 'select',
    '#title' => t('Select the Field(s) to reparse'),
    '#options' => $options,
    '#required' => TRUE,
    '#multiple' => TRUE,
    '#size' => count($options),
    '#description' => t("NB: this action will overwrite any edits added to the selected fields following the original parsing. Please exercise caution."),
    '#default_value' => isset($settings['settings']['hero']) ? $settings['settings']['hero'] : '',
  );
  return $form;
}

function _cals_reparse_record_details_submit($form, $form_state) {
  $return = array();
  $fields = $form_state['values']['fields_to_parse'];
  $fieldnames = implode(", ", $fields);
  $msg = "updated " . $node->title .": " . $fieldnames;
  drupal_set_message($msg);
  $return['fields_to_parse'] = $fields;
  return $return; //Note, return value here must be an array.
}
function _cals_reparse_record_details(&$node, $context) {
  $fields = $context['fields_to_parse'];
  _cals_parse_record_details($node, $fields);

  $message = t('Reparsing: %title %fields',
    array('%title' => $node->title, '%fields' => implode(", ", $fields),)
  );
  drupal_set_message($message);
}

/**
 * Iterates thru mapped fields to parse full node or selected fields
 *
 * @param $node
 *   the node passed to the parser
 * @param $xml
 *   the marcxml to be parsed
 * @param $map
 *   the field mappings
 * @param $op
 *   the operation, e.g., parse (all fields) or reparse (selected fields)
 *
 * @return
 *   $node
 *
 */
function _cals_importer_iterate_field_map(&$node, $xml, $map, $op) {

  $fc_fields = array(); // container for field collection array
  foreach($map as $k => $v) {
    $type = $v['type'];

    if(!empty($v['parser'])) {
      if(!isset($v['parser']['function'])) printAndDie("???", $v);
      $function = $v['parser']['function'];
      if(isset($v['parser']['tags'])) {
        $tags = $v['parser']['tags'];
        $function($xml, $k, $map, $tags);
      }
      else $function($xml, $k, $map);
      $node->{$k} = array();
      $key = ($type == 'taxonomy') ? "tid" : "value";
      if(is_array($map[$k]['values'])) {
        $values = $map[$k]['values'];
        foreach($values as $value) {
          if(!empty($value)) $node->{$k}[LANGUAGE_NONE][][$key] = $value;
        }
      }
      if( ($type != 'field_collection' && $type != 'taxonomy') || $k == 'field_title_uniform') {
        _cals_importer_normalize_node_field($node, $k);
      }
      if($type == 'field_collection') {
        _cals_importer_delete_field_collection($node, $k);
        $fc_fields[$k] = $map[$k];
      }
      if($k == 'title_field')  $node->title = $node->{$k}[LANGUAGE_NONE][0]['value'];

      if($k == 'title')  $node->title_field[LANGUAGE_NONE][0]['value'] = $node->title;
    }
  }
  if($op == 'parse') _cals_delete_duplicate_oclc_numbers($node);

  //save the populated node
  //$node->status = 1;
  node_save($node);
  //populate the field collections
  if(count($fc_fields)) _cals_importer_populate_repos_item_fieldcollections($node, $fc_fields);


}

/*
 *   foreach($map as $k => $v) {
    $type = $v['type'];

    if(!empty($v['parser'])) {
      if(!isset($v['parser']['function'])) printAndDie("???", $v);
      $function = $v['parser']['function'];

      if(isset($v['parser']['tags'])) {
        $tags = $v['parser']['tags'];
        $function($xml, $k, $map, $tags);
      }
      else $function($xml, $k, $map);

      //strip out existing values and repopulate
      if($k != 'title' && $type != 'field_collection') {
        $node->{$k} = array();
        $key = ($type == 'taxonomy') ? "tid" : "value";


        if(is_array($map[$k]['values'])) {
          $values = $map[$k]['values'];
          foreach($values as $value) {
            if(!empty($value)) $node->{$k}[LANGUAGE_NONE][][$key] = $value;
          }
        }
        //else dpm($map[$k]);
      }
      if($k == 'title') {
        $node->title = $map[$k]['values'][0];
      }
      if( ($type != 'field_collection' && $type != 'taxonomy') || $k == 'field_title_uniform') {
        _cals_importer_normalize_node_field($node, $k);
      }
      if($type == 'field_collection') {
        _cals_importer_delete_field_collection($node, $k);
        $fc_fields[$k] = $map[$k];

      }

    }
  }

 */