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
 * @ see also: http://drupal.stackexchange.com/questions/68765/how-to-properly-delete-a-field-collection
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

/**
 * Deletes field collection values by entity_id
 * @param $entity_id
 */
function _cals_importer_delete_field_collection_by_id($entity_id) {
	entity_delete('field_collection_item', $entity_id);
}