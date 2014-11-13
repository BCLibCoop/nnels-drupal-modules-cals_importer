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
    
    $xml_test = _cals_get_marc_xml_string($node);
    if($xml_test !== 0) {
      $str =  $node->field_xml_string[LANGUAGE_NONE][0]['value'];
      file_put_contents($filename, $str, FILE_APPEND | LOCK_EX);
    } 
  }
  else {
    dpm(l($node->title, "node/" . $nid . "/edit"));
  }

  if ($context['progress']['current'] == $context['progress']['total']) {
    /*
    $xml =  .   $context['xml-string'] . '</marc:collection>';     

    $xml_string = simplexml_load_string($xml);
    
    */
    $parent_nid = $node->field_record_set[LANGUAGE_NONE][0]['nid'];
    $parent = node_load($parent_nid);
    $header = '<marc:collection 
            xmlns:marc="http://www.loc.gov/MARC21/slim" 
            xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
            xsi:schemaLocation="http://www.loc.gov/MARC21/slim http://www.loc.gov/standards/marcxml/schema/MARC21slim.xsd"
          >';
    //add closing marc:collection tag
    $content = $header . file_get_contents($filename) . '</marc:collection>';
    $xml = simplexml_load_string($content);
    $file = file_save_data($xml->asXml(), $filename, FILE_EXISTS_REPLACE);
    $file->display = 1;
    $file->description = '';
 

    node_save($parent);
  }



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
      //dpm($xml_test);
      if($xml_test !== 0) {
        $str =  $node->field_xml_string[LANGUAGE_NONE][0]['value'];
        //dpm($context);
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
  
  return;

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
    // We save the file to the root of the files directory.
  
    
    //save node
  
  }


}