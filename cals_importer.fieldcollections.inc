<?php

/**
 * @file
 * populates various field collections associated with MARC XML parsing operations
 */
 


/**
 * Populates Field Collection values
 *
 * @param $node
 *   the node passed to the parser
 * @param $fields
 *   array of field collection values
 *
 */ 
function _cals_importer_populate_repos_item_fieldcollections(&$node, $fields){
  foreach($fields as $field) {
    if(is_array($field['values']) && count($field['values']) > 0) {
      $values = $field['values'];

      foreach($values as $value) {
	      $e = entity_create('field_collection_item', $value);
	      // Attach the field_collection entity to the application node.
	      $e->setHostEntity('node', $node);
	      // Save the entity. 
	      $e->save();     
      }
    }
  }

}

/**
 * Deletes Field Collection values
 *
 * @param $node
 *   the node passed to the function
 * @param $field
 *   string value corresponding to the drupal field collection field
 *
 */ 

function _cals_importer_delete_field_collection(&$node, $field) {

	if (isset($node->{$field}[LANGUAGE_NONE]) ) {
		//collection field values for multiple deletion!
		foreach( $node->{$field}[LANGUAGE_NONE] as $v) {
		  //$ids[] = $v['value']; 
		  $fci = field_collection_item_load($v['value']);
		  if($fci) $fci->delete();
		}
		unset($node->{$field}[LANGUAGE_NONE]);    
	}

}



/*
           [0] => field_date
            [1] => field_dc_relation
            [2] => field_cataloguing_agency
            [3] => field_dc_description
            [4] => field_dc_coverage
            [5] => field_file_resource
            [6] => field_admin_audit
            [7] => field_commercial_sources
*/


/**
 * delete existing field collection items before resaving the node!
 * @ see also: http://drupal.stackexchange.com/questions/68765/how-to-properly-delete-a-field-collection
 */
function _cals_delete_existing_field_collection_items_TO_DEPRECATE($node, $keys) {
  $ids = array();
	
  foreach($keys as $field) {
    if (isset($node->{$field}[LANGUAGE_NONE]) ) {
      //collection field values for multiple deletion!
      foreach( $node->{$field}[LANGUAGE_NONE] as $v) {
        //$ids[] = $v['value']; 
        $fci = field_collection_item_load($v['value']);
        if($fci) $fci->delete();
      }
      unset($node->{$field}[LANGUAGE_NONE]);    
    }
    
  }
 
 // Delete field collection items. Commented out due to bug reported here: https://www.drupal.org/node/2384323
 //entity_delete_multiple('field_collection_item', $ids);
 return $node;


}
