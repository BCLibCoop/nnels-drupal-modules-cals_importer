<?php

/**
 * Simple wrapper to call main parser; if preferred, can invoke batch api instead. 
 */

function _cals_importer_update_repository_item($node) {

	//_cals_parse_record_details($node);
	//drupal_set_message("{$node->title} MARC XML file parsed and saved.");
	
	//can also push this thru the batch api instead. To do so, comment out above and uncomment
	//the following function call.
	
	printAndDie($node);
	_cals_importer_update_repository_items($node);

	//take user back to main node page
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
	    drupal_set_message("{$node->title} MARC XML file parsed and saved.");
		}
	  else {
			drupal_set_message("Invalid selection: not a repos item");
	  }

  }//end main if
 
	return "";
  
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

	$xml = '';
	if(!isset($node->field_xml_string[LANGUAGE_NONE]) ) return ''; //get out of here if not xml string

	if(isset($node->field_xml_string[LANGUAGE_NONE])) 
	  $xml = _cals_get_xml_string($node->field_xml_string[LANGUAGE_NONE][0]['value']);

	//create an array of taxonomies to be used in parsing field collections, etc.
	foreach(taxonomy_get_vocabularies() as $k => $v) {
		$arr_tax[$v->name] = $k;
	} 

	//if(!strlen($xml)) return "no xml!";
	$arr = array();
	
	//set file access to "Open Access"
	$arr['repos']['field_access_restrictions'][0] = 0;
	//let's grab date issued
	$date_issued =  _cals_get_date_issued($xml);
	
	if(!empty($date_issued)) 
	  $arr['fc']['field_date'][] = array("field_qualifier_date" => "Issued", "field_dc_date" => $date_issued);
	//descriptions
	_cals_get_descriptors_from_marc($arr, $xml);
	
	//isbns
	_cals_get_isbn_sysnums_from_marc($arr, $xml);

		//grabs some of the generic fields, e.g., titles, uri, publisher number, etc.
	_cals_get_commom_fields_from_marc($arr, $xml);

	//subjects
	_cals_get_subject_from_marc($arr, $xml);
	
	//genre
	_cals_get_genre_from_marc($arr, $xml);
	

	//get the rest of the fields by looping thru the record
	foreach($xml->children(NAME_SPACE) as $record) {
		_cals_get_array_from_xml($record, $arr);
	}
	
  $node->title = _cals_get_truncated_value($arr['repos']['title'], 250);				

	//delete the field collections so we have a fairly stripped down node if reparsing
	$node = _cals_delete_existing_field_collection_items($node, array_keys($arr['fc']));
	$node->status = 1;
	
	
	foreach($arr['repos'] as $k => $v) {

		if(is_array($v)) {
			//clean up existing values so we don't append
			$node->{$k}[LANGUAGE_NONE] = array();
			
			//populate fields
			foreach($v as $k2 => $v2) {
				if(strlen($v2)) $node->{$k}[LANGUAGE_NONE][]['value'] = $v2;
			}
		}
		else {
			($k == "title") ? $node->{$k} = $v : $node->{$k}[LANGUAGE_NONE][]['value'] = $v; 
		}

	} 

	//genre
	if(array_key_exists("repos_tf", $arr) ) {
		foreach($arr['repos_tf']['field_genre'] as $k => $v) {
			$vid = $arr_tax['Genre'];
			$name = $v['value'];
			$tid = _cals_get_tid_by_name($vid, $name);
			$node->field_genre[LANGUAGE_NONE][$k]['tid'] = $tid;
		}
	
	}
	//save the populated node
	node_save($node);
	
	//populate the field collections
	_cals_add_entity_values2($node, $arr_tax, $arr['fc']);
	
}