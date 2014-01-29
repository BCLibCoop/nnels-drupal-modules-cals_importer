<?php

/**
 * Invokes Drupal's batch api to normalize various fields (titles, authors, publishers, etc.) 
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



