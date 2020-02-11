<?php

/**
 * Attempts to find an appropriate genre term to apply
 * @param $xpathvalue
 * @param $fieldname
 * @param $map
 * @throws \SearchApiException
 */

function cals_importer_find_mapped_genre($xpathvalue, $fieldname, &$map) {

  //Load freshly parsed 650 values
  $candidate_subjects = array();
  foreach ($map["field_subject"]["values"] as $v) {
    $candidate_subjects[] = entity_metadata_wrapper('taxonomy_term', $v);
  }

  foreach ($candidate_subjects as $subject) {
    //Knock out bogus entries early
    if (empty($subject->value())) {
      continue;
    }
    $subject_name = rtrim(explode("--", strtolower($subject->value()->name))[0]);

    //Attempt Solr query on Subject Stems
    $results = search_api_query('mapped_subject_stems')
      ->condition('name', $subject_name, '=')
      ->execute();

    //Query yields a loadable term
    if (!empty($results['results'])) {

      /** Cannot use EMWs for fields added manually, without hook_field_info
       * https://drupal.stackexchange.com/a/162400
       **/
      $loaded_term = taxonomy_term_load((int) key($results['results']));

      //break early if nothing to apply.
      if ( empty($loaded_term->field_genre_mapped)) break;

      //Load conditional terms - Multiple values!
      $must_have_fragment = $loaded_term->field_must_have_subject
        ?: array();
      $avoid_subject_fragment = $loaded_term->field_avoid_subject
        ?: array();

      //check against conditions across all parsed subjects
      $must = empty($must_have_fragment) ? TRUE : in_array($must_have_fragment,
        $candidate_subjects);
      $exclude = in_array($avoid_subject_fragment, $candidate_subjects);

      if ($must && !$exclude) {
        //Apply Genre term
        $map[$fieldname]['values'][] = $loaded_term->field_genre_mapped["und"][0]["tid"];
      }
      else {
        //No match found from entry_subject query
        continue;
      }
    }
  }
}