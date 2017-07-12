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
 * returns
 * @param $tag
 * @param null $ind1
 * @param null $ind2
 * @param null $code
 * @return string
 */
function _cals_importer_get_xpath_path($tag, $ind1 = NULL, $ind2 = NULL, $code = NULL) {
  $path = "//marc:datafield[@tag=$tag]";
  if(isset($ind1)) $path .= "[@ind1='{$ind1}']";
  if(isset($ind2)) $path .= "[@ind2='{$ind2}']";
  if(isset($code)) $path .= "/marc:subfield[@code='{$code}']";
  return $path;
}
/**
 * Returns an array of string values from an xpath statement
 * @param $path
 * @return array
 */
function _cals_importer_get_xpath_values($tag, $ind1, $ind2, $code, $xml) {
  $arr = array();
  $path = _cals_importer_get_xpath_path($tag, $ind1, $ind2, $code);
  $xpathvalue = $xml->xpath($path);
  //$tmp = $xml->xpath($path);
  if (count($xpathvalue)) {
    foreach ($xpathvalue as $me)
      $arr[] = (string) trim($me[0]);
  }
  return $arr;
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
 * Returns an array of indicators for given tag
 * @param $xml
 * @param $tag
 * @return array
 */
function _cals_importer_get_marc_indicators($xml, $tag, $int) {
  $parent = $xml->xpath("//marc:datafield[@tag=$tag]");
  $ind = array();
  printAndDie(__FUNCTION__, $parent);
  foreach($parent as $i => $child) {
    /*
    $ind = array(
      1 => (string) $parent[$i]->attributes()->ind1,
      2 =>  (string) $parent[$i]->attributes()->ind2,
    );
    */
    return ($int1 == 1) ? (string) $parent[$i]->attributes()->ind1 :
      (string) $parent[$i]->attributes()->ind2;

  }


}

/**
 * Returns number of nonfiling characters (for title fields, etc.)
 * @param $value
 * @return int
 */

function _cals_importer_exporter_filing_characters($value) {
  $value = strtolower($value);
  $arr = array(
    "a ", "a'", "al ", "al- ", "am ", "an ", "an t- ", "ane ", "ang ", "ang mga ",
    "as ", "az ", "bat ", "bir ", "d'", "da ", "das ", "de ", "dei ", "dem ",
    "den ", "der ", "des ", "det ", "di ", "die ", "dos ", "e ", "'e ", "een ",
    "eene ", "egy ", "ei ", "ein ", "eine ", "einem ", "einen ", "einer ",
    "eines ", "eit ", "el ", "el- ", "els ", "en ", "enne ", "et ", "ett ",
    "eyn ", "eyne ", "gl'", "gli ", "ha- ", "hai ", "he ", "hē ", "he- ",
    "heis ", "hen ", "hena ", "henas ", "het ", "hin ", "hina ", "hinar ", "hinir ",
    "hinn ", "hinna ", "hinnar ", "hinni ", "hins ", "hinu ", "hinum ", "hið ",
    "ho ", "hoi ", "i ", "ih'", "il ", "il- ", "in ", "it ",
    "ka ", "ke ", "l'", "l- ", "la ", "las ", "le ", "les ", "lh ", "lhi ",
    "li ", "lis ", "lo ", "los ", "lou ", "lu ", "mga ", "mia ", "'n ", "na ",
    "na h- ", "njē ", "ny ", "'o ", "o ", "os ", "'r ", "'s ", "'t ", "ta ",
    "tais ", "tas ", "tē ", "tēn ", "tēs ", "the ", "to ", "tō ", "tois ",
    "ton ", "tōn ", "tou ", "um ", "uma ", "un ", "un'", "una ", "une ",
    "unei ", "unha ", "uno ", "uns ", "unui ", "us ", "y ", "ye ", "yr ");

  foreach($arr as $char) {
    $len = strlen($char);
    $substr = substr($value, 0, $len);
    if($substr == $char) return $len;
  }
  return 0;

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

/**
 * @param $arr
 * @param $entries
 * @return array
 */

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

function printPrettyArray() {
  $num = func_num_args();
  $args = func_get_args();
  $html = '<hr><blockquote>';
  for ($i = 0; $i < $num; $i++) {
    //$my = var_export($args[$i],true);
    $html .= '<pre>'; print_r($args[$i]); echo'</pre>';
  }
  $html .= "</pre></blockquote>";
  return $html;
}

function printPre() {
  $num = func_num_args();
  $args = func_get_args();
  print '<hr><blockquote>';
  $functions = debug_backtrace()[1]['function'];
  //get prior function, if exists
  if(debug_backtrace()[2]['function']) $functions = debug_backtrace()[2]['function'] . ' => ' . $functions;
  print $functions;
  for ($i = 0; $i < $num; $i++) {
    //$my = var_export($args[$i],true);
    print '<pre>'; print_r($args[$i]); echo'</pre>';
  }
  print("</pre></blockquote>");
}

function printAndDie() {
  $num = func_num_args();
  $args = func_get_args();
  print '<div class="indented">';

  $functions = debug_backtrace()[1]['function'];
  //get prior function, if exists
  if(debug_backtrace()[2]['function']) $functions = debug_backtrace()[2]['function'] . ' => ' . $functions;
  print $functions;

  for ($i = 0; $i < $num; $i++) {
    //$my = var_export($args[$i],true);
    print '<pre>';
    print_r($args[$i]);
    print '</pre>';
  }
  print '</div>';
  //$functions = debug_backtrace()[1]['function'];
  //get prior function, if exists


  print '</div>';
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

/**
 * To deprecate?
 */

/**
 * Iterate thru taxonomy terms (field_subject, field_genre) and add values to
 * current record
 *
 * @param $node
 * @param $record
 */
function _cals_importer_exporter_add_nnels_values_marcxml($node, &$record) {
  //if vocab, $term = 'field_subject';
  $fieldnames =
    array(
      //'field_system_control_number' => array('001', '', '', '', NULL),
      'field_dc_creator' => array('040', 'a', 1, '', NULL),
      'title_field' => array(245, 'a', 1, 4, 'normalize_title'),

      //'field_access_restrictions' => array(),
    );

  foreach($fieldnames as $fieldname => $marc) {
    $tag = $marc[0];
    $code = $marc[1];
    $ind1 = $marc[2];
    $ind2 = $marc[3];
    $callback = $marc[4];
    $values = field_view_field('node', $node, $fieldname, array('default'));

    if(is_array($values)) {
      foreach($values['#items'] as $value) {
        $subfields = array();
        $safe_value = trim($value['value']);
        if(!empty($callback)) $callback($safe_value);
        $subfields = array();
        if($tag != '001') $subfields[] = new File_MARC_Subfield($code, $safe_value);
        $field = new File_MARC_Data_Field($tag, $subfields, 0, null);
        $field->setIndicator(1, $ind1);
        $field->setIndicator(2, $ind2);
        _cals_importer_exporter_insert_field($record, $field, $key);
      }
    }

  }
  //return new File_MARC_Data_Field('650', $subfields, 0, null);
}
function _iterate_tags_deprecate() {

  $fieldnames =
    array(
      //'field_system_control_number' => array('001', '', '', '', NULL),
      'field_dc_creator' => array('040', 'a', 1, '', NULL),
      'title_field' => array(245, 'a', 1, 4, 'normalize_title'),

      //'field_access_restrictions' => array(),
    );

  foreach($fieldnames as $fieldname => $marc) {
    $tag = $marc[0];
    $code = $marc[1];
    $ind1 = $marc[2];
    $ind2 = $marc[3];
    $callback = $marc[4];
    $values = field_view_field('node', $node, $fieldname, array('default'));

    if(is_array($values)) {
      foreach($values['#items'] as $value) {
        $subfields = array();
        $safe_value = trim($value['value']);
        if(!empty($callback)) $callback($safe_value);
        $subfields = array();
        if($tag != '001') $subfields[] = new File_MARC_Subfield($code, $safe_value);
        $field = new File_MARC_Data_Field($tag, $subfields, 0, null);
        $field->setIndicator(1, $ind1);
        $field->setIndicator(2, $ind2);
        _cals_importer_exporter_insert_field($record, $field, $key);
      }
    }

  }

  foreach ($record->getFields() as $tag=>$value) {

    if ($value instanceof File_MARC_Control_Field) {

    }
    else {
      $sub = array();
      foreach ($value->getSubfields() as $code=>$subdata) {
        $sub[$code] = array(
          'value' => $subdata->getData(),
        );
      }
      if(!empty($tag)) {
        //$tag2 = str_replace($tag,'', 0, 0);
        if(substr($tag, 0, 1) == '0') {
          $tag = ltrim($tag, '0');

        }
        $df[$tag] = array(
          'ind1' => $value->getIndicator(1),
          'ind2' => $value->getIndicator(2),
          'sub' => $sub,

        );

      }

    }
  }
  ksort($df);
}


/**
 * Taxonomy terms (field_subject, field_genre) and add values to
 * current record
 *
 * @param $node
 * @param $record
 */
function _cals_importer_exporter_add_taxonomy_terms_marcxml($node, &$record) {
  //if vocab, $term = 'field_subject';
  $fieldnames =
    array(
      'field_subject' => array(650, 'a', 1, 4),
      'field_genre' => array(655, 'a', ' ', 7),
    );

  foreach($fieldnames as $fieldname => $marc) {
    $tag = $marc[0];
    $code = $marc[1];
    $ind1 = $marc[2];
    $ind2 = $marc[3];
    $terms = field_view_field('node', $node, $fieldname, array('default'));
    if(is_array($terms)) {
      foreach($terms["#items"] as $term){
        $subfields = array();
        $subfields[] = new File_MARC_Subfield($code, $term["taxonomy_term"]->name);
        $field = new File_MARC_Data_Field($tag, $subfields, 0, null);
        $field->setIndicator(1, $ind1);
        $field->setIndicator(2, $ind2);
        _cals_importer_exporter_insert_field($record, $field, $tag);
      }
    }
  }
}
