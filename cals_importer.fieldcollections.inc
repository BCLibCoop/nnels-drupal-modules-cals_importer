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
            // Only loads a value for field_file_resource type FCs
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

/**
 * @param $settings
 * @param $form_state
 * @return mixed
 */

function _cals_transfer_file_resource_entity_form($settings, &$form_state) {

  $options = array();

  foreach ($form_state['selection'] as $key => $nid) {
    $node_entity = entity_metadata_wrapper('node', $nid);
    $files = $node_entity->field_file_resource->value();
    $destinations[$nid] = $nid . ' - ' . $node_entity->title->value();

    foreach ($files as $file) {
      $fc_entity = entity_metadata_wrapper('field_collection_item', $file);
      $fields = $fc_entity->getPropertyInfo();
      $format = $fc_entity->field_file_format->label();
      $item_id = $fc_entity->item_id->value();

      //    $field_count = count($fields);
      //    $total = 0;
      //      foreach ($fields as $field) {
      //        if ( $fc_entity->$field->value() ) {
      //          ++$total;
      //        }
      //      }

      $options[$item_id] = sprintf('NID: %d - %s - ItemID: %d', $nid, $format,
        $item_id);
    }
  }

  $form['file_resources'] = array(
    '#type' => 'select',
    '#title' => t('Select the File Attachment to transfer '),
    '#options' => $options,
    '#required' => TRUE,
    '#multiple' => TRUE,
    '#size' => count($options),
    '#description' => t("NB: MSG"),
    '#default_value' => isset($options[0]) ? $options[0] : '',
  );

  $form['destination_node'] = array(
    '#type' => 'select',
    '#title' => t('Select the destination Repository Item'),
    '#options' =>  $destinations,
    '#required' => TRUE,
    '#size' => count($destinations),
    '#description' => t("NB: MSG"),
    '#default_value' => isset($destinations) ? reset($destinations) : NULL,
  );

  return $form;
}

/**
 * @param $form
 * @param $form_state
 * @return array|bool
 */

function _cals_transfer_file_resource_entity_submit($form, $form_state) {
  $return = array();

  $return['file_resources'] = $form_state['values']['file_resources'];
  $return['destination_node'] = $form_state['values']['destination_node'];

  //Do not attempt a same-entity transfer
  $selection_source_host = entity_load_single('field_collection_item',
    $form_state['values']['file_resources']);
  if ($selection_source_host->hostEntity()->nid == $return['destination_node']) {
    drupal_set_message("Cannot transfer File Resource to the Repository Item from which it originates.",
      'error');
    drupal_goto(check_url($_SERVER['REDIRECT_URL'])); //Go back to the record
    // set node path
    return FALSE;
  }

  return $return;
}

/**
 * @param stdClass $node
 * @param array $context
 * @return boolean
 */

function _cals_transfer_file_resource_entity(&$node, $context) {

  //Only run this once for the group of selected nodes
  if ($context['progress']['current'] > 1) {
    return FALSE;
  }

  //Forbidden Drupal Entity Fields
  $forbidden = array(
    'item_id',
    'uuid',
    'url',
    'feed_nid',
    'feed_node',
    'feeds_item_guid',
    'feeds_item_url',
    'host_entity',
    'revision_id',
    'field_name',
    'archived'
  );

  //Load Destination Node from $context
  $dest_node = node_load($context['destination_node']);

  //Iterate over the selected transferrable file resources one-to-many
  foreach ($context['file_resources'] as $transferrable_id) {
    $transfer_fc_wrapped = entity_metadata_wrapper('field_collection_item',
      $transferrable_id);
    $source_node_wrapped = entity_metadata_wrapper('node', $transfer_fc_wrapped->host_entity->value());

    //Get the possible fields from the File Resource field collection,
    // excepting those that cannot be written except during construction.
    $fields = $transfer_fc_wrapped->getPropertyInfo();
    foreach ($forbidden as $forbid) {
      unset($fields[$forbid]);
    }

    //Find our File Resource by item_id in situ
    foreach ($source_node_wrapped->field_file_resource->value() as $delta =>
             $item) {
      //Check for a match on this round
      if ($item->item_id == $transferrable_id) {
        //The transfer target entity was found
        //Create a new File Resource instance, set to Destination Node & wrap-it
        $new_entity = entity_create('field_collection_item', array('field_name' => 'field_file_resource'));
        $new_entity->setHostEntity('node', $dest_node);
        $new_entity_wrapped = entity_metadata_wrapper('field_collection_item',
          $new_entity);

        //Loop through allowed fields and extract the values
        foreach ($fields as $index => $field) {
          try {
            $new_entity_wrapped->$index->set($transfer_fc_wrapped->$index->value
            ());
          } catch (Exception $exception) {
            drupal_set_message($exception->getMessage(), 'error');
          }
        }
        //All done: save Destination Node with new entity referenced.
        node_save($dest_node);

        //Unset the old instance to avoid duplicates
        unset($source_node_wrapped->field_file_resource[$delta]);
        entity_save($source_node_wrapped->type(), $source_node_wrapped);
      }
    }

    //Might not have format set for certain items
    try {
      $format = $transfer_fc_wrapped->field_file_format->label();
    } catch (EntityMetadataWrapperException $exception) {
      $format = NULL;
      watchdog('entity_api', $exception->getMessage());
    }

    //Build results array for message output
    $result_info = array(
      $transfer_fc_wrapped->item_id->value(),
      $format,
      substr($source_node_wrapped->title->value(), 0, 30),
      $source_node_wrapped->nid->value(),
      substr($dest_node->title, 0, 30),
      $dest_node->nid
    );
    $message = vsprintf("Item ID %d (%s) was transferred from %s... (%d) \r\n to
  %s... (%d)", $result_info);

    drupal_set_message($message);
  } //End selected $transferrable foreach
}