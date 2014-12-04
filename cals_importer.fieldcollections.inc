<?php

/**
 * @file
 * populates various field collections associated with MARC XML parsing operations
 */
 
/**
 * Helper function to return the nid for a given system number (MARC)
 * 
 * @param $num
 *   the system number
 */
 

/**
 *
 * @todo
 *   Document this function. Important... 
 *
 */ 
 
function _cals_get_entity_value($key, $arr, $arr_tax) {
  
  $values = array();
  
  foreach($arr as $k => $v) {
  
    switch($key) {
      case "field_date":
        if (!isset($dates) ) {
          $dates[$v['field_qualifier_date']] = 0;
        }
        
        $dates[$v['field_qualifier_date']]++;
        $allow = ($dates[$v['field_qualifier_date']] == 1) ? TRUE : FALSE;
        if ($allow) {
          $values[] = array(
            'field_name' =>  "field_date",
            'field_dc_date' => array(
              LANGUAGE_NONE => array(array('value' => $v['field_dc_date']) ),
            ),
            'field_qualifier_date' => array(
              LANGUAGE_NONE => array(array('value' => $v['field_qualifier_date']) ),
            ),
            
          );
      
        }
      
        break;
    
      case "field_dc_relation":
        $values[] = array(
          'field_name' =>  "field_dc_relation",
          'field_dc_relation_value' => array(
            LANGUAGE_NONE => array(array('value' => $v['value']) ),
          ),
          'field_dc_relation_qualifiers' => array(
            LANGUAGE_NONE => array(array('value' => $v['type']) ),
          ),
          
        );
        
        break;
    
      case "field_dc_subject":
        $vid = $arr_tax['Subject'];
        $name = $v['value'];
        //$name = $name . " A.z.";
      // Get the term's ID.
        $tid = _cals_get_tid_by_name($vid, $name);
      
        $me = array(
          'field_name' => 'field_dc_subject',
        );
        $me['field_dc_subject_value'][LANGUAGE_NONE][]['tid'] = $tid;
        if (isset($v['scheme']) && $v['scheme'] != 'undefined' && $v['scheme'] != '') {
          $me['field_dc_subject_scheme'][LANGUAGE_NONE][]['value'] = strtoupper($v['scheme']);
        }
        $values[] = $me;
        //printPre($values);
        break;
    
      case "description":
            
        $values[] = array(
          'field_name' => 'field_dc_description',
          'field_dc_description_type' => array(
            LANGUAGE_NONE => array(array('value' => $v['type'])),
          ),
          'field_value' => array(
            LANGUAGE_NONE => array(array('value' => $v['value'])),
          ),
          
        );

        break;
      case "field_dc_coverage":
            
        $values[] = array(
          'field_name' => 'field_dc_description',
          'field_dc_coverage_type' => array(
            LANGUAGE_NONE => array(array('value' => $v['field_dc_coverage_type'])),
          ),
          'field_marc_tag' => array(
            LANGUAGE_NONE => array(array('value' => $v['field_marc_tag'])),
          ),
          'field_dc_coverage_scheme' => array(
            LANGUAGE_NONE => array(array('value' => $v['field_dc_coverage_scheme'])),
          ),
          'field_value' => array(
            LANGUAGE_NONE => array(array('value' => $v['field_value'])),
          ),
        );
 
        
        
        
                
        break;
    
      case "field_cataloguing_agency":
      
        if (strlen($v['field_agency_type'])) {
        
          $values[] = array(
            'field_name' => $key,
            'field_value' => array(
              LANGUAGE_NONE => array(array('value' => $v['value'])),
            ),
            'field_agency_type' => array(
              LANGUAGE_NONE => array(array('value' => $v['field_agency_type'])),
            ),
          );
        }
        else {
          $values[] = array(
            'field_name' => $key,
            'field_value' => array(
              LANGUAGE_NONE => array(array('value' => $v['value'])),
            ),
          );
        } 

        break;
    
    }
  
    
  
  
  }


  return $values;


}

/**
 * delete existing field collection items before resaving the node!
 * @ see also: http://drupal.stackexchange.com/questions/68765/how-to-properly-delete-a-field-collection
 */
function _cals_delete_existing_field_collection_items($node, $keys) {
  $ids = array();

  foreach($keys as $field) {
    if (isset($node->{$field}[LANGUAGE_NONE]) ) {
      //collection field values for multiple deletion!
      foreach( $node->{$field}[LANGUAGE_NONE] as $v) {
        $ids[] = $v['value']; 
      }
      unset($node->{$field}[LANGUAGE_NONE]);    
      //printPre($node->{$field});
    
    }
    
  }
  // Delete field collection items.
 
  if (count($ids)) entity_delete_multiple('field_collection_item', $ids);
  return $node;


}
/**
 * Populates Field Collection values
 *
 * @param $node
 *   the node passed to the parser
 * @param $arr_tax
 *   taxonomy values
 * @param $arr
 *   array of field collection values
 *
 * @return
 *   an array. 
 *
 */ 
function _cals_importer_populate_repos_item_fieldcollections( $node, $arr_tax, $arr ) {
  
  foreach($arr as $k => $v) {
    $values = array();
    //run thru a switch so we can isolate fields that still
    //need to be worked on. Later, can remove switch? 
    switch($k) {
    
      case "field_dc_subject":
      case "field_dc_relation":
      case "field_cataloguing_agency":
      case "field_dc_description":
      case "field_dc_coverage":
      case "field_date":
        $values = _cals_get_entity_value($k, $v, $arr_tax);
        //if($k == 'field_dc_subject') printAndDie($values);
        //printPre("ya", $v);
        break;

    
      default:
        printAndDie(__FUNCTION__, $k, $v);
        break;
    
      
    
    }
    
    if (count($values)) {
      foreach ($values as $value) {
        if (is_array($value)) {
          $e = entity_create('field_collection_item', $value);
          // Attach the field_collection entity to the application node.
          $e->setHostEntity('node', $node);
          // Save the entity. 
          $e->save(); 
        }
      }
    }   
      

  }
  
    

}

