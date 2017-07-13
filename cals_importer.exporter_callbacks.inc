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
function _cals_importer_export_callback_008($node) {

  $val = $node->changed;
  //yyyymmddhhmmss.f
  $val = date('dmy', $val[0]);
  $val =  str_pad($val, 30);
  $lgs = array();
  //$lgs = $node->field_iso_language[LANGUAGE_NONE];
  $lg = (!empty($node->field_iso_language)) ?
    $node->field_iso_language[LANGUAGE_NONE][0]['value']:
    'eng';
  $val .= $lg . '|';
  return $val;
}
/**
 * Returns restrictions tag: 0 = no restrictions, 1 = restrictions
 * @param $vars
 * @return mixed
 */
function _cals_importer_exporter_callback_040($vars) {
  $node = $vars['node'];
  $tag = $vars['tag'];
  $wrapper = $vars['node_wrapper'];
  $value = $wrapper->field_dc_creator->value();
  //$label = ($value == 0) ? "no restrictions" : "restrictions";
  $arr['040'] = array(
    'ind1' => '',
    'ind2' => '',
    'subfields' => array(
      'c' => array('BVNNELS'),
      'd' => array('BVNNELS')
      //array('value' => $label, 'subfield' => 'a'),
      //array('value' => 'BVNNELS', 'subfield' => 'b'),
    ),
  );
  return $arr;
}


/**
 * Uniform title, include if this exists in the original marc xml
 *
 * @param $node
 * @param $record
 */
function _cals_importer_exporter_callback_240x($vars) {
  //if vocab, $term = 'field_subject';
  //printAndDie($vars);
  $subfields = array();
  $tag = $vars['tag'];
  $code = $vars['code'];
  $subs1 = $tag['subfields'];
  $subs2 = $tag['original']['sub'];

  $keys = array_keys($subs1);


  foreach($subs1 as $k => $v) {
    $code = $v['field_subfield_code'];
    //printPre("CODE = $code");

  }
  foreach($subs2 as $kk => $vv) {

    $subfields = $subfields + $subs2[$kk];
    if($kk == $code) {

    }
  }

  printAndDie("HERE?", $subfields,  $tag);
  $arr = array(240, 650);

  foreach ($record->getFields() as $tag => $subfields) {
    // Skip everything except for 650 fields
    if ($tag == '650' || $tag == '240') {
      foreach ($subfields->getSubfields() as $code => $value) {
        $subfields[] = array(
          'value' => $value,
          'subfields' => 'a',
        );
      }
    }
  }


  printAndDie("HERE?",$subfields);

  $tag = $vars['tag'];
  $tag['field_indc1'] = '';
  $tag['field_ind2'] = '';

  $wrapper = $vars['node_wrapper'];
  $subfields = array();
  $values = $wrapper->field_dc_creator->value();
  foreach($values as $value) {
    $subfields[] = array(
      'value' => $value,
      'subfields' => 'a',
    );
  }

  $subfields[] = array(
    'value' => "Kisly, Dave",
    'subfields' => 'e',
  );

  return _cals_build_marc_subfield_array($tag, $subfields);
  die;
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
    $value = trim($value);
    $ind2 = _cals_importer_exporter_filing_characters($value);
    $rec[245]['subfields'][] = array(
      'value' => $value . '.', 'subfields' => 'a',
    );
  }
  $rec[245]['ind1'] = $tag['field_ind1'];
  $rec[245]['ind2'] = $ind2;
  printAndDie($rec);
  return $rec;

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
    $rec[245]['subfields']['a'][] = $value;
  }
  $rec[245]['ind1'] = 0;
  $rec[245]['ind2'] = $ind2;
  return $rec;

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
    $s = ($k == 0) ? 'a' : 'b';
    $v = trim($v);
    //$ind2 = _cals_importer_exporter_filing_characters($value);
    $arr[264]['subfields'][$s][] = $v;
  }

  $dateset = FALSE;
  foreach($wrapper->field_date as $key => $date) {
    $q = strtolower($date->field_qualifier_date->value());
    if($q == 'created') {
      $d = $date->field_dc_date->value();
      //printPre("Q = $q | DATE = $d");
      $dateset = TRUE;
    }
  }
  if($dateset) {
    $arr[264]['subfields']['c'][] = $d;
  }
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
  $label = ($value == 0) ? "no restrictions" : "restrictions";
  $arr[506] = array(
    'ind1' => '',
    'ind2' => '',
    'subfields' => array('a' => array($label)),
  );
  return $arr;
}

function _cals_importer_exporter_callback_511($vars) {
  $node = $vars['node'];
  $tag = $vars['tag'];
  $subfields['a'][] = 'Kisly, Dave';
  $me =  _cals_build_marc_subfield_array($tag, $subfields);
  $me[$tag]['ind1'] = 0;
  $me[$tag]['ind2'] = '';
  return $me;

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
  $subfields = array();
  $values = $wrapper->field_publisher_source->value();
  foreach($values as $value) {
    $subfields['c'][] = $value;
  }

  $arr =  _cals_build_marc_subfield_array($tag, $subfields);
  $arr[$tag]['ind1'] = '';
  $arr[$tag]['ind2'] = '';
  return $arr;

}
/**
 * Added Entry--Uncontrolled Name/Name
 * $a should be the Creator field in Drupal
 * $e = author
 * @param $vars
 * @return mixed
 */
function _cals_importer_exporter_callback_7201($vars) {
  $node = $vars['node'];
  $tag = $vars['tag'];
  $tag['field_indc1'] = '';
  $tag['field_ind2'] = '';

  $wrapper = $vars['node_wrapper'];
  $subfields = array();
  $values = $wrapper->field_dc_creator->value();
  foreach($values as $value) {
    $subfields['a'][] = $value;
  }

  $subfields['e'][] =  'Kisly, Dave';

  return _cals_build_marc_subfield_array($tag, $subfields);
}


function _cals_importer_exporter_callback_7201_e($vars) {
  printAndDie($vars);
  $node = $vars['node'];
  $tag = $vars['tag'];
  $tag['field_indc1'] = '';
  $tag['field_ind2'] = '';

  $wrapper = $vars['node_wrapper'];
  $subfields = array();
  $values = $wrapper->field_dc_creator->value();
  foreach($values as $value) {
    $subfields['a'][] = $value;
  }

  $subfields['e'][] =  'Kisly, Dave';

  return _cals_build_marc_subfield_array($tag, $subfields);
}

/**
 * @param $tag
 * @param $subfields
 * @return array
 */
function _cals_build_marc_subfield_array($tag, $subfields) {
  //if($tag['field_code'] == 720) printAndDie($tag);
  //printPre($tag);
  //if($tag['field_indc1'] == "''") $tag['field_indc1'] = '';
  //if($tag['field_ind2'] == "''") $tag['field_ind2'] = '';
  return array($tag => array(
    'subfields' => $subfields,
  ));

}

function _cals_importer_exporter_callback_856($vars) {
  $tag = $vars['tag'];
  $node = $vars['node'];
  //field_publisher
  $nid = $node->nid;
  $arr[856] = array(
    'ind1' => '',
    'ind2' => '',
  );
  $arr[856]['subfields']['u'][] = 'http://nnels.ca/node/' . $nid;

  return $arr;
}