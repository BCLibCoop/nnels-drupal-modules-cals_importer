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
  _cals_parse_record_details($node);

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
      _cals_parse_record_details($node);
      drupal_set_message(t("%t MARC XML file parsed and saved."), array('%t' => $node->title));
    }
    else {
      drupal_set_message(t("Invalid selection: not a repos item"));
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
function _cals_get_fields_repository_item() {
  $fields = field_info_instances("node", "repository_item");
	//set title as first array element
  $map = array(
   "title" => array(
     'type' => 'text',
     'required' => 1,
     'values' => '',
     'parser' => array("function" => '_cals_get_title_from_marc'),
   ),
  );

	$tmp = array(
	  "_autocomplete", "_with_summary", "_hidden", "_buttons", "_select", "_textfield", "_embed", "_textarea",
  );

  foreach($fields as $k => $v) {
    $type = $v['widget']['type'];
    $t2 = $type;
    foreach($tmp as $strip) $t2 = str_replace($strip, '', $t2);
    $req = $v['required'];
    $map[$k] = array(
      'type' => $t2,
      'required' => $req,
      'values' => '',
      'parser' => _cals_importer_repo_field_callback($k),
    );
    //set a default if defined
    $map[$k]['default'] = (isset($req) && isset($v['default_value'][0]['value'])) ? $v['default_value'][0]['value'] : '';
    $types[$t2][] = $k;
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
		//'title_field' => array('function' => '_cals_get_title_from_marc'),  
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
        array('tag' => 800, 'code' => 'a', 'qualifier' => "type", 'enum' => "IsPartOf"),
				array('tag' => 830, 'code' => 'a', 'qualifier' => "type", 'enum' => "IsPartOf"),
				array('tag' => 760, 'code' => 'a', 'qualifier' => "type", 'enum' => "IsPartOf"),
				array('tag' => 530, 'code' => 'a', 'qualifier' => "type", 'enum' => "IsFormatOf"),
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
		//'field_system_control_number' => array('function' => '_cals_importer_get_system_numbers_from_marc'),
		
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
	switch($field) {
	  case "title":
	  case "title_field":
			_cals_importer_strip_square_brackets($node, $field);
			_cals_importer_normalize_title($node);
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
function _cals_parse_record_details($node) {
	$xml = _cals_get_marc_xml_string($node);
	if($xml === 0) return;
  //create an array of taxonomies to be used in parsing field collections, etc.
  foreach (taxonomy_get_vocabularies() as $k => $v) $arr_tax[$v->name] = $k;
  $arr_fields = _cals_get_fields_repository_item();
  $map = $arr_fields['map'];
  //if (!strlen($xml)) return "no xml!";
  $arr = array();
  
  $fc_fields = array(); // container for field collection array

  //loop thru map to populate node
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

	    //strip out existing values and repopulate
	    if($k != 'title' && $type != 'field_collection') {
	      $node->{$k} = array();
	      $key = ($type == 'taxonomy') ? "tid" : "value";
	      
	      
	      if(isset($map[$k]['values'])) {
		      $values = $map[$k]['values'];
		      foreach($values as $value) $node->{$k}[LANGUAGE_NONE][][$key] = $value;
	      }
	    }
	    if($type != 'field_collection' && $type != 'taxonomy') {
	      _cals_importer_normalize_node_field($node, $k);
	    }
	    if($type == 'field_collection') {
	      _cals_importer_delete_field_collection($node, $k);
	      $fc_fields[$k] = $map[$k];
	    }
	    
    }
  }
  _cals_delete_duplicate_oclc_numbers($node);
  //save the populated node
  $node->status = 1;
  node_save($node);
  //populate the field collections
  //_cals_importer_populate_repos_item_fieldcollections($node, $arr_tax, $arr['fc']);
  _cals_importer_populate_repos_item_fieldcollections($node, $fc_fields);

  //populate audit trail
	$action['description'] = "";
  $action['activity'] = 2;
  _cals_importer_update_admin_audit($node,  $action);
	$action['description'] = "";
  $action['activity'] = 3;
  _cals_importer_update_admin_audit($node,  $action);

}