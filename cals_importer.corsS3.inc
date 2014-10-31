<?php

/**
 * @file
 * CALS Importer Cors s3.
 */
 
/**
 * Invokes Drupal's batch api to update existing s3 paths into the managed file table
 * 
 * @param $nid
 *   the node id
 */
 
 function _cals_importer_update_cors_file_table($node) {
  _cals_importer_populate_field_file_resource($node);
  drupal_goto("node/" . $node->nid);
 }  


 
/**
 * The main batch process to update S3 paths to Field Collections
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
      _cals_importer_populate_field_file_resource($node, $filepath);
      //drupal_set_message("Invalid selection: not a repos item");
    }
    else{
      drupal_set_message("Invalid selection: not a repos item");
    }

  }//end main if
 
  return "";

}


function _cals_update_filecollection_file($node) {

  printAndDie($node);


}


/**
 * Creates new file, if not in the database
 * 
 * @param $node
 *   the node
 * @param $filepath
 *   the s3 filepath
 */
function _cals_importer_s3path_to_file_updater($nid, $item_id) {
  global $user;
		$link = t("No file available");
		if($user->uid == 1) {
			dpm($nid);
			dpm($item_id);

			$entity = entity_load('field_collection_item', array($item_id));
			dpm($entity->field_s3_path['und'][0]['safe_value']);
		  if(empty($entity->field_s3_file_upload)) {
		    $link .= ": " .  l("update file attachment from S3 path", "admin/%node/update-file-upload-from-field-collection");
		    //$file = _cals_importer_get_file_from_filepath($entity->field_s3_path['und'][0]['safe_value']);
		  	//printPre($file);
			  dpm($file);
		  }
			
		}
		/*
	  if($user->uid == 1 & isset($row->field_field_s3_path[0]['raw']['value']) && 
	  	empty($row->file_managed_field_data_field_s3_file_upload_fid) ) {
	  	
	  	$link .= ": " . "ADD FILE THINGIE HERE??";
	  	
	  }	
    */
	return $link;
}
/**
 * Creates new file, if not in the database
 * 
 * @param $node
 *   the node
 * @param $filepath
 *   the s3 filepath
 */
function _cals_importer_get_file_from_filepath($filepath) {

  $filepath = str_replace("download.nnels.ca/", "", $filepath);
  $filepath = str_replace("download2.nnels.ca/", "", $filepath);

  //We need to check if the current file exists in the file_managed table
  //drush sqlq "select * from file_managed limit 0, 100"
  $test = db_query('SELECT n.fid FROM {file_managed} n WHERE n.uri = :uri', array(':uri' => $filepath));
  
  
  if($test->rowCount()>0){
  
  	foreach($test as $row) {
  	  $file = file_load($row->fid);
			if(is_object($file)) return $file;
  	}
  }
  //$filepath = "s3://foo";
  // Create managed File object and associate with zip. Not currently in use, but leave intact for now
  if(!empty($filepath)) {
    $file = (object) array(
      'uid' => 1,
      'uri' => $filepath,
      'filename' => $filepath, 
      'filemime' => "application/zip",    //application/zip or multipart/x-zip
      'status' => 1,
      'display' => 1
    );
    try {
			file_save($file);
  	}
		//catch exception
		catch(Exception $e) {
		  drupal_set_message(t('Could not update @title due to the following issue: @msg', 
		    array('@title' => $node->title, @msg => $e->getMessage()) )) ;
		  return;
		}
    
    
   	return $file;  
  }



}

/**
 * Creates new file, if not in the database
 * 
 * @param $node
 *   the node
 * @param $filepath
 *   the s3 filepath
 */
function _cals_importer_file_updater($node, $filepath) {

  $filepath = str_replace("download.nnels.ca/", "", $filepath);
  $filepath = str_replace("download2.nnels.ca/", "", $filepath);

  //We need to check if the current file exists in the file_managed table
  //drush sqlq "select * from file_managed limit 0, 100"
  $test = db_query('SELECT n.fid FROM {file_managed} n WHERE n.uri = :uri', array(':uri' => $filepath));
  
  
  if($test->rowCount()>0){
  
  	foreach($test as $row) {
  	  $file = file_load($row->fid);

			if(is_object($file)) return $file;

  	}
    $test2 = db_query("select distinct fr.entity_id as nid 
        from field_data_field_s3_file_upload fu 
        	inner join file_managed fm on fu.field_s3_file_upload_fid = fm.fid
        	inner join field_data_field_file_resource fr on fu.entity_id = fr.field_file_resource_value
        where fm.uri = :uri", 
        	array(':uri' => $filepath));


    $candidates = array(); 
    foreach($test2 as $row) {
      $candidates[] = l("node/{$row->nid}", "node/{$row->nid}");
      printPre($row);
    }  
    $candidates = implode(", ", $candidates);
    if(strlen($candidates)) {
      drupal_set_message(t("Please check the following node(s): @candidates", array('@candidates' =>$candidates) ));   
			$node->field_duplicate_managed_file[LANGUAGE_NONE][0]['value'] = 1;
	    node_save($node);
 
    }
    else {
    	drupal_set_message(t("Please update @title manually - uri currently exists in file managed table.", 
        array("@title" => l($node->title, "node/" . $node->nid))) );   
    }
    return 0;
  }
  //$filepath = "s3://foo";
  // Create managed File object and associate with zip. Not currently in use, but leave intact for now
  if(!empty($filepath)) {
    $file = (object) array(
      'uid' => 1,
      'uri' => $filepath,
      'filename' => $filepath, 
      'filemime' => "application/zip",    //application/zip or multipart/x-zip
      'status' => 1,
      'display' => 1
    );
    try {
			file_save($file);
  	}
		//catch exception
		catch(Exception $e) {
		  drupal_set_message(t('Could not update @title due to the following issue: @msg', 
		    array('@title' => $node->title, @msg => $e->getMessage()) )) ;
		  return;
		}
    
    
   	return $file;  
  }



}

 
/**
 * Creates and populates new field collection entity, saves node.
 * 
 * @param $node
 *   the node
 * @param $filepath
 *   the s3 filepath
 */
 //$filepath
function _cals_importer_populate_field_file_resource($node) {

  $filepath = '';
  if(!empty($node->field_s3_path[LANGUAGE_NONE]) ) $filepath = $node->field_s3_path[LANGUAGE_NONE][0]['value'];

  $arr = _cals_importer_parse_marcxml_cors3($node);

  // Create new field collection item.
  $fc_item = entity_create('field_collection_item', array('field_name' => 'field_file_resource')); 

  // Attach it to the node.
  $fc_item->setHostEntity('node', $node); 
  
  $filepath = str_replace("download.nnels.ca/", "", $filepath);
  $filepath = str_replace("download2.nnels.ca/", "", $filepath);

  //We need to check if the current file exists in the file_managed table
  //drush sqlq "select * from file_managed limit 0, 100"
  $test = db_query('SELECT n.fid FROM {file_managed} n WHERE n.uri = :uri', array(':uri' => $filepath));
  if($test->rowCount()>0){
    $test2 = db_query("select distinct fr.entity_id as nid 
        from field_data_field_s3_file_upload fu inner join file_managed fm on fu.field_s3_file_upload_fid = fm.fid 
        inner join field_data_field_file_resource fr on fu.entity_id = fr.field_file_resource_value
        where fm.uri = :uri", array(':uri' => $filepath));

    $node->field_duplicate_managed_file[LANGUAGE_NONE][0]['value'] = 1;
    node_save($node);

    $candidates = array(); 
    foreach($test2 as $row) {
      $candidates[] = l("node/{$row->nid}", "node/{$row->nid}");
    }  
    $candidates = implode(", ", $candidates);
    if(strlen($candidates)) {
      drupal_set_message(t("Please check the following node(s): @candidates", array('@candidates' =>$candidates) ));    
    }
    else {
    	drupal_set_message(t("Please update @title manually - uri currently exists in file managed table.", 
        array("@title" => l($node->title, "node/" . $node->nid))) );   
    }

    return '';
  }
  //$filepath = "s3://foo";
  // Create managed File object and associate with zip. Not currently in use, but leave intact for now
  if(!empty($filepath)) {
    $file = (object) array(
      'uid' => 1,
      'uri' => $filepath,
      'filename' => $filepath, 
      'filemime' => "application/zip",    //application/zip or multipart/x-zip
      'status' => 1,
      'display' => 1
    );
    try {
			file_save($file);
  	}
		//catch exception
		catch(Exception $e) {
		  drupal_set_message(t('Could not update @title due to the following issue: @msg', 
		    array('@title' => $node->title, @msg => $e->getMessage()) )) ;
		  return;
		}
    //$fc_wrapper = entity_metadata_wrapper('field_collection_item', $fc_item);
    $fc_item->field_s3_file_upload[LANGUAGE_NONE][0]  =  (array)$file;
  
  }

  $fc_item->field_user[LANGUAGE_NONE][0]['value'] = 1;
  //$fc_item->field_performer[LANGUAGE_NONE][0]['value'] = "TEST";
  if(isset($arr['fc_files']['field_performer'][0])) {
    $fc_item->field_performer[LANGUAGE_NONE] = $arr['fc_files']['field_performer'];
  }

  if(isset($arr['fc_files']['field_running_time'][0])) {
    $fc_item->field_running_time[LANGUAGE_NONE] = $arr['fc_files']['field_running_time'];
  }
  
  //hard-coded values (for now):
  
  $fc_item->field_availability_status[LANGUAGE_NONE][0]['value'] = 1;
  
  $fc_item->field_organization[LANGUAGE_NONE][0]['nid'] = 15450;
  $fc_item->field_user[LANGUAGE_NONE][0]['uid'] = 1;
  $fc_item->field_file_upload_details[LANGUAGE_NONE][0]['value'] = "Original collection reparsed";
  
  if(!empty($filepath)) $fc_item->field_s3_path[LANGUAGE_NONE][0]['value'] = $filepath;
  $fc_item->save(); // Save field-collection item.

  node_save($node);
  drupal_set_message(t("updated s3 path: @title", array('@title' => l($node->title, "node/" . $node->nid) )));



}

/**
 * Populates Field Collection values
 *
 * @param $node
 *   the node passed to the parser
 * @param $arr_tax
 *   taxonomy values
 * @param $arr
 *   array of field collection values
 *
 * @return
 *   an array. 
 *
 */ 
function _cals_importer_populate_fieldcollection_file_resource( $node, $arr_tax, $arr ) {
  //field_file_resource
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

    
      default:
        printAndDie($k, $v);
        break;
    
      
    
    }
    
    if (count($values)) {
      foreach ($values as $value) {
        if (is_array($value)) {
          $e = entity_create('field_collection_item', $value);
          // Attach the field_collection entity to the application node.
          $e->setHostEntity('node', $node);
          // Save the entity. 
          $e->save(); 
        }
      }
    }   
      

  }
  
    

}


/**
 * parses the original marcxml string to return array of values related to cors multi-value field collection
 *
 * @param $node
 *   the node passed to the parser
 *
 * @return
 *   an array. 
 *
 */ 
 
function _cals_importer_parse_marcxml_cors3($node) {

  //create an array of taxonomies to be used in parsing field collections, etc.
  foreach (taxonomy_get_vocabularies() as $k => $v) {
    $arr_tax[$v->name] = $k;
  } 
  
  //if (!strlen($xml)) return "no xml!";
  $arr = array();
	$xml = _cals_get_marc_xml_string($node);
  //skip this running times and performer updates.
  if($xml !== 0) { 
	  //get running time
	  _cals_importer_get_runningtime_from_marcxml($arr, $xml);
	  //get performer
	  _cals_importer_get_performer_from_marcxml($arr, $xml);
	}

 
  return $arr;
    
}