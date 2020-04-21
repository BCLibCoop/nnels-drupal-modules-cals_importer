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

  //Prepare a simple array of values for later intersection
  $simple_array_subjects = array();
  foreach ($candidate_subjects as $candidate){
    $simple_array_subjects[] = $candidate->value()->name;
  }

  foreach ($candidate_subjects as $subject) {
    //Knock out bogus entries early
    if (empty($subject->value())) {
      continue;
    }

    //split off geographic subdivisions and date starts
    $subject_processed = preg_split( "/(--|,\s\d\d+)/",rtrim($subject->value()
      ->name));

    $subject_name = (string) $subject_processed[0];

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
      if ( empty($loaded_term->field_genre_harmonized)) break;

      //Load conditional terms - Multiple values!
      $must_have_fragments = $loaded_term->field_must_have_subject
        ?: array();
      $avoid_fragments = $loaded_term->field_avoid_subject
        ?: array();

      $must_haves = array();
      foreach($must_have_fragments['und'] as $m) {
        $must_haves[] = $m['safe_value'];
      }

      $to_avoid = array();
      foreach($avoid_fragments['und'] as $a) {
        $to_avoid[] = $a['safe_value'];
      }

      //use array_intersect iterate over $must_haves, set as true default,
      // flip if any intersect call fails
      $must = TRUE;
      if (! empty($must_haves)) {
        foreach ($must_haves as $mh) {
          $must = (bool) in_array($mh, $simple_array_subjects);
        }
      }
      //True if present: don't pass-through. No match: proceed.
      $exclude = (bool) array_intersect($to_avoid, $simple_array_subjects);

      if ($must && !$exclude) {
        //Apply Genre term
        $map[$fieldname]['values'][] =
          $loaded_term->field_genre_harmonized["und"][0]["tid"];
      }
      else {
        //No match found from entry_subject query
        continue;
      }
    }
  } //end foreach
}