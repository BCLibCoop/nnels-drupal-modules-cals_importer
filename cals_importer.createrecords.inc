<?php
/**
 * @file
 * CALS Importer Create Stub Records.
 */


/**
 * import / create skeletal marc records, i.e., just the title
 *
 * @param $node
 *   the node passed to the parser
 */
function _cals_importer_create_repo_items($node) {
  global $user;
  $file = drupal_realpath($node->field_marc_xml_file[LANGUAGE_NONE][0]['uri']); // FILE_FOLDER . ;
  //to track errors
  libxml_use_internal_errors(true);
  //load file
  $xml = simplexml_load_file($file);
  if ($xml===FALSE) {
    drupal_set_message(t("Oops, could not parse the file. Might not be valid xml?"), "error");
     foreach (libxml_get_errors() as $error) {
        drupal_set_message($error->message, "error");
    }
    drupal_goto("node/" . $node->nid);
    exit;
  } 
  $batch = array(
    'title' => t('Generate Records'),
    'operations' => array(array('_cals_importer_create_repo_items_batch', array($node)), //unfortunately, we can't pass xml in this way
    
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
 * The main batch process to generate new stub records
 *   
 * @param $node
 *   the node, i.e., the recordset node that contains the file being parsed
 *
 */
 
function _cals_importer_create_repo_items_batch($node, &$context) {
  $limit = 10; //10;
  if (empty($context['sandbox']['progress']) ) {
  	
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['xml'] = ''; //$xml;
    $context['sandbox']['current'] = 0;
    $context['sandbox']['node'] = $node; //$node; // node_load($nid);
  	$file = drupal_realpath($node->field_marc_xml_file[LANGUAGE_NONE][0]['uri']); // FILE_FOLDER . ;
  	$xml = simplexml_load_file($file); //load the file
  	$context['sandbox']['xml'] = $xml;
  	$context['sandbox']['max'] = $xml->children(NAME_SPACE)->count();
  	$context['sandbox']['parsed'] = array(); 

  }
  //$i = $context['sandbox']['progress'];
  $total = $context['sandbox']['progress'] + $limit;
	$pnode = $context['sandbox']['node']; //parent node, i.e., the record set node
	$xml = $context['sandbox']['xml'];
	
  if ($total > $context['sandbox']['max']) $total = $context['sandbox']['max']; //reset total to max on final loop
  
  for ($i = $context['sandbox']['progress']; $i < $total; $i++) {
  	$record = $xml->children(NAME_SPACE)->$i;
    //$arr = _cals_get_title_from_xml($record);
    $title =_cals_get_simple_title($record);
    
    $result = '';
    if (!isset($arr['repos']['title']) || strlen($arr['repos']['title']) == 0) {
    	//do something???
    }
    else {
	    $new_record = TRUE;
	    
	  
	    $n = _cals_prepare_new_item_repos();
	    $result = "Skeletal record added. ";

			//oclc / sys control number
			_cals_get_syscontrol_numbers($record, $arr);

	    //if ($new_record || $n->title != $arr['repos']['title'] || empty($n->field_xml_string)) {
	    $n->status = 0;
	    $n->title = $title;
	    //we'll add string to xml field.
	    $n->field_xml_string[LANGUAGE_NONE][]['value'] = $record->saveXML();

	    //save the new/existing repository item node  
	    $n->field_record_set[LANGUAGE_NONE][0]['nid'] = $pnode->nid;
	    $n->field_access_restrictions[LANGUAGE_NONE][0]['value'] = $pnode->field_access_restrictions[LANGUAGE_NONE][0]['value'];
	    
			node_save($n);
	    
	    $context['sandbox']['parsed'][]  = l(t("edit"), "node/" . $n->nid . "/edit" ) . " | " . l(t("view"), "node/" . $n->nid) . " - {$n->title}" 
	      . " " . $result;  

	    dpm("parsing row # " . $context['sandbox']['progress'] . ": " . $n->title);  
    
    }
      

    if($i == $context['sandbox']['max']) {
    
		  $pnode->field_status[LANGUAGE_NONE][0]['value'] = 2;
		  //save one last time.
		  node_save($pnode);
		  
		  $num_records = count($context['sandbox']['parsed']);
 			dpm($context['sandbox']);
		  $vars = array("items" => $context['sandbox']['parsed'] , "title" => "parsed list: $num_records records", "type" => "ol", "attributes" => array());
		  $vars = theme_item_list($vars); 
		  drupal_set_message(filter_xss($vars, $allowed_tags = array('a', 'em', 'strong', 'cite', 'blockquote', 'code', 'ul', 'ol', 'li', 'dl', 'dt', 'dd')));
     
    }		
		
		 // Update our progress information.
    $context['sandbox']['progress']++;
    $context['results'][] = check_plain($n->title); // Track nodes updated
    $context['message'] = t('Now processing %node', array('%node' => $n->title));
  }

  if ($context['sandbox']['progress'] != $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }

}




/**
 * Preview titles in new collection
 *   
 * @param $node
 *   the node, i.e., the recordset node that contains the file being parsed
 *
 */

/**
 * import / create skeletal marc records, i.e., just the title
 *
 * @param $node
 *   the node passed to the parser
 */
function _cals_importer_preview_repo_items($node) {

  global $user;
  $file = drupal_realpath($node->field_marc_xml_file[LANGUAGE_NONE][0]['uri']); // FILE_FOLDER . ;
  //to track errors
  libxml_use_internal_errors(true);
  //load file
  $xml = simplexml_load_file($file);
	$xml->registerXPathNamespace("marc", NAME_SPACE);

  if ($xml===FALSE) {
    drupal_set_message(t("Oops, could not parse the file. Might not be valid xml?"), "error");
     foreach (libxml_get_errors() as $error) {
        drupal_set_message($error->message, "error");
    }
    drupal_goto("node/" . $node->nid);
    exit;
  } 
  
  	$max = $xml->children(NAME_SPACE)->count();
		
		/*	
		$titles = $xml->xpath("//marc:datafield[@tag='245']/marc:subfield[@code='a']");

		for ($i = 0; $i < $max; $i++) {
		  	$record = $xml->children(NAME_SPACE)->$i;
		    //$arr = _cals_get_title_from_xml($record);
		    $title =_cals_get_simple_title($record);
		    printPre($title);

		}
  	die;
  	*/
  $batch = array(
    'title' => t('Generate Records'),
    'operations' => array(array('_cals_importer_preview_repo_items_batch', array($node)), //unfortunately, we can't pass xml in this way
    
    ),
    'progress_message' => t('Update. Operation @current out of @total.'),
    'error_message' => t('Error!'),
    'finished' => '_cals_importer_create_repo_items_finished',
  );
  //($batch);
  batch_set($batch);
  batch_process("node/" . $node->nid);
}  

 
function _cals_importer_preview_repo_items_batch($node, &$context) {
  $limit = 10; //10;
  if (empty($context['sandbox']['progress']) ) {
  	
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['xml'] = ''; //$xml;
    $context['sandbox']['current'] = 0;
    $context['sandbox']['node'] = $node; //$node; // node_load($nid);
  	$file = drupal_realpath($node->field_marc_xml_file[LANGUAGE_NONE][0]['uri']); // FILE_FOLDER . ;
  	$xml = simplexml_load_file($file); //load the file
  	$context['sandbox']['xml'] = $xml;
  	$context['sandbox']['max'] = $xml->children(NAME_SPACE)->count();
  	$context['sandbox']['parsed'] = array(); 

  }
 // $i = $context['sandbox']['progress'];
  $total = $context['sandbox']['progress'] + $limit;
	$pnode = $context['sandbox']['node']; //parent node, i.e., the record set node
	$xml = $context['sandbox']['xml'];
	
  if ($total > $context['sandbox']['max']) $total = $context['sandbox']['max']; //reset total to max on final loop
  
  for ($i = $context['sandbox']['progress']; $i < $total; $i++) {
  	$record = $xml->children(NAME_SPACE)->$i;
    //$arr = _cals_get_title_from_xml($record);
    $title = _cals_get_simple_title($record);
    
    $result = '';
    if (strlen($title) == 0) {
    	//do something???
    }
    else {

    }
	    $new_record = TRUE;

	    $result = "Skeletal record added. ";
	    dpm("Row # " . $context['sandbox']['progress'] . ": " . $title);  
    
    if($i == $context['sandbox']['max']) {
    
		  $num_records = count($context['sandbox']['parsed']);
 			dpm($context['sandbox']);
		  $vars = array("items" => $context['sandbox']['parsed'] , "title" => "parsed list: $num_records records", "type" => "ol", "attributes" => array());
		  $vars = theme_item_list($vars); 
		  drupal_set_message(filter_xss($vars, $allowed_tags = array('a', 'em', 'strong', 'cite', 'blockquote', 'code', 'ul', 'ol', 'li', 'dl', 'dt', 'dd')));
     
    }		
		
		 // Update our progress information.
    $context['sandbox']['progress']++;
    $context['results'][] = check_plain($n->title); // Track nodes updated
    $context['message'] = t('Now processing %node', array('%node' => $n->title));
  }

  if ($context['sandbox']['progress'] != $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }

}




/**
 * Finish batch
 */
function _cals_importer_create_repo_items_finished($success, $results, $operations) {
  if ($success) {
    $message = format_plural(count($results), 'One node updated', '@count nodes created / updated.');
  } 
  else {
    $message = t('Finished with an error.');
  }
  drupal_set_message($message);
  //watchdog('update', 's3 paths updated');
}


function _cals_get_simple_title($xml) {
  $tmp = $xml->xpath("marc:datafield[@tag='245']/marc:subfield[@code='a']");
 	$title = (string) $tmp[0];
  $tmp = $xml->xpath("marc:datafield[@tag='245']/marc:subfield[@code='b']");
  $tmp = (string) $tmp[0];
  if( strlen($tmp) ) $title .=  " " . $tmp;
  
  return _cals_get_truncated_value( rtrim($title, ", /"), 250);
}

function _cals_get_syscontrol_numbers($xml, &$arr) {

  $str = $xml->xpath("//marc:datafield[@tag='035']/marc:subfield[@code='a']");
  if(!empty($str)) {
	  if (strpos($str, "(OCoLC)") !== false) {
	    $arr['repos']["field_oclc_number"][] = $str;
	  }	
	  else {
	    $arr['repos']["field_system_control_number"][] = $str;
	  }
  }
}

/**
 * Not currently in use, might replace the get_value_from_tag model (see below)
 *
 *
 * @return
 *   an array of MARC XML -> Drupal mappings
 *
 */

function _cals_get_title_from_xmlxxx($record) {
  $arr = array();
  //loop thru datafields
  $arr['repos']['title'] = '';
  foreach ($record->children(NAME_SPACE)->datafield as $datafield) {
    $tag = (string) $datafield->attributes()->tag;
    $ind1 = (string) $datafield->attributes()->ind1;
    $ind2 = (string) $datafield->attributes()->ind2;
    
    
    foreach ($datafield as $subfield) {
      $code = (string) $subfield->attributes()->code;
      $str = trim((string) $subfield);
      $str = trim($str);

      switch ($tag) {
        case 245:
          if ($code == 'a') $arr['repos']['title'] =  $str;
          if ($code == 'b') $arr['repos']['title'] .=  " " . $str;
          $arr['repos']['title'] = _cals_get_truncated_value($arr['repos']['title'], 250);
          break;
          
        case "035":
          if ($code == 'a') {
            if (strpos($str, "(OCoLC)") !== false) {
              $arr['repos']["field_oclc_number"][] = $str;
            }
            else {
              $arr['repos']["field_system_control_number"][] = $str;
            }
          } 
          break;  
      }
    }
  }
  return $arr;
  
}
