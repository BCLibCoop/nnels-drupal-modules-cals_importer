<?php

/**
 * Attempts to find an appropriate genre term to apply
 * First iteration
 * @param $xpathvalue
 * @param $fieldname
 * @param $map
 */
function _cals_importer_find_mapped_genre_test($xpathvalue, $fieldname,
                                                &$map) {
  $anchor = 0;
  foreach ($xpathvalue as $term) {

    $candidate = trim( rtrim( $term, "." ) );

    //Try straight match query
    $straight_match = (new EntityFieldQuery)
      ->entityCondition('entity_type', 'taxonomy_term')
      ->entityCondition('bundle', 'genre_mapped')
      ->propertyCondition('name', $candidate )
      ->execute();

    //Try related match query
    $related_match = (new EntityFieldQuery)
      ->entityCondition('entity_type', 'taxonomy_term')
      ->entityCondition('bundle', 'genre_mapped')
      ->fieldCondition('field_related_terms', 'value', $candidate, 'CONTAINS')
      ->execute();

    if ( $match = cals_importer_term_parse_process($straight_match) ) {
      $map[$fieldname]['values'][] = $match;
      break;
    } elseif ( $match = cals_importer_term_parse_process($related_match) ) {
      $map[$fieldname]['values'][] = $match;
    }
  }
}

/**
 * Attempts to find an appropriate genre term to apply
 * @param $xpathvalue
 * @param $fieldname
 * @param $map
 * @return bool
 */
function cals_importer_find_mapped_genre($xpathvalue, $fieldname, &$map) {

  //Load freshly parsed 650 values
  $candidate_subjects = array();
  foreach ($map["field_subject"]["values"] as $v) {
    $candidate_subjects[] = entity_metadata_wrapper('taxonomy_term', $v);
  }

  foreach ($candidate_subjects as $subject) {
    //Knock out bogus entries early
    if (empty($subject->value())) continue;
    $subject_name = strtolower($subject->value()->name);
    //$candidate_genre = trim(rtrim($term, "."));

    //Find genre term with matching subject
    $subject_match = (new EntityFieldQuery)
      ->entityCondition('entity_type', 'taxonomy_term')
      ->entityCondition('bundle', 'genre_mapped')
      //@todo try LIKE partial matching?
      ->fieldCondition('field_entry_subject', 'value', "%".$subject_name."%", "LIKE")
      //@todo remove
      ->addTag('debug')
      ->execute();

    //Query yields a loadable term
    if (! empty($subject_match['taxonomy_term'])) {

      $loaded_term = entity_metadata_wrapper(
        'taxonomy_term', (key($subject_match['taxonomy_term']))
      );

      //Load conditional terms - Multiple values!
      $must_have_fragment = $loaded_term->field_must_have_subject->value()
        ?: array();
      $avoid_subject_fragment = $loaded_term->field_avoid_subject->value()
        ?: array();

      //check against conditions across all parsed subjects
      $must = empty($must_have_fragment) ? TRUE : in_array($must_have_fragment,
        $candidate_subjects);
      $exclude = in_array($avoid_subject_fragment, $candidate_subjects);

      if ($must && !$exclude) {
        //Apply subject term
        if ($term = cals_importer_term_parse_process($loaded_term)) {
          $map[$fieldname]['values'][] = $term;
          break;
        }
      }

    } else {
      //No match found from entry_subject query
      continue;
    }
  }
}


  /**
   * Interprets messy query result data into boolean
   * @param $result
   * @return bool
   */

function cals_importer_term_parse_process($result) {
  $anchor = 0;
  if (isset($result['taxonomy_term'])) {

    //Take first result
    $key = key($result["taxonomy_term"]);

    //Return the matching tids
    return $result["taxonomy_term"][$key]->tid;

  }
  else {
    return FALSE;
  }
}
