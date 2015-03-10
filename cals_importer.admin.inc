<?php
/**
 * @file
 * Define admin settings form
 */

/**
 * Implements module settings.
 *
 */
/**
 * Page callback: normalization config form settings
 *
 * @see current_posts_menu()
 */
function cals_importer_config_form($form, &$form_state) {
  $form['cals_importer_stop_words'] = array(
    '#type' => 'textarea',
    '#title' => t('End of String Stopwords (one per line)'),
    '#default_value' => variable_get('cals_importer_stop_words'),
    '#rows' => 10,
    '#resizable' => TRUE,
    '#description' => t('The normalizer will skip over each of these words when they appear at the end of the line.'),
    '#required' => TRUE,   
  );
  $form['cals_importer_prefix_stopwords_narrators'] = array(
    '#type' => 'textarea',
    '#title' => t('String to strip out of first narrator value'),
    '#default_value' => variable_get('cals_importer_prefix_stopwords_narrators'),
    '#rows' => 10,
    '#resizable' => TRUE,
    '#description' => t('The normalizer will strip out the whole string that matches in the first narrator entry. Will not strip subsequent narrator entries.'),
    '#required' => TRUE,   
  );
  return system_settings_form($form);
}
