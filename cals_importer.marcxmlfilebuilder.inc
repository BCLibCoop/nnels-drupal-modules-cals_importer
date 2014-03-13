<?php


/**
 * Invokes Drupal's batch api to generate new marc xml files with 856 tags, etc.
 * 
 */
 
function _cals_importer_generate_marcxml_files(&$entity, $context = array() ) {

	$nid = $context['node']->nid;
	$node = node_load($nid);
	//static variable: we want to populate the parent recordset
	//static $parent_nid = $node->field_record_set[LANGUAGE_NONE][0]['nid'];
	static $xml = '';
	//static 
	
	$parent_nid = $node->field_record_set[LANGUAGE_NONE][0]['nid'];

	if(	!empty($parent_nid) &&
			!empty($node->field_xml_string) &&
			!empty($node->field_nnels_856_tag) &&
			$node->field_nnels_856_tag[LANGUAGE_NONE][0]['value'] == 1 ) {

		$xml .=  $node->field_xml_string[LANGUAGE_NONE][0]['value'];

	}

	if ($context['progress']['current'] == $context['progress']['total']) {
		$filename = 'nnels_marcxml_' . $parent_nid . '.xml';
		$xml = '<marc:collection 
					xmlns:marc="http://www.loc.gov/MARC21/slim" 
					xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
					xsi:schemaLocation="http://www.loc.gov/MARC21/slim http://www.loc.gov/standards/marcxml/schema/MARC21slim.xsd"
				>' . $xml . '</marc:collection>';			

		$xml = simplexml_load_string($xml);
		$file = file_save_data($xml->asXml(),'public://' . $filename);
		$file->display = 1;
		$file->description = '';
		
		$parent = node_load($parent_nid);
		$parent->field_marc_xml_output_file[LANGUAGE_NONE][0] = (array)$file;
		node_save($parent);
	}



}

/**
 * Simple wrapper to call main parser; if preferred, can invoke batch api instead. 
 */

function _cals_importer_generate_marcxml_file($node) {

	$xml = _cals_importer_build_marcxml($node);
	if(strlen($xml)) {
	  $fileBuilder = new MarcXmlFileBuilder($node, $xml);
		$fileBuilder->createFile();
	}
	//printAndDie("here", $xml);	
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
	if(!empty($parent_nid) &&
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
		$dom->preserveWhiteSpace = false;
		$dom->formatOutput = true;
		//$dom->loadXML($cmr->asXML());
		
		$xml_string = 
			'<marc:collection 
					xmlns:marc="http://www.loc.gov/MARC21/slim" 
					xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
					xsi:schemaLocation="http://www.loc.gov/MARC21/slim http://www.loc.gov/standards/marcxml/schema/MARC21slim.xsd"
				>' . $this->xml . '</marc:collection>';
		$xml = simplexml_load_string($xml_string);
		

		$file = file_save_data($xml->asXml(),'public://' . $filename);
		$file->display = 1;
		$file->description = '';
		//printAndDie($file);
		
		//$file = file_copy($file, 'public://');
	
		//$xml->asXml($filename);
		$parent->field_marc_xml_output_file[LANGUAGE_NONE][0] = (array)$file;
		node_save($parent);  
		//printAndDie($file, $parent);
 
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