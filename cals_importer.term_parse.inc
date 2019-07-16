<?php

/**
 * @param $xpathvalue
 * @param $fieldname
 * @param $map
 */
function _cals_importer_term_parse_find_term($xpathvalue, $fieldname, &$map) {

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
 * @param $result
 * @return bool
 */

function cals_importer_term_parse_process($result) {

  if ( isset($result['taxonomy_term']) ) {

    //Take first result
    $key = key($result["taxonomy_term"]);

    //Return the matching tids
    return $result["taxonomy_term"][$key]->tid;

  } else {
    return FALSE;
  }
}