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
      _cals_importer_strip_square_brackets_value($field);
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
 * Main Parser Function
 *
 * @param $node
 *  The node context passed in via bulk operation
 * @param $fields
 *  Target fields for the field mapper
 * @param bool $reparse_flag
 *  Initial parse (default) or re-parse
 */
function _cals_parse_record_details($node, $fields, $reparse_flag = FALSE) {

  $xml = _cals_get_marc_xml_string($node);
  if($xml === 0) return; //Can't act on a stub node without MARCXML record

  //Full (initial parse mode)
  if ( $reparse_flag === FALSE ) {
    $arr_fields = _cals_get_fields_repository_item();
    $map = $arr_fields['map'];
  }

  //Using Item-level mode of reparse operation
  if ( $reparse_flag === TRUE ) {
    unset($map);
    $map = $fields;
  }
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

/**
 * @param $settings
 * @param $form_state
 * @return array
 */
function _cals_reparse_record_details_form($settings, &$form_state) {
  $form = array();
  $arr = _cals_get_fields_repository_item();
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

  $scope = array(0 => t('Item MARCXML'), 1 => t('Record set MARCXML'));

  $form['scope_of_reparse'] = array(
    '#type' => 'radios',
    '#title' => t('Reparsing scope'),
    '#default_value' => $scope[0],
    '#required' => TRUE,
    '#options' => $scope,
    '#description' => t('Item-level will use field_marcxml_string for each node. Record set uses the field_marc_xml_file upload.'),
  );


  $form['record_set_id'] = array(
    '#type' => 'hidden',
    '#value' => $settings['view']->argument['field_record_set_nid']->argument,
  );

  return $form;
}

/**
 * @param $form
 * @param $form_state
 * @return array
 */

function _cals_reparse_record_details_submit($form, $form_state) {

  //Pass along our settings
  $return = array();
  $return['fields_to_parse'] = $form_state['values']['fields_to_parse'];
  $return['scope_of_reparse'] = $form_state['values']['scope_of_reparse'];

  //Pre-load the record set parent node if we are in Mode 1
  if ($form_state['values']['scope_of_reparse'] == 1) {
    $xml_bearing_node = node_load($form_state['values']['record_set_id']);
    $contents = @file_get_contents($xml_bearing_node->field_marc_xml_file['und'][0]['uri']);

    //Validation before we proceed with Reparse
    //Check for a MARC XML file uri
    if ($contents === FALSE) {
      $error['no_file'] = TRUE;
    } else { //File URI exist, load XML
      $xml = simplexml_load_string($contents);
    }
    //XML was good, supply this to function
    if ($xml === FALSE) {
      $error['bad_xml'] = TRUE;
    } else {
      $return['record_set_xml'] = $contents;
    }

    if ($error) {
      switch ($error) {
        case $error['no_file']:
          drupal_set_message("There is no MARC XML file associated with this record set. Please check the upload: ", 'warning');
          break;
        case $error['bad_xml']:
          drupal_set_message("Invalid XML associated with this record set. Please check the upload: ", 'warning');
          break;
      }
      drupal_goto(check_url($_SERVER['REDIRECT_URL'])); //Go back to the record
    }

    $indexed_collection = array();

    foreach ($xml->children(NAME_SPACE) as $record) {
      $candidate_SCN_positions = array(
        '028' => _cals_importer_xpath_value($record, 'datafield', '028',
          'a'),
        '035' => _cals_importer_xpath_value($record, 'datafield', '035',
          'a'),
        '037' => _cals_importer_xpath_value($record, 'datafield', '037',
          'a'),
        '001' => _cals_importer_xpath_value($record, 'controlfield', '001')
      );
      $found_switch = FALSE;

      foreach ($candidate_SCN_positions as $code) {
        if (! empty($code) && $found_switch != TRUE) $index = (string) $code[0];
        $found_switch = TRUE;
      }

      $indexed_collection[$index] = $record->asXML(); //store as string,
      // gets serialized when passed to bulk action function
    }

    ksort($indexed_collection);
    $return['indexed_collection'] = $indexed_collection;
  }//end scope of reparse mode 1

  return $return; //Note, return value here must be an array.
}

/**
 * @param $node
 * @param $context
 * @return bool
 */

function _cals_reparse_record_details(&$node, $context) {
  $arr_fields = _cals_get_fields_repository_item();

  //Initialize up the field map with target fields that are valid for
  // RepositoryItem
  $reparse_field_map = array_intersect_key($arr_fields['map'], $context['fields_to_parse']);

  //Reparse Mode 0 - default: use only the repo item's MARCXML record
  if ( $context['scope_of_reparse'] == 0 ) {
    _cals_parse_record_details($node, $reparse_field_map, TRUE);
  }
  //Reparse Mode 1 - Use the index built in submit handler to iterate
  elseif ( $context['scope_of_reparse'] == 1 ) {
    $SCNs = entity_metadata_wrapper('node', $node)
      ->field_system_control_number->value();

    //Catch if we are dealing with an unparsed or node lacking SCN at
    // all
    if (empty($SCNs)) {
      drupal_set_message("There were not any SCNs in the selected nodes to match on.");
      return FALSE;
    }

    $parsed = '';

    foreach ($SCNs as $SCN) {
      if (
        //Find a match between the node in context and our index collection of
        // records
        array_key_exists($SCN, $context['indexed_collection']) &&
        !$parsed
      ) {
        //Load the matched record as a single-item SimpleXML collection for field
        // map iterator
        $loaded = MARC_HEADER . "\n" . $context['indexed_collection'][$SCN] . "</marc:collection>";
        $prepared_simplexml_record = simplexml_load_string($loaded);

        _cals_importer_iterate_field_map(
          $node,
          $prepared_simplexml_record,
          $reparse_field_map, //the fields to reparse
          "parse"
        );
        $parsed = TRUE; //Don't reparse in case of SCN duplicate in index
      }
    } //end foreach
  } //end scope of reparse mode 1

  $message = t('Reparsed: (%fields) on Title: "%title"',
    array(
      '%title' => $node->title,
      '%fields' => implode(", ", $context['fields_to_parse'])
    )
  );
  drupal_set_message($message);
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
  /*
  variable_set('error_level', 1);
  dpm(ddebug_backtrace(TRUE));
  variable_set('error_level', 1);
  */
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