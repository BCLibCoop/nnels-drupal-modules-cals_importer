<?php

/**
 * Updates the s3 paths
 */
function _cals_load_s3_file($start, $limit) {

	$nid = arg(4);
	$file = drupal_get_path('module', "cals_importer") . "/20130717-cals-systemnumber-file-all-dave.csv";
	$arr = array();
	$row = 1;
	$num_updated = 0;
	if (($handle = fopen($file, "r")) !== FALSE) {
	    while (($data = fgetcsv($handle, 1000, "\t")) !== FALSE) {
				if($row > 1) {
					$arr[] = $data;
					$bibid = $data[0];
					$sysnum = $data[1];
					$path = $data[2];
					$nid = _cals_get_node_by_sysnum($sysnum);
					if($nid > 0) {
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
				if($row > 1) {
					$arr[] = $data;
					$bibid = $data[0];
					$sysnum = $data[1];
					$path = $data[2];
					$nid = _cals_get_node_by_sysnum($sysnum);
					if($nid > 0) {
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


