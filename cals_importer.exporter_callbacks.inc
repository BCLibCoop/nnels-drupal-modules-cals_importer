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
function _cals_importer_exporter_callback_leader($wrapper) {
  $vals['00-04'] = 00000; //handled by File_MARC_Record::toRaw() once record
  // is built
  $vals['05'] = 'n';
  //$vals['06'] = 'i'; //Provisioned by GMD callback per file entity, just
  // need to merge in. It's returned pre-keyed for byte 6
  $vals['07'] = 'm';
  $vals['08'] = ' ';
  $vals['09'] = ' ';
  $vals['10'] = 2;
  $vals['11'] = 2;
  $vals['12-16'] = 'XXXXX'; //Base address of data
  // - leader + directory +
  // field terminator (␞), 5 bytes
  // handled by File_MARC_Record::toRaw()
  //see https://github.com/BCLibCoop/nnels/issues/133
  $vals['17'] = 2;
  $vals['18'] = 'u';
  $vals['19'] = ' ';
  $vals['20-23'] = '4500';

  $val = trim(implode('', $vals));

  //printAndDie($val, strlen($val), $vals);

  return $val;
}

/**
 * @param $record
 */
//function _cals_importer_exporter_calc_base_address($record) {
//
//  $base = sprintf('%08d', strlen($dirLength) + 24 + 1);
//  return array( 'base' => $base);
//}

function _cals_importer_exporter_calc_dir_length($raw_record) {
  //Where that directory at?
  preg_match('/4500(\d+)\x1E[\D\w]/', $raw_record, $matches);
  //24 (ldr) + dir + 1
  $dirLength = strlen($matches[1]) + 24 + 1;
  //Zero-pad base address for 5 bytes (12-16)
  return str_pad($dirLength, 5, '0', STR_PAD_LEFT);
}

function _cals_importer_exporter_calc_ldr_length($raw_record) {
  //Number of bytes of record + 24-byte leader + 1 record terminator byte
  $ldrLength = strlen($raw_record) + 24 + 1;
  //Zero-pad base address for 5 bytes (00-04)
  return str_pad($ldrLength, 5, '0', STR_PAD_LEFT);
}

/**
 * @param array $supplied_bytes Should be array of byte positions + values
 * @return array
 */
function _cals_importer_exporter_build_final_leader($supplied_bytes) {

  $ldr = array_fill(0, 23, MARC_SPACE);
  $ldr[5] = 'n';
  //@todo: Re-check byte 6 in case new format-dependent bytes in future
  $ldr[6] = $supplied_bytes[6] ? $supplied_bytes[6] : MARC_SPACE;
  $ldr[7] = 'm';
  $ldr[8] = MARC_SPACE;
  $ldr[9] = MARC_SPACE;
  $ldr[10] = 2;
  $ldr[11] = 2;
  $ldr[17] = 'u';
  $ldr[18] = 'u';
  $ldr[19] = MARC_SPACE;

  //Initialize with string type
  $ldr_out = (string) '';

  //Process into string
  //PHP 5.6 casts this as array if using the string pos notation so we append.
  foreach ($ldr as $key => $char) {
    $ldr_out .= $char;
  }

  //Byte positions 00-04, 06, 12-16 and 20-23 handled by
  //_cals_importer_exporter_calc_{ldr_length/dir_length}
  $returned['value'] = $ldr_out;
  return $returned;
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

  //initialize
  $arr['040'] = array(
    'ind1' => '',
    'ind2' => '',
    'subfields' => array()
  );

  $tmp = $xml->xpath("//marc:datafield[@tag='040']/marc:subfield[@code='a']");
  if ( !empty ( (string) $tmp[0] ) ) {
    $a = (string) $tmp[0];
  }
  else { //Required default for subfield a
    $a = 'BVNNELS';
  }
  //Add found subfield or default
  $arr['040']['subfields'][0]['a'] = trim($a);

  $tmp = $xml->xpath("//marc:datafield[@tag='040']/marc:subfield[@code='b']");
  if ( !empty ( (string) $tmp[0] ) ) {
    $b = (string) $tmp[0];
    $arr['040']['subfields'][0]['b'] = trim($b);
  }
  //Remaining non-variable identifiers for subfields c, d
  $arr['040']['subfields'][0]['c'] = 'BVNNELS';
  $arr['040']['subfields'][0]['d'] = 'BVNNELS';

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
  $lang = reset($wrapper->field_iso_language->value());
  $subfields = array();

  $term = $wrapper->field_uniform_title->value();
  if(is_object($term)) {
    $value = trim($term->name);
    $code = 240;
    $subfield = 'a';
    $ind2 = 0;
    $ind2 = _cals_importer_exporter_filing_characters($value, $lang);
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

  $lang = reset(entity_metadata_wrapper('node', $node)
    ->field_iso_language->value());

  foreach($safe_values as $value) {
    normalize_title($value);
    $ind2 = _cals_importer_exporter_filing_characters($value, $lang, 0, 0);
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
  $lang = reset($wrapper->field_iso_language->value());

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
    //Why was this disabled?
    //$ind2 = _cals_importer_exporter_filing_characters($value, $lang);
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

  //loop thru the following to build up the 534 tag
  $tmp = array(
    'field_publisher_source' => 'c',
    'field_isbn' => 'z',
    'field_isbn_audio' => 'z',
  );

  $subfields = array();

  $i = 0;
  foreach($tmp as $fieldname => $code) {
    //printAndDie($code, $fieldname, $wrapper);
    $stored_subfield_array =
      _cals_importer_exporter_get_subfield_array_from_drupal_fields($code, $fieldname, $wrapper);

    if(!empty($stored_subfield_array) ) {
      //if(count($me) == 1) $subfields[$i] = $me[0];
      if(count($stored_subfield_array) >= 1) $subfields = array_merge
      ($subfields, $stored_subfield_array);
      //Now $subfields is populated.
      $i++;
    }
  }

  //Concat subfield $z since it can only come from Drupal field
  foreach ($subfields as $sub => $key) {
    if (key($key) == 'z') {
      $take[] = $subfields[$sub]['z'];
      unset($subfields[$sub]);
    }
  }
  if (isset($take) && count($take) > 1) $subfields[]['z'] = implode(' ; ',
    $take);
  else $subfields[]['z'] = $take[0];

  //Default to MARCXML value for 534c
  $xml_534c = reset(_cals_importer_get_xpath_values($tag, NULL, NULL, 'c',
    $vars['xml']->record));

  //If we don't have it from MARCXML, build of it from Drupal fields
  if ( empty( $xml_534c ) ) {

    $pub_parts = array(
      0 => '',
      1 => '',
      2 => '',
    );

    //Cleanup values and then filter our repeats
    $test_values = array();
    foreach ($subfields as $instance => $subfield) {
      if ( isset($subfield['c']) ) {
        //Remove leading spaces, square brackets and
        $subfield['c'] =
          _cals_importer_strip_square_brackets_value(
          trim( $subfield['c'] )
        );
        if ( in_array( $subfield['c'], $test_values ) ) {
          unset($subfields[$instance]);
          $subfield['c'] = NULL;
        }
        if ($subfield['c']) $subfields[$instance]['c'] = $test_values[] =
          $subfield['c'];
      }
    }
    //Rekey in case we filtered out any repeat values above
    $subfields = array_values( $subfields );

    //0 => publisher, 1 => location
    //if there's already two instances of publisher_source
    if ( isset( $subfields[1]['c']) ) {
      $pub_parts[0] = $subfields[0]['c'];
      $pub_parts[1] = $subfields[1]['c'];
      unset($subfields[1]); //No repeat instances of subfield c allowed.
    }
    else {
      //Single in old form Englewood Cliffs, N.J. : Prentice Hall, 1974.
      //Explode on colon and check for copyright to discard in favour of DC Date
      $exploded = explode(':', $subfields[0]['c']);
      if (! isset($exploded[1])) {
        $exploded_retry = explode('.,', $subfields[0]['c']);
        if (isset($exploded_retry[1])) {
          //Add the exploded period (.) back for abbreviation
          $exploded_retry[0] .= '.';
          //Replace if we tried second method
          $exploded = $exploded_retry;
        }
      } else {
        $exploded[1] = preg_replace(
          "/([\w\s]+?),?\s?[\©c\[\]\d]+\.?/",
          "$1",
          $exploded[1]
        );
      }
      foreach ($exploded as $instance => $piece) {
        $pub_parts[$instance] = $piece;
      }
    } //end else

    //Attempt to backfill for empty values
    $placeholders = array(
      0 => '[place of publication not identified]',
      1 => '[publisher not identified]',
      2 => '[date of publication not identified]',
    );

    $j = -1;
    foreach ($pub_parts as $key => $value) {
      if (empty($value)) {
        $pub_parts[$key] = $placeholders[$key];
        $j++;
      }
      //All three were empty, produce no 534
      if ($j == 2) {
        return;
      }
    }

    //Re-glue the parts, name + location
    $subfields[0]['c'] = $pub_parts[0] . ' : ' . $pub_parts[1];

    //Date published, ensure appends with a comma and ends with period.
    //Find an Issued or Copyrighted DC Date to append
    $qualifiers = array(
      'Created',
      'Copyrighted'
    );
    foreach ( $wrapper->field_date->getIterator() as $delta => $fc_instance) {
      if (in_array($fc_instance->field_qualifier_date->value(), $qualifiers)) {
        $pub_parts[2] = $fc_instance->field_dc_date->value();
        break 1;
      }
    }
    if (isset($pub_parts[2])) {
      $subfields[0]['c'] .= ', ' . $pub_parts[2];
    }
    $subfields[0]['c'] .= '.';
  }
  else {
    //Otherwise replace C instances
    foreach ($subfields as $sub => $key) {
      if (key($subfields[$sub]) == 'c') unset($subfields[$sub]);
    }
    $subfields[]['c'] = $xml_534c;
  }

  //Rekey for middle C
  $subfields[]['c'] = $subfields[0]['c'];
  unset($subfields[0]['c']);

  //Set subfield p. Required.
  $subfields[]['p'] = "Original version: ";

  //Re-key, flatten-merge recursively and slap in an instance
  //This is to produce a single 534 instance with subfields and $p on top.
  krsort($subfields);
  $subfields = array_reduce(
    array_values($subfields),
    'array_merge',
    array()
  );
  $subfield_instance[] = $subfields;

  $arr =  _cals_build_marc_subfield_array( $tag, $subfield_instance );
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
  $n = $i;
  $values = $wrapper->field_dc_contributor->value();
  foreach($values as $i => $value) {
    $n++;
    $subfields[$n]['a'] = $value;
    $subfields[$n]['e'] = 'contributor';

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