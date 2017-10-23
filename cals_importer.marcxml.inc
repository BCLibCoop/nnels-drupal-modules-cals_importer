<?php
/**
 * @file
 * Manages marc xml ingests
 */

/**
 * Simple wrapper to call main parser; if preferred, can invoke batch api instead. 
 * @param $node
 */
function _cals_importer_update_repository_item($node) {
  //can also push this thru the batch api instead. To do so, comment out above and uncomment
  //the following function call.
  _cals_parse_record_details($node, $fields=array());

  //return user back to main node page
  drupal_goto("node/" . $node->nid);
}


/**
 * invoked via batch api view
 *
 * @param $entity
 *   the node passed to the parser
 *
 */  
 
function _cals_importer_update_repository_items(&$entity, $context = array()) {
  if (isset($entity->nid)) {
    $node = node_load($entity->nid);
    //safety check to make sure we're parsing repos items!!    
    $content_types = array('repository_item');
    if (in_array($node->type, $content_types)  ) {
      _cals_parse_record_details($node, $fields=array());
      //dpm($node->title);
      //drupal_set_message(t("%t MARC XML file parsed and saved."), array('%t' => $node->title));
    }
    else {
      //dpm(t("Invalid selection: not a repos item"));
    }

  }//end main if
 
  return "";
  
}



/**
 * Normalizes title, creator, contributor and other fields
 *
 * @param $node
 *   the node passed to the parser
 * @param $field
 *   the $node->{field} acted on
 *
 */ 
function _cals_importer_normalize_node_field(&$node, $field) {
  //drupal_set_message(__FUNCTION__ . ": " . $field);
	switch($field) {
	  case "title":
    case "title_field":
    case "field_title_uniform":
    case "field_title_alternate":
		  _cals_importer_strip_square_brackets($node, $field);
      _cals_importer_strip_trailing_periods($node, $field);
      break;
	  case "field_dc_creator":
		case "field_dc_contributor":
			_cals_importer_strip_square_brackets($node, $field);
			_cals_importer_normalize_creators($node, $field);
	  	break;
	  case "field_publisher":
			_cals_importer_strip_square_brackets($node, $field);
	  	_cals_importer_normalize_publisher($node);
	  	break;
    case "field_publisher_source":
      _cals_importer_strip_trailing_periods($node, $field);
      break;


  }

}

function _cals_reparse_title_edition($node) {
  $fields = array("title", "title_field", "field_uniform_title", "field_title_alternate", "field_dc_relation");
  _cals_parse_record_details($node, $fields);
    //return user back to main node page
  drupal_goto("node/" . $node->nid);
}

/**
 * The main MARC parser
 *
 * @param $node
 *   the node passed to the parser
 *
 * @return
 *   an array.
 *
 */
function _cals_parse_record_details($node, $fields) {
  $xml = _cals_get_marc_xml_string($node);
  if($xml === 0) return;
  $arr_fields = _cals_get_fields_repository_item($fields);
  $map = $arr_fields['map'];
  //loop thru map to populate node
  _cals_importer_iterate_field_map($node, $xml, $map, "parse");
  $atts = array("attributes" => array("target" => "_blank"));
  $l1 = l("view", "node/" . $node->nid, $atts) . " | " . l("edit", "node/" . $node->nid . "/edit", $atts);
  drupal_set_message($l1 . " : " . $node->title);
  if(count($fields)) {
    //populate audit trail
    $action['description'] = "";
    $action['activity'] = 6;
    _cals_importer_update_admin_audit($node,  $action);
  }
  else {
    //populate audit trail
    $action['description'] = "";
    $action['activity'] = 2;
    _cals_importer_update_admin_audit($node,  $action);
    $action['description'] = "";
    $action['activity'] = 3;
    _cals_importer_update_admin_audit($node,  $action);
  }


}


function _cals_reparse_record_details_form($settings, &$form_state) {
  $form = array();
  $fields = array();
  $arr = _cals_get_fields_repository_item();
  //$fields = field_info_instances("node", "repository_item");
  $map = $arr['map'];
  foreach($map as $k => $v) $options[$k] = $v['label'];

  $form['fields_to_parse'] = array(
    '#type' => 'select',
    '#title' => t('Select the Field(s) to reparse'),
    '#options' => $options,
    '#required' => TRUE,
    '#multiple' => TRUE,
    '#size' => count($options),
    '#description' => t("NB: this action will overwrite any edits added to the selected fields following the original parsing. Please exercise caution."),
    '#default_value' => isset($settings['settings']['hero']) ? $settings['settings']['hero'] : '',
  );
  return $form;
}

function _cals_reparse_record_details_submit($form, $form_state) {
  $return = array();
  $fields = $form_state['values']['fields_to_parse'];
  $fieldnames = implode(", ", $fields);
  $msg = "updated " . $node->title .": " . $fieldnames;
  drupal_set_message($msg);
  $return['fields_to_parse'] = $fields;
  return $return; //Note, return value here must be an array.
}
function _cals_reparse_record_details(&$node, $context) {
  $fields = $context['fields_to_parse'];
  _cals_parse_record_details($node, $fields);

  $message = t('Reparsing: %title %fields',
    array('%title' => $node->title, '%fields' => implode(", ", $fields),)
  );
  //drupal_set_message($message);
}

/**
 * Iterates thru mapped fields to parse full node or selected fields
 *
 * @param $node
 *   the node passed to the parser
 * @param $xml
 *   the marcxml to be parsed
 * @param $map
 *   the field mappings
 * @param $op
 *   the operation, e.g., parse (all fields) or reparse (selected fields)
 *
 * @return
 *   $node
 *
 */
function _cals_importer_iterate_field_map(&$node, $xml, $map, $op) {
  variable_set('error_level', 1);
  dpm(ddebug_backtrace(TRUE));
  variable_set('error_level', 1);
  $fc_fields = array(); // container for field collection array
  foreach($map as $k => $v) {
    $type = $v['type'];

    if(!empty($v['parser'])) {
      if(!isset($v['parser']['function'])) printAndDie("???", $v);
      $function = $v['parser']['function'];
      if(is_array($function)) {
        foreach($function as $f) {
          $function($xml, $k, $map);
        }
      }
      else{
        if(isset($v['parser']['tags'])) {
          $tags = $v['parser']['tags'];
          $function($xml, $k, $map, $tags);
        }
        else $function($xml, $k, $map);

      }
      $node->{$k} = array();
      $key = ($type == 'taxonomy') ? "tid" : "value";
      if(is_array($map[$k]['values'])) {
        $values = $map[$k]['values'];
        foreach($values as $value) {
          if($type == 'link_field') {
            $node->{$k}[LANGUAGE_NONE][] = $value;
          }
          else{
            if(is_string($value)) $value = trim($value);
            if(!empty($value)) {
              if($k == 'field_contents') {
                $node->{$k}[LANGUAGE_NONE][] = array(
                  $key => $value,
                  "format" => 'filtered_html',
                );
              }
              else //otherwise, just set the value
                $node->{$k}[LANGUAGE_NONE][][$key] = $value;
            }

          }
        }
      }

      if( ($type != 'field_collection' && $type != 'taxonomy' && $type != 'link_field') || $k == 'field_title_uniform') {
        _cals_importer_normalize_node_field($node, $k); //normalizes various field values
      }
      if($type == 'field_collection') {
        _cals_importer_delete_field_collection($node, $k);
        $fc_fields[$k] = $map[$k];
      }

    }
  }
  if($op == 'parse') _cals_delete_duplicate_oclc_numbers($node);
  //save the populated node
  //$node->status = 1;
  $node->title = $node->title_field[LANGUAGE_NONE][0]['value'];
  node_save($node);

  //populate the field collections
  if(count($fc_fields)) _cals_importer_populate_repos_item_fieldcollections($node, $fc_fields);
}