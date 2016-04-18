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
    'field_performer' => array('function' => '_cals_importer_get_performer_from_marcxml'),

    "field_subject" => array("function" => '_cals_get_subject_from_marc'),
    "field_genre" => array("function" => '_cals_get_genre_from_marc'),


    'field_access_rights' => array('function' => '_cals_importer_get_basic_values_from_marc',
      "tags" => array(
        array('tag' => 506, 'code' => 'a'),
        array('tag' => 506, 'code' => 'd'),
        array('tag' => 540, 'code' => 'a'),
        array('tag' => 540, 'code' => 'd'),
      )
    ),
    'field_publisher' => array('function' => '_cals_importer_get_basic_values_from_marc',
      "tags" => array(
        array('tag' => 264, 'code' => 'a'),
        array('tag' => 264, 'code' => 'b'),
        array('tag' => 260, 'code' => 'a'),
        array('tag' => 260, 'code' => 'b'),

      )
    ),
    'field_publisher_source' => array('function' => '_cals_importer_get_basic_values_from_marc',
      "tags" => array(
        array('tag' => 534, 'code' => 'c'),
      )
    ),
    'field_dc_contributor' => array('function' => '_cals_importer_get_basic_values_from_marc',
      "tags" => array(
        array('tag' => 700, 'code' => 'a'),
      )
    ),
    'field_dc_creator' => array('function' => '_cals_importer_get_basic_values_from_marc',
      "tags" => array(
        array('tag' => 100, 'code' => 'a'),
      )
    ),
    'field_title_alternative' => array('function' => '_cals_importer_get_basic_values_from_marc',
      "tags" => array(
        array('tag' => 130, 'code' => 'a'),
        array('tag' => 730, 'code' => 'a'),
      )
    ),
    'field_uri' => array('function' => '_cals_importer_get_basic_values_from_marc',
      "tags" => array(
        array('tag' => 856, 'code' => 'u'),
      )
    ),

    //relations

    'field_dc_relation' => array('function' => '_cals_importer_get_dc_relations_from_marc',
      "tags" => array(
        array('tag' => 250, 'code' => 'a', 'qualifier' => "type", 'enum' => "isVersionOf"),
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

    'field_oclc_number' => array('function' => '_cals_importer_get_system_numbers_from_marc'),
    'field_system_control_number' => array('function' => '_cals_importer_get_system_numbers_from_marc'),

    'field_cataloging_agency' => array('function' => "_cals_importer_get_cataloging_agency_from_marc"),



  );

  if(array_key_exists($fieldname, $arr)) return $arr[$fieldname];

  return '';
}

/**
 * Returns fields for repository item
 *
 * @return
 *   an array of flattened field defs
 *
 * @todo
 *   wrap into a static variable?
 *
 */
function _cals_get_fields_repository_item($fieldnames) {
  $fields = field_info_instances("node", "repository_item");

  //set title as first array element
  /*
  $map = array(
    "title" =>
      array(
        'type' => 'text',
        'required' => 1,
        'values' => '',
        'parser' => array("function" => '_cals_get_title_from_marc'),
      ),
  );
  */
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
        'values' => '',
        'parser' => _cals_importer_repo_field_callback($k),
      );
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
      $path = "//marc:datafield[@tag='245']/marc:subfield[@code='{$code}']";
      $tmp = $xml->xpath($path);
      foreach ($tmp as $me) $title[] = (string) trim($me[0]);
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

    $aud = substr($str, 22, 1);
    if (strlen($aud)) $map['field_audience']['values'][] = $aud;
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

  $path = "//marc:datafield[@tag='651']/marc:subfield[@code='a']";
  $vals = $xml->xpath($path);
  foreach ($vals as $val) {
    $str = (string) trim($val[0]);
//  	if(strlen($str)) $map[$k]['values'][] = array(
//      "field_value" => $str,
//      "field_dc_coverage_type" => "Spatial",
//      "field_dc_coverage_scheme" => "lcsh",
//      "field_marc_tag" => 651,
//    );
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
          LANGUAGE_NONE => array(array('value' => "lcsh")),
        ),
        'field_value' => array(
          LANGUAGE_NONE => array(array('value' => $str)),
        ),
      );
    }

  }
}




//function _cals_get_date_issued($xml) {
function _cals_get_date_issued_from_marc($xml, $k, &$map) {
  //let's grab the date issued
  $dates = array();

  $tmp = $xml->xpath("//marc:controlfield[@tag='008']");
  if (count($tmp) >= 1) {
    $str = (string) $tmp[0];
    $str = substr($str, 7, 4); // 008/07-10 should be numeric
    if (is_numeric($str)) $dates['008'] = _cals_get_iso_year($str);
  }

  $two64 = $xml->xpath("//marc:datafield[@tag='264']/marc:subfield[@code='c']");
  if (count($two64) >= 1) {
    $str = (string) $two64[0];
    $dates['264'] = $str;
  }
  else $two64fallback = TRUE; //fallback on 260 value

  if ( isset($two64falback) ) {
    $two60 = $xml->xpath("//marc:datafield[@tag='260']/marc:subfield[@code='c']");
    if (count($two60) >= 1) {
      $str = (string) $two60[0];

      $dates['260'] = $str;
    }
  }

  $five_thirty3 = $xml->xpath("//marc:datafield[@tag='533']/marc:subfield[@code='d']");
  if (count($five_thirty3) >= 1) {
    if(!isset($dates['533'])) $dates['533'] = '';
    $str = (string) $five_thirty3[0];
    $dates['533'] .= $str;
  }

  $five_thirty4 = $xml->xpath("//marc:datafield[@tag='534']/marc:subfield[@code='c']");
  if (count($five_thirty4) >= 1) {
    if(!isset($dates['534'])) $dates['534'] = '';
    $str = (string) $five_thirty4[0];
    $dates['534'] .= $str;
  }

  $dates_dupes = array( //qualified dates but potentially dupes
    //array()
  );

//Set qualifiers for entries in $dates based on origin field and load the array for deduping

  if ( ! empty( $dates ) ) {
    foreach ( $dates as $code => $date_value ) {
      switch ($code) {
        case '008':

          $date_dupes[] = array(
            'qualifier' => 'Issued', 'date' => $date_value
          );
          break;

        case '533':
          //Might have copyright symbol, if not set to Created
          $check_copyright = _cals_parse_date_issue_copyright($date_value);
          $date_value = _cals_get_iso_year($date_value);

          if ( $check_copyright == TRUE ) $date_dupes[] = array(
            'qualifier' => 'Copyrighted', 'date' => $date_value
          );
          else $date_dupes[] = array(
            'qualifier' => 'Created', 'date' => $date_value
          );
          break;

        case '260':
        case '264':
        case '534':
          //Might have copyright symbol, if not set to Issued
          $check_copyright = _cals_parse_date_issue_copyright($date_value);
          $date_value = _cals_get_iso_year($date_value);

          if ( $check_copyright == TRUE ) $date_dupes[] = array(
            'qualifier' => 'Copyrighted', 'date' => $date_value
          );

          else $date_dupes[] = array(
            'qualifier' => 'Issued', 'date' => $date_value
          );

          break;

      }
    }
  }

  //Most recent years first
  $dates = _cals_array_sort($date_dupes, 'date', SORT_DESC);

  //Dedupe based on our Qualifier Precedence Policy
  $dates = _cals_parse_date_issue_dedupe_qualifiers($dates);

  foreach($dates as $qual => $date) {
    $map[$k]['values'][] = array(
      'field_name' =>  "field_date",
      'field_dc_date' => array(
        LANGUAGE_NONE => array(array('value' => _cals_get_truncated_value($date, 20)) ),
      ),
      'field_qualifier_date' => array(
        LANGUAGE_NONE => array(array('value' => $qual) ),
      ),
    );
  }


}

//Curiously particular deduper based on qualifier precedence as well as recency.

function _cals_parse_date_issue_dedupe_qualifiers($dx_sorted) {

  $save = array(); //keeping
  $years = array(); //duplicates

  $i = -1; //start just before first index

  foreach ( $dx_sorted as $k => $v ) { //Loop through array of arrays

    $i++;

    $years[] .= $v['date'];
    $qual = $v['qualifier'];
    $year = $v['date'];

    if ( $i > 0 && in_array( $year, $years ) ) { //After the first run, check if year is in dupe list

      switch ( $qual ) {
        default:
          if ( ! array_key_exists( $qual, $save ) && ! in_array( $year, $save ) ) { //if we havent already saved qualifier or the year (no match)
            $save[$qual] = $year;
          }

          break;

        case 'Copyrighted':
        case 'Created':
          $superceded = array_search($year, $save ); //check for the current year in keeping array

          if ( $superceded && $superceded != 'Copyrighted' && $superceded != 'Created' ) {
            if ($save[$superceded] <= $year) { //check if the superceded key has value newer or same as one we're on,
              unset($save[$superceded]);  //unset if so, we have a newer key-value
            }
          }
          if ( ! array_key_exists( $qual, $save ) ) $save[$qual] = $year; //save the Copyrighted or Created value to supercede it

          break; //otherwise keep the newer value
      } //end switch
    }

    elseif ( ! array_key_exists( $qual, $save ) ) {
      $save[$qual] = $year;
    }

  }//foreach

  return $save;

}

function _cals_parse_date_issue_copyright ($raw_date_value) {
  $copyright = "/^c\\d{4}/";

  preg_match($copyright, $raw_date_value, $matches);

  if (count($matches)) {
    //$date_value_c =  explode('c', $matches[0] ); //remove the 'c'
    //dpm($date_value_c, "Copyright returning: ");
    return true;
  }

  else return;
}

/**
 * @param $arr
 *   the drupal ready array of fields
 * @param $xml
 *   the original xml string
 *
 * @return
 *   an array of Drupal-ready fields
 *
 * parse 041 to grab language values
 * see: http://www.loc.gov/marc/bibliographic/bd041.html
 */
//function _cals_importer_get_isolanguage_from_marcxml(&$arr, $xml) {

function _cals_importer_get_isolanguage_from_marcxml($xml, $k, &$map) {
  //look for 041 tag code a
  $tmp = $xml->xpath("//marc:datafield[@tag='041']/marc:subfield[@code='a']");
  foreach ($tmp as $me) {
    $val = (string) trim($me[0]);
    $val = trim($val);
    /* $arr['repos']['field_iso_language'][] = $val; */
    $map[$k]['values'][] = $val;
  }
  if( !isset($map[$k]['values']) ) $map[$k]['values'][]  = 'eng';


}


function _cals_get_date_from_008($str) {
  return _cals_get_iso_date(substr($str, 7, 4));
}

/*
 * Parse taxonomy terms: genre (vid = 2) and subject (vid = 3)
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

//function _cals_get_subject_from_marc(&$arr, &$node, $xml, &$map) {
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



//function _cals_get_descriptors_from_marc(&$arr, $xml) {
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
//check w/ Jonathan as to the $flag parameter
function _cals_importer_get_system_numbers_from_marc($xml, $k, &$map) {
  $str = array();
  $path = "//marc:datafield[@tag='035']/marc:subfield[@code='a']";
  $tmp = $xml->xpath($path);
  if(!count($tmp)) return;
  $str[] = (string) trim($tmp[0]);

  //if ( empty($str) ) { //fallback to 037 when 035 isn't present
  $path = "//marc:datafield[@tag='037']/marc:subfield[@code='a']";
  $tmp = $xml->xpath($path);
  if(count($tmp)) $str[] = (string) trim($tmp[0]);
  //}

  if ( !empty($str) ) { //proceed if we got something
    //oclc check
    foreach ($str as $key => $value) {
      if (strpos($value, "(OCoLC)") !== false ) {
        //new check on valid oclc numbers
        if ( _cals_isvalid_oclc_number($value) === 1 && !isset($flag) ) {
          $map['field_oclc_number']['values'][] = $value;
          $flag = true; //single-value field
        }
      }
      else if ($k == 'field_system_control_number') {
        $map['field_system_control_number']['values'][] = $value;
      }
    }
  }

}

function _cals_importer_get_access_restrictions($xml, $k, &$map) {
  $codes = array("a", "d");
  $title = array();
  $vals = array();
  foreach($codes as $code) {
    $path = "//marc:datafield[@tag='506']/marc:subfield[@code='{$code}']";
    $tmp = $xml->xpath($path);
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
//"field_isbn" => array("key" => "repos", "datafield" => 534, "subfield" => "z"),
  $tmp = $xml->xpath("//marc:datafield[@tag='534']/marc:subfield[@code='z']");
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


function _cals_importer_get_performer_from_marcxml(&$arr, $xml) {
  //"field_isbn" => array("key" => "repos", "datafield" => 534, "subfield" => "z"),
  $path = "//marc:datafield[@tag='511']/marc:subfield[@code='a']";
  $tmp = $xml->xpath($path);
  foreach ($tmp as $me) {

    $val = (string) trim($me[0]);
    //replace ";" to ","
    $val = str_replace("Narrated by ", "", $val);
    $val = str_replace(".", "", $val);

    $val = _cals_importer_get_normalized_performer($val);
    //valid??
    $tmp2 = explode(",", $val);
    foreach ($tmp2 as $val2) {
      $arr['fc_files']['field_performer']['values'][] = trim($val2);
      //$arr['field_performer']['values'][] = trim($val2);
    }
  }

}


/**
 *
 * @param $arr
 *   the drupal ready array of fields
 * @param $xml
 *   the original xml string
 *
 * @return
 *   an array of Drupal-ready fields
 *
 */
//function _cals_importer_get_performer_from_marcxml(&$arr, $xml) {

function _cals_importer_get_performer_from_marcxmlx($xml, $k, &$map) {
  //look for 511 tags
  printAndDie($xml);
  $path = "//marc:datafield[@tag='511']/marc:subfield[@code='a']";
  if($xml->xpath($path) === NULL) return;
  $tmp = $xml->xpath($path);

  foreach ($tmp as $me) {

    $val = (string) trim($me[0]);
    //replace ";" to ","
    $val = str_replace("Narrated by ", "", $val);
    $val = str_replace(".", "", $val);

    $val = _cals_importer_get_normalized_performer($val);

    //valid??
    $tmp2 = explode(",", $val);

    foreach ($tmp2 as $val2) {
      $arr['fc_files']['field_performer']['values'][] = trim($val2);
      $map[$k]['values'][] = trim($val2);
    }
  }

}


function _cals_importer_get_dc_relations_from_marc($xml, $k, &$map, $tags) {
  foreach($tags as $tag) {
    $t = $tag['tag'];
    $c = $tag['code'];
    $q = $tag['qualifier'];
    $e = $tag['enum'];

    /*
    array('tag' => 250, 'code' => 'a', 'qualifier' => "type", 'enum' => "isVersionOf"),
		array('tag' => 800, 'code' => 'a', 'qualifier' => "type", 'enum' => "IsPartOf"),
		array('tag' => 830, 'code' => 'a', 'qualifier' => "type", 'enum' => "IsPartOf"),
		array('tag' => 760, 'code' => 'a', 'qualifier' => "type", 'enum' => "IsPartOf"),
		array('tag' => 530, 'code' => 'a', 'qualifier' => "type", 'enum' => "IsFormatOf"),
		*/

    $path = "//marc:datafield[@tag='{$t}']/marc:subfield[@code='{$c}']";
    $tmp = $xml->xpath($path);
    foreach ($tmp as $me) {
      $val = (string) trim($me[0]);
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

function _cals_importer_get_cataloging_agency_from_marc($xml, $k, &$map, $tags) {
  $tags = array("003", "040");
  foreach($tags as $tag) {
    $tmp = $xml->xpath("//marc:datafield[@tag='{$tag}']]");
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

function _cals_importer_get_basic_values_from_marc($xml, $k, &$map, $tags) {
  foreach($tags as $tag) {
    $t = $tag['tag'];
    $c = $tag['code'];
    $path = "//marc:datafield[@tag='{$t}']/marc:subfield[@code='{$c}']";
    $tmp = $xml->xpath($path);
    foreach ($tmp as $me) {
      $val = (string) trim($me[0]);
      $map[$k]['values'][] = (isset($tag['qualifier'])) ? array( $tag['qualifier'] => $tag['enum'], "value" => $val ) : $val;
    }
  }
  //printPre(__FUNCTION__, $k, $map[$k]['values'], "path = " . $path);
}

/**
 *
 * @param $string
 *   the peformer value to normalize
 *
 * @return
 *   the normalized value
 *
 */
function _cals_importer_get_normalized_performer($string) {

  $prefixes = variable_get('cals_importer_prefix_stopwords_narrators');
  $arr = explode("\r\n", $prefixes );
  return trim(str_replace($arr, "", $string));
}

/**
 *
 * @param $arr
 *   the drupal ready array of fields
 * @param $xml
 *   the original xml string
 *
 * @return
 *   an array of Drupal-ready fields
 *
 * see: http://www.loc.gov/marc/bibliographic/bd306.html
 */
function _cals_importer_get_runningtime_from_marcxml(&$arr, $xml) {

  //files field collection
  if(!isset($arr['fc_files'])) $arr['fc_files'] = array();
  //look for 306 tag code a
  $tmp = $xml->xpath("//marc:datafield[@tag='306']/marc:subfield[@code='a']");

  foreach ($tmp as $me) {
    $val = (string) trim($me[0]);
    $val = trim($val);
    //field format: $fc_item->field_running_time[LANGUAGE_NONE][$k]['value']
    if(is_integer($val)) $arr['fc_files']['field_running_time'][]['value'] = $val;
  }

}

function _cals_get_commom_fields_from_marc(&$arr, $xml) {


  $fields = array(

    /*
"field_dc_creator" => array("key" => "repos", "datafield" => 100, "subfield" => "a"),
    "field_dc_contributor" => array("key" => "repos", "datafield" => 700, "subfield" => "a"),
      //uniform titles: 130 0# $a, 730  0# $a
    "field_title_alternative" => array("key" => "repos", "datafield" => 130, "subfield" => "a"),
    "field_title_alternative" => array("key" => "repos", "datafield" => 730, "subfield" => "a"),
*/

    /**
     * alt titles:
     *   240 ind 10 $a
     *   246 ind 13 $a
     *   210?
     *   242?
     */
    /*
"field_title_alternative" => array("key" => "repos", "datafield" => 240, "subfield" => "a"),
    "field_title_alternative" => array("key" => "repos", "datafield" => 246, "subfield" => "a"),
*/

    /*
"field_uri" => array("key" => "repos", "datafield" => 856, "subfield" => "u"),

    //identifiers

    "publisher_number" => array("key" => "identifier", "datafield" => "028", "subfield" => "a"),
    "producer" => array("key" => "identifier", "datafield" => "028", "subfield" => "a"),
*/

    /**
     * RELATIONS:
     *   800 $a  = IsPartOf
     *   830 $a  = IsPartOf
     *   530 $a  = IsFormatOf
     *   800 $a  = IsPartOf
     *   800 $a  = IsPartOf
     *
     */

    /*
        "field_dc_relation" => array("key" => "fc", "datafield" => 800, "subfield" => "a",
          'qualifier' => "type", 'enum' => "IsPartOf"),
        "field_dc_relation" => array("key" => "fc", "datafield" => 830, "subfield" => "a",
          'qualifier' => "type", 'enum' => "IsPartOf"),

        "field_dc_relation" => array("key" => "fc", "datafield" => 760, "subfield" => "a",
          'qualifier' => "type", 'enum' => "IsPartOf"),

        "field_dc_relation" => array("key" => "fc", "datafield" => 530, "subfield" => "a",
          'qualifier' => "type", 'enum' => "IsPartOf"),
    */

    /*
    NO SUBFIELD???
    case 760:
      $arr['fc']['field_dc_relation'][] = array("value" => $str, "type" => 'IsPartOf');
      break;

    */

  );


  foreach ($fields as $fieldname => $map) {

    /*

      $tmp = $xml->xpath("//marc:datafield[@tag='264']/marc:subfield[@code='b']");
      if (count($tmp)) {
        $str = (string) $tmp[0];
        return _cals_get_iso_date($str);
      }

    */
    $key = $map['key'];
    $df = $map['datafield'];
    $sf = $map['subfield'];
    $str = "//marc:datafield[@tag='" . $df . "']/marc:subfield[@code='" . $sf  . "']";
    $tmp = $xml->xpath($str);
    //$tmp = $xml->xpath("//marc:datafield[@tag='260']/marc:subfield[@code='c']");
    //printPre($fieldname, $map, $tmp, $str);

    if (count($tmp)>=1) {
      foreach ($tmp as $me) {
        $val = (string) trim($me[0]);
        if (isset($map['qualifier']) ) {
          $arr[$key][$fieldname][] = array( $map['qualifier'] => $map['enum'], "value" => $val );
        }
        else {
          $arr[$key][$fieldname][] = $val;
        }
      }

    }


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
function _cals_importer_get_controlfiel_values_from_marc($record, &$arr) {
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