<?php

class MarcXmlFileBuilder {

  public function __construct($node, $xml) {
  
    $this->node = $node;
    $this->xml = $xml;
  
  }
  
  public function createFile() {
  
		$filename = 'nnels_marcxml_' . $this->node->nid . '.xml';
		$dom = new DOMDocument('1.0');
		$dom->preserveWhiteSpace = false;
		$dom->formatOutput = true;
		//$dom->loadXML($cmr->asXML());
		$dom->loadXML($this->xml);
		$dom->save($filename);
		$filepath = drupal_realpath($filename);
			
	  // Create managed File object and associate with Image field.
	  $file = (object) array(
	    'uid' => 1,
	    'uri' => $filepath,
	    'filemime' => file_get_mimetype($filepath),
	    'display' => 1, 
	    'description' => '',
	    'status' => 1,
	  );
	  
	  try {
	  	$file = file_copy($file, 'public://');
	 	} catch (Exception $e) {
			printAndDie($e->getMessage());
			drupal_set_message($e->getMessage());
	    watchdog('my_error', $e->getMessage());
	  }
	  // We save the file to the root of the files directory.
	  $node->field_marcxml_export_file[LANGUAGE_NONE][0] = (array)$file;
	  
	  //save node
		node_save($node);  
  
  }


}