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

  //printAndDie($node->field_s3_path);
  /*
  $filepath = 'http://s.libraries.coop/download.nnels.ca/downloads/1401740666/97ab1897a9938200dac05724d28b8221/River_God_Chivers_Edition_S9164.zip';
  $filepath = 'http://s.libraries.coop/download.nnels.ca/downloads/1401740666/97ab1897a9938200dac05724d28b8221/River_God_Chivers_Edition_S9164.zip';  
  
  $filepath = 'http://s3://download.nnels.ca/AU/WA/ABWA/restricted/S9164/River_God_Chivers_Edition_S9164.zip';
  
  $filepath = 'http://s.libraries.coop/download.nnels.ca/AU/WA/ABWA/restricted/S9164/River_God_Chivers_Edition_S9164.zip';
  if(!empty($node->field_s3_path[LANGUAGE_NONE])) _cals_importer_update_field_collection_field_file_resource($node, $node->field_s3_path[LANGUAGE_NONE][0]['value']);  
  */
  _cals_importer_update_field_collection_field_file_resource($node);
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
 //$filepath
function _cals_importer_update_field_collection_field_file_resource($node) {


  $fc = entity_load('field_collection_item', array(56456));

  //printAndDie($fc,$node);
  
  
  unset($node->field_file_resource[LANGUAGE_NONE]);
  $filepath = '';
  if(!empty($node->field_s3_path[LANGUAGE_NONE]) ) $filepath = $node->field_s3_path[LANGUAGE_NONE][0]['value'];

  $arr = _cals_importer_parse_marcxml_cors3($node);
  
  // Create new field collection item.
  $fc_item = entity_create('field_collection_item', array('field_name' => 'field_file_resource')); 

  // Attach it to the node.
  $fc_item->setHostEntity('node', $node); 
  
  $filepath = str_replace("download.nnels.ca/", "", $filepath);
  $filepath = str_replace("download2.nnels.ca/", "", $filepath);

  
  //$filepath = "s3://foo";
  // Create managed File object and associate with zip. Not currently in use, but leave intact for now
  if(!empty($filepath)) {
    $file = (object) array(
      'uid' => 1,
      'uri' => $filepath,
      'filename' => $filepath, //str_replace("s3://", "", $filepath),
      'filemime' => "application/zip",    //application/zip or multipart/x-zip
      'status' => 1,
      'display' => 1
    );
    
    file_save($file);
   
    //$fc_wrapper = entity_metadata_wrapper('field_collection_item', $fc_item);
    $fc_item->field_s3_file_upload[LANGUAGE_NONE][0]  =  (array)$file;
  
  }
  
  $fc_item->field_user[LANGUAGE_NONE][0]['value'] = 1;
  //$fc_item->field_performer[LANGUAGE_NONE][0]['value'] = "TEST";
  if(isset($arr['fc_files']['field_performer'][0])) {
    $fc_item->field_performer[LANGUAGE_NONE] = $arr['fc_files']['field_performer'];
    //printAndDie($fc_item->field_performer, $arr['fc_files']['field_performer']);
  }

  if(isset($arr['fc_files']['field_running_time'][0])) {
    $fc_item->field_running_time[LANGUAGE_NONE] = $arr['fc_files']['field_running_time'];
    //printAndDie($fc_item->field_running_time);
  }
  
  //hard-coded values (for now):
  
  $fc_item->field_availability_status[LANGUAGE_NONE][0]['value'] = 1;
  
  $fc_item->field_organization[LANGUAGE_NONE][0]['nid'] = 15450;
  $fc_item->field_user[LANGUAGE_NONE][0]['uid'] = 1;
  $fc_item->field_file_upload_details[LANGUAGE_NONE][0]['value'] = "Original collection reparsed";
  
  if(!empty($filepath)) $fc_item->field_s3_path[LANGUAGE_NONE][0]['value'] = $filepath;
  $fc_item->save(); // Save field-collection item.
  
  //printAndDie($file, $fc_item);

  node_save($node);
  drupal_set_message(t("updated s3 path: ") . $node->title);

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

  $xml = '';
  if (!isset($node->field_xml_string[LANGUAGE_NONE]) ) return ''; //get out of here if not xml string

  if (isset($node->field_xml_string[LANGUAGE_NONE])) 
    $xml = _cals_get_xml_string($node->field_xml_string[LANGUAGE_NONE][0]['value']);

  //create an array of taxonomies to be used in parsing field collections, etc.
  foreach (taxonomy_get_vocabularies() as $k => $v) {
    $arr_tax[$v->name] = $k;
  } 
  
  //if (!strlen($xml)) return "no xml!";
  $arr = array();
  
  //get running time
  _cals_importer_get_runningtime_from_marcxml($arr, $xml);
  
  //get performer
  _cals_importer_get_performer_from_marcxml($arr, $xml);

 
  return $arr;
    
}