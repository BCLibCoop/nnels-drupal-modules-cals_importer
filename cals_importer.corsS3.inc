<?php


/**
 * Invokes Drupal's batch api to update existing s3 paths into the managed file table
 * 
 * @param $nid
 *   the node id
 */
 
 function _cals_importer_update_cors_file_table($node) {

	//printAndDie($node->field_s3_path);
	/*
	$filepath = 'http://s.libraries.coop/download.nnels.ca/downloads/1401740666/97ab1897a9938200dac05724d28b8221/River_God_Chivers_Edition_S9164.zip';
	$filepath = 'http://s.libraries.coop/download.nnels.ca/downloads/1401740666/97ab1897a9938200dac05724d28b8221/River_God_Chivers_Edition_S9164.zip';	
	
	$filepath = 'http://s3://download.nnels.ca/AU/WA/ABWA/restricted/S9164/River_God_Chivers_Edition_S9164.zip';
	
	$filepath = 'http://s.libraries.coop/download.nnels.ca/AU/WA/ABWA/restricted/S9164/River_God_Chivers_Edition_S9164.zip';
	
	*/
	$filepath = $node->field_s3_path[LANGUAGE_NONE][0]['value'];
	if(!empty($filepath)) _cals_importer_update_field_collection_field_file_resource($node, $filepath);
	drupal_goto("node/" . $node->nid);

   
 
 }
/**
 * Creates and populates new field collection entity, saves node.
 * 
 * @param $node
 *   the node
 * @param $filepath
 *   the s3 filepath
 */
function _cals_importer_update_field_collection_field_file_resource($node, $filepath) {

		// Create new field collection item.
		$fc_item = entity_create('field_collection_item', array('field_name' => 'field_file_resource')); 

		// Attach it to the node.
		$fc_item->setHostEntity('node', $node); 
		
		$filepath = str_replace("download.nnels.ca/", "", $filepath);
		
		//printAndDie($filepath);
		//$filepath = "s3://foo";
		// Create managed File object and associate with zip. Not currently in use, but leave intact for now
		$file = (object) array(
		  'uid' => 1,
		  'uri' => $filepath,
		  'filename' => $filepath, //str_replace("s3://", "", $filepath),
		  'filemime' => "application/zip", 		//application/zip or multipart/x-zip
		  'status' => 1,
		  'display' => 1
		);
		
		//printAndDie($file, $node->field_file_resource);
		file_save($file);
		 
		//$fc_wrapper = entity_metadata_wrapper('field_collection_item', $fc_item);
		$fc_item->field_s3_file_upload[LANGUAGE_NONE][0]  =  (array)$file;
		$fc_item->field_user[LANGUAGE_NONE][0]['value'] = 1;
		$fc_item->field_performer[LANGUAGE_NONE][0]['value'] = "TEST";
		$fc_item->field_s3_path[LANGUAGE_NONE][0]['value'] = $filepath;
		$fc_item->save(); // Save field-collection item.
		node_save($node);
		drupal_set_message(t("updated s3 path: ") . $node->title);

 }


 
/**
 * The main batch process to update S3 paths in Field Collections
 *	 
 * @param $nid
 *   the node id
 *
 * @return
 *   $val, a string
 */
 
function _cals_run_batch_update_field_collection_s3_path(&$entity, $context = array()) {
  if (isset($entity->nid)) {
    $node = node_load($entity->nid);
    //safety check to make sure we're parsing repos items!!    
    $content_types = array('repository_item');
    
    if (in_array($node->type, $content_types)  ) {
    	$filepath = $node->field_s3_path[LANGUAGE_NONE][0]['value'];
      _cals_importer_update_field_collection_field_file_resource($node, $filepath);
      //drupal_set_message("Invalid selection: not a repos item");
    }
    else{
      drupal_set_message("Invalid selection: not a repos item");
    }

  }//end main if
 
  return "";

}