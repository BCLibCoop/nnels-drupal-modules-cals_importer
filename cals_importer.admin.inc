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

  $form['cals_importer_non_filing_characters'] = array(
    '#type' => 'textarea',
    '#title' => t('Non Filing Characters'),
    '#default_value' => variable_get('cals_importer_non_filing_characters'),
    '#rows' => 10,
    '#resizable' => TRUE,
    '#description' => t('Strings that are counted in calculating 245 indicators. Please add one per line. Use \s to indicate trailing blank space.'),
    '#required' => TRUE,
  );
  $form['cals_importer_stop_words'] = array(
    '#type' => 'textarea',
    '#title' => t('End of String Stopwords (one per line)'),
    '#default_value' => variable_get('cals_importer_stop_words'),
    '#rows' => 5,
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
  $form['cals_importer_url_patterns'] = array(
    '#type' => 'textarea',
    '#title' => t('Map 856 url patterns to specific contributors'),
    '#default_value' => variable_get('cals_importer_url_patterns'),
    '#rows' => 5,
    '#resizable' => TRUE,
    '#description' => t('Add entries as [contributor / label]|[url-pattern], one per line.
      This will populate the URL label based on string matches (lowercase).
      The order is relevant as the parser will exit once its found a match.'),
    '#required' => TRUE,
  );
  $form['cals_importer_url_patterns_ignore'] = array(
    '#type' => 'textarea',
    '#title' => t('URL String patterns to ignore'),
    '#default_value' => variable_get('cals_importer_url_patterns_ignore'),
    '#rows' => 5,
    '#resizable' => TRUE,
    '#description' => t('These are string patterns that will be skipped when parsing 856 tags.'),
    '#required' => TRUE,
  );

  return system_settings_form($form);
}
