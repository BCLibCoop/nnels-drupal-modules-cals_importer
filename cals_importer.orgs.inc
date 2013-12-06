<?php

/**
 * Invokes Drupal's batch api to update org emails, etc. 
 * Borrows code from flacoman91: https://github.com/flacoman91/csvimport:
 * - Provide form for upload of a CSV file.
 * - On submission, trigger a batch task which iterates through each row in the file.
 * 
 */

/**
 * Build a form to upload CSV to.
 */
function cals_org_emails_import_form() {
  $form['#attributes'] = array(
    'enctype' => 'multipart/form-data'
  );
  $form['csvfile'] = array( 
    '#title' => t('CSV File'),
    '#type'  => 'file',
    '#description' => ($max_size = parse_size(ini_get('upload_max_filesize'))) ? t('Due to server restrictions, the <strong>maximum upload file size is !max_size</strong>. Files that exceed this size will be disregarded.', array('!max_size' => format_size($max_size))) : '',    
  ) ;
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update Org Emails'),
  ) ;
  $form['#validate'] = array(
    '_cals_csv_validate_fileupload',
    '_cals_csv_form_validate',
  ) ;
  return $form ;
}

/**
 * Validate the file upload. It must be a CSV, and we must
 * successfully save it to our import directory.
 */
function _cals_csv_validate_fileupload(&$form, &$form_state) {
  $validators = array(
    'file_validate_extensions' => array( 'csv CSV' ),
  ) ;
  if ( $file = file_save_upload('csvfile', $validators, 'temporary://') ) {
    // The file was saved using file_save_upload() and was added to
    // the files table as a temporary file. We'll make a copy and let
    // the garbage collector delete the original upload.
    $csv_dir = 'temporary://csvfile';
    $directory_exists = file_prepare_directory($csv_dir, FILE_CREATE_DIRECTORY);
    if ($directory_exists) {
      $destination = $csv_dir .'/' . $file->filename;
      if (file_copy($file, $destination, FILE_EXISTS_REPLACE)) {
        $form_state['values']['csvupload'] = $destination;
      }
      else {
        form_set_error('csvimport', t('Unable to copy upload file to !dest', array('!dest' => $destination)));
      }      
    }
  }
}

/**
 * Validate the upload. Ensure that the CSV looks something like we
 * expect it to.
 */
function _cals_csv_form_validate(&$form, &$form_state) {
  if ( isset( $form_state['values']['csvupload'] ) ) {
    if ( $handle = fopen($form_state['values']['csvupload'], 'r') ) {
      $line_count = 1 ;
      $first = TRUE ;
      if ( $line = fgetcsv($handle, 4096) ) {
        /**
         * Validate the uploaded CSV here. 
         *
         * The example CSV happens to have cell A1 ($line[0]) as
         * below; we validate it only.
         *
         * You'll probably want to check several headers, eg:
         *   if ( $line[0] == 'Index' || $line[1] != 'Supplier' || $line[2] != 'Title' ) 
         */
        // if ( $line[0] != 'Example CSV for csvimport.module - http://github.com/xurizaemon/csvimport' ) {
        //   form_set_error('csvfile', t('Sorry, this file does not match the expected format.')) ;
        // }
      }
      fclose($handle);
    }
    else {
      form_set_error('csvfile', t('Unable to read uploaded file !filepath', array('!filepath' => $form_state['values']['csvupload'])));
    }
  }
}

/**
 * Handle form submission. Read the CSV into a set of batch operations
 * and fire them off.
 */
function cals_org_emails_import_form_submit(&$form, &$form_state) {
  $batch = array(
    'title' => t('Importing CSV ...'),
    'operations' => array(),
    'init_message' => t('Commencing'),
    'progress_message' => t('Processed @current out of @total.'),
    'error_message' => t('An error occurred during processing'),
    'finished' => '_cals_org_emails_finished',
		'file' => drupal_get_path('module', 'cals_importer') .  '/cals_importer.orgs.inc',
  ) ;
  if ( isset( $form_state['values']['csvupload'] ) ) {
    if ( $handle = fopen($form_state['values']['csvupload'], 'r') ) {
      $batch['operations'][] = array('_cals_org_emails_remember_filename', array( $form_state['values']['csvupload'] ) ) ;
      $line_count = 1 ;
      $first = TRUE ;
      $line = fgetcsv($handle, 4096);
      while ( $line = fgetcsv($handle, 4096) ) {
		  //printAndDie($node);
        /**
         * we use base64_encode to ensure we don't overload the batch
         * processor by stuffing complex objects into it
         */
        $batch['operations'][] = array('_csvimport_import_line', array($line));
      }
      fclose($handle);
    } // we caught this in _cals_csv_form_validate()
  } // we caught this in _cals_csv_form_validate()
	//printAndDie($batch);
  batch_set($batch);
}

/**
 * Process a single line.
 */
function _csvimport_import_line($line, &$context) {
  $context['results']['rows_imported']++;
	if(!empty($line[1])) {
		$illcode = $line[0];
		$nid = db_query(
			"select entity_id as nid
				FROM {field_data_field_library_code} 
				where field_library_code_value = :code ",
				array(":code" => $line[0] ))->fetchField();
	  if($nid > 0) {
			$node = node_load($nid);
			$node->field_email[LANGUAGE_NONE] = array();
			$emails = explode(" ", $line[1]);
			//some of the csv files have multiple email addresses.
			foreach($emails as $mail) {
			  $node->field_email[LANGUAGE_NONE][] = array('email' => $mail);
				drupal_set_message("updated ill-code = $illcode | email = $mail : {$node->title} " . l("view", "node/" . 
					$nid) ." | "  . l("edit", "node/" . $nid . "/edit") );
			}
			//resave the node
			node_save($node);
	  }
	}
  /**
   * Simply show the import row count.
   */
  $context['message'] = t('updating row !c', array( '!c' => $context['results']['rows_imported'] ));
  //$context['message'] = t('Importing %title', array('%title' => $line[2]));

  /**
   * In order to slow importing and debug better, we can uncomment
   * this line to make each import slightly slower.
   */
  usleep(2500);

  /**
   * If the first two columns in the row are "ROW", "FAILS" then we
   * will add that row to the CSV we'll return to the importing person
   * after the import completes.
   */
  if ( $line[1] == 'ROW' && $line[2] == 'FAILS' ) {
    $context['results']['failed_rows'][] = $line ;
  }
}

 

/**
 * Handle batch completion.
 */
function _cals_org_emails_finished($success, $results, $operations) {
  if ( !empty($results['failed_rows']) ) {
    $dir = 'public://csvimport' ;
    if (file_prepare_directory( $dir, FILE_CREATE_DIRECTORY ) ) {
      $csv_filename = 'failed_rows-'. basename($results['uploaded_filename']); // we validated extension on upload
      $csv_filepath = $dir .'/'. $csv_filename;
      $targs = array(
        '!csv_url' => l(check_plain($csv_filename), file_create_url($csv_filepath)),
        '%csv_filename' => $csv_filename,
        '%csv_filepath' => $csv_filepath,
      ) ;
      if ( $handle = fopen($csv_filepath, 'w+') ) {
        foreach( $results['failed_rows'] as $failed_row ) {
          fputcsv($handle, $failed_row);
        }
        fclose($handle);
        drupal_set_message(t('Some rows failed to import. You may download a CSV of these rows: !csv_url', $targs), 'error');
      }
      else {
        drupal_set_message(t('Some rows failed to import, but unable to write error CSV to %csv_filepath', $targs), 'error');
      }
    }
    else {
      drupal_set_message(t('Some rows failed to import, but unable to create directory for error CSV at %csv_directory', $targs), 'error');
    }
  }
  return t('The CSV import has completed.');
}

/**
 * Remember the uploaded CSV filename
 *
 * @TODO is there a better way to pass a value from inception of the
 * batch to the finished function?
 */
function _cals_org_emails_remember_filename($filename, &$context) {
  $context['results']['uploaded_filename'] = $filename ;
}