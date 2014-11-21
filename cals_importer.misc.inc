<?php

/**
 * @file
 * Misc helper functions
 */


 
/**
 * Inserts new entries into the "field_admin_audit" field collection.
 *
 * @param $node
 *   the node being updated
 * @param $action
 *   an array containing the description, activity, etc.
 *
 **/

function _cals_importer_update_admin_audit($node, $action) {

  global $user;
  $value = array(
    'field_name' => 'field_admin_audit',

    'field_activity' => array(
      LANGUAGE_NONE => array(array('value' => $action['activity']))
    ),
    'field_user' => array(
      LANGUAGE_NONE => array(array('uid' => $user->uid)),
    ),
    'field_date_action' => array(
      LANGUAGE_NONE => array(array('value' => time())),
    ),
  );
  if( !empty($action['description']) ) {
  	$value['field_description'][LANGUAGE_NONE][0]['value'] = $action['description'];
  }
  
  $e = entity_create('field_collection_item', $value);
  // Attach the field_collection entity to the application node.
  $e->setHostEntity('node', $node);
  // Save the entity. 
  $e->save(); 
}


 
/**
 * Inserts new entries into the "field_admin_audit" field collection.
 *
 * @param $node
 *   the node being updated
 * @param $action
 *   an array containing the description, activity, etc.
 *
 **/

function _cals_importer_update_admin_audit($node, $action) {

  global $user;
  $value = array(
    'field_name' => 'field_admin_audit',

    'field_activity' => array(
      LANGUAGE_NONE => array(array('value' => $action['activity']))
    ),
    'field_user' => array(
      LANGUAGE_NONE => array(array('uid' => $user->uid)),
    ),
    'field_date_action' => array(
      LANGUAGE_NONE => array(array('value' => time())),
    ),
  );
  if( !empty($action['description']) ) {
  	$value['field_description'][LANGUAGE_NONE][0]['value'] = $action['description'];
  }
  
  $e = entity_create('field_collection_item', $value);
  // Attach the field_collection entity to the application node.
  $e->setHostEntity('node', $node);
  // Save the entity. 
  $e->save(); 
}

 
/**
 * Helper function to return the nid for a given system number (MARC)
 * 
 * @param $num
 *   the system number
 */
 
function _cals_get_node_by_sysnum($num) {
  $nid = 0;
  $rs = db_query(
    "select *  from {field_data_field_system_control_number} as sysnum 
      where sysnum.field_system_control_number_value = :sysnum and bundle = :bundle", 
      array(":sysnum" => $num, ":bundle" => "repository_item") 
  );
  
  foreach($rs as $row) {
    $nid = $row->entity_id;
    //drupal_set_message("nid = $nid / num = $num");
  }
  return $nid;
}


/**
 * returns message re. malformed / missing  xml
 */
function _cals_get_message_invalid_xml($node) {

	drupal_set_message(
		t('Invalid XML. Please review: <a href="@url">@title</a>.', 
			array('@url' => url('node/' . $node->nid . "/edit"), '@title' => $node->title )
		),
		'error'
	);
	
}

/**
 * Loads XML string from node.
 */

function _cals_get_marc_xml_string($node) {
  libxml_use_internal_errors(true);
  $value = '<xyz>'; //set to something that will fail by default
	if (isset($node->field_xml_string[LANGUAGE_NONE]) ) {
		$value = $node->field_xml_string[LANGUAGE_NONE][0]['value'];
		
	}	
  /*
		keeping this handy for testing validty, etc.
	$value = '<marc:recordsss><marc:leader>01299nam a2200289 a 4500</marc:leader>
	<marc:controlfield tag="001">lbx00091968 </marc:controlfield>
	<marc:controlfield tag="005">20120718105332.0</marc:controlfield><marc:controlfield tag="008">120403s2012    abc           000 j eng  </marc:controlfield><marc:datafield tag="035" ind1=" " ind2=" "><marc:subfield code="a">(OCoLC)764312682</marc:subfield></marc:datafield><marc:datafield tag="035" ind1=" " ind2=" "><marc:subfield code="a">020FB</marc:subfield></marc:datafield><marc:datafield tag="040" ind1=" " ind2=" "><marc:subfield code="a">lbi</marc:subfield><marc:subfield code="d">lbi</marc:subfield><marc:subfield code="d">MvI</marc:subfield><marc:subfield code="d">BVILSB</marc:subfield></marc:datafield></marc:record>';
	*/
  $str = '<?xml version="1.0" encoding="UTF-8"?>
<marc:collection xmlns:marc="http://www.loc.gov/MARC21/slim" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://www.loc.gov/MARC21/slim http://www.loc.gov/standards/marcxml/schema/MARC21slim.xsd">' . 
  $value .
'</marc:collection>';

  $xml = simplexml_load_string($str, null, 0, 'marc', true);
	if(!$xml) {
		_cals_get_message_invalid_xml($node);
		return 0; // return 0, allows individual calling functions to do their own thing
	}
  $xml->registerXPathNamespace("marc", NAME_SPACE);
  return $xml;
}


function _cals_entity_wrapper($arr) {
  $node = new EntityDrupalWrapper('node', 1);
  $node->field_reference[0]->set(463); // Note that this is a multi value field.
  $node->field_text->set('Some Value'); // Note that this is a single value field.
}





function _cals_get_nid_by_title($title) {
  $type = "repository_item";
  
  $result = db_query(
    "SELECT n.nid FROM {node} n WHERE n.title = :title AND n.type = :type", 
    array(":title"=> $title, ":type"=> $type));  
  
  $nid = $result->rowCount();
  if ($nid > 0) $nid = $result->fetchField();
 
  return $nid;

}


function _cals_get_taxonomy_vid($name) {
  return db_query("SELECT vid FROM {taxonomy_vocabulary} WHERE machine_name = :name", 
    array(":name" => $name))->fetchField();

}

function _cals_get_tid_by_name($vid, $name) {
  //$name = $v['value'];
  // Get the parent term's ID.
	$name = _cals_importer_strip_trailing_periods_value($name);

  $tid = db_query(
    "SELECT tid FROM {taxonomy_term_data} 
    WHERE vid = :vid and name = :name
    ORDER BY tid DESC LIMIT 1", 
    array(':vid' => $vid, ':name' => $name)
  )->fetchField();
  
  //if no tid, save current value
  if (!is_numeric($tid) ) {
    $term = (object) array(
      'name' => $name,
      'vid' => $vid
    );
    taxonomy_term_save($term);
    return $term->tid;
  
  }
  
  return $tid;

}

function _cals_get_iso_year($value) {
  $value = (string) $value;
  
  $value = str_replace(array(".", "c"), "", $value);
  
  if (strlen($value) == 4 && is_numeric($value)) return $value;


  //parse dates like 2013/01/01 and 01/01/2013
  if (strlen($value) == 10) {
    
    $tmp = explode("/", $value);
    if (count($tmp) == 3){
      foreach($tmp as $newvalue) {
        if (strlen($newvalue) == 4 && is_numeric($newvalue)) return $newvalue;

      }
    }
    
    $tmp = explode("-", $value);
    if (count($tmp) == 3){
      foreach($tmp as $newvalue) {
        if (strlen($newvalue) == 4 && is_numeric($newvalue)) return $newvalue;
      }
    }
  
  }

  
  
  return $value;

}


function _cals_get_iso_date($value) {
  $value = (string) $value;
  
  $value = str_replace(array(".", "c"), "", $value);
  
  //if the separator is a slash (/), then the American m/d/y is assumed. 
  $value = strtotime($value);
  
  $value = date("Y-m-d", $value);
 
  return $value;

}



function _cals_get_keyword_index_values($arr, $entries) {


  //printPre($entries, $arr);
  foreach($entries as $entry) {
    //$entry = _cals_get_truncated_value($entry, 250);
    if ( strlen(trim($entry) ) && (!in_array($entry, $arr) ) ) {
      $arr[] = trim($entry);
    }
  }
  return $arr;

}
function _cals_get_truncated_value($value, $length) {
  if (strlen($value) > $length) return trim(substr($value, 0, $length) . "...");
  return trim($value);
}

function _cals_get_subject_value($value, $ind) {
  $tmp = array(
    0 => "lcsh", 
    1 => "LC Children's Subject heading",
    2 => "MeSH",
    4 => "undefined",
    6 => "Repertoire des vedettes-matiere",
    8 => "Sears",
  
  );
  $value = _cals_get_truncated_value($value, 250);
  if (array_key_exists($ind, $tmp)) {
    return array("scheme" => $tmp[$ind], "value" => (string) $value);
  } 
  return array("scheme" => "undefined", "value" => (string) $value);

}


/* * *********** Helper / output FUNCTIONS **************** */


//http://www.automation-excellence.com/blog/extending-drupal-7-text-field-character-limit
//e.g., field_isbn, 50

function _cals_increase_field_size($fieldname = "field_isbn", $size = "30") {

  //printAndDie($fieldname, $size);

  $field_to_update = $fieldname; //'field_text_field_to_extend'; //Replace with field slug
  
  $new_chars = $size; //'500';  //Replace with extended character limit
   
  $result1 = db_query('ALTER TABLE {field_data_'.$field_to_update.'} CHANGE '.$field_to_update.'_value '.$field_to_update.'_value VARCHAR('.$new_chars.')');
  $result2 = db_query('ALTER TABLE {field_revision_'.$field_to_update.'} CHANGE '.$field_to_update.'_value '.$field_to_update.'_value VARCHAR('.$new_chars.')');
   
  $result3 = db_query('SELECT CAST(data AS CHAR(10000) CHARACTER SET utf8) data FROM {field_config} WHERE field_name = \''.$field_to_update.'\'');
  foreach ($result3 as $result) {
    $data = $result->data;
  }
  $data_array = unserialize($data);
  $data_array['settings']['max_length'] = $new_chars;
  $new_data = serialize($data_array);
   
  $result4 = db_query('UPDATE {field_config} SET data = :data WHERE field_name = :field', array(':data' => $new_data, ':field' => $field_to_update));

  return $fieldname . " fieldsize increased to " . $size;

}

function printPre() {

    $num = func_num_args();
    $args = func_get_args();

    echo '<hr><blockquote>';

    for ($i = 0; $i < $num; $i++) {
        //$my = var_export($args[$i],true);
        echo '<pre>'; print_r($args[$i]); echo'</pre>';
    }

    echo '</blockquote>';


    //print("<hr><blockquote><pre>"); print_r($tmp);
    print("</pre></blockquote>");
}

function printAndDie() {
    $num = func_num_args();
    $args = func_get_args();

    echo '<div class="indented">';

    for ($i = 0; $i < $num; $i++) {
        //$my = var_export($args[$i],true);
        echo '<pre>';  print_r($args[$i]);  echo'</pre>';
    }
    echo '</div>';
    die;
}
function printSQL($sql) {
  $sql = trim($sql);
  $sql = preg_replace( '/\s+/', ' ', $sql );  
  $sql = str_replace("{", "", $sql);
  $sql = str_replace("}", "", $sql);
  $sql = str_replace("\n", "", $sql);

  $sql = str_replace("FROM", "\n" . "from", $sql);
  $sql = str_replace("left join", "\n" . "left join", $sql);
  $sql = str_replace("inner join", "\n" . "inner join", $sql);
  $sql = str_replace("and", "\n" . "&nbsp; &nbsp; " . "and", $sql);

  $sql = str_replace("where", "\n" . "where", $sql);
  $sql = str_replace("order by", "\n" . "order by", $sql);
  
  return printPre(trim($sql));

}
function printInNiceFormat($tmp) {
    //$my = var_export($tmp, true);
    return print("<pre>");
    print_r($tmp);
    print("<pre>");
} 