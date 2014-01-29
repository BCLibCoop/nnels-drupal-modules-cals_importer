<?php


/**
 * Invokes Drupal's batch api to de-dupe records based on 035 and other field matches. 
 * 
 */
 

function _cals_dedupe_035(&$entity, $context = array()) {

	$nid = $context['node']->nid;
	
	STATIC $tmp = array("sysnum" =>  0, "nid" => 0); //, "nid" => $context['node']->nid) ;
	
	//STATIC $nid = $context['node']->nid;

	if( $context['node']->field_system_control_number[LANGUAGE_NONE][0]['value'] != $tmp['sysnum'] ) {
		$tmp['sysnum'] = $context['node']->field_system_control_number[LANGUAGE_NONE][0]['value'];
		$tmp['nid'] = $context['node']->nid;

	}
	else {
		$n = node_load($entity->nid);
		$n->field_node_succeeded_by[LANGUAGE_NONE][0]['nid'] = $tmp['nid'];
		$n->status = 0;
		node_save($n);
	//	dpm($n);
		drupal_set_message("node saved!");
	}

	//dsm($context);
	

}

