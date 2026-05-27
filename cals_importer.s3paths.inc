<?php

/**
 * @file
 * Updates s3 paths via batch update views
 */


/**
 * Sets up the batch process
 * 
 * @param $node
 *  
 */
function _cals_batch_update_s3_paths_by_node($node) {
  global $user;
  $arr = _cals_get_array_csv($node->nid);
  if (empty($arr)) {
    drupal_set_message(t("Problem parsing the file: can't access the file or generate an array. This is probably due to an issue reading / opening the file."), "error");
    drupal_goto("node/" . $node->nid);
  }
  else {
    $tmp = strpos($arr[0][1], "s3://"); //check first row to ensure well structured.
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
  ini_set('max_execution_time', 400); //~6.67 minutes, 1000 paths @ 2.5 a second

  $limit = 10; //10;
  // Get Count of products
  if (empty($context['sandbox']['progress']) ) {
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['max'] = count($arr); //count($pids);
    $context['sandbox']['arr'] = $arr;
    $context['sandbox']['current'] = 0;
    $context['sandbox']['node'] = node_load($nid);

    // Prune only orphaned rows so valid mappings can survive repeat parsing
    // while the admin iterates on unresolved entries.
    _cals_delete_s3_mapping($context['sandbox']['node']);
    
    //watchdog('import', 'update s3 paths');
    //dpm($arr);
  }
  $i = $context['sandbox']['progress'];
  $total = $i + 10;
  if ($total > $context['sandbox']['max']) $total = $context['sandbox']['max'];
  for ($i = $context['sandbox']['progress']; $i < $context['sandbox']['max']; $i++) {
 
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
  if ($nid2 > 0) {
      _cals_populate_s3_mapping($context['sandbox']['node'], $nid2, $path, $sysnum);
    }
    else {
      watchdog('cals_importer_s3paths', 'Skipping unresolved system control number @sysnum for record set @nid.', array(
        '@sysnum' => $sysnum,
        '@nid' => $context['sandbox']['node']->nid,
      ), WATCHDOG_NOTICE);
    }
    
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
  $existing = _cals_get_s3_mapping_by_sysnum($node, $sysnum);

  if ($existing) {
    $fc_item = $existing;
  }
  else {
    // Create new field collection item only when this sysnum has no mapping yet.
    $fc_item = entity_create('field_collection_item', array('field_name' => 'field_s3_mapping'));
    $fc_item->setHostEntity('node', $node);
  }

  $fc_item->field_system_control_number[LANGUAGE_NONE][0]['value'] = $sysnum;
  $fc_item->field_s3_path[LANGUAGE_NONE][0]['value'] = $path;
  $fc_item->field_nid_s3[LANGUAGE_NONE] = array();
  $fc_item->field_nid_s3[LANGUAGE_NONE][0]['nid'] = $nid;

  $fc_item->save(); // Save field-collection item.
  node_save($node);
  drupal_set_message( t("updated field collection: sysnum = @sysnum | nid = @nid", 
    array('@sysnum' => $sysnum, '@nid' => $nid) ) );

}

/**
 * Deletes all existing S3 mapping field collection items for a record set node.
 *
 * This keeps field_s3_mapping as a rebuilt projection of the current CSV.
 *
 * @param stdClass $node
 *   The s3_record_set node.
 */
function _cals_delete_s3_mapping($node) {
  if (empty($node->field_s3_mapping[LANGUAGE_NONE])) {
    return;
  }

  $deleted = FALSE;
  foreach ($node->field_s3_mapping[LANGUAGE_NONE] as $delta => $value) {
    if (empty($value['value'])) {
      continue;
    }
    $fci = field_collection_item_load($value['value']);
    if ($fci && _cals_s3_mapping_is_orphaned($fci)) {
      $fci->delete();
      $deleted = TRUE;
    }
  }
  if ($deleted) {
    node_save($node);
  }
}

/**
 * Returns the first S3 mapping field collection item for a given sysnum.
 *
 * @param stdClass $node
 *   The s3_record_set node.
 * @param string $sysnum
 *   The system control number to match.
 *
 * @return FieldCollectionItemEntity|null
 *   The existing field collection item when found.
 */
function _cals_get_s3_mapping_by_sysnum($node, $sysnum) {
  if (empty($node->field_s3_mapping[LANGUAGE_NONE])) {
    return NULL;
  }

  foreach ($node->field_s3_mapping[LANGUAGE_NONE] as $value) {
    if (empty($value['value'])) {
      continue;
    }
    $fci = field_collection_item_load($value['value']);
    if (!$fci || !$fci->hostEntity()) {
      continue;
    }
    if (!empty($fci->field_system_control_number[LANGUAGE_NONE][0]['value']) &&
      $fci->field_system_control_number[LANGUAGE_NONE][0]['value'] == $sysnum) {
      return $fci;
    }
  }

  return NULL;
}

/**
 * Determines whether an S3 mapping field collection item is orphaned.
 *
 * A mapping is considered orphaned when it does not point at a live repository
 * item nid, which makes it safe to prune on rebuild.
 *
 * @param FieldCollectionItemEntity $fci
 *   The field collection item to inspect.
 *
 * @return bool
 *   TRUE when the mapping should be pruned.
 */
function _cals_s3_mapping_is_orphaned($fci) {
  if (empty($fci->field_nid_s3[LANGUAGE_NONE][0]['nid'])) {
    return TRUE;
  }

  return !node_load($fci->field_nid_s3[LANGUAGE_NONE][0]['nid']);
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
  //$records = explode("\n", $records);
  $records = str_getcsv($records, "\n");
  foreach($records as $k => $v) {
    if($k > 0) $arr[] = str_getcsv($v, "\t"); 
  }
  return $arr;
  //NOTE TO SELF, why does this return here???
  
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
