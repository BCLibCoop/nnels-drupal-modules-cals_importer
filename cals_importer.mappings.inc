<?php
/**
 * @file
 * parses various marc xml elements
 */

/**
 * maps fieldnames to parser functions below
 *
 * @return
 *   an array of fieldname to functions and params
 *
 */

function _cals_importer_repo_field_callback($fieldname) {

  $arr = array(

    //"title" => array('function' => '_cals_get_title_from_marc'),

    "title_field" => array('function' => '_cals_get_title_from_marc'),
    "field_title_alternate" => array('function' => '_cals_get_alternate_title_from_marc'),
    "field_uniform_title" => array('function' => '_cals_get_uniform_title_from_marc'),

    'body'  => array('function' => '_cals_importer_get_basic_values_from_marc',
      "tags" => array(
        array('tag' => 520, 'code' => 'a'),
      )
    ),
    "field_iso_language" => array('function' => '_cals_importer_get_isolanguage_from_marcxml'),
    "field_date" => array('function' => '_cals_get_date_issued_from_marc'),

    "field_dc_coverage" => array('function' => '_cals_importer_get_coverage_marc'),
    "field_dc_description" => array('function' => '_cals_get_descriptors_from_marc'),

    "field_isbn" => array('function' => '_cals_importer_get_isbn_sysnums_from_marc'),
    "field_isbn_audio" => array('function' => '_cals_importer_get_isbn_sysnums_from_marc'),

    'field_performer' => array('function' => '_cals_importer_get_performer_from_marcxml'),

    "field_subject" => array("function" => '_cals_get_subject_from_marc'),
    "field_genre" => array("function" => '_cals_get_genre_from_marc'),

    //'field_uri' => array('function' => '_cals_importer_get_uri_from_marc'),
    'field_urls_external' => array('function' => '_cals_importer_get_urls_from_marc',
      "patterns" => variable_get('cals_importer_url_patterns'),
      "exceptions" => variable_get('cals_importer_url_patterns_ignore'),
    ),
    'field_literary_format' => array('function' => '_cals_get_literary_form_from_marc'),
    'field_access_rights' => array('function' => '_cals_importer_get_basic_values_from_marc',
      "tags" => array(
        array('tag' => 506, 'code' => 'a'),
        array('tag' => 506, 'code' => 'd'),
        array('tag' => 540, 'code' => 'a'),
        array('tag' => 540, 'code' => 'd'),
      )
    ),
    'field_publisher_source' => array('function' => '_cals_importer_get_basic_values_from_marc',
      "tags" => array(
        array('tag' => 534, 'code' => 'c'),
        array('tag' => 264, 'code' => 'a'),
        array('tag' => 264, 'code' => 'b'),
        array('tag' => 260, 'code' => 'a'),
        array('tag' => 260, 'code' => 'b'),
      )
    ),
    'field_dc_creator' => array('function' => '_cals_importer_get_creator_from_marc',
      "tags" => array(
        array('tag' => 100, 'code' => 'a'),
        array('tag' => 110, 'code' => 'a'),
        array('tag' => 110, 'code' => 'b'),
        array('tag' => 111, 'code' => 'a'),
      )
    ),

    'field_dc_contributor' => array('function' => '_cals_importer_get_contributor_from_marc',
      "tags" => array(
        //array('tag' => 700, 'code' => 'a'),
        //array('tag' => 700, 'code' => 'e'),
        //array('tag' => 700, 'code' => '4'),
        array('tag' => 710, 'code' => 'a'),
        array('tag' => 710, 'code' => 'b'),
        array('tag' => 711, 'code' => 'a'),

      )
    ),
    'field_title_alternative' => array('function' => '_cals_importer_get_basic_values_from_marc',
      "tags" => array(
        array('tag' => 130, 'code' => 'a'),
        array('tag' => 730, 'code' => 'a'),
      )
    ),
    //relations

    'field_dc_relation' => array('function' => '_cals_importer_get_dc_relations_from_marc',
      "tags" => array(
        array('tag' => 250, 'code' => 'a', 'qualifier' => "type", 'enum' => "isVersionOf"),
        array('tag' => 534, 'code' => 'b', 'qualifier' => "type", 'enum' => "isVersionOf"),
        array('tag' => 800, 'code' => 'a', 'qualifier' => "type", 'enum' => "IsPartOf"),
        array('tag' => 830, 'code' => 'a', 'qualifier' => "type", 'enum' => "IsPartOf"),
        array('tag' => 760, 'code' => 'a', 'qualifier' => "type", 'enum' => "IsPartOf"),
        array('tag' => 534, 'code' => 'e', 'qualifier' => "type", 'enum' => "IsFormatOf"),
      )
    ),

    'field_access_rights' => array('function' => '_cals_importer_get_basic_values_from_marc',
      "tags" => array(
        array('tag' => 540, 'code' => 'a',),
        array('tag' => 540, 'code' => 'd',),
        array('tag' => 506, 'code' => 'a',),
        array('tag' => 506, 'code' => 'd',),
      )

    ),

    'field_rights_holder' => array('function' => '_cals_importer_get_basic_values_from_marc',
      "tags" => array(
        array('tag' => 542, 'code' => 'd'),
      )
    ),

    'field_access_restrictions' => array('function' => '_cals_importer_get_access_restrictions'),

    'field_oclc_number' => array('function' => '_cals_importer_get_oclc_number_from_marc'),
    'field_system_control_number' => array('function' => '_cals_importer_get_system_numbers_from_marc'),

    'field_audience' => array('function' => '_cals_importer_get_audience_from_marc'),
    //'field_cataloging_agency' => array('function' => "_cals_importer_get_cataloging_agency_from_marc"),

    'field_contents' => array('function' => '_cals_importer_get_contents_from_marc'),


  );
  if($fieldname == 'all') return $arr;
  if(array_key_exists($fieldname, $arr)) return $arr[$fieldname];
  return '';
}

/**
 * Returns fields for repository item
 *
 * @return array
 *   an array of flattened field defs
 *
 * @todo
 *   wrap into a static variable?
 *
 */
function _cals_get_fields_repository_item() {
  $fields = field_info_instances("node", "repository_item");
  $fieldnames = array_keys($fields);

  $tmp = array(
    "_autocomplete", "_with_summary", "_hidden", "_buttons", "_select", "_textfield", "_embed", "_textarea",
  );
  //create an array of taxonomies to be used in parsing field collections, etc.
  foreach (taxonomy_get_vocabularies() as $k => $v) {
    $vocabularies[$v->machine_name] = $k;
  }
  if (count($fieldnames)) {
    foreach($fields as $k => $v) {
      if (!in_array($k, $fieldnames)) {
        unset($fields[$k]);
      }
    }
  }

  foreach($fields as $k => $v) {
    $parser = _cals_importer_repo_field_callback($k);
    if(!empty($parser)) {
      $type = $v['widget']['type'];
      $label = $v['label'];
      $t2 = $type;
      foreach($tmp as $strip) $t2 = str_replace($strip, '', $t2);
      $req = $v['required'];

      $map[$k] = array(
        'label' => $label,
        'type' => $t2,
        'required' => $req,
        'values' => array(),
        'parser' => _cals_importer_repo_field_callback($k),
      );
      if($t2 == 'options')
        $map[$k]['allowed_values'] =
          list_allowed_values(field_info_field($k));
      if($t2 == 'taxonomy') {
        $info = field_info_field($k);
        $vocab_keys = array();
        foreach ($info['settings']['allowed_values'] as $item) {
          if(!empty($item['vocabulary'])) $map[$k]['vid'] = $vocabularies[$item['vocabulary']];
        }
      }

      //set a default if defined
      $map[$k]['default'] = (isset($req) && isset($v['default_value'][0]['value'])) ? $v['default_value'][0]['value'] : '';
      $types[$t2][] = $k;
    }
  }
  $arr = array("types" => $types, "map" => $map);
  return $arr;
}

/**
 * helper function to parse xml path
 * @param $record_xml
 *   the original xml string
 * @param $tag
 *   the tag to parse
 * @param $code
 *   the optional code to parse*
 *
 * @return
 *   an array of values
 *
 */
function _cals_importer_xpath_value($record_xml, $fieldname, $tag, $code = NULL) {
  $arr = array();
  $path = ".//marc:{$fieldname}[@tag='" . $tag . "']";
  if(!empty($code) ) $path .= "/marc:subfield[@code='" . $code . "']";
  $tmp = $record_xml->xpath($path);
  //dpm(__FUNCTION__, $path, $tmp);
  if (count($tmp) >= 1) {
    foreach ($tmp as $me){
      $str = (string) strip_tags( trim($me[0]) );
      if(!empty($str)) $arr[] = $str;
    }
  }
  return $arr;
}
function _get_tmp($xml) {


}
function _cals_importer_xpath_control_field_value($xml, $tag, $code = NULL) {
  $arr = array();
  $path = "//marc:controlfield[@tag='" . $tag . "']";
  if(!empty($code) ) $path .= "/marc:subfield[@code='" . $code . "']";
  $tmp = $xml->xpath($path);
  if (count($tmp) >= 1) {
    foreach ($tmp as $me)
      $arr[] = (string) trim($me[0]);
  }
  //if(!count($tmp)) return;
  //printPre(__FUNCTION__, $arr, $path);
  return $arr;

}


/**
 * returns the title
 *
 *
 * @return
 *   an array of MARC XML -> Drupal mappings
 *
 */
function _cals_get_title_from_marc($xml, $k, &$map) {

  $title = array();
  $path = "//marc:datafield[@tag='245']/marc:subfield";
  $tag = $xml->xpath($path);
  foreach($tag as $subfield ) {
    $code = (string) $subfield->attributes()->code;
    if($code != 'c') {
      if($code == 'b' && substr(rtrim($tmp[0]), -1) != ":") {
        $title[0] .= " :";
      }
      $tmp = _cals_importer_xpath_value($xml, "datafield", 245, $code);
      foreach ($tmp as $me) $title[] = $me;
    }

  }
  $title = implode(" ", $title);
  $title = _cals_get_truncated_value($title, 250);
  if(strlen($title)) $map[$k]['values'][] = $title;
}

/*
 * see:https://github.com/BCLibCoop/nnels/issues/99
 *
 * "Similarly, Alternate Title should pull from 246 if the first indicator is 0, 1 or 3, subfields $abnp
 *
 */

function _cals_get_alternate_title_from_marc($xml, $k, &$map) {
  $title = array();
  $codes = array("a", "b", "n", "p");
  $parent = $xml->xpath("//marc:datafield[@tag='246']"); //need to grab abbreviated path to find indicator
  $inds = array(0, 1, 3);
  if(count($parent)) {
    foreach($parent as $i => $child) {
      $ind = (string) $parent[$i]->attributes()->ind1;
      if(in_array($ind, $inds)) {
        $title = array(); //holds the title elements for this child
        foreach ( $child->children('marc', true)->subfield as $subfield ) {
          $code = (string) $subfield->attributes()->code;
          if(in_array($code, $codes)) {
            $title[] = (string) trim($subfield[0]);
          }
        }
        if(count($title)) {
          $title = implode(" ", $title);
          $title = _cals_get_truncated_value($title, 250);
          $map[$k]['values'][] = $title;
        }
      }

    }
  }


}
/*
 * @see https://github.com/BCLibCoop/nnels/issues/99
 *
 * "Uniform title should pull from 130 $anp or if 130 does not exist, 240 where
 * first indicator is 1, subfields $anp"
 *
 */
function _cals_get_uniform_title_from_marc($xml, $k, &$map) {
  $title = array();
  $path = "//marc:datafield[@tag='130']/marc:subfield"; //reset path
  $tag = $xml->xpath($path);
  $codes = array("a", "n", "p");
  foreach($tag as $subfield ) {
    $code = (string) $subfield->attributes()->code;
    if(in_array($code, $codes)) {
      $path = "//marc:datafield[@tag='130']/marc:subfield[@code='{$code}']";
      $tmp = $xml->xpath($path);
      foreach ($tmp as $me) $title[] = (string) trim($me[0]);
    }
  }
  if(!count($title)) {
    $parent = $xml->xpath("//marc:datafield[@tag='240']"); //need to grab parent path to find indicator
    if(!count($parent)) return;
    $ind = (string) $parent[0]->attributes()->ind1;

    $path = "//marc:datafield[@tag='240']/marc:subfield"; //reset path
    $tag = $xml->xpath($path);
    if($ind == 1) {
      foreach($tag as $subfield ) {
        $code = (string) $subfield->attributes()->code;
        if(in_array($code, $codes)) {
          $path = "//marc:datafield[@tag='240']/marc:subfield[@code='{$code}']";
          $tmp = $xml->xpath($path);
          foreach ($tmp as $me) $title[] = (string) trim($me[0]);
        }

      }
    }
  }

  $title = implode(" ", $title);
  $title =  _cals_get_truncated_value($title, 250) ;
  $vid = $map[$k]['vid'];
  $map[$k]['values'][] = _cals_get_tid_by_name($vid, $title);
  //_cals_get_taxonomy_terms_from_xpath($vid, $title, $k, $map);
}

/* Parse field_audience via 006/5 or 008/2
 *
 * @see https://github.com/BCLibCoop/nnels/issues/117
 */

function _cals_importer_get_audience_from_marc($xml, $k, &$map) {
  $allowed_values = $map[$k]['allowed_values'];
  $audience = '';

  $arr = array("006" => 5, "008" => 22);
  foreach($arr as $tag => $pos) {
    //$tmp = _cals_importer_xpath_control_field_value($xml, $tag, $code = NULL);
    $tmp = _cals_importer_xpath_value($xml, "controlfield", $tag, $code = NULL);
    foreach($tmp as $val) {
      $str = substr($val, $pos, 1);
      if(array_key_exists($str, $allowed_values)) {
        $audience = $str;
        break;
      }
    }
    if($audience != '') {
      $map[$k]['values'][] = $audience;
      break;
    };
  }

}

function _cals_importer_get_coverage_marc($xml, $k, &$map){
  $path = "//marc:controlfield[@tag='008']";
  $vals = $xml->xpath($path);
  foreach ($vals as $val) {
    $str = substr($val, 15, 3);
    //field collection
    $map[$k]['values'][] = array(
      //'field_name' => 'field_dc_description',
      'field_name' => 'field_dc_coverage',
      'field_dc_coverage_type' => array(
        LANGUAGE_NONE => array(array('value' => 'Spatial')),
      ),
      'field_marc_tag' => array(
        LANGUAGE_NONE => array(array('value' => '008')),
      ),
      'field_dc_coverage_scheme' => array(
        LANGUAGE_NONE => array(array('value' => "http://www.loc.gov/marc/countries")),
      ),
      'field_value' => array(
        LANGUAGE_NONE => array(array('value' => $str)),
      ),
    );

  }

  $tags = array('043', '044');
  foreach($tags as $tag) {
    $path = "//marc:datafield[@tag='" . $tag . "']";
    $vals = $xml->xpath($path);
    foreach ($vals as $val) {
      $str = (string) trim($val[0]);
      if(strlen($str)) {
        $map[$k]['values'][]= array(
          'field_name' => 'field_dc_coverage',
          'field_dc_coverage_type' => array(
            LANGUAGE_NONE => array(array('value' => 'Spatial')),
          ),
          'field_marc_tag' => array(
            LANGUAGE_NONE => array(array('value' => $tag)),
          ),
          'field_dc_coverage_scheme' => array(
            LANGUAGE_NONE => array(array('value' => "ISO 3166")),
          ),
          'field_value' => array(
            LANGUAGE_NONE => array(array('value' => $str)),
          ),
        );
      }
    }
  }
  $tag = 651; //??
  $path = "//marc:datafield[@tag='651']/marc:subfield[@code='a']";
  $vals = $xml->xpath($path);
  $vals = _cals_importer_xpath_value($xml, "datafield", 651, "a");
  foreach ($vals as $str) {
    if(strlen($str)) {
      $map[$k]['values'][]= array(
        'field_name' => 'field_dc_coverage',
        'field_dc_coverage_type' => array(
          LANGUAGE_NONE => array(array('value' => 'Spatial')),
        ),
        'field_marc_tag' => array(
          LANGUAGE_NONE => array(array('value' => $tag)), //not sure what this is?
        ),
        'field_dc_coverage_scheme' => array(
          LANGUAGE_NONE => array(array('value' => "lcsh")),
        ),
        'field_value' => array(
          LANGUAGE_NONE => array(array('value' => $str)),
        ),
      );
    }
  }
}


function _cals_get_date_issued_from_marc($xml, $k, &$map) {

  $dates_dupes = array();
  $dates = array();

  //Background: https://github.com/BCLibCoop/nnels/issues/104#issuecomment-229739259
  //Get common subfields for this section

  $orig_pub_date_534c = _cals_importer_xpath_value($xml, "datafield", 534, "c");
  $date_pub_264c = _cals_importer_xpath_value($xml, "datafield", 264, "c");
  $date_pub_260c = _cals_importer_xpath_value($xml, "datafield", 260, "c");

  if ( count ( $orig_pub_date_534c ) ) {

    foreach ($orig_pub_date_534c as $orig_pub_date_part) {

      //There can be more comma-separated parts in
      //array returned from _cals_importer_xpath_value
      $orig_pub_date_part = explode(',', trim( $orig_pub_date_part, '.' ) );
      _cals_map_date_qualifiers
      ($orig_pub_date_part, 534, $dates, TRUE);
    }

    $reprod_tag = '533';
    $reprod_date_533d = _cals_importer_xpath_value($xml, "datafield",
      $reprod_tag, "d");

  } else {
    //No 533-4 so fallback is 008
    $reprod_tag = '008';
    $reprod_date_008c = _cals_importer_xpath_value($xml, "controlfield",
      $reprod_tag,
      NULL);
  }

  $reprod_date_shared = $reprod_date_533d ? $reprod_date_533d :
    $reprod_date_008c;

  if ( count( $date_pub_264c ) )
    foreach ( $date_pub_264c as $pub_date_part ) {
      $pub_date_parts = explode(',', trim( $pub_date_part, '.' ) );
      foreach ( $pub_date_parts as $part ) _cals_map_date_qualifiers($part, 264, $dates, TRUE);
    }
  else if (count($date_pub_260c))
    foreach ($date_pub_260c as $pub_date_part)
      _cals_map_date_qualifiers($pub_date_part, 260, $dates, TRUE);

  //Whatever was found for reproduction date
  else if (count($reprod_date_shared))
    foreach ($reprod_date_shared as $reprod_date_part)
      _cals_map_date_qualifiers($reprod_date_part, $reprod_tag, $dates, TRUE);

  foreach($dates as $qualifier => $date)
    $map[$k]['values'][] = array(
      'field_name' =>  "field_date",
      'field_dc_date' => array(
        LANGUAGE_NONE => array(array('value' => $date) ),
      ),
      'field_qualifier_date' => array(
        LANGUAGE_NONE => array(array('value' => $qualifier) ),
      ),
    );
}


/**
 * @param $str
 *   the date str to be parsed
 * @param $tag
 *   the marc xml tag
 * @param $dates
 *   the $dates array passed by reference
 * @param $is_present
 *   boolean that tells us if there's a 534 tag included
 */
function _cals_map_date_qualifiers($str, $tag, &$dates, $is_present) {
  if($tag == '008') {
    $str = substr($str, 7, 4); // 008/07-10 should be numeric
  }
  else {
    //Matches against c1999 or p1999 (UTF-8 for Ⓟ, sound recording copyright
    // symbol)
    $check_copyright = _cals_parse_date_issue_copyright($str);
  }
  $date_value = _cals_get_iso_year($str);
  if(!empty($date_value)) {
    if($check_copyright) {
      $qualifier = "Copyrighted";
    }
    else {
      if($is_present) //if there's 534 included, it takes "issued" qualifier, otherwise "created"
        $qualifier = ($tag == 534) ? "Issued" : "Created";
      else
        $qualifier = "Issued";
    }
    $tmp[$qualifier] = $date_value;
    //Replace the year w/ the most recent
    if($tmp[$qualifier] > $dates[$qualifier] || !isset($dates[$qualifier]))
      $dates[$qualifier] = $tmp[$qualifier];
  }

}

/**
 * @param $raw_date_value
 * @return bool
 */
function _cals_parse_date_issue_copyright ($raw_date_value) {
  //$copyright = "/^c\\d{4}/"; - original pattern
  $pattern = "/[cp]\d{4}/"; //new pattern, modified
  // 2016-06-30 - see:
  // https://github.com/BCLibCoop/nnels/issues/104#issuecomment-229709076
  preg_match($pattern, $raw_date_value, $matches);
  if (count($matches))
    return TRUE;

  return FALSE;
}

/**
 * @param $xml
 * @param $k
 * @param $map
 *
 * parse 041 to grab language values
 * see: http://www.loc.gov/marc/bibliographic/bd041.html
 */
function _cals_importer_get_isolanguage_from_marcxml($xml, $k, &$map) {
  //look for 041 tag code a
  //$tmp = $xml->xpath("//marc:datafield[@tag='041']/marc:subfield[@code='a']");
  $vals = _cals_importer_xpath_value($xml, "datafield", '041', 'a');
  foreach ($vals as $val)
    if(array_key_exists($val, $map[$k]['allowed_values'])) $map[$k]['values'][] = $val;
  if( empty($map[$k]['values']) ) {
    //$vals = _cals_importer_xpath_control_field_value($xml, "008", $code = NULL);
    $vals = _cals_importer_xpath_value($xml, "controlfield", "008", $code = NULL);
    foreach($vals as $val) {
      $str = substr($val, 35, 3);
      if(array_key_exists($str, $map[$k]['allowed_values'])) $map[$k]['values'][] = $str;
    }
  }
  if( empty($map[$k]['values']) ) $map[$k]['values'][]  = 'eng';
}

/**
 * Parses 008 field to grap year value
 * @param $str
 * @return bool|int|mixed|string
 *
 */
function _cals_get_date_from_008($str) {
  return _cals_get_iso_date(substr($str, 7, 4));
}

/**
 * Parse taxonomy terms: genre (vid = 2) and subject (vid = 3)
 * @param $xpathvalue
 * @param $fieldname
 * @param $map
 */
function _cals_get_taxonomy_terms_from_xpath($xpathvalue, $fieldname, &$map) {
  $vid = $map[$fieldname]['vid'];
  $terms = array();
  if (count($xpathvalue) >= 1) {
    foreach ($xpathvalue as $me) {
      $str = (string) $me[0];
      $str = rtrim($str, ".");
      if(!in_array($str, $terms) ) $terms[] = $str;
    }
  }

  asort($terms);
  foreach($terms as $term) $map[$fieldname]['values'][] = _cals_get_tid_by_name($vid, $term);

}
//function _cals_get_genre_from_marc(&$arr, $xml) {
function _cals_get_genre_from_marc($xml, $k, &$map) {
  $xpathvalue = $xml->xpath("//marc:datafield[@tag='655'][@ind1=' ']/marc:subfield[@code='a']|//marc:datafield[@tag='655'][@ind2=' ']/marc:subfield[@code='a']");
  _cals_get_taxonomy_terms_from_xpath($xpathvalue, $k, $map);
}

/**
 * To be deprecated
 * @param $xml
 * @param $k
 * @param $map
 */
function _cals_importer_get_uri_from_marc($xml, $k, &$map) {
  $vals = _cals_importer_xpath_value($xml, "datafield", '856', 'u');
  $arr = array("nnels.ca", "license", "loc.gov");
  foreach ($vals as $val) {
    $add = 1;
    foreach($arr as $ignore )
      if(strpos($val, $ignore) !== FALSE) $add = 0;
    if($add == 1) $map[$k]['values'][] = $val;
  }
}

/**
 * @param $xml
 * @param $k
 * @param $map
 */
function _cals_importer_get_urls_from_marc($xml, $k, &$map) {
  $vals = _cals_importer_xpath_value($xml, "datafield", '856', 'u');
  $arr = array("nnels.ca", "license", "loc.gov");
  $exceptions = trim($map[$k]['parser']['exceptions']);
  $exceptions = explode("\r\n", $exceptions);

  $patterns = explode("\r\n", trim($map[$k]['parser']['patterns']));
  foreach ($vals as $val) {
    $val = strtolower($val);
    $add = 1; //should we add this? default = yes
    $added = 0; // track if we've added this already.
    foreach($exceptions as $ignore )
      if(strpos($val, $ignore) !== FALSE) $add = 0;
    if($add == 1 && $added == 0) {
      foreach($patterns as $pattern) {
        $tmp = explode("|", $pattern);
        if(strpos($val, trim($tmp[1]) ) !== FALSE ) {
          $map[$k]['values'][] = array('url' => $val, "title" => trim($tmp[0]));
          $added = 1;
        }
      }
      if($added == 0) $map[$k]['values'][] = array('url' => $val);
    }
  }
}

/*
 * Based on 006/16 or if 006 not present, 008/33;
 * see: https://github.com/BCLibCoop/nnels/issues/113#event-656015813
 *
 * @todo
 *   decide if we need to loop thru $tmp, or just begin w/ $tmp[0]
 *
 */
function _cals_get_literary_form_from_marc($xml, $k, &$map) {
  //$ignore = array("u", "|", " ", '', 'n');
  $allowed_values = $map[$k]['allowed_values'];
  $litform = ''; //holds the value; blank if none
  //$tmp = _cals_importer_xpath_control_field_value($xml, "006", $code = NULL);
  //$tmp = _cals_importer_xpath_value($xml, "controlfield", "006", $code = NULL);
  $arr = array("006" => 16, "008" => 33);
  foreach($arr as $tag => $pos) {
    //$tmp = _cals_importer_xpath_control_field_value($xml, $tag, $code = NULL);
    $tmp = _cals_importer_xpath_value($xml, "controlfield", $tag, $code = NULL);
    if(is_array($tmp) && count($tmp)) {
      foreach($tmp as $val) {
        $str = substr($val, $pos, 1);
        if(!empty($str) && array_key_exists($str, $allowed_values)) {
          $litform = $str;
          break;
        }
      }
    }
    if($litform != '') {
      $map[$k]['values'][] = $litform;
      break;
    };
  }
  //if(empty($litform)) $map[$k]['values'][] = 'u';
}

function _cals_get_subject_from_marc($xml, $k, &$map){
  $paths = array();
  $paths[] =  "//marc:datafield[@tag='600']/marc:subfield[@code='a']";

  //the codes we need to look for on 650:
  $codes = array("a", "x", "v", "y", "z");
  foreach ($codes as $code) $paths[] = "//marc:datafield[@tag='650']/marc:subfield[@code='" . $code . "']";
  $path = implode("|", $paths);
  $xpathvalue = $xml->xpath($path);
  _cals_get_taxonomy_terms_from_xpath($xpathvalue, $k, $map);
}

/**
 * Grabs table of contents per https://github.com/BCLibCoop/nnels/issues/121
 *
 * @param $xml
 * @param $k
 * @param $map
 */
function _cals_importer_get_contents_from_marc($xml, $k, &$map) {
  $tag = 505;
  $codes = array("a", "g", "r", "t");
  $parent = $xml->xpath("//marc:datafield[@tag='505']");
  $content = '';
  foreach ($parent as $i => $child) {
    //$ind = (string) $parent[$i]->attributes()->ind1;
    $title = array(); //holds the title elements for this child
    foreach ($child->children(NAME_SPACE)->subfield as $subfield) {
      $code = (string) $subfield->attributes()->code;
      if(in_array($code, $codes)) {
        $str = (string) trim($subfield[0]);
        //add blank just in case the subfield doesn't include spaces:
        if(strlen($str)) $content .= " " . $str;
      }
    }
  }
  if(strlen($content)) {
    $content = str_replace("  ", " ", $content);
    $replacements = array(" -- ", "-- ", " --");
    $content = str_replace($replacements, "--", $content);
    $items = array_values(array_filter(explode("--", $content)));
    $vars = array("title" => '', "type" => "ul", "items" => $items, "attributes" => array());
    $map[$k]['values'][] = theme("item_list", $vars);
  }
}
/*
 * to deprecate??
 *
 * @param $xml
 * @param $k
 * @param $map
 */
function _cals_get_descriptors_from_marc($xml, $k, &$map) {
  $fields = array(500, 504, 505, 511, 516, 533, 538, 586, 595, 598, 599, 541, );
  foreach ($fields as $field) {
    $path = "//marc:datafield[@tag='" . $field . "']";
    $path = "//marc:datafield[@tag='" . $field . "']/marc:subfield";
    $tmp = $xml->xpath($path);
    if (count($tmp) >= 1) {
      foreach ($tmp as $me) {
        $val = (string) trim($me[0]);
        $val = _cals_get_truncated_value($val, 1000);
        $type = _cals_get_description_type($field);
        $map[$k]['values'][] = array(
          'field_name' => 'field_dc_description',
          'field_dc_description_type' => array(
            LANGUAGE_NONE => array(array('value' => $type)),
          ),
          'field_value' => array(
            LANGUAGE_NONE => array(array('value' => $val)),
          ),
        );
      }
    }
  }
}

/**
 * @param $xml
 * @param $k
 * @param $map
 * @param $flag
 */
//function _cals_importer_get_system_numbers_from_marc($xml, $k, &$map, $flag) {
//check w/ Jonathan as to the $flag parameter: can't find elsewhere in code
function _cals_importer_get_system_numbers_from_marc($xml, $k, &$map) {
  $str = array();

  $scn_targets = array(
    '028' => 'a',
    '035' => 'a',
    '037' => 'a',
  );

  foreach ($scn_targets as $tag => $subfield) {
    $tmp = _cals_importer_xpath_value($xml, "datafield", $tag, $subfield);
    foreach($tmp as $value) {
      $map[$k]['values'][] = $value;
    }
    unset($tmp);
  }
}


function _cals_importer_get_oclc_number_from_marc($xml, $k, &$map) {
  $arr = _cals_importer_xpath_value($xml, "datafield", '035', 'a');
  foreach($arr as $value) {
    //new check on valid oclc numbers
    if (strpos($value, "(OCoLC)") !== false  &&
      _cals_isvalid_oclc_number($value) === 1 &&
      count($map[$k]['values']) < 2 ) {
      $map[$k]['values'][] = $value;
    }
  }
}

function _cals_importer_get_access_restrictions($xml, $k, &$map) {
  $codes = array("a", "d");
  $title = array();
  $vals = array();
  foreach($codes as $code) {
    $tmp = _cals_importer_xpath_value($xml, "datafield", '506', $code);
    if (count($tmp) >= 1) {
      $vals[] = (string) trim($tmp[0]);
      if(count($vals)) {
        $str = implode(" ", $vals);
        $pos = strpos(strtolower($str),"restricted");
        $map[$k]['values'][] = (is_numeric($pos)) ? 1 : 0;
      }
    }
  }
}




function _cals_get_description_type($tag) {

  $tmp =  array(
    505 => "tableofcontents",
    520 => 'abstract',
  );

  /*
  other possible enumerations
  504 => "Bibliography",
  511 => 'Performer',
  516 => 'Type of file',
  533 => 'Reproduction',
  538 => 'System Details / Requirements',
  586 => 'Awards Notes',
  595 => 'Extent of Original',
  598 => 'Keywords, Local',
  599 => 'Local Notes',
  541 => 'Accrual Method',
  */

  if (array_key_exists($tag, $tmp)) return $tmp[$tag];
  return "";
}


//function _cals_importer_get_isbn_sysnums_from_marc(&$arr, $xml) {
function _cals_importer_get_isbn_sysnums_from_marc($xml, $k, &$map) {

  $path = ($k == 'field_isbn_audio') ? "//marc:datafield[@tag='020']/marc:subfield[@code='a']" :
    "//marc:datafield[@tag='534']/marc:subfield[@code='z']" ;
  $tmp = $xml->xpath($path);
  foreach ($tmp as $me) {
    $val = (string) trim($me[0]);
    //replace ";" to ","
    $val = str_replace(";", ",", $val);
    if (strpos($val, "[")) $val = trim(substr($val, 0, strpos($val, "[") ));
    if (strpos($val, "(")) $val = trim(substr($val, 0, strpos($val, "(") ));
    //in case its multi valued
    $tmp2 = explode(",", $val);
    foreach ($tmp2 as $val2) {
      $val2 =  preg_replace("/[^0-9Xx]+/", "", $val2);
      $val2 =  str_replace(" ", "", $val2);
      //$val2 =  str_ireplace("xx", "x", $val2); //in case there's 2 x's
      $val2 = ltrim($val2, "X|x"); //strip on off chance there's a prepended x

      if(stripos($val2, "x") == 12 && is_numeric(substr($val2, 0, 12)))
        $map[$k]['values'][] = substr($val2, 0, 13);
      if (stripos($val2, "x") == 10 && is_numeric(substr($val2, 0, 9)))
        $map[$k]['values'][] = substr($val2, 0, 10);
      if(!empty($val2) && is_numeric($val2) && (strlen($val2) == 13 || strlen($val2) == 10) )
        $map[$k]['values'][] = trim($val2);
    }
  }
}

/**
 * @param $arr
 *   the drupal ready array of fields
 * @param $xml
 *   the original xml string
 *
 *
 */
function _cals_importer_get_performer_from_marcxml(&$arr, $xml) {

  $raw_511_a = _cals_importer_xpath_value( $xml, "datafield", 511, "a" );
  $prepared_narrator_values = array();

  foreach ($raw_511_a as $value) {

    //Transform raw string and obtain normalized Performer value(s)
    $normalized_narrator_values = _cals_importer_get_normalized_performer(
      _cals_importer_strip_trailing_periods_value(
        str_replace("Narrated by ", "", $value
        )
      )
    );

    //Loop through our normalized values and breakdown any lists
    foreach ( $normalized_narrator_values as $key => $candidate ) {
      $exploded = explode(",", $candidate);
      $de_listed_values = array_values(
        ( $exploded ? $exploded : $candidate )
      );
      $prepared_narrator_values = array_merge($prepared_narrator_values,
        $de_listed_values);
    }

    //Trim values and add to file resource fields
    foreach ($prepared_narrator_values as $prepared_value)
      $arr['fc_files']['field_performer'][]['value'] = trim( $prepared_value );
  }
}
function xmlToArray($xml, $options = array()) {
  $defaults = array(
    'namespaceSeparator' => ':',//you may want this to be something other than a colon
    'attributePrefix' => '@',   //to distinguish between attributes and nodes with the same name
    'alwaysArray' => array(),   //array of xml tag names which should always become arrays
    'autoArray' => true,        //only create arrays for tags which appear more than once
    'textContent' => '$',       //key used for the text content of elements
    'autoText' => true,         //skip textContent key if node has no attributes or child nodes
    'keySearch' => false,       //optional search and replace on tag and attribute names
    'keyReplace' => false       //replace values for above search values (as passed to str_replace())
  );
  $options = array_merge($defaults, $options);
  $namespaces = $xml->getDocNamespaces();
  $namespaces[''] = null; //add base (empty) namespace

  //get attributes from all namespaces
  $attributesArray = array();
  foreach ($namespaces as $prefix => $namespace) {
    foreach ($xml->attributes($namespace) as $attributeName => $attribute) {
      //replace characters in attribute name
      if ($options['keySearch']) $attributeName =
        str_replace($options['keySearch'], $options['keyReplace'], $attributeName);
      $attributeKey = $options['attributePrefix']
        . ($prefix ? $prefix . $options['namespaceSeparator'] : '')
        . $attributeName;
      $attributesArray[$attributeKey] = (string)$attribute;
    }
  }

  //get child nodes from all namespaces
  $tagsArray = array();
  foreach ($namespaces as $prefix => $namespace) {
    foreach ($xml->children($namespace) as $childXml) {
      //recurse into child nodes
      $childArray = xmlToArray($childXml, $options);
      list($childTagName, $childProperties) = each($childArray);
      printPre($childTagName);

      //replace characters in tag name
      if ($options['keySearch']) $childTagName =
        str_replace($options['keySearch'], $options['keyReplace'], $childTagName);
      //add namespace prefix, if any
      if ($prefix) $childTagName = $prefix . $options['namespaceSeparator'] . $childTagName;

      if (!isset($tagsArray[$childTagName])) {
        //only entry with this key
        //test if tags of this type should always be arrays, no matter the element count
        $tagsArray[$childTagName] =
          in_array($childTagName, $options['alwaysArray']) || !$options['autoArray']
            ? array($childProperties) : $childProperties;
      } elseif (
        is_array($tagsArray[$childTagName]) && array_keys($tagsArray[$childTagName])
        === range(0, count($tagsArray[$childTagName]) - 1)
      ) {
        //key already exists and is integer indexed array
        $tagsArray[$childTagName][] = $childProperties;
      } else {
        //key exists so convert to integer indexed array with previous value in position 0
        $tagsArray[$childTagName] = array($tagsArray[$childTagName], $childProperties);
      }
    }
  }

  //get text content of node
  $textContentArray = array();
  $plainText = trim((string)$xml);
  if ($plainText !== '') $textContentArray[$options['textContent']] = $plainText;

  //stick it all together
  $propertiesArray = !$options['autoText'] || $attributesArray || $tagsArray || ($plainText === '')
    ? array_merge($attributesArray, $tagsArray, $textContentArray) : $plainText;

  //return node as array
  return array(
    $xml->getName() => $propertiesArray
  );
}

/**
 * Parses the creator values
 * @param $xml
 *   the original xml string
 * @param $k
 *   the fieldname
 * @param $map
 *   the array of fields
 * @param $tags
 *   the marcxml fields that are parsed
 *
 * @return
 *   $map is returned by reference with values appended
 */
function _cals_importer_get_creator_from_marc($xml, $k, &$map, $tags) {
  _cals_importer_get_basic_values_from_marc($xml, $k, $map, $tags);

  $tmp = '';
  if(count($map[$k]['values'])) {
    $tmp = $map[$k]['values']; //from 700 series, see: https://github.com/BCLibCoop/nnels/issues/116
    $map[$k]['values'] = array();
  }

  if(count($tmp)) $map[$k]['values'] = array_merge($map[$k]['values'],  $tmp);
}

/**
 * Parses contributor values and also creator values in the 700a
 * series where subfield $e = aut or subfield $4 = author
 * @param $xml
 *   the original xml string
 * @param $k
 *   the fieldname
 * @param $map
 *   the array of fields
 * @param $tags
 *   the marcxml fields that are parsed
 *
 * @return
 *   $map is returned by reference with values appended
 */
function _cals_importer_get_contributor_from_marc($xml, $k, &$map, $tags) {
  _cals_importer_get_basic_values_from_marc($xml, $k, $map, $tags);

  $tmp = array();
  $datafields = array( 700, 720 );
  foreach ( $datafields as $datafield ) {
    if(count(_cals_importer_xpath_value($xml, "datafield", $datafield, "a"))) {
      //find parent path
      $parent = $xml->xpath("//marc:datafield[@tag='$datafield']");
      foreach ($parent as $i => $child) {
        //$ind = (string) $parent[$i]->attributes()->ind1;
        $title = array(); //holds the title elements for this child
        foreach ($child->children(NAME_SPACE)->subfield as $subfield) {
          $code = (string) $subfield->attributes()->code;
          $tmp[$i][$code] = (string) trim($subfield[0]);
        }
      }
      foreach ($tmp as $v) {

        if (array_key_exists("e", $v) && stripos($v['e'], "author") !== FALSE ||
          array_key_exists("4", $v) && stripos($v['4'], "aut") !== FALSE
        ) {
          $map['field_dc_creator']['values'][] = $v['a'];
        }
        else {
          $map[$k]['values'][] = $v['a'];
        }
      }
    }
  }
}
/**
 * Parses DC Contributor values
 * @param $xml
 *   the original xml string
 * @param $k
 *   the fieldname
 * @param $map
 *   the array of fields
 * @param $tags
 *   the marcxml fields that are parsed
 *
 * @return
 *   $map is returned by reference with values appended
 */
function _cals_importer_get_dc_relations_from_marc($xml, $k, &$map, $tags) {
  foreach($tags as $tag) {
    $t = $tag['tag'];
    $c = $tag['code'];
    $q = $tag['qualifier'];
    $e = $tag['enum'];

    /*
    array('tag' => 250, 'code' => 'a', 'qualifier' => "type", 'enum' => "isVersionOf"),
    array('tag' => 534, 'code' => 'b', 'qualifier' => "type", 'enum' => "isVersionOf"),
		array('tag' => 800, 'code' => 'a', 'qualifier' => "type", 'enum' => "IsPartOf"),
		array('tag' => 830, 'code' => 'a', 'qualifier' => "type", 'enum' => "IsPartOf"),
		array('tag' => 760, 'code' => 'a', 'qualifier' => "type", 'enum' => "IsPartOf"),
		array('tag' => 530, 'code' => 'a', 'qualifier' => "type", 'enum' => "IsFormatOf"),
		*/

    $tmp = _cals_importer_xpath_value($xml, "datafield", $t, $c);
    foreach ($tmp as $val) {
      $val = _cals_importer_strip_square_brackets_value($val);
      $val = _cals_importer_strip_trailing_periods_value($val);
      $map[$k]['values'][] = array(
        'field_name' =>  "field_dc_relation",
        'field_dc_relation_value' => array(
          LANGUAGE_NONE => array(array('value' => $val) ),
        ),
        'field_dc_relation_qualifiers' => array(
          LANGUAGE_NONE => array(array('value' => $e) ),
        ),
      );
    }
  }
}
/**
 * Parses cataloguing agency from 003 and 040
 * @param $xml
 *   the original xml string
 * @param $k
 *   the fieldname
 * @param $map
 *   the array of fields
 * @param $tags
 *   the marcxml fields that are parsed
 *
 * @return
 *   $map is returned by reference with values appended
 */

function _cals_importer_get_cataloging_agency_from_marc($xml, $k, &$map, $tags) {
  $tags = array("003", "040");
  foreach($tags as $tag) {
    $tmp = _cals_importer_xpath_value($xml, "datafield", $tag, '');

    foreach ($tmp as $me) {
      $val = (string) trim($me[0]);
      if($tag == '040') {
        $agency = _cals_importer_get_array_cataloging_agency();
        if (array_key_exists($code, $agency)) {
          $map[$k]['values'][] = array(
            "field_agency_type" => $agency[$code],
            "value" => $val,
          );
        }
      }
      else {
        $map[$k]['values'] = array(
          "field_agency_type" => "",
          "value" => $val,
        );
      }
    }
  }
}

/**
 * Get an array of cataloging agencies
 *
 * @return
 *   an array of cataloging agencies
 */

function _cals_importer_get_array_cataloging_agency() {
  /*
  1|Original Cataloguing agency (NR)
  2|transcribing_agency
  3|modifying_agency

  */
  return array(
    "a" => 1, //"original_cataloguing_agency",
    //"b" => "cataloguing_language",
    "c" => 2, //"transcribing_agency",
    "d" => 3, // "modifying_agency",
  );


}

function _cals_importer_get_basic_values_from_marc($xml, $k, &$map, $tags) {
  foreach($tags as $tag) {
    $t = $tag['tag'];
    $c = $tag['code'];
    $tmp = _cals_importer_xpath_value($xml, "datafield", $t, $c);
    foreach ($tmp as $val) {
      $map[$k]['values'][] = (isset($tag['qualifier'])) ? array( $tag['qualifier'] => $tag['enum'], "value" => $val ) : $val;
    }
  }
}

/**
 *
 * @param $string
 *   the performer value to normalize
 *
 * @return
 *   the normalized value
 *
 */
function _cals_importer_get_normalized_performer($string) {

  $prefixes = variable_get('cals_importer_prefix_stopwords_narrators');
  $arr = explode("\r\n", $prefixes );

  $filtered_string = trim(str_replace($arr, "", $string));
  //Split potential narrator list on conjunctions
  $filtered_split_string = preg_split( "/ (and|&) /", $filtered_string);

  return $filtered_split_string;
}

/**
 *
 * @param $arr
 *   the drupal ready array of fields
 * @param $xml
 *   the original xml string
 *
 * see: http://www.loc.gov/marc/bibliographic/bd306.html
 */
function _cals_importer_get_runningtime_from_marcxml(&$arr, $xml) {

  if (!isset($arr['fc_files'])) {
    $arr['fc_files'] = array();
  }

  //look for 306 tag code a
  $runtime_306a = _cals_importer_xpath_value($xml, "datafield", 306, 'a');

  if (!empty($runtime_306a)) { //empty array from xpath_value if not found
    foreach ($runtime_306a as $val) {
      $val = trim($val);
      if (is_numeric(trim($val))) {
        $arr['fc_files']['field_running_time'][]['value'] = trim($val);
      }
    }
    return;
  }

  //Try 300 $a - more common?
  $runtime_300a = _cals_importer_xpath_value($xml, "datafield", 300, 'a');

  //If we have a runtime from 300$a and lacks "page as a substring"
  if ( ! empty( $runtime_300a ) && ( ! strpos( $runtime_300a[0], "page" ) ) ) {

    //Match possible running times with units...oy vey
    preg_match("/\((\d+.?[\d\s]?\/?\d*\s?[hr]*[min]*)\.*[\,\s]*([\w\s]*)\.*\)/",
      $runtime_300a[0], $matches);

    if ($matches[1]) {
      preg_match("/(\d+).*/", $matches[1], $whole_hour);
      $hours = trim($whole_hour[1]);
      $minutes = '00'; //may get replaced

      //Check for a decimal number or fraction, should not be mixed
      preg_match("/\d+\s(\d*\/?\d*)/", $matches[1], $fractional_min);
      preg_match("/\d\.(\d*)/", $matches[1], $decimal_min);

      if ((string) $fractional_min[1] == '1/2') $minutes = 30;
      if ((string) $fractional_min[1] == '1/4') $minutes = 15;

      if ($decimal_min[1]) {
        //count digits
        $count = strlen((string) $decimal_min[1]);
        $divisor = pow(10, $count);
        $minutes = (int) (60 * ($decimal_min[1] / $divisor) );
      }
      //otherwise whole minutes
      if ($matches[2]) {
        $min = explode(" ", $matches[2]);
        $minutes = $min[0];
      }
    }
    //Format in hhmmss for field
    $timestamp = sprintf('%02d%02d00', $hours, (int) $minutes);
    $arr['fc_files']['field_running_time'][]['value'] = $timestamp;
  }
  else {
    /* No useable runtime value found.
     * Leaves $arr['fc_files'] empty for next field callback
     */
  }
}


/**
 *
 * @param $record
 *   the tag derived from the xml file
 *
 * @return
 *   an array of Drupal-ready fields
 *
 */
function _cals_importer_get_controlfield_values_from_marc($record, &$arr) {
  //leader field
  $leader = (string) $record->children(NAME_SPACE)->leader;
  $arr["leader"] = $leader;
  $type = substr($leader, 6, 1);
  if ($type != '')   $arr['repos']['field_dc_type'][] = $type;

  /*
  http://www.oclc.org/bibformats/en/fixedfield/006.html
  */
  //loop thru controlfield
  foreach ($record->children(NAME_SPACE)->controlfield as $controlfield) {
    $tag = (string) $controlfield->attributes()->tag;
    $tags['tag'] = $tag;
    $tags['ind1'] = (string) $controlfield->attributes()->ind1;
    $tags['ind2'] = (string) $controlfield->attributes()->ind2;
    $tags['code'] = '';
    $tags['str'] = (string) $controlfield;
    if ($tag == '008') _cals_get_value_from_tag($tags, $arr);
  }
}
/**
 *
 * @param $record
 *   the tag derived from the xml file
 *
 * @return
 *   an array of Drupal-ready fields
 *
 */
function _cals_importer_get_array_from_marcxml($record, &$arr) {
  //leader field
  $leader = (string) $record->children(NAME_SPACE)->leader;
  $arr["leader"] = $leader;
  $type = substr($leader, 6, 1);
  if ($type != '')   $arr['repos']['field_dc_type'][] = $type;

  /*
  http://www.oclc.org/bibformats/en/fixedfield/006.html
  */
  //loop thru controlfield
  foreach ($record->children(NAME_SPACE)->controlfield as $controlfield) {
    $tag = (string) $controlfield->attributes()->tag;
    $tags['tag'] = $tag;
    $tags['ind1'] = (string) $controlfield->attributes()->ind1;
    $tags['ind2'] = (string) $controlfield->attributes()->ind2;
    $tags['code'] = '';
    $tags['str'] = (string) $controlfield;
    if ($tag == '008') _cals_get_value_from_tag($tags, $arr);

  }

  //loop thru datafields
  foreach ($record->children(NAME_SPACE)->datafield as $datafield) {
    $tag = (string) $datafield->attributes()->tag;
    $tags['tag'] = $tag;
    $tags['ind1'] = (string) $datafield->attributes()->ind1;
    $tags['ind2'] = (string) $datafield->attributes()->ind2;


    foreach ($datafield as $subfield) {
      $tags['code'] = (string) $subfield->attributes()->code;
      $tags['str'] = (string) $subfield;
      if ( strlen($tags['str']) ) _cals_get_value_from_tag($tags, $arr);
    }
  }



}

/**
 * DEPRECATED, retained as quick reference
 * Maps MARC tag to Drupal Field
 *
 * its a big switch block: could probably replace with a $map array and run
 * more efficiently, but some tags require additional logic
 *
 * @param $tag
 *   the tag derived from the xml file
 *
 * @param $arr
 *   the big arr
 *
 *
 */
function _cals_get_value_from_tag($tags, &$arr) {
  $code = '';
  $ind1 = '';
  $ind2 = '';
  $tag = '';
  $str = '';
  foreach ($tags as $k => $v) {
    ${$k} = $v;
  }
  switch ($tag) {

    /*
          //main title
          case 245:
            if ($code == 'a') $arr['repos']['title'] =  $str;
            if ($code == 'b') $arr['repos']['title'] .=  " " . $str;
            break;
    */
    /**
     * COVERAGE.SPATIAL:
     *   651 #0 $a: Geo Name indicator 0 = lcsh; 6 = Répertoire de vedettes-matièr
     *   043 Geographic Code iso 3166
     *   044 Country of publishing
     *   008/15-17 prov country of publishing : http://www.loc.gov/marc/countries/
     */
    /*

          case "008":
            //field collection
            $substr = substr($str, 15, 3);
            $arr['fc']['field_dc_coverage'][] = array(
              "field_value" => $substr,
              "field_dc_coverage_type" => "Spatial",
              "field_dc_coverage_scheme" => "http://www.loc.gov/marc/countries",
              "field_marc_tag" => $tag,
            );

            $aud = substr($str, 22, 1);
            if (strlen($aud)) $arr['repos']['field_audience'][] = $aud;
            break;
    */

    /*
          case "043":
          case "044":
            $arr['fc']['field_dc_coverage'][] = array(
              "field_value" => $str,
              "field_dc_coverage_type" => "Spatial",
              "field_dc_coverage_scheme" => "ISO 3166",
              "field_marc_tag" => $tag,
            );

            break;
    */

    /*
          case 264:
            if ($code == 'a' || $code == 'b' ) $arr['repos']['field_publisher'][] = $str;
            break;
    */
    /*

          case 651:
            if ($code == 'a') {
              $arr['fc']['field_dc_coverage'][] = array(
                "field_value" => $str,
                "field_dc_coverage_type" => "Spatial",
                "field_dc_coverage_scheme" => "lcsh",
                "field_marc_tag" => $tag,
              );
              //lets add this to the keyword index?
              $arr['repos']['field_keyword_index'][] = $str;

            }
            break;
    */


    /**
     * RIGHTS:
     *   506 $ad = access_rights
     *   540 $ad = access_rights
     *   542 $d  = rights_holder
     */
    /*
          case 540:
            if ($code == 'a' || $code == 'd' ) $arr['repos']['field_access_rights'][] = $str;
            break;

          case 506:
            if ($code == 'a' || $code == 'd' ) $arr['repos']['field_access_rights'][] = $str;
            //if 506 includes the word "restricted", set the file access to licensed (1); else set to open (0).
            //default is 0.
            $tmp = strpos(strtolower($str),"restricted");
            $arr['repos']['field_access_restrictions'][0] = (is_numeric($tmp)) ? 1 : 0;

            break;
          case 542:
            if ($code == 'd' ) $arr['repos']['field_rights_holder'][] = $str;
            break;
    */

    /**
     * IDENTIFIERS:
     *   028 $a  = publisher number
     *   028 $b (producer)
     *   035 ## $a system control number
     *   035 ## $a dcterms;Oclc 035 $a(OCoLC)153910628
     *   856 4# $u .URI - use for primary URL, alt URLS, DOI
     *   534 ## $z ISBN
     */

    /*
          case "035":

            if ($code == 'a') {
              $str = (string)$str;
              //oclc
              if (strpos($str, "(OCoLC)") !== false) {
                //new check on valid oclc numbers
                if (_cals_isvalid_oclc_number($str) === 1) $arr['repos']["field_oclc_number"][] = $str;
              }
              else {
                $arr['repos']["field_system_control_number"][] = $str;
              }
            }

            break;
    */

    //body field; description
    /*
          case 520:
            if ($code == 'a')  {
              //to do - rewrite this more efficiently!!
              if (isset($arr['repos']['body'][0])) {
                $arr['repos']['body'][0] .= $str;
              }
              else {
                $arr['repos']['body'][0] = $str;
              }
            }

            break;
    */


    /**
     * CATALOGUING AGENCY:
     *   040 $a  = Original Cataloguing agency (NR)
     *   040 $b = cataloguing_language
     *   040 $c = transcribing_agency
     *   040 $d = modifying_agency
     */
    /*
          case "003":
            $arr['fc']['field_cataloguing_agency'][] = array(
              "field_agency_type" => "",
              "value" => $str,
            );
            break;

          case "040":
            $tmp = _cals_importer_get_array_cataloging_agency();
            if (array_key_exists($code, $tmp)) {
              $arr['fc']['field_cataloguing_agency'][] = array(
                "field_agency_type" => $tmp[$code],
                "value" => $str
              );
            }
            break;
    */
  } //end switch

}// end function
