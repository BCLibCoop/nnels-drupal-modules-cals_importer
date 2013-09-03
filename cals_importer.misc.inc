<?php
function _cals_entity_wrapper($arr) {
	
	$node = new EntityDrupalWrapper('node', 1);

	$node->field_reference[0]->set(463); // Note that this is a multi value field.
	$node->field_text->set('Some Value'); // Note that this is a single value field.


	//$node->save();



}

function _cals_get_entity_value($key, $arr, $arr_tax) {

	$values = array();
	
	
	
	foreach($arr as $k => $v) {
	
		switch($key) {
		
		
			case "field_date":
				if(!isset($dates) ) {
				
					$dates[$v['field_qualifier_date']] = 0;
					
				}
				
				$dates[$v['field_qualifier_date']]++;
				$allow = ($dates[$v['field_qualifier_date']] == 1) ? TRUE : FALSE;

				//"field_qualifier_date" => "Issued", "field_dc_date"
				
				if($allow) {
				
				
					$values[] = array(
					  'field_name' =>  "field_date",
					  'field_dc_date' => array(
					    LANGUAGE_NONE => array(array('value' => $v['field_dc_date']) ),
					  ),
					  'field_qualifier_date' => array(
					    LANGUAGE_NONE => array(array('value' => $v['field_qualifier_date']) ),
					  ),
					  
					);
			
				}
			
				break;
		
			case "field_dc_relation":
			
				$values[] = array(
				  'field_name' =>  "field_dc_relation",
				  'field_dc_relation_value' => array(
				    LANGUAGE_NONE => array(array('value' => $v['value']) ),
				  ),
				  'field_dc_relation_qualifiers' => array(
				    LANGUAGE_NONE => array(array('value' => $v['type']) ),
				  ),
				  
				);
				
				break;
		
			case "field_dc_subject":
			
				
			
				$vid = $arr_tax['Subject'];
				$name = $v['value'];
			// Get the term's ID.
				$tid = _cals_get_tid_by_name($vid, $name);
			
				$me = array(
				  'field_name' => 'field_dc_subject',
  			);
				$me['field_dc_subject_value'][LANGUAGE_NONE][]['tid'] = $tid;
				if(isset($v['scheme']) && $v['scheme'] != 'undefined' && $v['scheme'] != '') {
					$me['field_dc_subject_scheme'][LANGUAGE_NONE][]['value'] = strtoupper($v['scheme']);
				}
				$values[] = $me;
				//printPre($values);
				break;
		
			case "description":
						
				$values[] = array(
				  'field_name' => 'field_dc_description',
				  'field_dc_description_type' => array(
				    LANGUAGE_NONE => array(array('value' => $v['type'])),
				  ),
				  'field_value' => array(
				    LANGUAGE_NONE => array(array('value' => $v['value'])),
				  ),
				  
				);

				//printAndDie($values);		
				break;
			case "field_dc_coverage":
						
				$values[] = array(
				  'field_name' => 'field_dc_description',
				  'field_dc_coverage_type' => array(
				    LANGUAGE_NONE => array(array('value' => $v['field_dc_coverage_type'])),
				  ),
				  'field_marc_tag' => array(
				    LANGUAGE_NONE => array(array('value' => $v['field_marc_tag'])),
				  ),
				  'field_dc_coverage_scheme' => array(
				    LANGUAGE_NONE => array(array('value' => $v['field_dc_coverage_scheme'])),
				  ),
				  'field_value' => array(
				    LANGUAGE_NONE => array(array('value' => $v['field_value'])),
				  ),
				);
 
				
				
				
								
				break;
		
			case "field_cataloguing_agency":
			
				if(strlen($v['field_agency_type'])) {
				
					$values[] = array(
					  'field_name' => $key,
					  'field_value' => array(
					    LANGUAGE_NONE => array(array('value' => $v['value'])),
					  ),
						'field_agency_type' => array(
					    LANGUAGE_NONE => array(array('value' => $v['field_agency_type'])),
					  ),
					);
				}
				else {
					$values[] = array(
					  'field_name' => $key,
					  'field_value' => array(
					    LANGUAGE_NONE => array(array('value' => $v['value'])),
					  ),
					);
				}	

				break;
		
		}
	
		
	
	
	}


	return $values;


}

/**
 * delete existing field collection items before resaving the node!
 * @ see also: http://drupal.stackexchange.com/questions/68765/how-to-properly-delete-a-field-collection
 */
function _cals_delete_existing_field_collection_items($node,$keys) {
	$ids = array();

	foreach($keys as $field) {
		if(isset($node->{$field}[LANGUAGE_NONE]) ) {
			//collection field values for multiple deletion!
			foreach( $node->{$field}[LANGUAGE_NONE] as $v) {
				$ids[] = $v['value'];	
			}
			unset($node->{$field}[LANGUAGE_NONE]);		
			//printPre($node->{$field});
		
		}
		
	}
	// Delete field collection items.
 
 	if(count($ids)) entity_delete_multiple('field_collection_item', $ids);
	return $node;


}

function _cals_add_entity_values2( $node, $arr_tax, $arr ) {

	foreach($arr as $k => $v) {
	
		$values = array();


		//run thru a switch so we can isolate fields that still
		//need to be worked on. Later, can remove switch?	
		switch($k) {
		
			case "field_dc_subject":
			case "field_dc_relation":
			case "field_cataloguing_agency":
			case "field_dc_description":
			case "field_dc_coverage":
			case "field_date":

				$values = _cals_get_entity_value($k, $v, $arr_tax);
				//printPre("ya", $v);
				break;
			
					
		
			case "field_datex":
			
				printAndDie($arr);
				break;
		
			default:
				printAndDie($k, $v);
				break;
		
			
		
		}
		if($k != 'field_dc_subjectx') {
			if(count($values)) {
				foreach ($values as $value) {
					if(is_array($value)) {
						//printPre($n->title, $k, $value);

						$e = entity_create('field_collection_item', $value);
						// Attach the field_collection entity to the application node.
						$e->setHostEntity('node', $node);
						// Save the entity. 
						$e->save();	
					
					}
				
				}
			//printPre($k,$values);	
			}		
			
		}
		else {
			//$nn = node_load(92);
			//$e = entity_load('field_collection_item', array(212));
			//printAndDie($values, $e, $nn);
		
		
		}
		

		
	
	
		
	
	}
	
	//printAndDie("here", $arr);
		

}


function _cals_deprecated_yyy() {

		$arr2 = array("subject", "cat_agency", "description");
			//$arr2 = array("subject");
			
			foreach($arr2 as $key) {
			
				//printAndDie($arr2, $arr);
				if(isset($arr[$key])) {
				
					foreach($arr[$key] as $k => $v) {
						_cals_add_entity_values($n, $key, $v, $arr_tax);
					}
				}
				
			}
			

}

 
function _cals_add_entity_values($node, $fieldname, $v, $arr_tax) {
	$values = array();
	$n2 = node_load(47);
	//printAndDie($n2);
	
	switch($fieldname) {
		case "description":
			$values = array(
			  'field_name' => 'field_dc_description',
			  'field_dc_description_type' => array(
			    LANGUAGE_NONE => array(array('value' => $v['type'])),
			  ),
			  'field_value' => array(
			    LANGUAGE_NONE => array(array('value' => $v['value'])),
			  ),
			  
			);
			//$values['field_dc_description_type2'][LANGUAGE_NONE][0]['value'] = $v['type'];
			//$values['field_value2'][LANGUAGE_NONE][0]['value'] = $v['value'];
			 
			 
			//$values['field_dc_description_type'][LANGUAGE_NONE][$delta][]
			
			
			//printAndDie($values);
			break;
			
		case "cat_agency":
			$values = array(
				'field_name' => "field_cataloguing_agency",
	 
			);
			
			$values['field_value'][LANGUAGE_NONE][]['value'] = "yahoo";
		
			break;	
			
		case "type":
			$vid = $arr_tax['Genre'];
			$name = $v['value'];
			
			$arr['type'][] = array("type" => "genre", "scheme" => "lcsh", "value" => "Black humor (Literature)");

			
			$values = array(
			  'field_name' => 'field_dc_subject',
			  'field_dc_subject_value' => array(
			    LANGUAGE_NONE => array(array('tid' => $tid)),
			  ),  

			  
			);
			
			break;
		case "subject":
		
			$vid = $arr_tax['Subject'];
			$name = $v['value'];
			// Get the term's ID.
			$tid = _cals_get_tid_by_name($vid, $name);
			

			$values = array(
			  'field_name' => 'field_dc_subject',
			  'field_dc_subject_value' => array(
			    LANGUAGE_NONE => array(array('tid' => $tid)),
			  ),  

			  
			);
			
			if(isset($v['scheme']) && $v['scheme'] != 'undefined' && $v['scheme'] != '') {
				$values['field_dc_subject_scheme'][LANGUAGE_NONE]['value'] = $v['scheme'];
			}
			break;
	
 
	
	
	
	
	}//end switch

	$e = entity_create('field_collection_item', $values);
	
	  // Attach the field_collection entity to the application node. This has to
	  // happen separately so that it actually works -- you can't just specify
	  // this via the $values array.
	
	//printAndDie($node);  
	  
	$e->setHostEntity('node', $node);

	
	  // Save the entity. Since it's attached to the right application node, this
	  // will both create the field_collection entity and update the application
	  // node to point to the new field_activity_log record.
	$e->save();				
	/*
	$delta = 2;			
	$node->field_dc_description[LANGUAGE_NONE][$delta]['value'] = $entity->item_id;
	$node->field_dc_description[LANGUAGE_NONE][$delta]['revision_id'] = $entity->revision_id;
	*/
	
}

function _cals_get_nid_by_title($title) {
	$type = "repository_item";
	
	$result = db_query(
		"SELECT n.nid FROM {node} n WHERE n.title = :title AND n.type = :type", 
		array(":title"=> $title, ":type"=> $type));  
	
	$nid = $result->rowCount();
	if($nid > 0) $nid = $result->fetchField();
 
	return $nid;

}


function _cals_get_taxonomy_vid($name) {
	return db_query("SELECT vid FROM {taxonomy_vocabulary} WHERE machine_name = :name", 
		array(":name" => $name))->fetchField();

}

function _cals_get_tid_by_name($vid, $name) {
	//$name = $v['value'];
	// Get the parent term's ID.
  $tid = db_query(
  	"SELECT tid FROM {taxonomy_term_data} 
  	WHERE vid = :vid and name = :name
  	ORDER BY tid DESC LIMIT 1", 
  	array(':vid' => $vid, ':name' => $name)
  )->fetchField();
  
	//if no tid, save current value
	if(!is_numeric($tid) ) {
		$term = (object) array(
			'name' => $name,
			'vid' => $vid
		);
		taxonomy_term_save($term);
		return $term->tid;
	
	}
	
	return $tid;

}

function _cals_get_iso_date($value) {
	$value = (string) $value;
	
	$value = str_replace(array(".", "c"), "", $value);
	//$value = str_replace("c", "", $value);
	
	
	return $value;

}

function _cals_get_description_type($tag) {

	$tmp =	array( 
		500 => '', 
		//504 => "Bibliography",
		505 => "tableofcontents",
		520 => 'abstract',
		/*
		511 => 'Performer',
		516 => 'Type of file',
		
		533 => 'Reproduction',
		538 => 'System Details / Requirements',
		586 => 'Awards Notes', 
		595 => 'Extent of Original', 
		598 => 'Keywords, Local',
		599 => 'Local Notes',
		541 => 'Accrual Method',
		*/
	);
	
	if(array_key_exists($tag, $tmp)) return $tmp[$tag];
	return "";




}


function _cals_get_keyword_index_values($arr, $entries) {


	//printPre($entries, $arr);
	foreach($entries as $entry) {
		//$entry = _cals_get_truncated_value($entry, 250);
		if( strlen(trim($entry) ) && (!in_array($entry, $arr) ) ) {
			$arr[] = trim($entry);
		}
	}
	return $arr;

}
function _cals_get_truncated_value($value, $length) {
	if(strlen($value) > $length) return trim(substr($value, 0, $length) . "...");
	return trim($value);
}

function _cals_get_subject_value($value, $ind) {
	$tmp = array(
		0 => "lcsh", 
		1 => "LC Children's Subject heading",
		2 => "MeSH",
		4 => "undefined",
		6 => "Repertoire des vedettes-matiere",
		8 => "Sears",
	
	);
	$value = _cals_get_truncated_value($value, 250);
	if (array_key_exists($ind, $tmp)) {
		return array("scheme" => $tmp[$ind], "value" => (string) $value);
	} 
	return array("scheme" => "undefined", "value" => (string) $value);

}