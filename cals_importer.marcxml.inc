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
  $map = array();
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
      'value' => '',
      'parser' => _cals_get_parsing_function_repo_item($k),
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

function _cals_get_parsing_function_repo_item($fieldname) {

  $arr = array(
  
    "field_dc_description" => array('function' => '_cals_get_descriptors_from_marc'),
    "field_subject" => array("function" => '_cals_get_subject_from_marc'   ),
    "field_genre" => array("function" => '_cals_get_genre_from_marc'), 
    "field_isbn" => array('function' => '_cals_importer_get_isbn_sysnums_from_marc'),
    'field_performer' => array('function' => '_cals_importer_get_performer_from_marcxml'),
    
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
    'field_rights_holder' => array('function' => '_cals_importer_get_basic_values_from_marc', 
      "tags" => array(
        array('tag' => 542, 'code' => 'd'),
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
    'field_rights_holder' => array('function' => '_cals_importer_get_basic_values_from_marc', 
      "tags" => array(
        array('tag' => 542, 'code' => 'd'),
      )
    ),
    'field_uri' => array('function' => '_cals_importer_get_basic_values_from_marc', 
      "tags" => array(
        array('tag' => 856, 'code' => 'u'),
      )
    ),
    
    //relations
    

    "field_dc_relation" => array('function' => '_cals_importer_get_basic_values_from_marc', 
      "tags" => array(
        array('tag' => 800, 'code' => 'a', 'qualifier' => "type", 'enum' => "IsPartOf"),
				array('tag' => 830, 'code' => 'a', 'qualifier' => "type", 'enum' => "IsPartOf"),
				array('tag' => 760, 'code' => 'a', 'qualifier' => "type", 'enum' => "IsPartOf"),
				array('tag' => 530, 'code' => 'a', 'qualifier' => "type", 'enum' => "IsFormatOf"),
      )
    ),
    
    
    
  );
  
  if(array_key_exists($fieldname, $arr)) return $arr[$fieldname];
  
  return '';  


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
  foreach (taxonomy_get_vocabularies() as $k => $v) {
    $arr_tax[$v->name] = $k;
  } 
  
  $arr_fields = _cals_get_fields_repository_item();
  $map = $arr_fields['map'];
  //if (!strlen($xml)) return "no xml!";
  $arr = array();
  
  //set file access to "Open Access" 
  // NEW, as of 2014-03-12: this will be defined by the parent record set
  //$arr['repos']['field_access_restrictions'][0] = 0;
  
  //let's grab date issued
  $date_issued =  _cals_get_date_issued($xml);
  
  if (!empty($date_issued)) $arr['fc']['field_date'][] = 
    array("field_qualifier_date" => "Issued", "field_dc_date" => $date_issued);
  
  
  foreach($map as $k => $v) {
    $type = $v['type'];
    
    
    if($k != 'title' && $type != 'field_collection') $node->{$k} = array();
  
  
    if(!empty($v['parser'])) {
      
      $function = $v['parser']['function'];
      if($k != 'xfield_subject') {
        $params = $v['parser']['params'];
      
       //$params = implode(", ", $params);
        if($function == '_cals_importer_get_basic_values_from_marc') {
          
          $tags = $v['parser']['tags'];
					

          $function($xml, $k, $map, $tags);
        }
        else {
          $function($xml, $k, $map);
        }
        
        
      
      }
    
    }
    
  
  }

	printAndDie($map, $node);

    
  //descriptions
  _cals_get_descriptors_from_marc($arr, $xml);
  
  //isbns
  _cals_importer_get_isbn_sysnums_from_marc($arr, $xml);

  //grabs some of the generic fields, e.g., titles, uri, publisher number, etc.
  _cals_get_commom_fields_from_marc($arr, $xml);

  //subjects
  //_cals_get_subject_from_marc($arr, $node, $xml, $vid = $arr_tax['Subject'], $map);
  
  //genre
  _cals_get_genre_from_marc($arr, $xml);
  
  //language
  _cals_importer_get_isolanguage_from_marcxml($arr, $xml);

  //get the rest of the fields by looping thru the record
  foreach ($xml->children(NAME_SPACE) as $record) {
    _cals_importer_get_array_from_marcxml($record, $arr);
  }
  
  $node->title = _cals_get_truncated_value($arr['repos']['title'], 250);    

  //newly ingested records are set to un-published. Leave status unchanged. 
  //$node->status = 0;
  
  _cals_delete_duplicate_oclc_numbers($node);
  
  //delete the field collections so we have a fairly stripped down node if reparsing
  $node = _cals_delete_existing_field_collection_items($node, array_keys($arr['fc']));
	
  printAndDie($map, $arr);

	
  foreach ($arr['repos'] as $k => $v) {

    if (is_array($v)) {
      //clean up existing values so we don't append
      $node->{$k}[LANGUAGE_NONE] = array();
      //populate fields
      foreach ($v as $k2 => $v2) {
        if (strlen($v2)) $node->{$k}[LANGUAGE_NONE][]['value'] = $v2;
      }
    }
    else {
    	switch($k) {
    	  case "title":
    	    $node->{$k} = $v;
    	    break;
    	  default:
    	    $node->{$k}[LANGUAGE_NONE][]['value'] = $v;
    	    break;
    	}
    }
		//normalize fields
		switch($k) {
		  case "title":
				_cals_importer_strip_square_brackets($node, $k);
				_cals_importer_normalize_title($node);
		  	break;
		  case "field_dc_creator":
			case "field_dc_contributor":	
				_cals_importer_strip_square_brackets($node, $k);
				_cals_importer_normalize_creators($node, $k);
		  	break;
		  case "field_publisher":
				//_cals_importer_strip_square_brackets($node, "title");
		  	_cals_importer_normalize_publisher($node);
		  	break;
		}
  } 

  //genre / subjects
  if (array_key_exists("repos_tf", $arr) ) {
  	$node->field_genre[LANGUAGE_NONE] = array();
  	//printAnddie($arr);
  	if(isset($arr['repos_tf']['field_genre'])) {
	    foreach($arr['repos_tf']['field_genre'] as $k => $v) {
	      $vid = $arr_tax['Genre'];
	      $name = $v['value'];
	      $tid = _cals_get_tid_by_name($vid, $name);
	      $node->field_genre[LANGUAGE_NONE][$k]['tid'] = $tid;
	    }
  	
  	}
  	$node->field_subject[LANGUAGE_NONE] = array();
  	if(isset($arr['repos_tf']['field_subject'])) {
	    asort($arr['repos_tf']['field_subject']);
	    foreach($arr['repos_tf']['field_subject'] as $k => $v) {
	      $vid = $arr_tax['Subject'];
	      $tid = _cals_get_tid_by_name($vid, $v);
	      $node->field_subject[LANGUAGE_NONE][$k]['tid'] = $tid;
	    }
    }
  }
  //printAndDie($node->field_subject, $arr['repos_tf']['field_subject']);
	//printAndDie($arr['repos_tf'],$node->field_subject);
  //save the populated node
  $node->status = 1;
  node_save($node);
  
  
  //populate the field collections
  _cals_importer_populate_repos_item_fieldcollections($node, $arr_tax, $arr['fc']);
  
  //populate audit trail
	$action['description'] = "";
  $action['activity'] = 2;
  _cals_importer_update_admin_audit($node,  $action);
	$action['description'] = "";
  $action['activity'] = 3;
  _cals_importer_update_admin_audit($node,  $action);

}