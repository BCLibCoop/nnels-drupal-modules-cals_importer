<?php
/**
 * @file
 * CALS Importer Clean Up.
 * Invokes Drupal's batch api to normalize various fields (titles, authors, publishers, etc.)
 * 
 */


/**
 * @param $entity
 */
function _cals_importer_exporter_update_publisher_entity($entity) {
  $wrapper = $entity->wrapper();
  $fc = entity_load_single('field_collection_item', $entity->item_id);
  $node = $fc->hostEntity();

  if(!empty($node->field_publisher)) {
    _cals_importer_exporter_update_publisher($wrapper, $node);
  }
  drupal_goto("node/{$node->nid}");
}

/**
 * Copies Publisher from node to File Attachments (field collections).
 *
 * @param $entity
 * @param array $context
 * @return string
 *
 */
function _cals_importer_exporter_update_publisher_vbo(&$entity, $context = array()) {
  if (isset($entity->item_id)) {
    $wrapper = $entity->wrapper();
    $fc = entity_load_single('field_collection_item', $entity->item_id);
    $node = $fc->hostEntity();
    $fieldname = $context['fieldname'];
    if( $node->type == 'repository_item' && !empty($node->$fieldname) ) {
      _cals_importer_exporter_update_publisher($wrapper, $node, $fieldname);
    }
  }
  return "";
}


/**
 * @param $settings
 * @param $form_state
 * @return mixed
 */
function _cals_importer_exporter_update_publisher_vbo_form($settings, &$form_state) {
  $form['fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Select Publisher Field to Copy'),
    '#collapsible' => TRUE, // Added
    '#collapsed' => FALSE,  // Added
  );
  $form['fieldset']['fieldname'] = array(
    '#type' => 'radios',
    '#title' => t('Date Qualifier'),
    '#required' => TRUE,
    '#size' => 20,
    '#options' => array(
      'field_publisher' => 'Publisher (Node)',
      'field_publisher_source' => 'Publisher Source (Node)'
    ),
    '#default_value' => isset($settings['settings']['publisher_field']) ?
      $settings['settings']['publisher_field'] : 'field_publisher',
  );

  return $form;
}

/**
 * @param $form
 * @param $form_state
 * @return mixed
 */
function _cals_importer_exporter_update_publisher_vbo_submit($form, $form_state) {
  $return['fieldname'] = $form_state['values']['fieldname'];
  return $return; //Note, return value here must be an array.
}



/**
 * Copies publisher and date created values from node to file collection
 */
function _cals_importer_exporter_update_publisher_and_datecreated_vbo(&$entity, $context = array()) {
  if (isset($entity->item_id)) {
    $wrapper = $entity->wrapper();
    $fc = entity_load_single('field_collection_item', $entity->item_id);
    $node = $fc->hostEntity();
    if($node->type == 'repository_item' &&
      ( !empty($node->field_publisher) || !empty($node->field_date) )
    ) {
      _cals_importer_exporter_update_publisher($wrapper, $node);
    }
  }
  return "";
}


/**
 * @param $dates
 * @return null
 */
function _cals_importer_exporter_update_publisher($wrapper, $node, $fieldname = NULL) {
  $values = $node->{$fieldname}[LANGUAGE_NONE];
  if(empty($values)) return;
  if(!($fieldname)) $fieldname = 'field_publisher_source';
  if($fieldname == 'field_publisher_source') {
    _cals_importer_exporter_update_publisher_field_publisher($values, $wrapper);
  }
  else {
    $wrapper->field_publisher_format = $values[0]['value'];
    $wrapper->save();
    _cals_importer_set_message($node, t('Publisher updated from Publisher Source (Node)'));
  }
  return;
}

/**
 * @param $values
 * @param $wrapper
 */
function _cals_importer_exporter_update_publisher_field_publisher($values, $wrapper) {
  $count = count($values);
  $pub = '';
  $place = '';
  if($count == 1){
    if(strpos($values[0]['value'], ":")) {
      $tmp = explode(":", $values[0]['value']);
      $place = trim($tmp[0]);
      $pub = trim($tmp[1]);
    }
    else{
      $pub = trim($values[0]['value']);
    }
  }
  elseif($count > 1) {
    //More than one value, loop through and use first index as place, later
    // as pub

    //Need some way of tagging the subfields a's and b's in earlier function

    //2 subfields = a:Place b:Pub
    //3 subfields = a:Place b:Pub, a:Place b:NULL
    //4 subfields = a:Place b:Pub, a:Place, b:Pub
    //Ignore > 4

    switch($count) {
      case 2:
        $place = $values[0]['value'];
        $pub = $values[1]['value'];
      break;

      case 3:
        $place .= $values[0]['value'];
        $place .= ', ' . $values[1]['value'];
        $pub .= $values[2]['value'];
        break;

      default:
      case 4:
        $place .= $values[0]['value'];
        $place .= ', ' . $values[1]['value'];
        $pub .= $values[2]['value'];
        $pub .= ', ' . $values[3]['value'];
        break;
    }
  }
  else {
    return;
  }
  if( !empty($pub)  ) $wrapper->field_publisher_format = str_replace( array('[',']'), '', $pub );
  if( !empty($place) ) $wrapper->field_place_of_publication = str_replace(
    array('[',']'), '', $place );
  //Don't use _cals_importer_strip_square_brackets, it bakes in node-level field
  $wrapper->save();
  //$url = url(drupal_get_path_alias('node/' . $node->nid));
  _cals_importer_set_fileres_message($wrapper, t('Publisher fields updated for file: '));
  //drupal_set_message(t('Publisher updated for <a href="@url">@title</a>@title', array('@url' => $url, '@title' => $node->title)));
}

/**
 * Messaging function for wrapped Resource entities
 * @param $wrapper
 * @param null $message
 */
function _cals_importer_set_fileres_message($wrapper, $message = NULL) {
  if (! $wrapper->getBundle() == 'field_file_resource') {
    drupal_set_message("The bulk operation used called the wrapper message function without passing a File Resource entity ", 'warning');
    //Trickle down to generic messaging for nodes
    cals_importer_set_message($wrapper, $message);
    return;
  }

  $url = url(drupal_get_path_alias('field-collection/field-file-resource/' .
    $wrapper->getIdentifier() . '/edit'));
  if($message)
    $msg = $message . ": " . t('<a href="@url">@title</a>', array('@url' =>
        $url, '@title' => $wrapper->host_entity->title->value() . " (" .
        $wrapper->field_file_format->label() . ") "));
  else
    $msg = t('Title updated: <a href="@url">@title</a>', array('@url' =>
      $url, '@title' => $wrapper->host_entity->title->value()));
  drupal_set_message($msg);
}

/**
 * Messsaging function for nodes
 * @param $node
 * @param null $message
 */
function _cals_importer_set_message($node, $message = NULL) {
  $url = url(drupal_get_path_alias('node/' . $node->nid));
  if($message)
    $msg = $message . ": " . t('<a href="@url">@title</a>', array('@url' => $url, '@title' => $node->title));
  else
    $msg = t('Title updated: <a href="@url">@title</a>', array('@url' => $url, '@title' => $node->title));
  drupal_set_message($msg);
}

/**
 * Updates Date Created
 *
 * @param $entity
 * @param array $context
 * @return string
 */
function _cals_importer_exporter_update_date_created_entity_vbo(&$entity, $context = array()) {
  if (isset($entity->item_id)) {
    $wrapper = $entity->wrapper();
    $fc = entity_load_single('field_collection_item', $entity->item_id);
    $node = $fc->hostEntity();
    $qualifier = ($context['date_type'] == '_none') ? '' : $context['date_type'];
    if(!empty($node->field_date) && $node->type == 'repository_item')
      _cals_importer_exporter_update_fc_date($wrapper, $node, $qualifier);
  }//end main if
  return "";
}

/**
 * @param $settings
 * @param $form_state
 * @return mixed
 */
function _cals_importer_exporter_update_date_created_entity_vbo_form($settings, &$form_state) {
  $form['fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Select Date Type to Copy'),
    '#collapsible' => TRUE, // Added
    '#collapsed' => FALSE,  // Added
  );
  $form['fieldset']['date_type'] = array(
    '#type' => 'radios',
    '#title' => t('Date Qualifier'),
    '#required' => TRUE,
    '#size' => 20,
    '#options' => array('_none' => 'No Dublin Core Qualifier', 'copyrighted' => 'copyrighted', 'created' => 'created', 'issued' => 'issued'),
    '#default_value' => isset($settings['settings']['date_type']) ? $settings['settings']['date_type'] : 'created',
  );

  return $form;
}

/**
 * update / add system control numbers
 */
function _cals_importer_exporter_update_date_created_entity_vbo_submit($form, $form_state) {
  $return['date_type'] = $form_state['values']['date_type'];
  return $return; //Note, return value here must be an array.
}


/**
 * @param $entity
 */
function _cals_importer_exporter_update_date_issued_entity($entity) {
  $wrapper = $entity->wrapper();
  $fc = entity_load_single('field_collection_item', $entity->item_id);
  $node = $fc->hostEntity();
  if(!empty($node->field_date)) {
    _cals_importer_exporter_update_fc_date($wrapper, $node, "issued");
  }
  drupal_goto("node/{$node->nid}");
}

/**
 * @param $entity
 */
function _cals_importer_exporter_update_date_copyrighted_entity($entity) {
  $wrapper = $entity->wrapper();
  $fc = entity_load_single('field_collection_item', $entity->item_id);
  $node = $fc->hostEntity();
  if(!empty($node->field_date)) {
    _cals_importer_exporter_update_fc_date($wrapper, $node, 'copyrighted');
  }
  drupal_goto("node/{$node->nid}");
}


/**
 * @param $entity
 */
function _cals_importer_exporter_update_date_created_entity($entity) {
  $wrapper = $entity->wrapper();
  $fc = entity_load_single('field_collection_item', $entity->item_id);
  $node = $fc->hostEntity();
  if(!empty($node->field_date)) {
    _cals_importer_exporter_update_fc_date($wrapper, $node, 'created');
  }
  drupal_goto("node/{$node->nid}");
}
/**
 * @param $entity
 */
function _cals_importer_exporter_update_date_noqualifier_entity($entity) {
  $wrapper = $entity->wrapper();
  $fc = entity_load_single('field_collection_item', $entity->item_id);
  $node = $fc->hostEntity();
  if(!empty($node->field_date)) {
    _cals_importer_exporter_update_fc_date($wrapper, $node, '');
  }
  drupal_goto("node/{$node->nid}");
}

/**
 * Saves the wrapper
 * @param $wrapper
 * @param $node
 */
function _cals_importer_exporter_update_fc_date($wrapper, $node, $date_qualifier) {
  $arr = _cals_importer_exporter_get_date_by_type($wrapper, $node, $date_qualifier);
  $value = $arr[0];
  if(is_array($arr) && is_numeric($value)) {
    $id = $arr[1];
    $wrapper->field_date_created = $value;
    $wrapper->save();
    $date = date("Y", $value);
    $msg = ($id == 0) ? t("Date Updated via File Timestamp") : t("Date Updated via @dq value",
      array('@dq' => $date_qualifier));
    _cals_importer_set_message($node, $msg);
  }
}

/**
 * @param $dates
 * @return null
 */
function _cals_importer_exporter_get_date_by_type($wrapper, $node, $date_qualifier) {
  $dates = $node->field_date[LANGUAGE_NONE];
  $timestamp = $wrapper->field_s3_file_upload[0]->file->value()->timestamp;
  if($date_qualifier == '_none') $date_qualifier = '';
  foreach($dates as $k => $v) {
    $id = $v['value'];
    $fc = entity_load_single('field_collection_item', $id);
    $wrapper = $fc->wrapper();
    $label = strtolower($wrapper->field_qualifier_date->value());

    if($label == 'copyright') $label = 'copyrighted';
    $value = trim($wrapper->field_dc_date->value());
    //printPre($label, $value);
    //if ($label == 'created')       printAndDie($value);
    if ($label == $date_qualifier) {
      //sometimes the values are YYYY
      if(strlen($value) == 4) $value .= '0101';
      if(strlen($value) == 6) $value .= '01';
      if(strlen($value) == 7) $value .= '-01';
      if(strlen($value) == 8 || strlen($value) == 10) {
        $value = strtotime($value);
        $arr = array($value, $id);
        return $arr;
      }
    }
  }
  if(!empty($timestamp)) {
    return array($timestamp, 0);
  }
  return;
}

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
    $action['description'] = '';
    $action['activity'] = 5;
    _cals_importer_update_admin_audit($node,  $action);
    
    node_save($node);
  
  }
}

/**
 * bulk operation action to normalize performer values.
 * Can likely delete this after run since the action is
 * now baked directly into the initial parse
 */

function _cals_normalize_performer(&$entity, $context = array()) {
  if(function_exists("_cals_importer_get_normalized_performer") && isset($entity->field_performer[LANGUAGE_NONE])) {
    $p1 = $entity->field_performer[LANGUAGE_NONE][0]['value'];
    $p2 = _cals_importer_get_normalized_performer($p1);
    if($p1 != $p2) {
      $entity->field_performer[LANGUAGE_NONE][0]['value'] = $p2;
      $entity->save();
    }
  }
} 

/**
 * bulk operation to copy field collection values to subjects.
 */

function _cals_copy_field_collection_values_to_subjects(&$entity, $context = array()) {
 
  if (isset($entity->nid)) {
  	$nid = $entity->nid;
    $node = node_load($nid);
    $items = field_get_items('node', $node, 'field_dc_subject');
		unset($node->field_subject[LANGUAGE_NONE]);
		foreach ($items as $item) {
	 		$fc = field_collection_field_get_entity($item);
	 		$tid = $fc->field_dc_subject_value['und'][0]['tid'];
			$term = taxonomy_term_load($fc->field_dc_subject_value['und'][0]['tid']);
			$terms[$tid] = $term->name;
		}
		asort($terms);
		foreach($terms as $tid => $name) {
		  $node->field_subject[LANGUAGE_NONE][]['tid'] = $tid;
		  $names[] = $name;
		}
		$names = implode(" | ", $names);
    node_save($node);
    $msg = l($node->title, "node/" . $nid) . " updated with " . count($terms) . " subjects: " . $names;
    drupal_set_message($msg);
  }//end main if
 
  return "";
}



/**
 * bulk operation to normalize taxonomy terms.
 */

function _cals_normalize_taxonomy_terms(&$entity, $context = array()) {
 
  if (isset($entity->tid)) {
    //$term = entity_load($entity->tid);
    $term = taxonomy_term_load($entity->tid);
    $name = _cals_importer_strip_trailing_periods_value($term->name, 'n/a');
    if($name !== $term->name) {
      $term->name = $name;
      taxonomy_term_save($term);
    }
    
  }//end main if
 
  return "";
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
      //$stopwords = $context['cals_importer_stop_words'];
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
  //create a shadow copy to track changes
  $node2 = new stdClass();
  $fields_to_update = array("title", "field_dc_creator", "field_dc_contributor", "field_publisher");
  $msg = array(); //used to track changes; if count > 1, we save node
  foreach($fields_to_update as $field) {
    if(!empty($node->{$field})) {
      $node2->{$field} = $node->{$field};
      switch($field) {
        case "title":
          _cals_importer_strip_square_brackets($node, $field);
          _cals_importer_strip_trailing_periods($node, "title");
          //_cals_importer_normalize_title_fields($node, $field); <- deprecate
          break;
        case "field_dc_creator":
        case "field_dc_contributor":
          _cals_importer_strip_square_brackets($node, $field);
          _cals_importer_normalize_creators($node, $field);
          break;
        case "field_publisher_source":
          //_cals_importer_strip_square_brackets($node, "title");
          //_cals_importer_normalize_publisher($node);
          _cals_importer_strip_trailing_periods($node, $field);
          break;
      
      
      }
      
      //generate error messages
      if($field == 'title') {
        if($node2->title != $node->title) $msg[] = t("title: ORIGINAL = !title || NEW title = !title2", 
          array("!title" => $node2->title, "!title2" => $node->title));
      }
      else {
        if(!empty($node->$field)) {
          foreach($node->{$field}[LANGUAGE_NONE] as $k => $v) {
            if($node2->{$field}[LANGUAGE_NONE][$k]['value'] != $v['value']) 
              $msg[] = t("!fieldname: ORIGINAL = @value || NEW = @value2",
                array("!fieldname" => $field, "@value" => $node2->{$field}[LANGUAGE_NONE][$k]['value'], "@value2" => $v['value'] ));
          }
        }
      }       
    }
  }
 
  if (count($msg)) {
    $dmsg = theme('item_list', array('items' => $msg, 'title' => t("Updated !title", array("!title" => $node->title) ) ));
    //$dmsg = t("Updated")  . $title . ": " . implode(" | ", $msg);  
    drupal_set_message($dmsg);
    $action['description'] = implode("\n", $msg);
    $action['activity'] = 3;
    _cals_importer_update_admin_audit($node,  $action);
    node_save($node);
  }
  else {
    drupal_set_message(t("Not updated: @title", array('@title' => $title ) ) );
  }

}

/**
 * re-sets the Publisher
 *
 */

function _cals_update_publisher_264_tag(&$node) {

  //$node->field_publisher_source
  if(!empty($node->field_publisher_source)) {
    foreach($node->field_publisher[LANGUAGE_NONE] as $k => $publisher) {
      //$tmp = _cals_importer_normalize_publisher_entry($publisher['value']);
      $tmp = _cals_importer_strip_trailing_periods_value($publisher['value'], 'field_publisher_source');
      if($tmp != $publisher['value']) {
        $node->field_publisher[LANGUAGE_NONE][$k]['value'] = $tmp;
      }
    }

  }

}


/**
 * loops thru publisher array, calling _cals_importer_normalize_publisher_entry() 
 *
 */

function _cals_importer_normalize_publisher(&$node) {

  if(!empty($node->field_publisher)) {
    foreach($node->field_publisher[LANGUAGE_NONE] as $k => $publisher) {
      //$tmp = _cals_importer_normalize_publisher_entry($publisher['value']);
      $tmp = _cals_importer_strip_trailing_periods_value($publisher['value']);
      if($tmp != $publisher['value']) {
        $node->field_publisher[LANGUAGE_NONE][$k]['value'] = $tmp;
      }
    }   
  
  }
  
}

/**
 * bulk operation action to covert creator names to title case.
 */

function _cals_titlecase_creators(&$entity, $context = array()) {
  if (isset($entity->nid)) {
    
    $node = node_load($entity->nid);
    //safety check to make sure we're parsing repos items!!    
    $content_types = array('repository_item');
    if (in_array($node->type, $content_types)  ) {
      _cals_importer_convert_to_title_case($node, "field_dc_creator");
    }
    else{
      drupal_set_message(t("Invalid selection: not a repos item"));
    }

  }//end main if
 
  return "";
} 


/**
 * Converts Creator fields to title case  
 */
function _cals_importer_convert_to_title_case(&$node, $field) {
	
  if(!empty($node->{$field})) {
    foreach($node->{$field}[LANGUAGE_NONE] as $k => $v) {
    	$tmp = $v['value'];
    	if(strpos($tmp, ",")) {
    	  
    	  $tmp2 = ucwords(strtolower($tmp)); 
    	  if($tmp2 != $tmp) {
    	  	$node->{$field}[LANGUAGE_NONE][$k]['value'] = $tmp2;
    	  	node_save($node);
    	  	$link = l("view record", "node/" . $node->nid, array('attributes' => array('target' => '_blank')));
					drupal_set_message($tmp . " => " . $tmp2 . " view record: " . $link);
    	  }
    	}
    }
  } 
}

/**
 * Strips opening and closing parentheses from creators,  
 */
function _cals_importer_normalize_creators(&$node, $field) {
  if(!empty($node->{$field})) {
    foreach($node->{$field}[LANGUAGE_NONE] as $k => $v) {
     $tmp = rtrim($v['value']); //trim various ascii chars
     //logic to determine if there's a ...
     $q = "?";
     $tmp = str_replace($q, "", $tmp);
     $len = strlen($tmp);
     $sub = substr($tmp, 0, ($len - 1)); //substring minus 1 character to figure out # of potential replacements
     $count = substr_count($sub, ".") + substr_count($sub, ",");  
     if ($count != 0) { 
      $tmp = _cals_importer_strip_years_from_creator_values($tmp);
      $tmp = rtrim($tmp, ", -"); //strip remaining trailing periods, dashes, etc.;
      //check to see if original value matches new value
      //$tmp = str_replace("??", "", $tmp);
      if ($tmp != $v['value']) {
        $node->{$field}[LANGUAGE_NONE][$k]['value'] = $tmp;
        //$msg[] = t("updated author:") . " " .  $tmp;
      }
     }
    }
    _cals_importer_strip_trailing_periods($node, $field);

  } 
}

/**
 * wrapper function to strip punctuation where applicable  
 */
function _cals_importer_strip_trailing_periods(&$node, $field) {
  if($field == 'title') {
    $tmp = _cals_importer_strip_trailing_periods_value($node->title, $field);
    if($tmp != $node->title) {
      $node->title = $tmp;
      $node->title_field[LANGUAGE_NONE][0]['value'] = $tmp;
    }
  }
  else {
    if(!empty($node->{$field})) {
      foreach($node->{$field}[LANGUAGE_NONE] as $k => $v) {
        $tmp = _cals_importer_strip_trailing_periods_value($v['value'], $field);
        if ($tmp != $v['value'] && !empty($tmp)) {
          $node->{$field}[LANGUAGE_NONE][$k]['value'] = $tmp;
        }
      }
    }
  }
}


/**
 * Strips trailing punctuation, except abbreviations, etc.  
 * e.g. a.b., A.B., A. B., Dave B.
 */
function _cals_importer_strip_trailing_periods_value($str, $field = NULL) {
  $tmp = str_replace("  ", " ", rtrim($str)); //trim various ascii chars
  $tmp = (string) trim($tmp, "  ");

  $patterns = array(",", " ", "!", "?", ":", ";", "/", "=", "-"); //leave out "." until end
  $imploded = implode("", $patterns);
  $flat = implode("", $patterns);
  $tmp = rtrim($tmp, $imploded);
  $len = strlen($tmp);

  //stopwords: e.g., ed. - we skip those
  $stopwords = explode("\r\n", variable_get('cals_importer_stop_words'));
  foreach($stopwords as $word) {
    $x = 0;
    $pos = strripos($tmp, $word);
    if($pos === $len - strlen($word) ) return $tmp; //return if we find a match at the end of the string!
  }
  if (_cals_importer_str_ends_abbreviation($tmp, $field)) return $tmp;

  $tmp = _cals_importer_recurse_trailing_characters($str);
  return $tmp;

}
/**
 * Recurse thru string to remove remaining non-aphanumeric strings
 */
function _cals_importer_recurse_trailing_characters($str) {
  $patterns = array(",", " ", "!", "?", ":", ";", "/", "\\", ".", "-");
  $implode = implode("", $patterns);
  $tmp = rtrim($str, $implode);
  $len = strlen($tmp);
  $last =  (substr(trim($tmp), -1) );
  if(!in_array($last, $patterns)) {
    return trim($tmp); //one last, in case we miss something... may need to recurse this?
  }
  else {
    $str = $tmp;
    return _cals_importer_recurse_trailing_characters($str);
  }


}

/**
 * Returns true if string ending is likely an abbreviation, e.g.,
 * Vancouver, B.C.
 */
function _cals_importer_str_ends_abbreviation($val, $field) {

  $val = str_replace("  ", " ", $val);
  //e.g., E.D. or E. D. - i.e., no spaces or one space
  $pattern = "/([a-zA-Z]+\.|)(\s)+([a-zA-Z]{1,3}\.)+$/";
  if (preg_match($pattern, $val, $matches)) return TRUE;

  return FALSE;

  /*
  $last = substr($val, -1);
  if ( (substr($val, -1) == '.')  ) {
    if(substr($val, -2) == '..') return FALSE;
    $last = substr($val, -6);
    //$segment = substr($val, -6, 5);
    $num_periods = substr_count($last, ".");
    //e.g., E.D. or E. D. - i.e., no spaces or one space
    if( preg_match('/[a-zA-Z]\.(| )[a-zA-Z]\.$/', $val, $matches)) {
      drupal_set_message("LAST = $last | num periods = $num_periods | val = $val", $matches);
      return TRUE;
    }
  }
  return FALSE;
  */
}

/**
 * Strips opening and closing parentheses from title, publisher fields  
 */
function _cals_importer_strip_square_brackets(&$node, $field) {
  //$str = str_replace(array("[", "]"), "", $str);
  if($field == 'title')
    $node->title = str_replace(array("[", "]"), "", $node->title);

  else {
    if(isset($node->{$field}[LANGUAGE_NONE])) {
    
	    foreach($node->{$field}[LANGUAGE_NONE] as $k => $v) {
	      $tmp = str_replace(array("[", "]"), "", $v['value']);
	      if($tmp != $node->{$field}[LANGUAGE_NONE][$k]['value']) $node->{$field}[LANGUAGE_NONE][$k]['value'] = $tmp;
	    }
    
    }
    
  }
}
/**
 * Strips opening and closing parentheses from title, publisher fields
 */
function _cals_importer_strip_square_brackets_value($value) {
  return str_replace(array("[", "]"), "", $value);
}

/**
 * Strips opening and closing parentheses from title, publisher fields  
 */
function _cals_importer_strip_parentheses_from_values($str) {
  $skip = 0;
  $str = rtrim($str);
  //end of string, e.g. : [xyz] 
  $regexp = "/ [\[a-zA-Z0-9_\]]{1,}$/";
  preg_match($regexp, $str, $matches, PREG_OFFSET_CAPTURE);
  if (count($matches) )  {
    //we know this should be a single match because its end of string
    $count = $matches[0][1];
    $str = substr($str, 0, $count) .  str_replace(array("[", "]"), "", substr($str, $count));
  }
  return rtrim($str);
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
 * Simple wrapper to preview the normalization process
 */

function _cals_normalize_record_value_preview($node) {
  _cals_importer_normalize_creators_preview_query($node);
  drupal_goto("node/" . $node->nid);
}

/**
 * Previews creator normalization  
 */
function _cals_importer_normalize_creators_preview_vbo(&$entity, $context = array()) {

  if (isset($entity->nid)) {
    $node = node_load($entity->nid);
    //safety check to make sure we're parsing repos items!!    
    $content_types = array('repository_item');
    if (in_array($node->type, $content_types)  ) {
      _cals_importer_normalize_creators_preview_query($node);
    }
    else{
      drupal_set_message(t("Invalid selection: not a repos item"));
    }

  }//end main if
 
  return "";
  
}


function _cals_importer_normalize_creators_preview_query($node) {
  _cals_importer_strip_square_brackets($node);
  _cals_importer_normalize_creators($node);
  _cals_importer_strip_trailing_periods($node);
  //dpm($node->field_dc_creator);
  foreach($node->field_dc_creator[LANGUAGE_NONE] as $k => $author) {
    //drupal_set_message("ORIGINAL: " . $author['safe_value'] . " | NORMALIZED: " . $author['value']);
  }

}

  
/**
 * Updates the publisher value
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
   $tmp = _cals_importer_strip_trailing_periods_value($tmp);
   printAndDie(__FUNCTION__, $tmp);
   return $tmp;
}