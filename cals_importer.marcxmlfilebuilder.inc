<?php
/**
 * @file
 * Generates basic marc xml files
 */


/**
 * Invokes Drupal's batch api to generate new marc xml files with 856 tags, etc.
 *
 */

function _cals_importer_generate_marcxml_files(&$entity, $context = array() ) {

  if (isset($entity->nid)) {
    $node = node_load($entity->nid);
    //safety check to make sure we're parsing repos items!!    
    $content_types = array('repository_item');
    if (!in_array($node->type, $content_types)  ) {
      drupal_set_message(t("Invalid selection: not a repos item"));
      return '';
    }

  }//end main ifx_affected_rows(resource result_id)
  $filename = 'public://' . $context['filename'];
  if($context['progress']['current'] == 1) {
    file_put_contents($filename, '');
  }

  // !empty($parent_nid) && -removed this from the if statement to relax requirements for generating export files

  if (!empty($node->field_xml_string) &&
    !empty($node->field_nnels_856_tag) &&
    $node->field_nnels_856_tag[LANGUAGE_NONE][0]['value'] == 1 ) {
    $xml_test = 1;
    //$xml_test = _cals_get_marc_xml_string($node);
    if($xml_test !== 0) {
      $str =   $node->field_xml_string[LANGUAGE_NONE][0]['value'];
      file_put_contents($filename, $str, FILE_APPEND | LOCK_EX);
    }
  }

  if ($context['progress']['current'] == $context['progress']['total']) {

    //add closing marc:collection tag
    $string = file_get_contents($filename);
    $xml = _cals_importer_get_marcxml_wrapper($string);
    $file = file_save_data($xml->asXml(), $filename, FILE_EXISTS_REPLACE);
    $file->display = 1;
    $file->description = '';
    $parent_nid = $node->field_record_set[LANGUAGE_NONE][0]['nid'];
    $parent = node_load($parent_nid);
    $parent->field_marc_xml_output_file[LANGUAGE_NONE][0] = (array)$file;
    node_save($parent);
  }

}


function _cals_importer_get_marcxml_wrapper($string) {
  $string = str_replace("</marc:collection>", "", $string);
  $header = '<marc:collection
          xmlns:marc="http://www.loc.gov/MARC21/slim" 
          xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
          xsi:schemaLocation="http://www.loc.gov/MARC21/slim http://www.loc.gov/standards/marcxml/schema/MARC21slim.xsd"
        >';
  $content = $header . $string . '</marc:collection>';
  $xml = simplexml_load_string($content);

  return $xml;
}

/**
 * Invokes Drupal's batch api to generate new marc xml files with 856 tags, etc.
 * Unlike _cals_importer_generate_marcxml_files, this function generates an
 * un-managed file called "nnels_marcxml_all_records.xml".
 *
 */

function _cals_importer_generate_marcxml_files_all_records(&$entity, $context = array() ) {

  if (isset($entity->nid)) {
    $node = node_load($entity->nid);
    //safety check to make sure we're parsing repos items!!    
    $content_types = array('repository_item');
    if (!in_array($node->type, $content_types)  ) {
      drupal_set_message(t("Invalid selection: not a repos item"));
      return '';
    }

  }//end main if

  //static $xml = '';
  $filename = 'public://nnels_marcxml_all_records.xml';
  if($context['progress']['current'] == 1) {
    file_put_contents($filename, '');
  }

  if (!empty($node->field_xml_string) &&
    !empty($node->field_nnels_856_tag) &&
    $node->field_nnels_856_tag[LANGUAGE_NONE][0]['value'] == 1 ) {
    $xml_test = _cals_get_marc_xml_string($node);
    if($xml_test !== 0) {
      $str =  $node->field_xml_string[LANGUAGE_NONE][0]['value'];
      file_put_contents($filename, $str, FILE_APPEND | LOCK_EX);
    }
  }

  if ($context['progress']['current'] == $context['progress']['total']) {
    $uri = file_create_url($filename);
    $xml = MARC_HEADER . $xml . '</marc:collection>';
    $content = MARC_HEADER . file_get_contents($filename) . '</marc:collection>';
    $xml = simplexml_load_string($content);
    $file = file_save_data($xml->asXml(), $filename, FILE_EXISTS_REPLACE);
    drupal_set_message(t("File saved: please download via <a href='@url'>@url</a>",
      array("@url" => $uri ) ) );
  }

}

function _cals_importer_generate_marcxml_files_search_interface_form($settings, &$form_state) {
  $form = array();
  dpm($form_state['values']);
  //printAndDie($form_state['values']);

  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title of new record-set'),
    '#required' => TRUE,
    '#size' => 20,
    '#default_value' => isset($settings['settings']['title']) ? $settings['settings']['title'] : 'my title',
  );
  $form['body'] = array(
    '#type' => 'textarea',
    '#rows' => 5,
    '#title' => t('Brief description'),
    '#required' => TRUE,
    '#size' => 20,
    '#default_value' => isset($settings['settings']['body']) ? $settings['settings']['body'] : 'my description',
  );


  return $form;
}

function _cals_importer_generate_marcxml_files_search_interface_submit($form, $form_state) {
  $return = array();
  $title = $form_state['values']['title'];
  $body = $form_state['values']['body'];


  $node = new StdClass();
  $node->type = 'marc_export_set';
  $node->language = LANGUAGE_NONE;
  node_object_prepare($node);

  $node->title = $title;
  $node->body[LANGUAGE_NONE][0]['value'] = $form_state['values']['body'];
  node_save($node);

  //Prepare folder
  $dir = 'public://marc_exports';
  file_prepare_directory($dir, FILE_CREATE_DIRECTORY);
  $filename = "public://marc_exports/marcxml_export_{$node->nid}.xml";
  file_put_contents($filename, '');

  $return['fields'] = array(
    "pnid" => $node->nid,
    "filename" => $filename
  );
  return $return; //Note, return value here must be an array.
}


/**
 * Generates marc xml sets with attached marc xml file.
 * See: https://github.com/BCLibCoop/nnels/issues/124
 * @param $entity
 * @param array $context
 */
function _cals_importer_generate_marcxml_files_search_interface(&$entity, $context = array() ) {
  $host = $entity->hostEntity();
  $nid = $host->nid;
  $filename = $context['fields']['filename'];
  $pnid = $context['fields']['pnid'];
  $pnode = node_load($pnid); //get updated parent node

  if (!empty($host->field_xml_string) &&
    !empty($host->field_nnels_856_tag) &&
    $host->field_nnels_856_tag[LANGUAGE_NONE][0]['value'] == 1 ) {
    $xml = _cals_get_marc_xml_string($host);
    if($xml !== 0) {
      $str =  $host->field_xml_string[LANGUAGE_NONE][0]['value'];
      file_put_contents($filename, $str, FILE_APPEND | LOCK_EX);
      $pnode->field_nid_entry[LANGUAGE_NONE][] = array("nid" => $nid);
      node_save($pnode);
    }
  }
  if ($context['progress']['current'] == $context['progress']['total']) {
    $uri = file_create_url($filename);
    //add closing marc:collection tag
    $content = MARC_HEADER . file_get_contents($filename) . '</marc:collection>';
    $xml = simplexml_load_string($content);
    $file = file_save_data($xml->asXml(), $filename, FILE_EXISTS_REPLACE);
    $file->display = 1;
    $pnode->field_marc_xml_output_file[LANGUAGE_NONE][0] = (array)$file;
    node_save($pnode);
    drupal_set_message(t("File saved: please download via <a href='@url'>@url</a>",
      array("@url" => url("node/" . $pnode->nid) ) ) );
  }
}


/**
 * Simple wrapper to call main parser; if preferred, can invoke batch api instead.
 */

function _cals_importer_generate_marcxml_file($node) {

  $xml = _cals_importer_build_marcxml($node);
  if (strlen($xml)) {
    $filebuilder = new MarcXmlFileBuilder($node, $xml);
    $filebuilder->createFile();
  }
  //take user back to main node page
  drupal_goto("node/" . $node->nid);

}


/**
 * builds the xml string
 *
 */

function _cals_importer_build_marcxml($node) {
  $xml = '';
  $parent_nid = $node->field_record_set[LANGUAGE_NONE][0]['nid'];

  //if there's a parent recordset and an 856 tag = 1
  if (!empty($parent_nid) &&
    !empty($node->field_xml_string) &&
    !empty($node->field_nnels_856_tag) &&
    $node->field_nnels_856_tag[LANGUAGE_NONE][0]['value'] == 1 ) {
    return $node->field_xml_string[LANGUAGE_NONE][0]['value'];
  }
  else {
    if(empty($parent_nid) ) drupal_set_message("No parent record-set to associate file with.");
    if(empty($node->field_xml_string) ) drupal_set_message("No Xml string to parse");
    if(empty($node->field_nnels_856_tag) || $node->field_nnels_856_tag[LANGUAGE_NONE][0]['value'] != 1)
      drupal_set_message("No 856 Tag");
    return;

  }


}




/**
 * Simple wrapper so we can invoke from main repo node page for testing purposes
 *
 * @param $node
 *   the node passed to the parser
 */

function _cals_importer_create_marc_stub_node($node) {
  _cals_importer_create_marc_stub($node);
  drupal_goto("node/" . $node->nid);

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
    foreach($terms["#items"] as $term){
      $subfields = array();
      $subfields[] = new File_MARC_Subfield($code, $term["taxonomy_term"]->name);
      $field = new File_MARC_Data_Field($tag, $subfields, 0, null);
      $field->setIndicator(1, $ind1);
      $field->setIndicator(2, $ind2);
      _cals_importer_exporter_insert_field($record, $field, $key);
    }
  }
  //return new File_MARC_Data_Field('650', $subfields, 0, null);
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
      // Iterate through the subfields in this data field
      foreach ($value->getSubfields() as $code=>$subdata) {
       printPre("TAG = $tag | CODE = $code | VALUE = " . $subdata->getData());
      }
    }
  }
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
    // Create the new field
    //$subfields[] = new File_MARC_Subfield('a', 'Scott, Daniel.');
    //$new_field = new File_MARC_Data_Field('100', $subfields, 0, null);

    //$subfields[] = new File_MARC_Subfield('a', 'Kisly, Dave.');
    //$new_field = new File_MARC_Data_Field('100', $subfields, 0, null);

    //printPre($subfields, $new_field);

    // Retrieve the target field for our insertion point
    //iterate through records
    //_cals_importer_export_iterate_record($record);
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
    //printAndDie($valid, $new_xml_stub);

  }

  $node->field_xml_stub[LANGUAGE_NONE][0]['value'] = $new_xml_stub;
  //_cals_importer_update_admin_audit($node,  $action);
  node_save($node);

}


class MarcXmlFileBuilder {

  public function __construct($node, $xml) {
    $this->node = $node;
    $this->xml = $xml;
  }

  public function createFile() {
    $parent_nid = $this->node->field_record_set[LANGUAGE_NONE][0]['nid'];
    $parent = node_load($parent_nid);

    //$myxml = $this->xml->asXml();

    $filename = 'nnels_marcxml_' . $parent->nid . '.xml';

    $dom = new DOMDocument('1.0');
    $dom->preserveWhiteSpace = FALSE;
    $dom->formatOutput = TRUE;
    //$dom->loadXML($cmr->asXML());

    $xml_string =
      '<marc:collection 
          xmlns:marc="http://www.loc.gov/MARC21/slim" 
          xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
          xsi:schemaLocation="http://www.loc.gov/MARC21/slim http://www.loc.gov/standards/marcxml/schema/MARC21slim.xsd"
        >' . $this->xml . '</marc:collection>';
    $xml = simplexml_load_string($xml_string);


    $file = file_save_data($xml->asXml(), 'public://' . $filename);
    $file->display = 1;
    $file->description = '';

    //output system message
    drupal_set_message(l("file", 'public://' . $filename));


    //$xml->asXml($filename);
    $parent->field_marc_xml_output_file[LANGUAGE_NONE][0] = (array)$file;
    node_save($parent);

    try {
      //$file = file_copy($file, 'public://');
    } catch (Exception $e) {
      printAndDie($e->getMessage());
      drupal_set_message($e->getMessage());
      watchdog('my_error', $e->getMessage());
    }


  }


}