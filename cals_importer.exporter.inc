<?php


/**
 * Simple wrapper so we can invoke from main repo node page for testing purposes
 *
 * @param $node
 *   the node passed to the parser
 */

function _cals_importer_create_marc_stub_node($node) {
  _cals_importer_create_marc_stub($node);
  drupal_goto("node/{$node->nid}/view-xml-stub");

}


/**
 * VBO to add populate the stub marc-xml string
 *
 * @param $entity
 *   the node passed to the parser
 */
function _cals_importer_create_marc_stub_vbo(&$entity, $context = array()) {
  if (isset($entity->nid)) {
    $node = node_load($entity->nid);
    //safety check to make sure we're parsing repos items!!
    $content_types = array('repository_item');
    if (in_array($node->type, $content_types)  ) {
      _cals_importer_create_marc_stub($node);
    }
    else{
      drupal_set_message(t("Invalid selection: not a repos item"));
    }

  }//end main if

  return "";

}

/**
 * Updates the node with stub marc-xml record
 *
 * @param $node
 *   the node passed to the parser
 *
 */
function _cals_importer_create_marc_stub($node) {
  require 'File/MARCXML.php';
  require 'File/MARC/Lint.php';
  $lint = new File_MARC_Lint();

  $xml_string = $node->field_xml_string[LANGUAGE_NONE][0]['value'];
  $xml_string = MARC_HEADER . $xml_string . '</marc:collection>';
  //$xml = simplexml_load_string($xml_string);
  $records = new File_MARCXML($xml_string, File_MARC::SOURCE_STRING, "http://www.loc.gov/MARC21/slim");

  $n = 0;
  while ($record = $records->next()) {
    $n++;

    _cals_importer_exporter_marcxml_remove_tags($record);
    _cals_importer_exporter_add_title_marcxml($node, $record);
    _cals_importer_exporter_add_taxonomy_terms_marcxml($node, $record);


    // Create the new field
    //$subfields[] = new File_MARC_Subfield('a', 'Scott, Daniel.');
    //$new_field = new File_MARC_Data_Field('100', $subfields, 0, null);

    //$subfields[] = new File_MARC_Subfield('a', 'Kisly, Dave.');
    //$new_field = new File_MARC_Data_Field('100', $subfields, 0, null);

    //printPre($subfields, $new_field);

    // Retrieve the target field for our insertion point
    //iterate through records
    _cals_importer_exporter_iterate_record($record);
    //die;
    $valid = $lint->checkRecord($record);
    if(count($valid)) {
      $vars = array("items" => $valid , "title" => "XML Stub Created with following errors", "type" => "ol", "attributes" => array());
      drupal_set_message(theme_item_list($vars));
    }
    else {
      drupal_set_message(t("XML Stub Created"));
    }

    $new_xml_stub = $record->toXML("UTF-8", FALSE, FALSE);
    //$new_xml_stub = $record->toRaw();
  }

  $node->field_xml_stub[LANGUAGE_NONE][0]['value'] = $new_xml_stub;
  //_cals_importer_update_admin_audit($node,  $action);
  node_save($node);

}
/**
 * Iterate thru taxonomy terms (field_subject, field_genre) and add values to
 * current record
 *
 * @param $node
 * @param $record
 */
function _cals_importer_exporter_add_nnels_values_marcxml($node, &$record) {
  //printAndDie($node);
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
        //printAndDie($safe_value);
        $subfields = array();
        if($tag != '001') $subfields[] = new File_MARC_Subfield($code, $safe_value);
        $field = new File_MARC_Data_Field($tag, $subfields, 0, null);
        $field->setIndicator(1, $ind1);
        $field->setIndicator(2, $ind2);
        //if($tag == '001') printAndDie($field);
        _cals_importer_exporter_insert_field($record, $field, $key);
      }
    }

  }
  //return new File_MARC_Data_Field('650', $subfields, 0, null);
}
/**
 * Iterate thru taxonomy terms (field_subject, field_genre) and add values to
 * current record
 *
 * @param $node
 * @param $record
 */
function _cals_importer_exporter_add_title_marcxml($node, &$record) {
  //if vocab, $term = 'field_subject';
  $fieldnames =
    array(
      //'field_system_control_number' => array('001', '', '', '', NULL),
      'field_dc_creator' => array(100, 'a', 1, '', NULL),
      'title_field' => array(245, 'a', 1, 4, 'normalize_title'),
      //'field_publisher' => array(264, 'a', '', 1, NULL),


      'body' => array(520, 'a', '', '', NULL),
      'field_publisher_source' => array(534, 'c', '', '', NULL),



      //'field_access_restrictions' => array(),
    );

  foreach($fieldnames as $fieldname => $marc) {
    $tag = $marc[0];
    $code = $marc[1];
    $ind1 = $marc[2];
    $ind2 = $marc[3];
    $callback = $marc[4];
    $values = field_view_field('node', $node, $fieldname, array('default'));

    $x = $record->deleteFields($tag);

    if(is_array($values)) {
      foreach($values['#items'] as $value) {
        $subfields = array();
        $safe_value = trim($value['value']);
        if(!empty($callback)) $callback($safe_value);
        //printAndDie($safe_value);
        $subfields = array();
        if($tag != '001') $subfields[] = new File_MARC_Subfield($code, $safe_value);
        $field = new File_MARC_Data_Field($tag, $subfields, 0, null);
        $field->setIndicator(1, $ind1);
        $field->setIndicator(2, $ind2);
        //if($tag == '001') printAndDie($field);
        _cals_importer_exporter_insert_field($record, $field, $key);
      }
    }

  }
  //printAndDie($node, $record);
  //return new File_MARC_Data_Field('650', $subfields, 0, null);
}
function normalize_title(&$value) {

  $last = substr($string, -1);
  if($last != '.') $value .= '.';

}


/**
 * Iterate thru taxonomy terms (field_subject, field_genre) and add values to
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
        _cals_importer_exporter_insert_field($record, $field, $key);
      }
    }
  }
  //return new File_MARC_Data_Field('650', $subfields, 0, null);
}

/**
 * Remove tags from record
 * @param $record
 */
function _cals_importer_exporter_marcxml_remove_tags(&$record) {
  $tags = array(
    '020', '040', 100, 245, 246,
    300, 336, 337, 338, 344,
    347, 538, 506, 520, 521, 534,
    650, 655);
  foreach($tags as $tag) {
    $x = $record->deleteFields($tag);
  }
}

/**
 * Placeholder function
 * @param $record
 */
function _cals_importer_exporter_iterate_record(&$record) {
  foreach ($record->getFields() as $tag=>$value) {

    if ($value instanceof File_MARC_Control_Field) {
      printPre("TAG = $tag | CODE = $code | VALUE = " . $value->getData());
    }
    else {

      $sub = array();
      foreach ($value->getSubfields() as $code=>$subdata) {
        //printPre("TAG = $tag | CODE = $code | VALUE = " . $subdata->getData());
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
  //printAndDie($df);
}

/**
 * Add field to record structure
 * @param $record
 * @param $field
 * @param $id
 *
 */
function _cals_importer_exporter_insert_field(&$record, $field, $id) {
  //e.g., id = 650
  //_cals_importer_exporter_iterate_record($record);
  $record->appendField($field);


  /*
  $id = 700;
  $tag = $record->getFields($id);

  // Insert the new field
  if (is_array($tag)) {
    $record->insertField($field, $tag[0], false);
  }
  elseif ($tags) {
    $record->insertField($field, $tag, true);
  }
  */
  //printAndDie(is_array($tag), $record);


}