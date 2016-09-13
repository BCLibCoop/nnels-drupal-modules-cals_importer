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
    $xml = '<marc:collection 
          xmlns:marc="http://www.loc.gov/MARC21/slim" 
          xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
          xsi:schemaLocation="http://www.loc.gov/MARC21/slim http://www.loc.gov/standards/marcxml/schema/MARC21slim.xsd"
        >' . $xml . '</marc:collection>';     

    $header = '<marc:collection 
            xmlns:marc="http://www.loc.gov/MARC21/slim" 
            xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
            xsi:schemaLocation="http://www.loc.gov/MARC21/slim http://www.loc.gov/standards/marcxml/schema/MARC21slim.xsd"
          >';
    //add closing marc:collection tag
    $content = $header . file_get_contents($filename) . '</marc:collection>';
    $xml = simplexml_load_string($content);
    $file = file_save_data($xml->asXml(), $filename, FILE_EXISTS_REPLACE);
    drupal_set_message(t("File saved: please download via <a href='@url'>@url</a>", 
      array("@url" => $uri ) ) );
  }

}

function _cals_importer_generate_marcxml_files_search_interface_form($settings, &$form_state) {
  $form = array();

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
    "node" => $node,
    "filename" => $filename
  );
  return $return; //Note, return value here must be an array.
}


/**
 * Invokes Drupal's batch api to generate new marc xml files with 856 tags, etc.
 * Unlike _cals_importer_generate_marcxml_files, this function generates an
 * un-managed file called "nnels_marcxml_all_records.xml".
 *
 */

function _cals_importer_generate_marcxml_files_search_interface(&$entity, $context = array() ) {

  $host = $entity->hostEntity();
  $nid = $host->nid;

  $filename = $context['fields']['filename'];
  $export_node = $context['fields']['node'];

  if (!empty($host->field_xml_string) &&
    !empty($host->field_nnels_856_tag) &&
    $host->field_nnels_856_tag[LANGUAGE_NONE][0]['value'] == 1 ) {
    $xml = _cals_get_marc_xml_string($host);
    if($xml !== 0) {
      $str =  $host->field_xml_string[LANGUAGE_NONE][0]['value'];
      file_put_contents($filename, $str, FILE_APPEND | LOCK_EX);
    }
  }
  if ($context['progress']['current'] == $context['progress']['total']) {
    $uri = file_create_url($filename);
    //add closing marc:collection tag
    $content = MARC_HEADER . file_get_contents($filename) . '</marc:collection>';
    $xml = simplexml_load_string($content);
    $file = file_save_data($xml->asXml(), $filename, FILE_EXISTS_REPLACE);
    $file->display = 1;
    $export_node->field_marc_xml_output_file[LANGUAGE_NONE][0] = (array)$file;
    node_save($export_node);
    drupal_set_message(t("File saved: please download via <a href='@url'>@url</a>",
      array("@url" => url("node/" . $export_node->nid) ) ) );
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