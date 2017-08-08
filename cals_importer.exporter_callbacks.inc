<?php

/**
 * Returns date and time of latest transaction
 * @param $vars
 * @return mixed
 */
function _cals_importer_exporter_callback_005($vars) {
  $node = $vars['node'];
  $tag = $vars['tag'];
  $wrapper = $vars['node_wrapper'];
  $value = $wrapper->changed->value();
  //yyyymmddhhmmss.f
  $me = date('YmdHis.0', $value);
 // printAndDie(__FUNCTION__, $me, $value, $wrapper);

  $arr['005'] = array(
    'ind1' => '',
    'ind2' => '',
    'subfields' => array(
      '' => array($me),
    ),
  );
  return $arr;
}

/**
 *
 * All 24 character positions (00-23) are mandatory.
 * 00-04 (record length) = entry is system generated (count the entire length of the bibliographic record)
 * 05 (record status) = default code n
 * 06 (type of record) = generate the code based on the file format in Drupal:
 *   MP3 = i
 *   DAISY, etext, PDF, epub, AZW3, Braille = a
 * 07 (bibliographic level) = default code m
 * 08 (type of control) = default code blank
 * 09 (character encoding scheme) = this value is either Unicode (code a) or MARC-8 (blank), I’m unsure which applies?
 * 10 (indicator count) = default code 2
 * 11 (subfield code count) = default code 2
 * 12-16 (base address of data) = always system generated...
 * 17 (encoding level) = default code 7
 * 18 (descriptive cataloguing form) = default code u
 * 19 (multipart resource record level) = default code blank
 * 20-23 (entry map) = default codes: 20=4, 21=5, 22=0, 23=0
 *
 * @param $node
 * @return mixed
 */
function _cals_importer_exporter_callback_ldr($wrapper) {
  $vals['00-04'] = 10000; //figure this out?
  $vals['05'] = 'n';
  $vals['06'] = 'i'; //Jonathan?
  $vals['07'] = 'm';
  $vals['08'] = ' ';
  $vals['09'] = ' ';
  $vals['10'] = 2;
  $vals['11'] = 2;
  $vals['12-16'] = 'XXXXX'; //not sure how to calculate this, either??
  $vals['17'] = 7;
  $vals['18'] = 'u';
  $vals['19'] = ' ';
  $vals['20-23'] = '4500';

  $val = implode('', $vals);

  //printAndDie($val, strlen($val), $vals); or dpm()

  return $val;
}
/**
 * 00-05 (Date entered on file) = system generated;
 * this is the date MARC record was first created; format must be: ddmmyy
 *
 * 35-37 (Language of the item) = use 3-digit MARC Code for Languages;
 * usually will either be eng (for English) or fre (for French) but
 * could be any of the languages listed in Drupal under Language(s)
 *
 * 39 (Cataloging source) = either use the fill character ( | ) or the
 * code in field 040 subfield $a if it exists in the MarcXML.
 *
 * @param $node
 * @return bool|string
 */
function _cals_importer_export_callback_008($node, $node_wrapper, $xml) {

  $tmp = $xml->xpath("//marc:datafield[@tag='040']/marc:subfield[@code='a']");
  $source = '|';
  if (count($tmp) >= 1) {
    $source = (string) $tmp[0];
  }

  //printAndDie($tmp, $node, $node_wrapper);
  $val = $node->changed;
  $val = $node->created;
  //yyyymmddhhmmss.f
  $val = date('dmy', $val);
  $val =  str_pad($val, 35);
  $lgs = array();
  //$lgs = $node->field_iso_language[LANGUAGE_NONE];
  $lg = (!empty($node->field_iso_language)) ?
    $node->field_iso_language[LANGUAGE_NONE][0]['value']:
    'eng';
  $val .= $lg . $source;
  return $val;
}
/**
 * $a = Original cataloguing agency (contains the code or name of the organization)
 * $b = language of cataloguing - include if present in MarcXML
 * $c and $d = hard-coded to BVNNELS
 *
 * @param $vars
 * @return mixed
 */
function _cals_importer_exporter_callback_040($vars) {
  $node = $vars['node'];
  $tag = $vars['tag'];
  $wrapper = $vars['node_wrapper'];
  $xml = $vars['xml'];
  $value = $wrapper->field_dc_creator->value();
  //$label = ($value == 0) ? "no restrictions" : "restrictions";

  $arr['040'] = array(
    'ind1' => '',
    'ind2' => '',
    'subfields' => array(
      array(
        'c' => 'BVNNELS',
        'd' => 'BVNNELS',
      ),
      //array('value' => $label, 'subfield' => 'a'),
      //array('value' => 'BVNNELS', 'subfield' => 'b'),
    ),
  );

  $tmp = $xml->xpath("//marc:datafield[@tag='040']/marc:subfield[@code='a']");
  if (count($tmp) >= 1) {
    $a = (string) $tmp[0];
    $arr['040']['subfields'][0]['a'] = trim($a);
  }

  $tmp = $xml->xpath("//marc:datafield[@tag='040']/marc:subfield[@code='b']");
  if (count($tmp) >= 1) {
    $b = (string) $tmp[0];
    $arr['040']['subfields']['b'][] = trim($b);
  }


  return $arr;
}

/**
 * Primary title, a bit tricky due to nonfiling characters.
 *
 * @param $node
 * @param $record
 */
function _cals_importer_exporter_callback_240($vars) {
  //if vocab, $term = 'field_subject';
  $node = $vars['node'];
  $tag = $vars['tag'];

  $wrapper = $vars['node_wrapper'];
  $subfields = array();

  $term = $wrapper->field_uniform_title->value();
  if(is_object($term)) {
    $value = trim($term->name);
    $code = 240;
    $subfield = 'a';
    $ind2 = 0;
    $ind2 = _cals_importer_exporter_filing_characters($value);
    $arr[240]['subfields'][]['a'] = $value;
    $arr[240]['ind1'] = 0;
    $arr[240]['ind2'] = $ind2;
    return $arr;
  }

}

/**
 * Primary title, a bit tricky due to nonfiling characters.
 *
 * @param $node
 * @param $record
 */
function _cals_importer_exporter_callback_245($vars) {
  //if vocab, $term = 'field_subject';
  $node = $vars['node'];
  $tag = $vars['tag'];
  $values = _cals_importer_exporter_field_value($node, 'title_field');
  //field_view_field('node', $node, 'title_field', array('default'));
  $safe_values = array();
  if(is_array($values)) {
    foreach($values['#items'] as $value) {
      $subfields = array();
      $safe_values[] = trim($value['value']);
    }
  }
  $code = 245;
  $subfield = 'a';
  $ind2 = 0;
  foreach($safe_values as $value) {
    normalize_title($value);
    $ind2 = _cals_importer_exporter_filing_characters($value);
    $arr[245]['subfields'][]['a'] = $value;
  }
  $arr[245]['ind1'] = 0;
  $arr[245]['ind2'] = $ind2;
  return $arr;

}


/**
 * Returns publisher tag: $a = place, $b = pub'r name, $c = date
 * @param $vars
 * @return mixed
 */
function _cals_importer_exporter_callback_264($vars) {
  $node = $vars['node'];
  $tag = $vars['tag'];
  $wrapper = $vars['node_wrapper'];
  $values = _cals_importer_exporter_field_value($node, 'field_publisher');

  $arr[264] = array(
    'ind1' => '',
    'ind2' => 2,
  );

  //field_view_field('node', $node, 'title_field', array('default'));
  $safe_values = array();
  if(is_array($values)) {
    foreach($values['#items'] as $value) {
      $safe_values[] = trim($value['value']);
    }
  }
  foreach($safe_values as $k => $v) {

    $subcode = ($k == 0) ? 'a' : 'b';
    $v = trim($v);

    //$ind2 = _cals_importer_exporter_filing_characters($value);
    $arr[264]['subfields'][0][$subcode] = $v;
  }
  $pubdate = NULL;
  foreach($wrapper->field_date as $key => $date) {
    $q = strtolower($date->field_qualifier_date->value());
    if($q == 'created') {
      $pubdate = $date->field_dc_date->value();
    }
  }
  if(!empty($pubdate))  $arr[264]['subfields'][0]['c'] = $pubdate;
  return $arr;

}

/**
 * Returns restrictions tag: 0 = no restrictions, 1 = restrictions
 * @param $vars
 * @return mixed
 */
function _cals_importer_exporter_callback_300($vars) {
  $arr[300] = array(
    'ind1' => '',
    'ind2' => '',
    'subfields' => array(array('a' => 'online resource', 'b' => 'digital')),
  );
  return $arr;
}

/**
 * Returns restrictions tag: 0 = no restrictions, 1 = restrictions
 * @param $vars
 * @return mixed
 */
function _cals_importer_exporter_callback_506($vars) {
  $node = $vars['node'];
  $tag = $vars['tag'];
  $wrapper = $vars['node_wrapper'];
  $value = $wrapper->field_access_restrictions->value();
  $label = 'Open access';
  $ind1 = 0;
  if($value != 0) {
    $label = 'Restricted to use by people with a documented print impairment';
    $ind1 = 1;
  }
  $arr[506] = array(
    'ind1' => $ind1,
    'ind2' => '',
    'subfields' => array(array('a' => $label)),
  );
  return $arr;
}

/**
 * Field 506 subfield a identifies restrictions imposed on individuals wishing
 * to see the materials.
 *
 * Open access ind2 = 0
 * Restricted ind2 = 1
 *
 * @param $vars
 * @return array
 *
 * @todo
 *   grab values from attachment
 */
function _cals_importer_exporter_callback_511($vars) {
  $node = $vars['node'];
  $tag = $vars['tag'];
  $subfields[]['a'] = 'Kisly, Dave';
  $wrapper = $vars['node_wrapper'];
  $values = $wrapper->field_access_restrictions->value();

  printAndDie($values);

  $arr =  _cals_build_marc_subfield_array($tag, $subfields);
  $arr[$tag]['ind1'] = 0;
  $arr[$tag]['ind2'] = '';
  return $arr;

}

/**
 * Include if present in Abstract (body) field in Drupal
 * @param $vars
 * @return array
 * @todo - review the proper way to extract body values.
 */
function _cals_importer_exporter_callback_520($vars) {

  $node = $vars['node'];
  $tag = $vars['tag'];
  $wrapper = $vars['node_wrapper'];
  $subfields = array();
  if(!isset($wrapper->body)) {
    return;
  }
  $values = $wrapper->body->value->value(array('decode' => TRUE));

  if(is_string($values)) $subfields[]['a'] = trim($values);
  if(is_array($values)) {
    foreach($values as $value) {
      $value = trim($value);
      if(!empty($value))
        $subfields[]['a'] = trim($value);
    }
  }
  $arr =  _cals_build_marc_subfield_array($tag, $subfields);
  $arr[$tag]['ind1'] = '';
  $arr[$tag]['ind2'] = '';
  return $arr;
}


/**
 * Include if Audience exists in Drupal
 * @param $vars
 * @return array
 */
function _cals_importer_exporter_callback_521($vars) {
  $node = $vars['node'];
  $tag = $vars['tag'];
  $wrapper = $vars['node_wrapper'];
  $def = field_info_field("field_audience");
  $enumerations = list_allowed_values($def);
  $subfields =  _cals_importer_exporter_get_subfield_array_from_drupal_fields('a', 'field_audience', $wrapper, $enumerations);


  $arr =  _cals_build_marc_subfield_array($tag, $subfields);
  if(!empty($arr)) {
    $arr[$tag]['ind1'] = '';
    $arr[$tag]['ind2'] = '';
  }
  return $arr;
}

/**
 * This should be the Publisher (Source) field in Drupal. For example:
 * $c Englewood Cliffs, N.J. : Prentice Hall, 1974.
 * Ideally, we would include the date from the DC Date Issued field in Drupal
 * if the date doesn’t exist in the Publisher (Source) field in Drupal.
 * @param $vars
 * @return array
 */
function _cals_importer_exporter_callback_534($vars) {
  $node = $vars['node'];
  $tag = $vars['tag'];
  $wrapper = $vars['node_wrapper'];
  $subfields = _cals_importer_exporter_get_subfield_array_from_drupal_fields('c', 'field_publisher_source', $wrapper);
  $arr =  _cals_build_marc_subfield_array($tag, $subfields);
  $arr[$tag]['ind1'] = '';
  $arr[$tag]['ind2'] = '';
  return $arr;

}
/**
 * Subject
 * @param $vars
 * @return mixed
 */
function _cals_importer_exporter_callback_653($vars) {
  $node = $vars['node'];
  $tag = $vars['tag'];

  $wrapper = $vars['node_wrapper'];
  $subfields = array();
  $values = $wrapper->field_subject->value();
  foreach($values as $value) {
    $subfields[]['a'] = $value->name;
  }
  return _cals_build_marc_subfield_array($tag, $subfields);
}

/**
 * Genre
 * @param $vars
 * @return array
 */
function _cals_importer_exporter_callback_655($vars) {
  $node = $vars['node'];
  $tag = $vars['tag'];

  $wrapper = $vars['node_wrapper'];
  $subfields = array();
  $values = $wrapper->field_genre->value();
  foreach($values as $value) {
    $subfields[]['a'] = $value->name;
  }
  $arr[$tag] = array(
    'ind1' => '',
    'ind2' => 4,
    'subfields' => $subfields,
  );
  return $arr;
  //return _cals_build_marc_subfield_array($tag, $subfields);
}



/**
 * Added Entry--Uncontrolled Name/Name
 * $a should be the Creator field in Drupal
 * $e = author
 * @param $vars
 * @return mixed
 */
function _cals_importer_exporter_callback_720($vars) {
  $node = $vars['node'];
  $tag = $vars['tag'];
  $wrapper = $vars['node_wrapper'];
  $subfields = array();
  $values = $wrapper->field_dc_creator->value();
  foreach($values as $i => $value) {
    $subfields[$i]['a'] = $value;
    $subfields[$i]['e'] = 'author';
  }


  return _cals_build_marc_subfield_array($tag, $subfields);
}

/**
 * returns 856 subfield.
 * @see https://www.loc.gov/marc/856guide.html
 *
 * @param $vars
 * @return mixed
 */
function _cals_importer_exporter_callback_856($vars) {
  $tag = $vars['tag'];
  $node = $vars['node'];
  //field_publisher
  $nid = $node->nid;
  $arr[856] = array(
    'ind1' => 4,
    'ind2' => 0,
  );
  $arr[856]['subfields'][0]['u'] = 'http://nnels.ca/node/' . $nid;

  return $arr;
}