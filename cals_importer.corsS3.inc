<?php

/**
 * @file
 * CALS Importer Cors s3.
 */



 

/**
 * Simple class to piggy back off AmazonS3StreamWrapper
 * 
*/
/*
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

*/
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
    module_load_include('inc', 'cals_s3', 'cals_s3.NNELSStreamWrapper.class');
	$wrapper = new \Drupal\cals_s3\NNELSStreamWrapper;
	$uri = $file->uri;
	$wrapper->setUri($uri);
	$filesize = $wrapper->get_filesize($wrapper->getUri());
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
    module_load_include('inc', 'cals_s3', 'cals_s3.NNELSStreamWrapper.class');
    $wrapper = new \Drupal\cals_s3\NNELSStreamWrapper;
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
  $limit = 400; //10;
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
		//dpm($context['sandbox']['progress'] . " of " . $context['sandbox']['max'] . ": " . $n->title );


    if($context['sandbox']['progress'] === $context['sandbox']['max']) {
  		$pnode->field_status[LANGUAGE_NONE][0]['value'] = 2;
		  //save one last time.
		  node_save($pnode);
		  $num_records = count($context['sandbox']['parsed']);

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
      'display' => 1,
      'type' => 'document',
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
  if(!empty($node->field_s3_path[LANGUAGE_NONE]) ) $filepath = trim($node->field_s3_path[LANGUAGE_NONE][0]['value']);

    // Bucket is now baked into the file_managed path by Field File Path
    // module with differing environment defaults.
    // E.g. dev : nnels-data-dev, prod : nnels-data-prod
    $filepath = str_replace("download.nnels.ca/", "nnels-data-prod/",
      $filepath);
    $filepath = str_replace("download2.nnels.ca/", "nnels-data-dev/",
      $filepath);

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

  $arr = _cals_importer_parse_marcxml_cors3($node);
  if(!empty($node->field_record_set)) {
    $rec_nid = $node->field_record_set[LANGUAGE_NONE][0]['nid'];
    _cals_importer_get_recordset_values_s3($rec_nid, $arr);
  }

  // Create new field collection item.
  $fc_item = entity_create('field_collection_item', array('field_name' => 'field_file_resource'));

  // Attach it to the node.
  $fc_item->setHostEntity('node', $node);


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

  //hard-coded values (for now):
  $fc_item->field_user[LANGUAGE_NONE][0]['value'] = 1;
  $fc_item->field_availability_status[LANGUAGE_NONE][0]['value'] = 1;

  //set values from file resource field callbacks
  $resource_fields_to_process = array(
    'field_performer',
    'field_running_time',
    'field_file_format',
    'field_accessibility_tags',
    'field_accessibility_summary',
    'field_certified_accessible_by',
  );

  foreach ( $resource_fields_to_process as $field ) {
    if ( count($arr['fc_files'][$field]) > 0 ) $fc_item->$field[LANGUAGE_NONE]
      = $arr['fc_files'][$field];
  }

  /** begin administrative metadata */

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

function _cals_importer_get_recordset_values_s3($nid, &$arr) {
  $node = node_load($nid);
  if(!empty($node->field_file_format))
    $arr['fc_files']['field_file_format'] = $node->field_file_format[LANGUAGE_NONE];
  if(!empty($node->field_access_restrictions))
    $arr['fc_files']['field_access_restrictions'] = $node->field_access_restrictions[LANGUAGE_NONE];
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
	  _cals_importer_get_running_time_from_marcxml($arr, $xml);
	  //get performer
      _cals_importer_get_performer_from_marcxml($arr, $xml);
	  //_cals_importer_get_performer_from_marcxml($xml, $k, $map);
      //get accessibility tags and summary
      _cals_importer_get_a11y_metadata($arr, $xml);
	}
  return $arr;
}

function _cals_mend_broken_file_resource_s3_paths_form($settings,
  &$form_state) {

  $form = array();

  $options = array(
    0 => t('Local Path Only'),
    1 => t('Full Mode')
  );

  $form['mend_mode'] = array(
    '#type' => 'radios',
    '#title' => t('Select Mend Mode'),
    '#required' => TRUE,
    '#description' => t("Local Path Only cleans faulty object path or prefix in Drupal only.
    <br><br>Full Mode attempts all methods on field_s3_path, file resource and upstream S3 object"),
    '#options' => $options,
  );

  return $form;
}


/**
 * @param $form
 * @param $form_state
 * @return array
 */

function _cals_mend_broken_file_resource_s3_paths_submit($form, &$form_state) {
  $returnable = array();
  $returnable['mode'] = $form_state['values']['mend_mode'];
  return $returnable;
}

/**
* The main batch process to amend broken S3 paths in File Resources
*
 * @param $nid
*   the node id
*
 * @return
 *   $val, a string
*/
function _cals_mend_broken_file_resource_s3_paths(&$entity, $context = array()) {

  if ($entity->type == 'repository_item') {
    foreach ( entity_metadata_wrapper('node', $entity)->field_file_resource->getIterator() as $delta => $wrapped) {
      full_mend_s3_path( $wrapped );
    }
  }
  elseif ($entity->type == 'field_collection_item') {
    $wrapped = entity_metdata_wrapper('field_collection_item', $entity);
    full_mend_s3_path( $wrapped );
    }
  elseif (property_exists($entity, 'filesize') ) {
    fix_s3_path( $entity );
  }
  return TRUE;
}

/**
 * @param $entity
 * Previously, removed the endpoint name (s3.libraries.coop) from S3 Path
 * which would sometimes occur. Now this seems deprecated against AWS S3,
 * although reverse problems occurs where bucket is now needed explicitly.
 */
function fix_s3_path( $entity ) {

  //Find the FC item entity associated with that file
  $query = new \EntityFieldQuery();
  $query->entityCondition('entity_type', 'field_collection_item')
    ->propertyCondition('field_name', 'field_file_resource')
    ->fieldCondition('field_s3_file_upload', 'fid', $entity->fid);
  $results = $query->execute();
  //Proceed with results
  if (isset($results['field_collection_item']) ) {
    //Load the entity and wrap
    $loaded_resource = entity_metadata_wrapper('field_collection_item',
      entity_load_single('field_collection_item',
        key($results['field_collection_item'])));
    //Fix it's field_s3_path first
    $pattern = '/s3:\/\/[a-z3.]+libraries.coop\//';
    //@todo Ensure server's default bucket gets inserted where there is NO
    // match in S3 path?
    //$pattern = '/s3:\/\/nnels-data-dev|prod\//';
    $loaded_resource->field_s3_path->set(
      preg_replace($pattern, "s3://", $loaded_resource->field_s3_path->value())
    );
    $loaded_resource->save();
    //Amend the filename, uri in the File entity directly
    $entity->filename = preg_replace($pattern, "s3://",
      $entity->filename);
    $entity->uri = preg_replace($pattern, "s3://",
      $entity->uri);
  } else {
    drupal_set_message("Aborted. File ID: $entity->fid is not associated with a field_file_resource instance");
  }
}

  /**
   * Clean urlencoded and problematic chars from the S3 Path for both the
   * file_managed entity and resource in S3
   * @param \EntityDrupalWrapper $wrapped
   */

function full_mend_s3_path( $wrapped ) {

  //Remove URL-encoded strings
  $fixed_s3 = preg_replace("/\%[0-9ABCDEF]{2}/", "",
    $wrapped->field_s3_path->value() );

  //Prepare problem punctuation
  $replacements = array(
    '—' => '-',
    '―' => '-',
    "'" => '',
    "’" => '',
  );
  //Replace
  $fixed_s3 = str_replace(array_keys($replacements), $replacements, $fixed_s3);

  //Map French-accented chars to unadorned equivalents due to S3-signing fail
  $fixed_s3 = html_entity_decode(
                preg_replace(
                  '~&([a-z]{1,2})(
                    ?:acute|cedil|circ|grave|lig|orn|ring|slash|th|tilde|uml
                    );~i',
                  '$1',
                  htmlentities($fixed_s3, ENT_QUOTES, 'UTF-8')
                ),
                ENT_QUOTES,
                'UTF-8'
              );

  $wrapped->field_s3_path->set($fixed_s3);
  $file = file_load((int) $wrapped->field_s3_file_upload->file->getIdentifier());
  $file->filename = $fixed_s3;

  //Effect changes on file entity and S3 object
  module_load_include('inc', 'cals_s3', 'cals_s3.NNELSStreamWrapper.class');
  $stream = new \Drupal\cals_s3\NNELSStreamWrapper;
  $stream->setUri(urldecode($file->uri));
  $stream->rename($stream->getUri(), $fixed_s3);

  //Finalize and save
  $file->uri = $fixed_s3;
  file_save($file);
  $wrapped->save();
}