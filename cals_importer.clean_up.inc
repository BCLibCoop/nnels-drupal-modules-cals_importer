<?php
/**
 * @file
 * CALS Importer Clean Up.
 */
 
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

function _cals_importer_add_856_tag($node) {
  _cals_add_856_tag($node);
  drupal_goto("node/" . $node->nid);

}


/**
 * VBO to add 856 tag to xml strings
 *
 * @param $entity
 *   the node passed to the parser
 */
function _cals_importer_generate_marc_xml_856(&$entity, $context = array()) {
  if (isset($entity->nid)) {
    $node = node_load($entity->nid);
    //safety check to make sure we're parsing repos items!!    
    $content_types = array('repository_item');
    if (in_array($node->type, $content_types)  ) {
      _cals_add_856_tag($node);
    }
    else{
      drupal_set_message(t("Invalid selection: not a repos item"));
    }

  }//end main if
 
  return "";
  
}


/**
 * Updates the node with NNELS 856 tag 
 *
 * @param $node
 *   the node passed to the parser
 *
 */
function _cals_add_856_tag($node) {

  $xml_string = $node->field_xml_string[LANGUAGE_NONE][0]['value'];
  $str = '<marc:datafield tag="856" ind1="4" ind2="1"><marc:subfield code="u">https://nnels.ca/node/' . $node->nid .
    '</marc:subfield></marc:datafield>';
   
  //printAndDie($node); 
    
  if (strpos($xml_string, $str) ) {
    drupal_set_message(t("Did not update XML string for %string: 856 tag already included") , array('%string' => $node->title));
    $node->field_nnels_856_tag[LANGUAGE_NONE][0]['value'] = 1;
    node_save($node);
  }
  else {
    $xml_string =  str_replace("</marc:record>", "", $xml_string) . $str . '</marc:record>';
   
    $node->field_xml_string[LANGUAGE_NONE][0]['value'] = trim($xml_string);
    $node->field_nnels_856_tag[LANGUAGE_NONE][0]['value'] = 1;
    $node->revision = 1;
    $node->log = "Added NNELS 856 tag to XML String:"; // Log message
    //drupal_set_message(t('%log' => array('%log', $node->log));
    node_save($node);
  
  }
}



/**
 * Simple wrapper to call main batch api parse to standardize author, title and publisher values
 */

function _cals_normalize_record_value($node) {
  _cals_normalize_record_entry($node);
  drupal_goto("node/" . $node->nid);
}

/**
 * VBO to normalize titles, authors, publishers 
 */
function _cals_normalize_record_values(&$entity, $context = array()) {
  if (isset($entity->nid)) {
    $node = node_load($entity->nid);
    //safety check to make sure we're parsing repos items!!    
    $content_types = array('repository_item');
    
    if (in_array($node->type, $content_types)  ) {
      _cals_normalize_record_entry($node);
    }
    else{
      drupal_set_message(t("Invalid selection: not a repos item"));
    }

  }//end main if
 
  return "";
  
}

/**
 * Normalize titles, authors, publishers 
 *
 * @todo Split individual code snippets into discrete functions similar to 
 *   _cals_importer_normalize_publisher_entry
 */
function _cals_normalize_record_entry($node) {

  //titles
  $title = rtrim($node->title);
  $msg = array();
  $title = rtrim($title, ", /"); 
  if ($title != $node->title) {
    $node->title = $title;
    //node_save($node);
    $msg[] = t("title: %t", array('%t' => $title) );
  }
  
  //creators
  foreach($node->field_dc_creator[LANGUAGE_NONE] as $k => $author) {

   $tmp = rtrim($author['value']); //trim various ascii chars
   //logic to determine if there's a ...
   $len = strlen($tmp);
   $sub = substr($tmp, 0, ($len - 1)); //substring minus 1 character to figure out # of potential replacements
   $count = substr_count($sub, ".") + substr_count($sub, ",");  
   if ($count != 0) { 
    $tmp = _cals_importer_strip_years_from_creator_values($tmp);
    $tmp = rtrim($tmp, ", .-"); //strip remaining trailing periods, dashes, etc.;
    //check to see if original value matches new value
    if ($tmp != $author['value']) {
      $node->field_dc_creator[LANGUAGE_NONE][$k]['value'] = $tmp;
      $msg[] = t("updated author:") . " " .  $tmp;
    }
   }
   

  } 
  
 //publisher
  foreach($node->field_publisher[LANGUAGE_NONE] as $k => $publisher) {
    $tmp = _cals_importer_normalize_publisher_entry($publisher['value']);
    if($tmp != $publisher['value']) {
      $node->field_publisher[LANGUAGE_NONE][$k]['value'] = $tmp;
      $msg[] = t("updated publisher:") . " " .  $tmp;
    }
  }   
  //if there's multiple messages, save node 
  if (count($msg) ) {
    $dmsg = t("Updated")  . $title . ": " . implode(" | ", $msg);  
    drupal_set_message($dmsg);
    node_save($node);
  }
  else {
    drupal_set_message(t("Not updated:") . " " . $title);
  }

 
}  


/**
 * Strips dates and date ranges from creator values  
 */
function _cals_importer_strip_years_from_creator_values($str) {

  $patterns = array();
  $patterns = '([0-9]{4}-[0-9]{4}|[0-9]{4})';
  $replacements = array();
  $replacements  = '';
  $tmp = preg_replace($patterns, $replacements, $str);
  $tmp = preg_replace( "{\s+}", ' ', $tmp );
  return rtrim($tmp); 
}
  
/**
 * Updates the node with NNELS 856 tag 
 *
 * @param $value
 *   the original publisher value
 * @return $tmp
 *   the scrubbed value (might be unaltered)
 *
 */
function _cals_importer_normalize_publisher_entry($value) {

   $tmp = rtrim($value); //trim various ascii chars
   //logic to determine if there's a ...
   $len = strlen($tmp);
   $sub = substr($tmp, 0, ($len - 1));
   $count = substr_count($sub, ":");  

   if ($count > 0) {
    return $value;
   }
   else {
     $tmp = rtrim($tmp, "."); //strip trailing periods;
     $tmp = rtrim($tmp, ","); //strip trailing periods;
     $tmp = rtrim($tmp, ":"); //strip trailing periods;
     $tmp = rtrim($tmp, " :"); //strip trailing periods;
     $tmp = rtrim($tmp, "]"); //strip trailing periods;
     $tmp = ltrim($tmp, "["); //strip trailing periods;
     $tmp = rtrim($tmp); //trim various ascii chars
     
     return $tmp;
   }
} 
 


 




function _cals_clean_up_creator_entry_TO_DEPRECATE($node) {

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
     $tmp = _cals_importer_strip_years_from_creator_values($tmp);
      $tmp = rtrim($tmp, ", .-"); //strip remaining trailing periods, dashes, etc.;
     $node->field_dc_creator[LANGUAGE_NONE][$k]['value'] = $tmp;
     $save_node = TRUE;
     drupal_set_message("updated: " . $author['value'] . " => " .  $tmp);
   }
   

  } 
  if ($save_node)  node_save($node);
 

}