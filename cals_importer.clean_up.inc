<?php

/**
 * Invokes Drupal's batch api to normalize various fields (titles, authors, publishers, etc.) 
 * 
 */



/**
 * Simple wrapper so we can invoke from main repo node page for testing purposes
 *
 * @param $node
 *   the node passed to the parser
 */

function _cals_importer_generate_marc_xml_856($node) {
 
	$xml_string =  str_replace("</marc:record>", "", $node->field_xml_string[LANGUAGE_NONE][0]['value']) . 
	  '<marc:datafield tag="856" ind1="4" ind2="1"><marc:subfield code="u">https://nnels.ca/node/' .  $node->nid .
	  '</marc:subfield></marc:datafield></marc:record>';
	
	
	/*
	leave this in case we need to do something fancier on the xml string, eg. replace existing 856 fields, etc.
		
	$xml = 
		'<?xml version="1.0" encoding="UTF-8" ?><marc:collection xmlns:marc="http://www.loc.gov/MARC21/slim" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://www.loc.gov/MARC21/slim http://www.loc.gov/standards/marcxml/schema/MARC21slim.xsd">' . $xml_string . '</marc:collection>';
	//$json = json_encode($xml);

	$sxe = new SimpleXMLElement($xml_string);
	$sxe->registerXPathNamespace('marc', NAME_SPACE);
	//$sxe = new SimpleXMLElement($xml);
	//$sxe->addAttribute('type', 'documentary');
	
	$datafield = $sxe->addChild('marc:datafield');
	$datafield->addAttribute('tag', '856');
	
	$subfield = $datafield->addChild('marc:subfield', 'https://nnels.ca/node/' . $node->nid);
	$subfield->addAttribute("code", "u");
	$xml2 = $sxe->asXML();
	$xml2 = str_replace("</", "</marc:", $xml2);
	$xml2 = str_replace("<", "<marc:", $xml2);
	$xml2 = str_replace("marc:/marc:", "/marc:", $xml2);
	$xml2 = str_replace('<marc:?xml version="1.0"?>', '', $xml2);
	
	
	*/
	$node->field_xml_string_856[LANGUAGE_NONE][0]['value'] = trim($xml_string);
	node_save($node);
	
	drupal_goto("node/" . $node->nid);

}


/**
 * Simple wrapper so we can invoke from main repos page for testing purposes
 *
 * @param $node
 *   the node passed to the parser
 */
function _cals_normalize_title($node) {
  _cals_normalize_title_entry($node);
  drupal_goto("node/" . $node->nid);

}

/**
 * VBO to normalize titles 
 */
function _cals_normalize_titles(&$entity, $context = array()) {
  if (isset($entity->nid)) {
    $node = node_load($entity->nid);
    //safety check to make sure we're parsing repos items!!    
    $content_types = array('repository_item');
    
    if (in_array($node->type, $content_types)  ) {
      _cals_normalize_title_entry($node);
    }
    else{
      drupal_set_message("Invalid selection: not a repos item");
    }

  }//end main if
 
  return "";
  
}


function _cals_normalize_title_entry($node) {

  $save_node = FALSE;
  $title = rtrim($node->title);
  $title = rtrim($title, ", /"); 
  if ($title === $node->title) {
  	drupal_set_message(t("skipped: " . $title));
	}
	else {
	  $node->title = $title;
	  node_save($node);
    drupal_set_message("updated: $title");
	}
	
}  
 
 
/**
 * Publisher
 *
 */
 
function _cals_normalize_publisher($node) {
  _cals_normalize_creator_entry($node);
  drupal_goto("node/" . $node->nid);

}
 
 
function _cals_normalize_publishers(&$entity, $context = array()) {
  if (isset($entity->nid)) {
    $node = node_load($entity->nid);
    //safety check to make sure we're parsing repos items!!    
    $content_types = array('repository_item');
    
    if (in_array($node->type, $content_types)  ) {
      _cals_normalize_creator_entry($node);
    }
    else{
      drupal_set_message("Invalid selection: not a repos item");
    }

  }//end main if
 
  return "";
  
}


function _cals_normalize_creator_entry($node) {

  $save_node = FALSE;
  foreach($node->field_publisher[LANGUAGE_NONE] as $k => $v) {
   //printPre($author['value']);
   $tmp = rtrim($v['value']); //trim various ascii chars
   //logic to determine if there's a ...
   $len = strlen($tmp);
   $sub = substr($tmp, 0, ($len - 1));
   $count = substr_count($sub, ":");  

   if ($count > 0) {
    drupal_set_message(t("skipped: " . $v['value']));
   }
   else {
     $tmp = rtrim($tmp, "."); //strip trailing periods;
     $tmp = rtrim($tmp, ","); //strip trailing periods;
     $tmp = rtrim($tmp, ":"); //strip trailing periods;
     $tmp = rtrim($tmp, " :"); //strip trailing periods;
     $tmp = rtrim($tmp, "]"); //strip trailing periods;
     $tmp = ltrim($tmp, "["); //strip trailing periods;
     $tmp = rtrim($tmp); //trim various ascii chars

     $node->field_publisher[LANGUAGE_NONE][$k]['value'] = $tmp;
     $save_node = TRUE;
   }

  } 
  if($save_node) {
    node_save($node);
    drupal_set_message("updated: " . $v['value'] . " => " .  $tmp);
  } 
} 
 

/**
 * Simple wrapper to call main batch api parse to clean-up author entries
 */

function _cals_clean_up_creator($node) {

  _cals_clean_up_creator_entry($node);
  drupal_goto("node/" . $node->nid);

}

 

/**
 * clean-up author entries, i.e., to strip off trailing commas, periods, etc.
 */
function _cals_clean_up_creators(&$entity, $context = array()) {
  if (isset($entity->nid)) {
    $node = node_load($entity->nid);
    //safety check to make sure we're parsing repos items!!    
    $content_types = array('repository_item');
    
    if (in_array($node->type, $content_types)  ) {
      _cals_clean_up_creator_entry($node);
    }
    else{
      drupal_set_message("Invalid selection: not a repos item");
    }

  }//end main if
 
  return "";
  
}

function _cals_strip_year_ranges($str) {

  $patterns = array();
  $patterns = '([0-9]{4}-[0-9]{4}|[0-9]{4})';
  $replacements = array();
  $replacements  = '';
  $tmp = preg_replace($patterns, $replacements, $str);
  $tmp = preg_replace( "{\s+}", ' ', $tmp );
  return rtrim($tmp); 
}
 
function _cals_clean_up_creator_entry($node) {

  $save_node = FALSE;
  foreach($node->field_dc_creator[LANGUAGE_NONE] as $k => $author) {
   //printPre($author['value']);
   $tmp = rtrim($author['value']); //trim various ascii chars

   //logic to determine if there's a ...
   $len = strlen($tmp);
   $sub = substr($tmp, 0, ($len - 1)); //substring minus 1 character to figure out # of potential replacements
   $count = substr_count($sub, ".") + substr_count($sub, ",");  


   if ($count == 0) { //
      drupal_set_message(t("skipped: " . $author['value']));

   }
   else {
     $tmp = _cals_strip_year_ranges($tmp);
      $tmp = rtrim($tmp, ", .-"); //strip remaining trailing periods, dashes, etc.;
     $node->field_dc_creator[LANGUAGE_NONE][$k]['value'] = $tmp;
     $save_node = TRUE;
     drupal_set_message("updated: " . $author['value'] . " => " .  $tmp);
   }
   

  } 
  if($save_node)  node_save($node);
 

}