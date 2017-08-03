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
  }
  return $nid;
}


/**
 * returns message re. malformed / missing  xml
 */
function _cals_get_message_invalid_xml($node) {

  //printAndDie($node);

	drupal_set_message(
		t('Invalid XML. Please review: <a href="@url">@title</a>.', 
			array('@url' => url('node/' . $node->nid . "/edit"), '@title' => $node->title )
		),
		'error'
	);
	
}

/**
 * Loads XML string from node.
 *
 * @param $node
 * @return int|\SimpleXMLElement
 */
function _cals_get_marc_xml_string($node) {
  libxml_use_internal_errors(true);
  $value = '<xyz>'; //set to something that will fail by default
	if (isset($node->field_xml_string[LANGUAGE_NONE]) )
		$value = $node->field_xml_string[LANGUAGE_NONE][0]['value'];
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

/**
 * @param \FieldCollectionItemEntity $entity
 * @return \EntityMetadataWrapper
 */
function _cals_wrap_fc($entity) {
    $fc_wrapper = entity_metadata_wrapper('field_collection_item', $entity);
  return $fc_wrapper;
}

/**
 * Given a wrapped FieldCollectionItemEntity, append each tag-value pair
 * from its snippet to its stub, pass through Data_Field sorting
 * function and return an XML string for writing to export file.
 *
 * @param \EntityMetadataWrapper $fc_wrapper
 * @param string $str
 * @return string $records
 */
function _cals_get_set_xml_snippet($fc_wrapper, $str) {

  require_once 'File/MARCXML.php';
  require_once 'File/MARC/Lint.php';

  $records = '';
  $stub_record = new File_MARCXML($str, File_MARC::SOURCE_STRING);
  $record = $stub_record->next();
  $this_snippet = $fc_wrapper->field_marc_xml_snippet[0]->value();

  $snippets = new File_MARCXML($this_snippet, File_MARC::SOURCE_STRING);

  if ($record) {
      //there's only one record snippet per file resource so just advance once
      $snippet_context = $snippets->next();

      foreach ($snippet_context->getFields() as $tag => $value) {
        //adjust byte 6 of leader to reflect added formats since ingest
        $record->appendField($value);
      }

      //Stub + new fields to sorter, returns XML string
      $records .= _cals_importer_exporter_sort_record_datafields($record) . "\n";
  }

  return $records;
}


function _cals_entity_wrapper($arr) {
  $node = new EntityDrupalWrapper('node', 1);
  $node->field_reference[0]->set(463); // Note that this is a multi value field.
  $node->field_text->set('Some Value'); // Note that this is a single value field.
}

function _cals_get_tid_by_name($vid, $name) {
  //$name = $v['value'];
  // Get the parent term's ID.
  $name = _cals_get_truncated_value(_cals_importer_strip_trailing_periods_value($name), 250);
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

/**
 * @param $value
 * @return mixed
 */
function _cals_get_iso_year($value) {
  $value = (string) $value;
  //get years from dates like 2013/01/01 and 01/01/2013
  if (strlen($value) == 10) {
    $tmp = explode("/", $value);
    if (count($tmp) == 3){
      foreach($tmp as $newvalue) {
        if (strlen($newvalue) == 4) return _cals_assert_numstring($newvalue);
      }
    }

    //Must be dashes as in 2013-01-01
    $tmp = explode("-", $value);
    if (count($tmp) == 3){
      foreach($tmp as $newvalue) {
        if (strlen($newvalue) == 4) return _cals_assert_numstring($newvalue);
      }
    }
  }

  else {
    //We have something else, "1999", or a 534$c with date prepended, or some bracketed form like [1999]
    //Copyright symbol was already filtered by _cals_parse_date_issue_copyright

    return _cals_assert_numstring($value);
  }

}

function _cals_assert_numstring($value = '') {
  return preg_replace("/[^0-9]+/", "", $value);
}


function _cals_get_iso_date($value) {
  $value = (string) $value;
  
  $value = str_replace(array(".", "c", "[", "]", "(", ")"), "", $value);
  
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
  if (strlen($value) > $length) return truncate_utf8($value, $length, FALSE, TRUE); //trim(substr($value, 0, $length) . "...");
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

//Array Sort Helper
function _cals_array_sort($array, $on, $order=SORT_ASC){

    $new_array = array();
    $sortable_array = array();

    if (count($array) > 0) {
        foreach ($array as $k => $v) {
            if (is_array($v)) {
                foreach ($v as $k2 => $v2) {
                    if ($k2 == $on) {
                        $sortable_array[$k] = $v2;
                    }
                }
            } else {
                $sortable_array[$k] = $v;
            }
        }

        switch ($order) {
            case SORT_ASC:
                asort($sortable_array);
                break;
            case SORT_DESC:
                arsort($sortable_array);
                break;
        }

        foreach ($sortable_array as $k => $v) {
            $new_array[$k] = $array[$k];
        }
    }

    return $new_array;
}