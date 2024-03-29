<?php

/**
 * Attempts to find an appropriate genre term to apply
 * @param $supplement_subjects array
 * @param $field_name
 * @param $map
 * @throws \SearchApiException
 */

function cals_importer_find_mapped_genre($supplement_subjects, $field_name, &$map) {
  $supplement_subjects = _cals_importer_map_harmonized_genres_exact_matches($supplement_subjects, $field_name, $map);

  $index = 'mapped_subject_stems'; //default

  //Too ambiguous on their own for full-text consideration
  $terms_too_generic = array(
    'Fiction',
    'FICTION',
    'FICTION ',
    'Non-fiction',
    'Nonfiction',
    'NON-FICTION',
    'NONFICTION',
    'NONFICTION ',
    'Stories',
    'Science',
    'Language',
    'Studies'
  );

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

  //Consider supplemental subjects from unparsed 655, 946 (BISAC)
  if (! empty($supplement_subjects)) {
    $simple_array_subjects = array_merge($supplement_subjects, $simple_array_subjects);
  }

  foreach ($simple_array_subjects as &$subject) {

    //Knock out ambiguous entries from loop early
    if (empty($subject) || in_array($subject, $terms_too_generic)) {
      continue;
    }

    //split off geographic subdivisions and date starts
    $subject_processed = preg_split( "/(--|,\s\d\d+)/",rtrim($subject));
    $subject_name = (string) $subject_processed[0];
    drupal_set_message("For candidate term: {$subject_name}");

    //Attempt Solr query on Subject Stems
    $query = search_api_query($index);
      /**
       * Previously used straight string search:
       * condition('name', $subject_name, '=');
       * To search over all fulltext fields: $query->keys('keywords')
       * To search only the aggregation field
       * ->condition('search_api_aggregation_1', $subject_name, '=');
       * N.B. if field is fulltext as above, default '=' operator is
       * interpreted as 'contains'.
       **/

    //determine BISAC status
    if ( determine_bisac_term($subject_name) ) {
      $bisac_mode = TRUE;
      //Ensures we only use BISAC flagged Subject Stems when mapping
      $query
        ->condition('field_bisac_flag', 1, '=')
        ->keys($subject_name);
    } else {
      $query
        ->condition('field_bisac_flag', 0, '=')
        ->keys($subject_name);
    }

    $results = $query->execute();

    //Check for results and handle retries
    if (empty($results['results'])) {
      if (isset($bisac_mode)) {
        //Trim off last most specific component
        $exploded = explode( " /", $subject_name );
        if ( count($exploded) > 1 ) {
          array_pop($exploded);

          //Reassemble
          $bisac_retry_subj = implode(" / ",
            $exploded);

          //Reload/retry if we got a new term not already queued and
          // not one of our generics without valid BISAC root
          if (! in_array($bisac_retry_subj, $simple_array_subjects) &&
            ! in_array($bisac_retry_subj, $terms_too_generic) ) {

            $simple_array_subjects[] = $bisac_retry_subj;
            drupal_set_message("Derived root {$bisac_retry_subj} added to queue for retry.");
            drupal_set_message("======\n");
          } else {
              drupal_set_message("No match and root ({$bisac_retry_subj}) is generic or was already queued.");
              drupal_set_message("======\n");
          }
        }
      } else {
        drupal_set_message("No result found.");
        drupal_set_message("======\n");
        continue; //@todo remove?
      }
    }
    else {  //Straight match on string map and harmonize
      $mapped_term_id = map_harmonized_terms($results, $simple_array_subjects);
      //avoid duplicates
      if ( ! in_array( $mapped_term_id, $map[$field_name]['values'] ) )
        $map[$field_name]['values'][] = $mapped_term_id;
    }
  } //end foreach
}

/**
 * @param $results array
 * @param $simple_array_subjects array
 * @return mixed bool|int
 */

function map_harmonized_terms($results, $simple_array_subjects) {

  /** Cannot use EMWs for fields added manually, without hook_field_info
   * https://drupal.stackexchange.com/a/162400
   **/
  $loaded_term = taxonomy_term_load((int) key($results['results']));
  drupal_set_message("Considering fragment from result: {$loaded_term->name}");

  //break early if nothing to apply.
  if ( empty($loaded_term->field_genre_harmonized)) return NULL;

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
  $exclude = FALSE;

  foreach ($must_haves as $mh) {
    //In multi-value runs, presence of the last MustHave can flip the whole
    foreach ($simple_array_subjects as $sma) {
      $components = explode(' ', $sma);
      if (!empty($components)) {
        $must = (bool) in_array($mh, $components);
        unset($components);
        if ($must) break; //return to must_haves loop
      }
    }
  }

  //True if present: don't pass-through. No match: proceed.
  foreach ($to_avoid as $av) {
    foreach ($simple_array_subjects as $sma) {
      $components = explode(' ', $sma);
      if (!empty($components)) {
        $exclude = (bool) array_intersect($av, $simple_array_subjects);
        unset($components);
      }
    }
  }

  if ($must && !$exclude) {
    //Return Genre term
    //@todo remove
    $term_name = taxonomy_term_load((int)
    $loaded_term->field_genre_harmonized["und"][0]["tid"])->name;
    drupal_set_message(
    "Applied harmonized genre: {$term_name} ");
    drupal_set_message("======\n");
    return $loaded_term->field_genre_harmonized["und"][0]["tid"];
  } else {
    drupal_set_message("Nothing applicable.");
    drupal_set_message("======\n");
    return NULL;
  }
}

/**
 * @param $subject_term
 * @return bool
 */
function determine_bisac_term($subject_term) {

  //check if BISAC tid cache has been set recently
  if ($available = cache_get('bisac_subject_tids')) {
    $current_BISAC_tagged = $available->data;
    if ( in_array($subject_term, $current_BISAC_tagged) ) return TRUE;
  } else {

    // BISAC regex pattern, could be improved
    // e.g. YOUNG ADULT FICTION / Magical Realism
    // YOUNG ADULT FICTION / Science Fiction / Dystopias
    $BISAC_pattern = "/^[A-Z\,\-]{2,}\s?.*[\/\s]?/";
    if ( (bool) preg_match($BISAC_pattern, $subject_term) ) {
      return TRUE;
    }
  }

  return FALSE; //default
}

/**
 * Map harmonized genres using an exact match (case insensitive).
 *
 * @param array $supplement_subjects
 *   The list of 655/946 values.
 * @param string $field_name
 *   The name of the field to modify (field_genre_harmonized always).
 * @param array $map
 *   The field map with values.
 *
 * @return array
 *   The modified supplement subjects.
 */
function _cals_importer_map_harmonized_genres_exact_matches($supplement_subjects, $field_name, &$map) {
  foreach ($supplement_subjects as $index => $subject) {
    drupal_set_message("For candidate term (exact match check): {$subject}");
    $terms = taxonomy_get_term_by_name($subject, 'genre_harmonized');
    if (count($terms)) {
      $term_id = array_keys($terms)[0];
      $map[$field_name]['values'][] = $term_id;
      drupal_set_message("Applied harmonized genre: {$terms[$term_id]->name} ");

      // Don't consider using any other methods if exact match.
      unset($supplement_subjects[$index]);
    }
    else {
      drupal_set_message("No result found.");
    }
    drupal_set_message("======\n");
  }

  return $supplement_subjects;
}