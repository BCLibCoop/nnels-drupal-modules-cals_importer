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
    _cals_importer_exporter_add_taxonomy_terms_marcxml($node, $record);
    _cals_importer_exporter_add_gmd_per_format($node, $record);
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
  //return new File_MARC_Data_Field('650', $subfields, 0, null);
}

/**
 * Given a collection of formats and corresponding Data Field pattern, create
 * the Data Fields per format and append to the record.
 *
 * @param $node
 * @param $record
 * @return int|void
 */

function _cals_importer_exporter_add_gmd_per_format($node, &$record) {

  $format_keys = array(
    'epub 2' => 'etext',
    'epub 3' => 'etext',
    'pdf' => 'etext',
    'e-text' => 'etext',
    'azw3' => 'etext',
    'mp3' => 'mp3',
    'daisy 202' => 'daisy',
    'daisy 3' => 'daisy',
  );

  /**
   * Electronic text (EPUB, PDF, e-text)
   *  =336 \\$a txt $2 rdacontent
   *  =337 \\$a computer $2 rdamedia
   * =338 \\$a online resource $2 rdacarrier
   **/

  $etext_pattern = array(
    0 => array(
      '336' => array (
        'a' => 'txt',
        2 => 'rdacontent',
      )
    ),
    1 => array(
      '337' => array(
        'a' => 'computer',
        2 => 'rdamedia',
      )
    ),
    2 => array(
      '338' => array(
        'a' => 'online resource',
        2 => 'rdacarrier',
      )
    ),
  );

  /**
   * MP3 books
   * =336  \\$a spoken word $b spw $2 rdacontent
   * =337  \\$a audio $b s $2 rdamedia
   * =337  \\$a computer $b c $2 rdamedia
   * =338  \\$a other $b sz $2 rdacarrier
   * =338  \\$a online resource $b cr $2 rdacarrier
   **/

  $mp3_pattern = array(
    0 => array(
      '336' => array (
        'a' => 'spoken word',
        'b' => 'spw',
        2 => 'rdacontent',
      )
    ),
    1 => array(
      '337' => array(
        'a' => 'audio',
        'b' => 's',
        2 => 'rdamedia',
      )
    ),
    2 => array(
      '337' => array(
        'a' => 'computer',
        'b' => 'c',
        2 => 'rdamedia',
      )
    ),
    3 => array(
      '338' => array(
        'a' => 'other',
        'b' => 'sz',
        2 => 'rdacarrier',
      )
    ),
    4 => array(
      '338' => array(
        'a' => 'online-resource',
        'b' => 'cr',
        2 => 'rdacarrier',
      ),
    ),
  );

  /**
   * DAISY books
   * =336  \\$a spoken word $b spw $2 rdacontent
   * =336  \\$a txt $2 rdacontent
   * =337  \\$a audio $b s $2 rdamedia
   * =337  \\$a computer $b c $2 rdamedia
   * =338  \\$a other $b sz $2 rdacarrier
   * =338  \\$a online resource $b cr $2 rdacarrier
   **/

  $daisy_pattern = array(
    0 => array(
      '336' => array (
        'a' => 'spoken word',
        'b' => 'spw',
        2 => 'rdacontent',
      )
    ),
    1 => array(
      '336' => array(
        'a' => 'txt',
        2 => 'rdacontent',
      )
    ),
    2 => array(
      '337' => array(
        'a' => 'audio',
        'b' => 's',
        2 => 'rdamedia',
      )
    ),
    3 => array(
      '337' => array(
        'a' => 'computer',
        'b' => 'c',
        2 => 'rdamedia',
      )
    ),
    4 => array(
      '338' => array(
        'a' => 'other',
        'b' => 'sz',
        2 => 'rdacarrier',
      ),
    ),
    5 => array(
      '338' => array(
        'a' => 'online-resource',
        'b' => 'cr',
        2 => 'rdacarrier',
      )
    ),
  );

  $node_wrapper = entity_metadata_wrapper('node', $node);

  foreach ($node_wrapper->field_file_resource->getIterator() as $delta =>
           $field_collection ) {
    $format = strtolower($field_collection->field_file_format->label());

    $pattern = (string) $format_keys[$format] . '_pattern';
    $loaded_pattern = $$pattern;

    if (empty($loaded_pattern)) {
      drupal_set_message($loaded_pattern, "No pattern found: ");
      return;
    }

    $subfields = array();

    foreach ($loaded_pattern as $instance => $tags) { //0 => array of tags
      foreach ($tags as $tag => $codes) { //336 => array of codes
        foreach ($codes as $key => $value) { //code => entry
          if (in_array($key, array('a', 'b', 1, 2, 3))) {
            $subfields[] = new File_MARC_Subfield($key, $value);
            unset($key);
          }
        }
        //Roll up all subfields into a DataField for each tag
        $field = new File_MARC_Data_Field($tag, $subfields, 0, NULL);
        unset($subfields);

        //Insert the current field and re-loop, multiple entries can exist
        _cals_importer_exporter_insert_field($record, $field, $tag);
        unset($field);
      }
    }
  }
}

/**
 * Remove tags from record
 * @param $record
 */
function _cals_importer_exporter_marcxml_remove_tags(&$record) {
  $tags = array(300, 336, 337, 338, 344, 347, 538, 650, 655);
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
  /* TODO: add the hinted functionality here to take a tag and insert it
  in-order */

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