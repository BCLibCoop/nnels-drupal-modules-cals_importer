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
  //dpm($form_state['values']);
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
  $pnode = node_load($context['fields']['pnid']); //get updated parent node$host
  /*if (!empty($host->field_nnels_856_tag) &&
    $host->field_nnels_856_tag[LANGUAGE_NONE][0]['value'] == 1 ) {*/
    $xml_string = _cals_get_marc_xml_string($host);

    //$xml_string indicates this repo item has at least been parsed
    if($xml_string !== 0) {

      //Build stub if it doesn't exist during context
      //check stub field bad previous runs in addition to not initially existing
      if ( empty( unserialize(
        $host->field_xml_stub[LANGUAGE_NONE][0]['value'] ) )
      || function_exists('dpm') //Debug mode is on, so rebuild fresh each time
      )
        _cals_importer_create_marc_stub($host);
      $host_wrapped = entity_metadata_wrapper('node', $host);
      $stub = unserialize($host_wrapped->field_xml_stub->value());

      //Always build snippet for each context
      $snippet = _cals_importer_exporter_build_file_resource_snippet
      ($entity);

      $merged = array();

      foreach ($stub as $field_key => $data) {
        //order here is important, we want leader from snippet to prevail.
        //In all others there should be no collisions between control,
        //datafields in snippet vs stub
        $merged[$field_key] =  $snippet[$field_key] + $stub[$field_key];
        ksort($merged[$field_key]);
      }

      //File_MARC_Record?
      $record = new File_MARC_Record(NULL);
      _cals_importer_exporter_build_new_record($record, $merged);

      //Build new leader
      $fixed_leader = _cals_importer_exporter_build_final_leader
      ( str_split($merged['controlfields']['ldr']['value']) );
      $record->setLeader($fixed_leader['value']);

      //Dump raw record
      $raw = $record->toRaw();

      //Calculate the final variable byte regions of LDR
      $record_length = _cals_importer_exporter_calc_ldr_length($raw);
      $base_address = _cals_importer_exporter_calc_dir_length($raw);

      //Set full leader
      $record->setLeaderLengths($record_length, $base_address);

      //Lint
      $linter = new File_MARC_Lint();
      try {
        $check_result = $linter->checkRecord($record);
        if( count( $check_result ) ) {
          $wrapped = entity_metadata_wrapper('field_collection_item', $entity);
          $url = url(drupal_get_path_alias('node/' . $wrapped->getIdentifier()));
          $formatted_message = sprintf(
            "Linter found a problem in exported record for the following item: %s - (%s). Check XML components at %s",
            $host_wrapped->title->value(),
            $wrapped->field_file_format->label(),
            $url
          );
          watchdog('
            cals_importer_exporter_FileMARC_Linter',
            $formatted_message,
            array(),
            WATCHDOG_WARNING
          );
        }
      }
      catch (Exception $e) {
        watchdog(
            'cals_importer_exporter_FileMARC_Linter',
            $e->getMessage(),
            WATCHDOG_ERROR
        );
      }

      //@todo now merge and persist in file
      //Single record XML string outside of collection, namespace
      $out_xml = simplexml_load_string(
        $record
          ->toXML())
        ->children(NAME_SPACE)
        ->asXML();

      //Append raw collection to file temporarily
      file_put_contents($filename, $out_xml, FILE_APPEND |
        LOCK_EX);

    //@todo determine if non-parsed items can appear in this view to begin with

    //Save export set node with ref back to marc_xml_string source for this run
    $pnode->field_nid_entry[LANGUAGE_NONE][] = array("nid" => $nid);
    node_save($pnode);
  }

  if ($context['progress']['current'] == $context['progress']['total']) {
    $uri = file_create_url($filename);
    //Wrap in marc:collection tag upon retrieval
    $content = MARC_HEADER . "\n" . file_get_contents($filename) .
      "</marc:collection>";

    $file = file_save_data( simplexml_load_string($content)->asXml(),
      $filename,
      FILE_EXISTS_REPLACE);
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