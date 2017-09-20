<?php
/**
 * @file
 * CALS Importer Batch Module.
 */

/**
 * Implements hook_submit()
 */
function cals_importer_form_submit($form, & $form_state) {
  global $user;
  $batch_proc = FALSE;
  $extensions = 'xml bib enw mrc ris txt';
  $validators['file_validate_extensions'] = array();
  $validators['file_validate_extensions'][0] = $extensions;
  $userid = $user->uid;
  $filetype = $form_state['values']['filetype'];
  $filesize = sprintf("%01.1f", $import_file->filesize / 1000);
  $filesize = " ($filesize KB)";
  $session_id = microtime();
  $batch = array(
    'title' => t('Importing') . " ",
    'operations' => array(
      array('cals_import_batch_operations', array($session_id, $user, $userid, $terms))
    ),
    'progressive' => TRUE,
    'finished' => 'cals_import_batch_finished',
    'init_message' => t('Parsing file...'),
    'progress_message' => t('Saving nodes...'),
  );
  batch_set($batch);
  return $batch;
  $base = variable_get('biblio_base', 'biblio');
  batch_process("$base/import");
}


function cals_import_batch_operations($session_id, $filename, $userid, &$context) {
  $limit = 10;
  $ns = "http://www.loc.gov/MARC21/slim";

  if (empty($context['sandbox'])) {

    // Initiate multistep processing.
    $context['results']['session_id'] = $session_id;
    $context['results']['userid']     = $userid;
    $context['sandbox']['progress']   = 0;
    $context['sandbox']['current_id'] = 0;
    $context['results']['nids'] = array();
    $context['filename'] = $filename; // FILE_FOLDER . ;
    $context['xml'] =  _cals_load_records($filename);
    $context['sandbox']['max'] = count($xml->children($ns));


    $context['sandbox']['itters'] = $context['sandbox']['max'] / $limit;
    $context['sandbox']['eta'] = 0;


  }
  // Bail out if the cache is empty.
  if ($context['sandbox']['max'] == 0) {
    return;
  }

  // Process the next 20 nodes.

  for ($i = $context['sandbox']['current_id']; $i < $context['sandbox']['current_id'] + $limit; ++$i) {
    drupal_set_message(t("processing record id: %i", array('%i' => $i));

    /*
    if ($node = unserialize(base64_decode($row->data))) {
      cals_importer_save_node($node, $terms);
      $context['results']['nids'][] = $node->nid;
    }
    */
    $context['sandbox']['progress']++;
    $context['sandbox']['current_id'] = $i;


  }
  // Multistep processing : report progress.
  if ($context['sandbox']['progress'] <= $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }
}
function cals_import_batch_finished($success, $results, $operations) {

  cals_importer_import_finalize($success, $results);
  //clean up import cache...
  db_delete('cals_importer_import_cache')
    ->condition('session_id', $results['session_id'])
    ->execute();
}

function cals_importer_import_finalize($success, $results) {
  global $user;
  $format = $results['format'];
  $nids = $results['nids'];
  $dups = $results['dups'];
  $total = count($nids) + count($dups);
  //    drupal_set_message(t("<i><b>%count</b></i> of <i><b>%total</b></i> nodes were successfully imported.", array('%count' => count($nids), '%total' => $total)), (count($nids) != $total)?'warning':'status');

  if ($success && (count($nids) || count($dups))) {
    $message = t("The file <i><b>@file</b></i> was successfully uploaded.", array('@file' => $results['file']->filename));
    drupal_set_message($message, 'status');
    watchdog($format, $message);
    $count = count($nids);
    $message = format_plural($count, 'One of @total node imported.', '@count of @total nodes imported.', array('@total' => $total));
    drupal_set_message($message, 'status');
    watchdog($format, $message, array('@count' => $count, '@total' => $total), WATCHDOG_INFO);
    if (count($dups)) {
      $count = count($dups);
      $message = format_plural($count, 'One duplicate node skipped.', '@count duplicate nodes skipped.');
      drupal_set_message($message, 'status');
      watchdog($format, $message, array('@count' => $count), WATCHDOG_INFO);
      foreach ($dups as $nid) {
        $message = '';
        $message = t('The item you are trying to import already exists in the database, see');
        $message .= " " . l(t('node/%n', array('%n' => $nid), 'node/' . $nid);

        drupal_set_message($message, 'status');
        watchdog($format, $message, array(), WATCHDOG_ERROR);
      }
    }
  }
  else {
    $count = count($nids);
    $message = t('Import finished with an error!') . ' ' . format_plural($count, 'One node imported.', '@count nodes imported.');
    drupal_set_message($message, 'error');
    watchdog($format, $message, array('@count' => $count), WATCHDOG_ERROR);
  }

  $userid = isset($results['userid']) ? $results['userid'] : $user->uid;

  if (user_access('administer biblio') && count($nids) && $user->uid != $userid) {
    db_update('node')
      ->fields(array('uid' => $results['userid']))
      ->condition('nid', $nids, 'IN')
      ->execute();
    db_update('node_revision')
      ->fields(array('uid' => $results['userid']))
      ->condition('nid', $nids, 'IN')
      ->execute();
  }

}
