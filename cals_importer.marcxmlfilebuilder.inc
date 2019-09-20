<?php
/**
 * @file
 * Generates basic marc xml files
 */

use \Drupal\amazons3\S3Client;
define('ALL_EXPORT_QUEUE_NAME', 'marcxml_generate_all_files');

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

function _cals_importer_generate_marcxml_files_all_records_old(&$entity,
                                                            $context = array() ) {

  if (isset($entity->nid)) {
    $node = node_load($entity->nid);
    //safety check to make sure we're parsing repos items!!
    $content_types = array('repository_item');
    if (!in_array($node->type, $content_types)  ) {
      drupal_set_message(t("Invalid selection: not a repos item"));
      return '';
    }

  }//end main if

  static $xml = '';
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

  $share_options = array(0 => t('All Records'), 1 => t('Partner Share Only'));
  $form['export_mode'] = array(
    '#type' => 'radios',
    '#title' => t('Export mode'),
    '#default_value' => 1,
    '#options' => $share_options,
    '#description' => t("Choose 'All Records' for exports to OCLC.\r\n
    'Partner Share Only' for only permitted records to other bodies (such as ABC)."),
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

  //Cast $partner_share as boolean for easier handling downstream
  $partner_share_mode = (boolean) $form_state["values"]["export_mode"];

  $return['fields'] = array(
    "pnid" => $node->nid,
    "filename" => $filename,
    "mode" => $partner_share_mode,
  );
  return $return; //Note, return value here must be an array.
}


/**
 * Generates marc xml sets with attached marc xml file.
 * See: https://github.com/BCLibCoop/nnels/issues/124
 * @param $entity
 * @param array $context
 * @throws \Exception
 */
function _cals_importer_generate_marcxml_files_search_interface(&$entity, $context = array() ) {

  //Setup
  $host = $entity->hostEntity();
  $host_wrapped = entity_metadata_wrapper('node', $entity->hostEntity());
  $nid = $host->nid;
  $filename = $context['fields']['filename'];
  $pnode = node_load($context['fields']['pnid']); //get updated parent node$host
  $partner_share_mode = $context['fields']['mode'];

  /*if (!empty($host->field_nnels_856_tag) &&
    $host->field_nnels_856_tag[LANGUAGE_NONE][0]['value'] == 1 ) {*/
  $xml_string = _cals_get_marc_xml_string($host_wrapped);

  //$xml_string indicates this repo item has at least been parsed
  if($xml_string !== 0) {

    $stub = _cals_importer_exporter_stub_generator($host_wrapped);

    //The main generator
    $out_xml = _cals_importer_exporter_snippet_generator(
      $entity,
      $host_wrapped,
      $stub,
      $partner_share_mode
      );

    //A record came out of the generator
    if ($out_xml ) {
      //Append raw collection to file temporarily
      file_put_contents($filename, $out_xml, FILE_APPEND |
        LOCK_EX);

      //Save export set node with ref back to marc_xml_string source for this run
      $pnode->field_nid_entry[LANGUAGE_NONE][] = array("nid" => $nid);
      try {
        node_save($pnode);
      } catch (Exception $e) {
        watchdog('cals_importer_exporter_marcxml_files', $e->getMessage(),
          WATCHDOG_ERROR);
      }
    } else { //Record to be generated was missing Partner Sharing flag
      //Set a warning and skip writing temp file, export set write.
      drupal_set_message('Item @title (@nid) - @format, skipped due to Partner Share flag .',
        array(
          "@title" => $host_wrapped->title->value(),
          "@nid" => $nid,
          "@format" => entity_metadata_wrapper('field_collection_item',
            $entity->item_id)->field_file_format->label(),
        ),
        WATCHDOG_NOTICE
      );
    }
  }
  else {
    drupal_set_message('No XML string for @nid, skipped.', array("@nid" =>
      $nid));
  }

  if ($context['progress']['current'] == $context['progress']['total']) {
    $uri = file_create_url($filename);
    //Wrap in marc:collection tag upon retrieval
    $content = MARC_HEADER . "\n" . file_get_contents($filename) .
      "\n</collection>";

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

/**
 * @param $entity
 * @param $host_wrapped
 * @param $stub
 * @param bool $partner_share_mode
 * @return bool|mixed
 */
function _cals_importer_exporter_snippet_generator(
  $entity, $host_wrapped, $stub, $partner_share_mode = FALSE){

  //Build snippet for each context
  $snippet = _cals_importer_exporter_build_file_resource_snippet($entity,
    $partner_share_mode);

  //Back out & return if snippet falls afoul of partner share mode
  if (! reset($snippet)) return FALSE;

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
  ( $merged['controlfields']['ldr']['value'] );
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

  //Single record XML string outside of collection, namespace
  return simplexml_load_string(
    $record
      ->toXML())
    ->children(NAME_SPACE)
    ->asXML();
}

/**
 * @param $host_wrapped
 * @return mixed
 */
function _cals_importer_exporter_stub_generator($host_wrapped){

    //Build stub if it doesn't exist during context
    //check stub field bad previous runs in addition to not initially existing
    if ( empty( unserialize(
        $host_wrapped->field_xml_stub->value() ) )
      || function_exists('dpm') //Debug mode is on, so rebuild fresh each time
      || variable_get("nnels_importer_rebuild_stubs" === TRUE)
    )
      _cals_importer_create_marc_stub($host_wrapped);

    $stub = unserialize($host_wrapped->field_xml_stub->value());

  return $stub;
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

function cals_importer_marcxml_all_exporter_form($form, &$form_state) {
  //Title set
  drupal_set_title("MARCXML All Record Export");

  //Default
  $queue_status = "<p>No All Export jobs in queue.</p>";

  //Get the current queue and count it
  $queue = (DrupalQueue::get(ALL_EXPORT_QUEUE_NAME));
  $number = $queue->numberOfItems();
  if ($number > 0) {

    //Check for existing timestamp
    $check = variable_get('latest_all_export_timestamp');

    if ($check) {
      //Setup for queue status
      $form_state['timestamp'] = $check;
      $owner = variable_get(ALL_EXPORT_QUEUE_NAME . "_notify_user")['name'];

      //Provide queue stats and status

      $queue_link = url('admin/config/system/queue-ui/inspect/' .
        ALL_EXPORT_QUEUE_NAME);
      $queue_status =
        "<p>The latest All Export job was queued at {$check}
         by <strong>{$owner}</strong></p>
        <p><strong>{$number}</strong> items remaining to process.</p>
        <p>Check individual queue items <a href='{$queue_link}'> here.</a></p>
        <p>Server-side cron runs every <strong>15 minutes</strong></p>
        <p>Upon job completion a message will be dispatched to your account email.</p>
        ";
    }

  }

  $form['element'] = array(
    '#markup' => "<div class='queue-status'>{$queue_status}</div>"
  );

  $share_options = array(0 => t('All Records'), 1 => t('Partner Share Only'));
  $form['export_mode'] = array(
    '#type' => 'radios',
    '#title' => t('Export mode'),
    '#default_value' => 1,
    '#options' => $share_options,
    '#description' => t("Choose 'All Records' for exports to OCLC.\r\n
    'Partner Share Only' for only permitted records to other bodies (such as ABC)."),
  );

  // Submit Buttons
  $form['start_all_export'] = array(
    '#name' => 'start',
    '#type' => 'submit',
    '#value' => t("Enqueue All Export job"),
    '#submit' => array('cals_importer_marcxml_all_exporter_form_submit'),
    '#attributes' =>
      array(
        'data-op' => 1
      ),
  );

  $form['cancel_all_export'] = array(
    '#name' => 'cancel',
    '#type' => 'submit',
    '#value' => t("Cancel All Export job"),
    '#submit' => array('cals_importer_marcxml_all_exporter_form_submit'),
    '#attributes' =>
      array(
        'data-op' => 2
    ),
  );

  //Store queue reference for submit callback
  $form_state['queue'] = $queue;

  return $form;
}

function cals_importer_marcxml_all_exporter_form_submit($form, &$form_state){

  global $user;
  variable_set(ALL_EXPORT_QUEUE_NAME . "_notify_user", array
  ('email' => $user->mail, 'name' => $user->name));

  //Cast $partner_share as boolean for easier handling downstream
  $partner_share_mode = (boolean) $form_state["values"]["export_mode"];

  //Start
  if ($form_state["clicked_button"]["#attributes"]["data-op"] == 1) {
    cals_importer_all_exporter_query_queue_setup($partner_share_mode);
    drupal_set_message("Started All Export job at 
    {$form_state['timestamp']}", 'status');
  }
  //Cancel
  if ($form_state["clicked_button"]["#attributes"]["data-op"] == 2) {
    //Delete queue
    $form_state['queue']->deleteQueue();
    //Reset timestamp variable
    variable_del('latest_all_export_timestamp');
  }
}

function cals_importer_cron_queue_info() {
  $queues = array();
  $queues[ALL_EXPORT_QUEUE_NAME] = array(
    'worker callback' => 'cals_importer_all_exporter_process', //function to call for each item
    'time' => 60, //seconds to spend working on the queue
    'skip on cron' => FALSE
  );
  return $queues;
}

function cals_importer_all_exporter_query_queue_setup($partner_share_mode = FALSE){

  $queue = (DrupalQueue::get(ALL_EXPORT_QUEUE_NAME));

  variable_set(
    'latest_all_export_timestamp',
    (new DateTime)->format('Y-m-d-H:i:s')
  );

  $record_sets = cals_importer_all_exporter_get_record_sets();

  foreach ($record_sets['node'] as $record_set) {
    $set_name = $record_set->nid ? str_replace(' ', '_', strtolower(node_load
    ($record_set->nid)->title)) : "missing_nid_set";

    $node_query = new \EntityFieldQuery();
    $node_query->entityCondition('entity_type', 'node')
      ->entityCondition('bundle', 'repository_item')
      ->propertyCondition('status', NODE_PUBLISHED)
      ->fieldCondition('field_record_set', 'nid', $record_set->nid, '=');

    //Get all the record set items
    $found_nodes = $node_query->execute();
    if ($found_nodes) {
        //Check for huge batches and split them up
      if (count($found_nodes['node']) > 199) {
        $chunks = array_chunk($found_nodes['node'], 100, TRUE);
      }
      if (isset($chunks)) {
        foreach ($chunks as $k => $chunk) {
          $queue->createItem(
            array(
              'nodes' => $chunk,
              'record_set' => $set_name . "_{$k}",
              'partner_share' => $partner_share_mode,
            )
          );
        }
      } else { //continue as normal
        $queue->createItem(
          array(
            'nodes' => $found_nodes["node"],
            'record_set' => $set_name,
            'partner_share' => $partner_share_mode,
          )
        );
      }
      unset($found_nodes); //cleanup
    }
  }// end record set loop

  //Get all loose items not apart of a record set
  $singles_query = db_select('node', 'n');

  $singles_query
    ->fields('n', array('nid'))
    ->condition('n.type', 'repository_item', '=')
    ->addJoin('LEFT', 'field_data_field_record_set', 'frs', 'frs.entity_id = n.nid');

    $singles_query->isNull('frs.field_record_set_nid');

    $e = $singles_query->execute();
    $r = $e->fetchAllAssoc('nid');

    $singles_results = $r ? array('node' => $r) : array();
    $singles_chunked = array_chunk($singles_results['node'], 50);
    foreach ($singles_chunked as $n => $result_chunk) {
      $queue->createItem(
        array(
          'nodes' => $result_chunk,
          'record_set' => "loose_records_{$n}",
          'partner_share' => $partner_share_mode,
        )
      );
    }
}

function cals_importer_all_exporter_get_record_sets() {
  $record_set_query = new \EntityFieldQuery();
  $record_set_query->entityCondition('entity_type', 'node')
    ->entityCondition('bundle', 'record_set')
    ->propertyCondition('status', NODE_PUBLISHED);

  return $record_sets = $record_set_query->execute() ?: FALSE;
}

/**
 * @param $items
 */
function cals_importer_all_exporter_process($items) {
  //Prepare S3 Client
  $s3_client = S3Client::factory(
    array(
      //Signature V4 not needed currently
      // 'signature' => 'v4',
      'Key'    => variable_get('amazons3_key'),
      'Secret' => variable_get('amazons3_secret'),
    )
  );

  //Default
  $bucket = variable_get('amazons3_bucket');
  $timestamp = variable_get('latest_all_export_timestamp');
  $s3_object_path = "all-records-exports/{$timestamp}";

  $records = '';

  foreach ($items['nodes'] as $item) {
    $parsed_wrapped = entity_metadata_wrapper('node', $item->nid);
    //Trigger the VBO exporter stub, snippet generators
    if ( _cals_get_marc_xml_string($parsed_wrapped) ) {
      $stub = _cals_importer_exporter_stub_generator($parsed_wrapped);
      foreach ($parsed_wrapped->field_file_resource->getIterator() as $delta
        => $file_resource_wrapper) {
        $record_generated = _cals_importer_exporter_snippet_generator(
          entity_load_single('field_collection_item', $file_resource_wrapper->getIdentifier()),
          $parsed_wrapped,
          $stub,
          $items['partner_share']
        );
        if ($record_generated) $records .= $record_generated;
        else break; //Record to be generated was missing Partner Sharing flag
      }
      unset($parsed_wrapped);
    }
  }

  //$records is populated, write to local file
  $content = simplexml_load_string(MARC_HEADER . "\n" . $records . "\n" . "</collection>")->asXML();

  //Set up filenames and path conventions
  $set_filename = $items["record_set"];
  $public_path = "temporary://{$set_filename}.xml";
  $zip_path = "temporary://{$set_filename}.zip";
  $s3_object_name = $s3_object_path . ".export/{$set_filename}.zip";

  try {
    $write_local = file_put_contents($public_path, $content);

    //Prepare a zip archive and drop in record set XML
    ($zip = new ZipArchive)->open(drupal_realpath($zip_path),
              ZipArchive::CREATE);
    $zip->addFile(drupal_realpath($public_path), "{$set_filename}.xml");
    $check_zip = $zip->close();

    if ( $check_zip && file_exists($zip_path) ){

      try {
        //Put Object in S3 at object timestamp path
        $result = $s3_client->putObject([
          'Bucket' => $bucket,
          'Key' => $s3_object_name,
          'SourceFile' => drupal_realpath($zip_path),
          'ACL'    => 'public-read'
        ]);

        if ($result) {
          //remove temporary files
          file_unmanaged_delete($public_path);
          file_unmanaged_delete($zip_path);
        }

      } catch(Exception $e) {
        drupal_set_message("Failed to upload to S3 with error: {$e->getMessage()}", 'error',
          FALSE);
      }
    }
  }
  catch(Exception $e) {
    drupal_set_message("Failed to write record set file: %e", 'error', array
    ($e->getMessage()));
  }

  // queue_notify
  $queue = (DrupalQueue::get(ALL_EXPORT_QUEUE_NAME));
  //If this is the last item in the queue, notify user of completion.
  if ($queue->numberOfItems() == 1) {
    $notify_path = "{$s3_client->getBaseUrl()}/{$bucket}/{$s3_object_path}/";
    cals_importer_all_exporter_queue_notify($notify_path);
  }
}

function cals_importer_all_exporter_queue_notify($notify_path) {
    $user = variable_get(ALL_EXPORT_QUEUE_NAME . "_notify_user");
    $from = 'NNELS.ca <system@nnels.ca>';
    $message = array(
      'id' => "cals_importer_all_exporter_notify",
      'from' => $from,
      'to' => '$user->mail',
      'subject' => 'NNELS System: All Export Job Finished',
      'body' =>
        "Greetings, {$user->name}:\r\n
        Your requested all-export job has completed.\r\n
        The final payload is deposited {$notify_path} here.
        ",
      'headers' => array(
        'From' => $from,
        'Sender' => $from,
        'Return-Path' => $from,
      ),
    );

    try {
      $system = drupal_mail_system("cals_importer_exporter", "cals_importer_all_exporter_notify");
      $system->mail($message);
      variable_del('latest_all_export_timestamp');
    }
    catch (Exception $e) {
      drupal_set_message($e->getMessage(), 'error');
    }
}

