<?php


/**
 * @file
 * Dedupes repository items
 */
 
 
/**
 * Invokes Drupal's batch api to de-dupe records based on 035 and other field matches.
 *
 * @param $entity
 * @param array $context
 */
function _cals_dedupe_035(&$entity, $context = array()) {

  $nid = $context['node']->nid;
  $sysnum = trim($context['node']->field_system_control_number[LANGUAGE_NONE][0]['value']);
  static $tmp = array("sysnum" =>  0, "nid" => 0); //, "nid" => $context['node']->nid) ;

  if ( $sysnum === $tmp['sysnum'] ) {
    //drupal_set_message("YES: sysnum = $sysnum | tmp_sysnum = {$tmp['sysnum']} " );
    $n = node_load($entity->nid);
    $n->field_node_succeeded_by[LANGUAGE_NONE][0]['nid'] = $tmp['nid'];
    $n->status = 0;
    node_save($n);
  //  dpm($n);
    drupal_set_message(t("node %t unpublished."), array('%t' => $n->title)); 
  }
  else {
    //drupal_set_message("NO: sysnum != $sysnum | tmp_sysnum != {$tmp['sysnum']} " );
    $tmp['sysnum'] = $sysnum;
    $tmp['nid'] = $nid;
  }


}



/**
 * Invokes Drupal's batch api to de-dupe organizational records based on ILL Code matches. 
 * 
 */
 
function _cals_dedupe_orgs_by_illcode(&$entity, $context = array()) {

  $nid = $context['node']->nid;
  $illcode = trim($context['node']->field_library_code[LANGUAGE_NONE][0]['value']);
  
  //static variable
  static $tmp = array("illcode" =>  0, "nid" => 0); //, "nid" => $context['node']->nid) ;

  if ( $illcode === $tmp['illcode'] ) {
    //drupal_set_message("YES: sysnum = $sysnum | tmp_sysnum = {$tmp['sysnum']} " );
    $n = node_load($entity->nid);
    $n->field_org_succeeded_by[LANGUAGE_NONE][0]['nid'] = $tmp['nid'];
    $n->status = 0;
    node_save($n);
    drupal_set_message(t("node %t unpublished."), array('%t' => $n->title));
  }
  else {
    //drupal_set_message("NO: sysnum != $sysnum | tmp_sysnum != {$tmp['sysnum']} " );
    $tmp['illcode'] = $illcode;
    $tmp['nid'] = $nid;
  }

}
