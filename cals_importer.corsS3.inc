<?php

/**
 * @file
 * CALS Importer Cors s3.
 */



 

/**
 * Simple class to piggy back off AmazonS3StreamWrapper
 * 
 */
 
class NNELSStreamWrapper extends AmazonS3StreamWrapper {
  public function get_filesize() {
    $size = $this->getS3()->get_object_filesize($this->bucket, $this->getLocalPath($uri));
    return $size;
  }
 
  public function getS3v2() {
    return $this->s3;
  }
	public function list_objects() {
		$bucket = $this->bucket;
		//$bucket = 'download.nnels.ca';
		//$this->s3->enable_path_style();
	  return $this->s3->list_objects($bucket);
	}
}


/**
 * bulk operation action to update file-size values
 */

function _cals_run_batch_update_field_collection_s3_filesizes(&$entity, $context = array()) {
  if(!empty($entity->fid)) {
 		$file = file_load($entity->fid); //not sure this is required, could probably update the entity directly
  	_cals_update_filesize_from_s3_value($file);
  }
} 

/**
 * update the filesize value based
 *
 * @param $file
 */
function _cals_update_filesize_from_s3_value($file) {
	//instantiate class
	$wrapper = new NNELSStreamWrapper;
	$uri = $file->uri;
	$wrapper->setUri($uri);
	$filesize = $wrapper->get_filesize();
  //dpm($file);
	if( $filesize != $file->filesize || empty($file->filesize) ) {
		if(empty($filesize)) $filesize = 1;

	  $file->filesize = $filesize;
	  //file_save($file) overrides and resets filesize back to 0, so let's process as below:
    drupal_write_record('file_managed', $file, 'fid');
    module_invoke_all('file_update', $file);
    module_invoke_all('entity_update', $file, 'file');
    $msg =  ($filesize == 1)? t("S3 Object not found. Filesize set to 1 byte") : t("Filesize updated"); 

	}
	else {
    $msg = t("Not updated");
	}
	$msg .=  " (" . $file->filename . ": " . $file->filesize . ")";
	drupal_set_message($msg);
}


/**
 * Updates single field collection (file_resource) filesize values
 * 
 * @param $nid
 *   the node id
 * @param $item_id
 *   the field collection item id value
 */

function _cals_update_filecollection_file_filesize($node, $item_id) {

	$ent = entity_load_single('field_collection_item', $item_id);
	//printPre($ent->field_s3_file_upload['und'][0]['filename'], $ent->field_s3_path[LANGUAGE_NONE][0]['value']);
	
	//	  $ent->field_s3_to_cors_update[LANGUAGE_NONE][0]['value'] == 1  &&
	if(!empty($ent->field_s3_file_upload['und'][0]['filename']) && empty($ent->field_s3_file_upload['und'][0]['filesize'])){

		if(empty($ent->field_s3_path)) {
			$file = file_load($ent->field_s3_file_upload['und'][0]['fid']);
		}
		else {
		  $file = _cals_importer_get_file_from_filepath($ent->field_s3_path[LANGUAGE_NONE][0]['value']);
		}
		_cals_update_filesize_from_s3_value($file);
	}
	else {
	  dpm($ent);
	  return "oops nothing to update here!";
	}
	drupal_goto("node/" . $node->nid);
	

}



/**
 * import / create skeletal marc records, i.e., just the title
 *
 * @param $node
 *   the node passed to the parser
 */
function _cals_importer_batch_update_filesizes($node) {
  global $user;
	$wrapper = new NNELSStreamWrapper;
	$ObjectsListResponse = $wrapper->list_objects();
	$arr = array();
	$Objects = $ObjectsListResponse->body->Contents;
	foreach ($Objects as $Object) {
		printPre($Object->Key[0]);
	  $arr[] = array($Object[0]);
	 // printPre($Object->Key);
	}	
  $batch = array(
    'title' => t('Update Filesizes'),
    'operations' => array(array('_cals_importer_filesizes_batch', array($array)), //unfortunately, we can't pass xml in this way
    
    ),
    'progress_message' => t('Update. Operation @current out of @total.'),
    'error_message' => t('Error!'),
    'finished' => '_cals_importer_create_repo_items_finished',
  );
  //($batch);
  batch_set($batch);
  batch_process("node/" . $node->nid);
}  


/**
 * The main batch process to update filesizes
 *   
 * @param $node
 *   the node, i.e., the recordset node that contains the file being parsed
 *
 */
 
function _cals_importer_filesizes_batch($node, &$context) {
  //$limit = 400; //10;
  if (empty($context['sandbox']['progress']) ) {
  	
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['xml'] = ''; //$xml;
    $context['sandbox']['current'] = 0;
    $context['sandbox']['node'] = $node; //$node; // node_load($nid);
  	$file = drupal_realpath($node->field_marc_xml_file[LANGUAGE_NONE][0]['uri']); // FILE_FOLDER . ;
  	$xml = simplexml_load_file($file); //load the file
  	$context['sandbox']['xml'] = $xml;
  	$context['sandbox']['max'] = $xml->children(NAME_SPACE)->count();

  }
 // $i = $context['sandbox']['progress'];
  $total = $context['sandbox']['progress'] + $limit;
	$pnode = $context['sandbox']['node']; //parent node, i.e., the record set node
	$xml = $context['sandbox']['xml'];
	
  if ($total > $context['sandbox']['max']) $total = $context['sandbox']['max']; //reset total to max on final loop
  
  for ($i = $context['sandbox']['progress']; $i < $context['sandbox']['max']; $i++) {
  	$record = $xml->children(NAME_SPACE)->$i;
		
		//builds and saves the new node.	
		$n = _cals_build_skeleton_node($record, $pnode);

    $result = "Skeletal record added. ";
		
		 // Update our progress information.
    $context['sandbox']['progress']++;
    $context['results'][] = check_plain($n->title); // Track nodes updated
    $context['message'] = t('Now processing %node', array('%node' => $n->title));
		dpm($context['sandbox']['progress'] . " of " . $context['sandbox']['max'] . ": " . $n->title );


    if($context['sandbox']['progress'] === $context['sandbox']['max']) {
  		$pnode->field_status[LANGUAGE_NONE][0]['value'] = 2;
		  //save one last time.
		  node_save($pnode);
		  $num_records = count($context['sandbox']['parsed']);
 			dpm($context['sandbox']);
		  
		  /*
		  $vars = array("items" => $context['sandbox']['max'] , "title" => "parsed list: $num_records records", "type" => "ol", "attributes" => array());
		  $vars = theme_item_list($vars); 
		  dpm(filter_xss($vars, $allowed_tags = array('a', 'em', 'strong', 'cite', 'blockquote', 'code', 'ul', 'ol', 'li', 'dl', 'dt', 'dd')));
     */
    }
   

    
  }
	if ($context['sandbox']['progress'] != $context['sandbox']['max']) {
	  $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
	}
}
/**
 * Finish batch: create records
 */
function _cals_importer_filesizes_finished($success, $results, $operations) {
  if ($success) {
    $message = format_plural(count($results), 'One node updated', '@count nodes created / updated.');
  } 
  else {
    $message = t('Finished with an error.');
  }
  drupal_set_message($message);
  //watchdog('update', 's3 paths updated');
}


 
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


function _cals_update_filecollection_file($node, $item_id) {
	$ent = entity_load_single('field_collection_item', $item_id);
	
	//	  $ent->field_s3_to_cors_update[LANGUAGE_NONE][0]['value'] == 1  &&

	if(!empty($ent->field_s3_path) && empty($ent->field_s3_file_upload)){

		$file = _cals_importer_get_file_from_filepath($ent->field_s3_path[LANGUAGE_NONE][0]['value']);
		$file2 = (array)$file; //cast to array 
		$file2['display'] = 1; //set file to display
    try {
		  $ent->field_s3_file_upload[LANGUAGE_NONE][0] = $file2;
		  entity_save('field_collection_item', $ent);
      node_save($node);  //not certain if this is necessary?
    } catch (Exception $e) {
      drupal_set_message($e->getMessage());
      //watchdog('my_error', $e->getMessage());
    }
	}
	drupal_goto("node/" . $node->nid);

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
			dpm($file);
  	}
		//catch exception
		catch(Exception $e) {
		  drupal_set_message(t('Could not update @title due to the following issue: @msg', 
		    array('@title' => $node->title, @msg => $e->getMessage()) )) ;
		  return;
		}
    //dpm($file);
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
      drupal_set_message(t("Please check the following node(s): @candidates", array('@candidates' => $candidates) ));    
    }
    else {
    	drupal_set_message(t("Please update @title manually - uri currently exists in file managed table.", 
        array("@title" => l($node->title, "node/" . $node->nid))) );   
    }

    return '';
  }
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
			dpm($filesize);
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
  //$arr['fc_files']['field_performer']['values'][] = trim($val2);

  if(isset($arr['fc_files']['field_performer']))
   $fc_item->field_performer[LANGUAGE_NONE] = $arr['fc_files']['field_performer'];

  if(isset($arr['fc_files']['field_running_time'][0]))
    $fc_item->field_running_time[LANGUAGE_NONE] = $arr['fc_files']['field_running_time'];

  //hard-coded values (for now):
	$fc_item->field_availability_status[LANGUAGE_NONE][0]['value'] = 1;
  
  global $user;
  $acct = user_load($user->uid);
	$fc_item->field_user[LANGUAGE_NONE][0]['uid'] = $acct->uid;
  
  $fc_item->field_organization[LANGUAGE_NONE][0]['nid'] = ( isset($acct->field_organization[LANGUAGE_NONE]) ) ?
    $acct->field_organization[LANGUAGE_NONE][0]['nid'] : 15450;

  $fc_item->field_file_upload_details[LANGUAGE_NONE][0]['value'] = "Original collection reparsed";
  
  if(!empty($filepath)) $fc_item->field_s3_path[LANGUAGE_NONE][0]['value'] = $filepath;

  //printAndDie($fc_item->field_performer[LANGUAGE_NONE], $arr['fc_files']);

  $fc_item->save(); // Save field-collection item.

  node_save($node);
  drupal_set_message(t("updated s3 path: ") . l($node->title, "node/" . $node->nid) );
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
  //if (!strlen($xml)) return "no xml!";
  $arr = array();
	$xml = _cals_get_marc_xml_string($node);
  //skip this running times and performer updates.
  if($xml !== 0) { 
	  //get running time
	  _cals_importer_get_runningtime_from_marcxml($arr, $xml);
	  //get performer
	 	_cals_importer_get_performer_from_marcxml($arr, $xml);
	  //_cals_importer_get_performer_from_marcxml($xml, $k, $map); 
	  
	}
  return $arr;
}