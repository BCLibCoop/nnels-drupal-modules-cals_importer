<?php

/**
 * @file
 * Updates s3 paths via batch update views
 */



function _cals_batch_update_s3_paths_by_node($node) {

  global $user;
  $arr = _cals_get_array_csv($node->nid);
  //printAndDie($arr);
  if (empty($arr)) {
    drupal_set_message(t("Problem parsing the file: can't access the file or generate an array. This is probably due to an issue reading / opening the file."), "error");
    drupal_goto("node/" . $node->nid);
  }
  else {
    //check first row to ensure well structured.
    $tmp = strpos($arr[0][1], "s3://");
    if ($tmp === FALSE) {
    
	    drupal_set_message(t("Problem parsing the file: please check its structure, esp. the S3 paths."), "error");
      drupal_goto("node/" . $node->nid);
    }
  }

  $batch = array(
    'title' => t('Update S3 Paths'),
    //'operations' => array('_cals_run_batch_update_s3_path', array($nid, $arr) ),
    'operations' => array(array('_cals_run_batch_update_s3_path', array($node->nid, $arr)), 
    
    ),
    //'progress_message' => t('Update. Operation @current out of @total.'),
    'error_message' => t('Error!'),
    'finished' => '_cals_batch_update_s3_path_finished',
  );
	($batch);
  batch_set($batch);
  batch_process('node/' . $node->nid);

}
 
/**
 * Invokes Drupal's batch api to update S3 paths
 * 
 * @param $nid
 *   the node id
 */
 
function _cals_batch_update_s3_paths($nid) {
  global $user;
  $null_path = "admin/content/cals/s3-record-sets";
  $msg_file_str = t("Can't parse the S3 file. Please check its structure.");
  if (empty($nid)) drupal_goto($null_path); //printPre("HEY, empty!");
  
  //get array from csv file. Pass this into the initial batch
  $arr = _cals_get_array_csv($nid);
  
  
  if (empty($arr)) {
    drupal_set_message(t("Problem parsing the file: can't access the file or generate an array. This is probably due to an issue reading / opening the file."), "error");
    drupal_goto($null_path);
  }
  else {
    //check first row to ensure well structured.
    $tmp = strpos($arr[0][1], "s3://");
    if ($tmp === FALSE) {
    
	    drupal_set_message(t("Problem parsing the file: please check its structure, esp. the S3 paths."), "error");
      drupal_goto($null_path);
    }
  }
  
  //printAndDie($arr);
  
  /*
  foreach ($arr as $v) {
    $sysnum = $v[0];
    $nid = _cals_get_node_by_sysnum($sysnum);
    printPre($nid);
  
  }
  
  */
  $batch = array(
    'title' => t('Update S3 Paths'),
    //'operations' => array('_cals_run_batch_update_s3_path', array($nid, $arr) ),
    'operations' => array(array('_cals_run_batch_update_s3_path', array($nid, $arr)), 
    
    ),
    //'progress_message' => t('Update. Operation @current out of @total.'),
    'error_message' => t('Error!'),
    'finished' => '_cals_batch_update_s3_path_finished',
  );
    ($batch);
  batch_set($batch);
  batch_process('admin/content/cals/s3-record-sets');
}

/**
 * The main batch process to update S3 paths
 *   
 * @param $nid
 *   the node id
 * @param $arr
 *   the array of data from the csv attached to the S3 recordset
 *
 * @return
 *   $val, a string
 */
 
function _cals_run_batch_update_s3_path($nid, $arr, &$context) {
  $limit = 10; //10;
  // Get Count of products
  if (empty($context['sandbox']['progress']) ) {
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['max'] = count($arr); //count($pids);
    $context['sandbox']['arr'] = $arr;
    $context['sandbox']['current'] = 0;
    $context['sandbox']['node'] = node_load($nid);
   
   	//_cals_delete_s3_mapping($context['sandbox']['node']);
    
    //watchdog('import', 'update s3 paths');
    //dpm($arr);
  }
  $i = $context['sandbox']['progress'];
  $total = $i + 10;
  if ($total > $context['sandbox']['max']) $total = $context['sandbox']['max'];
  for ($i = $context['sandbox']['progress']; $i < $total; $i++) {
 
    $current_row = $context['sandbox']['progress'];
    $data = $context['sandbox']['arr'][$current_row];
    
    //$bibid = $data[0];
    $sysnum = $data[0];
    $path = $data[1];

    $nid2 = _cals_get_node_by_sysnum($sysnum);
    
    if ($nid2 > 0) {
      $paths = array();
      $node = node_load($nid2);
      if (isset($node->field_s3_path[LANGUAGE_NONE]) && count($node->field_s3_path[LANGUAGE_NONE]) > 1) {
        foreach ($node->field_s3_path[LANGUAGE_NONE] as $k => $v) {
          $paths[] = $v['value'];
        }
      }
      $paths[] = $path;
      $paths = array_unique($paths);
      unset($node->field_s3_path[LANGUAGE_NONE]); //reset this variable so we can dedupe with unique values
      foreach ($paths as $path) {
        $node->field_s3_path[LANGUAGE_NONE][]['value'] = $path;
      }
      //$node->field_bibid[LANGUAGE_NONE][0]['value'] = $bibid;
      node_save($node);
      //drupal_set_message(t("%cr{$current_row}. updated node " . l($nid2, "node/" . $nid2) . ": " . $node->title);
      $context['results'][] = check_plain($node->title); // Track nodes updated
      $context['message'] = "updated node " . $node->nid;
    }
    else {
      drupal_set_message(t("%c. could not find %s"), array('%c' => $current_row, '%s' => $sysnum));
    }
		_cals_populate_s3_mapping($context['sandbox']['node'], $nid2, $path, $sysnum);
    
    $context['sandbox']['progress']++; // Used to keep track of rows processed
    //array_shift($context['sandbox']['arr']);
  }
    
 

  if ($context['sandbox']['progress'] != $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }
}

 
/**
 * Creates and populates new field collection entity, saves node.
 * 
 * @param $node
 *   the S3 recordset node
 * @param $nid
 *   the nid of the repository item being referenced
 * @param $path
 *   the s3 path
 * @param $sysnum
 *   the system number, i.e., the field is used to match the s3 path to the repos item
 */

function _cals_populate_s3_mapping($node, $nid, $path, $sysnum) {

  // Create new field collection item.
  $fc_item = entity_create('field_collection_item', array('field_name' => 'field_s3_mapping')); 

  // Attach it to the node.
  $fc_item->setHostEntity('node', $node); 

  $fc_item->field_system_control_number[LANGUAGE_NONE][0]['value'] = $sysnum;
  $fc_item->field_s3_path[LANGUAGE_NONE][0]['value'] = $path;
  if($nid > 0) $fc_item->field_nid_s3[LANGUAGE_NONE][0]['nid'] = $nid;

  $fc_item->save(); // Save field-collection item.
  node_save($node);
	drupal_set_message("updated field collection: $sysnum: " . $nid);

}
/**
 * Finish batch
 */
function _cals_batch_update_s3_path_finished($success, $results, $operations) {
  if ($success) {
    $message = format_plural(count($results), 'One node updated', '@count nodes updated.');
  } 
  else {
    $message = t('Finished with an error.');
  }
  drupal_set_message($message);
  //watchdog('update', 's3 paths updated');
}


/**
 * Reads CSV file; returns array
 *
 * @param $nid
 *   The node id with the CSV attachment
 *
 * @return
 *   an array.
 */
function _cals_get_array_csv($nid) {
  $node = node_load($nid);


  $records = $node->field_s3_paths[LANGUAGE_NONE][0]['value'];
  $arr = array();
	//printAndDie($arr);
	//$records = explode("\n", $records);
	$records = str_getcsv($records, "\n");
	foreach($records as $k => $v) {
	  if($k > 0) $arr[] = str_getcsv($v, "\t");	
	}
	return $arr;
  
  

  //printAndDie($records, $arr);
 	/* */
  $file = drupal_realpath($node->field_file_csv[LANGUAGE_NONE][0]['uri']);
  
  $row = 1;   
  $arr = array();
  if (($handle = fopen($file, "r")) !== FALSE) {
    //while (($data = fgetcsv($handle, 1000, "\t")) !== FALSE) {
    while (($data = fgetcsv($handle, 1000, ",")) !== FALSE) { 
      if ($row > 1 ) {
        $arr[] = $data;
      }
      $row++;
    }
    fclose($handle);
  }
  return $arr;
  
  
}

/**
 * Reads CSV file; returns array
 *
 * @param $file
 *   The full filepath
 *
 * @return
 *   an array.
 */

function _cals_get_array_csv_by_file($file) {

  $row = 1;   
  $arr = array();
  if (($handle = fopen($file, "r")) !== FALSE) {
    //while (($data = fgetcsv($handle, 1000, "\t")) !== FALSE) {
    while (($data = fgetcsv($handle, 1000, ",")) !== FALSE) { 
      if ($row > 1 ) {
        $arr[$data[0]] = $data[1];
      }
      $row++;
    }
    fclose($handle);
  }
  
  
  return $arr;
}



/**
 * TO DEPRECATE: Updates the s3 paths
 */
function _cals_load_s3_file($start, $limit) {

  $nid = arg(4);
  $file = drupal_get_path('module', "cals_importer") . "/20130717-cals-systemnumber-file-all-dave.csv";
  $arr = array();
  $row = 1;
  $num_updated = 0;
  if (($handle = fopen($file, "r")) !== FALSE) {
      while (($data = fgetcsv($handle, 1000, "\t")) !== FALSE) {
        if ($row > 1) {
          $arr[] = $data;
          $bibid = $data[0];
          $sysnum = $data[1];
          $path = $data[2];
          $nid = _cals_get_node_by_sysnum($sysnum);
          if ($nid > 0) {
            $node = node_load($nid);
            //
            //$node->field_system_control_number[LANGUAGE_NONE][]['value'] = $sysnum;
            
            $node->field_s3_path[LANGUAGE_NONE][]['value'] = $path;
            $node->field_bibid[LANGUAGE_NONE][0]['value'] = $bibid;
            node_save($node);
            $num_updated++;
            //drupal_set_message("saved {$node->title}: $path");
          }

        }
        $row++;
      }
      fclose($handle);
      return "updated {$num_updated}";
  }
  else {
    return "can't find file??";
  
  }

}

function _cals_update_s3_paths() {

  //require_once(drupal_get_path('module', "cals_importer") . "/cals_importer.mappings.inc");
  $file = drupal_get_path('module', "cals_importer") . "/20130717-cals-systemnumber-file-all-dave.csv";
  $arr = array();
  $row = 1;
  $num_updated = 0;

  if (($handle = fopen($file, "r")) !== FALSE) {
      while (($data = fgetcsv($handle, 1000, "\t")) !== FALSE) {
        if ($row > 1) {
          $arr[] = $data;
          $bibid = $data[0];
          $sysnum = $data[1];
          $path = $data[2];
          $nid = _cals_get_node_by_sysnum($sysnum);
          if ($nid > 0) {
            $node = node_load($nid);
            //
            //$node->field_system_control_number[LANGUAGE_NONE][]['value'] = $sysnum;
            
            $node->field_s3_path[LANGUAGE_NONE][]['value'] = $path;
            $node->field_bibid[LANGUAGE_NONE][0]['value'] = $bibid;
            node_save($node);
            $num_updated++;
            //drupal_set_message("saved {$node->title}: $path");
          }

        }
        $row++;
      }
      fclose($handle);
      return "updated {$num_updated}";
  }
  else {
    return "can't find file??";
  
  }



}


