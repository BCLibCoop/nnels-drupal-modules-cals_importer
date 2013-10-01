<?php

/**
 * Not currently in use, might replace the get_value_from_tag model (see below)
 *
 *
 * @return
 *   an array of MARC XML -> Drupal mappings
 *
 */

function _cals_get_marc_mappings() {
	$arr = array();

	$arr[260] = array(
		array("c", "date", "issued", "_cals_get_iso_date",),
	);
}

function _cals_get_date_issued($xml) {
	
	//let's grab the date issued

	$tmp = $xml->xpath("//marc:datafield[@tag='008']");
	if(count($tmp) == 1) {
		$str = (string) $tmp[0];
		return _cals_get_iso_year($str);
	}

	$tmp = $xml->xpath("//marc:datafield[@tag='264']/marc:subfield[@code='c']");
	if(count($tmp) >= 1) {
		$str = (string) $tmp[0];
		return _cals_get_iso_year($str);
	}

	$tmp = $xml->xpath("//marc:datafield[@tag='260']/marc:subfield[@code='c']");
	if(count($tmp) == 1) {
		$str = (string) $tmp[0];
		return _cals_get_iso_year($str);
	}
 
 
	return $date;


}

function _cals_get_date_from_008($str) {

	return _cals_get_iso_date(substr($str, 7, 4));
	
}

function _cals_get_genre_from_marc(&$arr, $xml) {

	//<marc:datafield tag="655" ind1=" " ind2=" ">

	$tmp = $xml->xpath("//marc:datafield[@tag='655'][@ind1=' ']/marc:subfield[@code='a']|//marc:datafield[@tag='655'][@ind2=' ']/marc:subfield[@code='a']");

	if(count($tmp) >= 1) {
		foreach($tmp as $me) {
			$value = (string) $me[0];
			if(!empty($value) ) {
				//if not set, add these two arrays
				if(!isset($arr['repos']['field_keyword_index'])) {
					$arr['repos']['field_keyword_index'] = array(); 
					$arr['fc']['field_dc_subject'] = array();
				} 
			
				$arr['repos_tf']['field_genre'][] = array("scheme" => "lcsh", "value" => $value);
				$arr['repos']['field_keyword_index'][] = _cals_get_truncated_value($value, 250);
			}
		
		}
 
	}


}
function _cals_get_subject_from_marc(&$arr, $xml) {

	//600
	$paths = array();
	$paths[] = 	"//marc:datafield[@tag='600']/marc:subfield[@code='a']";

	//the codes we need to look for on 650:
	$codes = array("a", "x", "v", "y", "z");
	foreach($codes as $code)  {
		$paths[] = "//marc:datafield[@tag='650']/marc:subfield[@code='" . $code ."']";
	}
	
	$path = implode("|", $paths);
	
	$tmp = $xml->xpath($path);
	
	foreach($tmp as $me) {
		$str = (string) $me[0];
		$str = _cals_get_truncated_value($str, 250);
		$ind2 = "";
		$atts = $me->attributes();
		foreach($atts as $k => $v) {
			if($k == 'ind2') $ind2 = (string) $v;
		}
		if(!isset($arr['repos']['field_keyword_index'])) {
			$arr['repos']['field_keyword_index'] = array(); 
			$arr['fc']['field_dc_subject'] = array();

		} 
		if(!in_array($str, $arr['repos']['field_keyword_index'] )) {
			$arr['repos']['field_keyword_index'][] = $str; 
			$arr['fc']['field_dc_subject'][] =_cals_get_subject_value($str, $ind2);

		}
	}

}

function _cals_old_subjects() {
 

	switch($nah) {

			/**
			* SUBJECTS: 
			*   600 10 $a	
			*   610 20 $a	
			*   630 00 $alx	
			*   650 #0 $azvxy: indicator 0 = lcsh; 6 = Répertoire de vedettes-matièr
			*/					
			//subjects
			//to do: find a way to compress this code and 650... v. redundant!
			case 600:
				if($code == 'a') {
					$str = _cals_get_truncated_value($str, 250);
					if(!isset($arr['fc']['field_dc_subject'])) $arr['fc']['field_dc_subject'] = array();
					if(!isset($arr['repos']['field_keyword_index'])) $arr['repos']['field_keyword_index'] = array();
					$entries = $str; //explode(".", $str);
					
					if(!in_array($str, $arr['repos']['field_keyword_index'] )) {
						$arr['repos']['field_keyword_index'][] = $str; 
						$arr['fc']['field_dc_subject'][] =_cals_get_subject_value($str, $ind2);
					}


					
				}	
		
				break;
			
			case 650:
				/**
				 *<marc:datafield tag="650" ind1=" " ind2="0">
				 * <marc:subfield code="a">Ukrainian Canadians</marc:subfield>
				 * <marc:subfield code="v">Fiction.</marc:subfield>
				 * </marc:datafield>
 				*/
									
					$codes = array("a", "x", "v", "y", "z");
					if (in_array($code, $codes)) { 
						$str = _cals_get_truncated_value($str, 250);
						if(!isset($arr['fc']['field_dc_subject'])) $arr['fc']['field_dc_subject'] = array();
						if(!isset($arr['repos']['field_keyword_index'])) $arr['repos']['field_keyword_index'] = array();
						//let's split multiple subject entries that are joined by a period.
						$entries = $str; // explode(".", $str);
						//$arr['repos']['field_keyword_index'] = _cals_get_keyword_index_values($arr['repos']['field_keyword_index'], $entries);
						if(!in_array($str, $arr['repos']['field_keyword_index'] )) {
						
							$arr['repos']['field_keyword_index'][] = $str; 
							$arr['fc']['field_dc_subject'][] =_cals_get_subject_value($str, $ind2);

						}

						
						/*
						foreach($entries as $entry) {
							$entry = _cals_get_truncated_value($str, 250);
							if(strlen(trim($entry)) && (!in_array($entry, $arr['repos']['field_keyword_index'])))) {
								
								$arr['repos']['field_keyword_index'][] = trim($entry);
							}
						}
						*/
					}	
				break;
 
	
	}
	


}

function _cals_get_descriptors_from_marc(&$arr, $xml) {
  
  $fields = array(500, 504, 505, 511, 516, 533, 538, 586, 595, 598, 599, 541,);
	foreach($fields as $field) {
	  $tmp = $xml->xpath("//marc:datafield[@tag='" . $field . "']/marc:subfield");
		if(count($tmp) >= 1) {
			foreach($tmp as $me) {
			
				$val = (string) trim($me[0]);
	
				$arr['fc']['field_dc_description'][] = array(
					"type" => _cals_get_description_type($field), 
					"value" => $val,
					"num_char" => strlen($val),
				);
			
			}
		
		}
	
	}   
	

}
function _cals_get_description_type($tag) {

	$tmp =	array( 
		505 => "tableofcontents",
		520 => 'abstract',
	);
	
		/*
		504 => "Bibliography",
		511 => 'Performer',
		516 => 'Type of file',
		
		533 => 'Reproduction',
		538 => 'System Details / Requirements',
		586 => 'Awards Notes', 
		595 => 'Extent of Original', 
		598 => 'Keywords, Local',
		599 => 'Local Notes',
		541 => 'Accrual Method',
		*/
	
	if(array_key_exists($tag, $tmp)) return $tmp[$tag];
	return "";




}

function _cals_get_commom_fields_from_marc(&$arr, $xml) {


	$fields = array(
	
		"field_dc_creator" => array("key" => "repos", "datafield" => 100, "subfield" => "a"),
		"field_dc_contributor" => array("key" => "repos", "datafield" => 700, "subfield" => "a"),
			//uniform titles: 130 0# $a, 730  0# $a	
		"field_title_alternative" => array("key" => "repos", "datafield" => 130, "subfield" => "a"),
		"field_title_alternative" => array("key" => "repos", "datafield" => 730, "subfield" => "a"),
	
			/**
			* alt titles: 
			*   240 ind 10 $a	
			*   246 ind 13 $a
			*   210?
			*   242?
			*/
		"field_title_alternative" => array("key" => "repos", "datafield" => 240, "subfield" => "a"),
		"field_title_alternative" => array("key" => "repos", "datafield" => 246, "subfield" => "a"),
				


		"field_uri" => array("key" => "repos", "datafield" => 856, "subfield" => "u"),
		"field_isbn" => array("key" => "repos", "datafield" => 534, "subfield" => "z"),


 
		//identifiers
		
		"publisher_number" => array("key" => "identifier", "datafield" => "028", "subfield" => "a"),
		"producer" => array("key" => "identifier", "datafield" => "028", "subfield" => "a"),


	
			/**
			* RELATIONS: 
			*   800 $a	= IsPartOf
			*   830 $a	= IsPartOf
			*   530 $a	= IsFormatOf
			*   800 $a	= IsPartOf
			*   800 $a	= IsPartOf
			*
			*/	

		"field_dc_relation" => array("key" => "fc", "datafield" => 800, "subfield" => "a", 
			'qualifier' => "type", 'enum' => "IsPartOf"),
		"field_dc_relation" => array("key" => "fc", "datafield" => 830, "subfield" => "a", 
			'qualifier' => "type", 'enum' => "IsPartOf"),
				
		"field_dc_relation" => array("key" => "fc", "datafield" => 760, "subfield" => "a", 
			'qualifier' => "type", 'enum' => "IsPartOf"),
			/*
			NO SUBFIELD???
			case 760:
				$arr['fc']['field_dc_relation'][] = array("value" => $str, "type" => 'IsPartOf');
				break;		

			*/		
		"field_dc_relation" => array("key" => "fc", "datafield" => 530, "subfield" => "a", 
			'qualifier' => "type", 'enum' => "IsPartOf"),


		
	);
	

	foreach($fields as $fieldname => $map) {
	
		/*

			$tmp = $xml->xpath("//marc:datafield[@tag='264']/marc:subfield[@code='b']");
			if(count($tmp)) {
				$str = (string) $tmp[0];
				return _cals_get_iso_date($str);
			}

		*/
		$key = $map['key'];
		$df = $map['datafield'];
		$sf = $map['subfield'];
		$str = "//marc:datafield[@tag='" . $df . "']/marc:subfield[@code='" . $sf  ."']";
		$tmp = $xml->xpath($str);
		//$tmp = $xml->xpath("//marc:datafield[@tag='260']/marc:subfield[@code='c']");
		//printPre($fieldname, $map, $tmp, $str);
		
		if(count($tmp)>=1) {
			foreach($tmp as $me) {
			
				$val = (string) trim($me[0]);
				if (isset($map['qualifier']) ) {
					$arr[$key][$fieldname][] = array( $map['qualifier'] => $map['enum'], "value" => $val );
				}
				else {
					$arr[$key][$fieldname][] = $val;
				}
			}
		
		}
			
	
	}



}


/**
 *
 * its a big switch block: could probably replace with a $map array and run
 * more efficiently, but some tags require additional logic
 *
 * @param $record
 *   the tag derived from the xml file
 *
 * @return
 *   an array of Drupal-ready fields
 *
 */
function _cals_get_array_from_xml($record, &$arr) {


	//leader field
	$leader = (string) $record->children(NAME_SPACE)->leader;
	$arr["leader"] = $leader; 
		
	$type = substr($leader, 6, 1);
	if($type != '') 	$arr['repos']['field_dc_type'][] = $type;
 
	/*
	http://www.oclc.org/bibformats/en/fixedfield/006.html
	*/
	//loop thru controlfield
	foreach($record->children(NAME_SPACE)->controlfield as $controlfield) {
		$tag = (string) $controlfield->attributes()->tag;
		$tags['tag'] = $tag;
		$tags['ind1'] = (string) $controlfield->attributes()->ind1;
		$tags['ind2'] = (string) $controlfield->attributes()->ind2;
		$tags['code']	= '';
		$tags['str'] = (string) $controlfield;
		
		
		if($tag == '008') printPre($tags);
		_cals_get_value_from_tag($tags, $arr);
		//dpm($me);

	}	
	
	//loop thru datafields
	foreach($record->children(NAME_SPACE)->datafield as $datafield) {
		$tag = (string) $datafield->attributes()->tag;
		$tags['tag'] = $tag;
		$tags['ind1'] = (string) $datafield->attributes()->ind1;
		$tags['ind2'] = (string) $datafield->attributes()->ind2;
	
		
		foreach($datafield as $subfield) {
			$tags['code']	= (string) $subfield->attributes()->code;
			$tags['str'] = (string) $subfield;

			if( strlen($tags['str'])) {
				_cals_get_value_from_tag($tags, $arr);
			}
 
		}
	}	

	

}

/**
 * Maps MARC tag to Drupal Field
 *
 * its a big switch block: could probably replace with a $map array and run
 * more efficiently, but some tags require additional logic
 *
 * @param $tag
 *   the tag derived from the xml file
 *
 * @param $arr
 *   the big arr
 *
 * @return
 *   an array. Could replace w/ a reference
 *
 */
function _cals_get_value_from_tag($tags, &$arr) {
	$code = '';
	$ind1 = '';
	$ind2 = '';
	$tag = '';
	$str = '';
	foreach($tags as $k => $v) {
 		${$k} = $v;
	}

	
	switch($tag) {
 
			//main title	
			case 245:
				if($code == 'a') $arr['repos']['title'] =  $str;
				if($code == 'b') $arr['repos']['title'] .=  " " .  $str;
	 

 
	
			/**
			* COVERAGE.SPATIAL: 
			*   651 #0 $a: Geo Name indicator 0 = lcsh; 6 = Répertoire de vedettes-matièr
			*   043 Geographic Code iso 3166
			*   044 Country of publishing
			*   008/15-17 prov country of publishing : http://www.loc.gov/marc/countries/
			*/	
 					
			case "008":
				//$str = (string) $datafield;
				
				/*
				$d = _cals_get_iso_date(substr($str, 7, 4));
				$arr['fc']['field_date'][$tag] = array("field_qualifier_date" => "Issued", "field_dc_date" => _cals_get_iso_date($d) );
				
				printAndDie($arr);
				*/
				$arr['fc']['field_dc_coverage'][] = array(
							"field_value" => substr($str, 15, 3), 
							"field_dc_coverage_type" => "Spatial",
							"field_dc_coverage_scheme" => "http://www.loc.gov/marc/countries",
							"field_marc_tag" => $tag,
				);
				
				$aud = substr($str, 22, 1);
				if(strlen($aud)) $arr['repos']['field_audience'][] = $aud;
				break;

			case "043":
			case "044":
				$arr['fc']['field_dc_coverage'][] = array(
							"field_value" => $str, 
							"field_dc_coverage_type" => "Spatial",
							"field_dc_coverage_scheme" => "ISO 3166",
							"field_marc_tag" => $tag,
				);

				break;	
			
			case 651:
					if($code == 'a') {
						$arr['fc']['field_dc_coverage'][] = array(
							"field_value" => $str, 
							"field_dc_coverage_type" => "Spatial",
							"field_dc_coverage_scheme" => "lcsh",
							"field_marc_tag" => $tag,
						);
						//lets add this to the keyword index?
						$arr['repos']['field_keyword_index'][] = $str;

					}	
				break;	

	
			/**
			* RIGHTS: 
			*   506 $ad	= access_rights
			*   540 $ad	= access_rights
			*   542 $d	= rights_holder
			*/					
			case 540:	
				if($code == 'a' || $code == 'd' ) $arr['repos']['field_access_rights'][] = $str;
				break;
							
			case 506:
				if($code == 'a' || $code == 'd' ) $arr['repos']['field_access_rights'][] = $str;
				//if 506 includes the word "restricted", set the file access to licensed (1); else set to open (0).
				//default is 0. 
				$tmp = strpos(strtolower($str),"restricted");
				$arr['repos']['field_access_restrictions'][0] = (is_numeric($tmp)) ? 1 : 0;	
				
				break;
			case 542:					
				if($code == 'd' ) $arr['repos']['field_rights_holder'][] = $str;
				break;
				
			/**
			* IDENTIFIERS: 
			*   028 $a	= publisher number
			*   028 $b (producer)
			*   035	## $a system control number
			*   035 ## $a dcterms;Oclc 035 $a(OCoLC)153910628
			*   856 4# $u .URI - use for primary URL, alt URLS, DOI	
			*   534 ## $z ISBN	
			*/					

			case "035":

				if($code == 'a') {
					$str = (string)$str;
					if(strpos($str, "(OCoLC)")) {
						$arr['repos']["field_oclc_number"][] = $str;
					}
					else {
						$arr['repos']["field_system_control_number"][] = $str;
					}
				} 
				break;	

			//body field; description
			case 520:
				if($code == 'a') 	{
					//to do - rewrite this more efficiently!!
					if(isset($arr['repos']['body'][0])) {
						$arr['repos']['body'][0] .= $str;
					}
					else {
						$arr['repos']['body'][0] = $str;
					}
				}

				break;
								

			/**
			* CATALOGUING AGENCY: 
			*   040 $a	= Original Cataloguing agency (NR)
			*   040 $b = cataloguing_language
			*   040 $c = transcribing_agency
			*   040 $d = modifying_agency
			*/
			case "003":
				$arr['fc']['field_cataloguing_agency'][] = 
					array(
						"field_agency_type" => "", 
						"value" => $str,
					);
				break;
									
			case "040":
			
				$tmp = _cals_importer_get_array_cataloging_agency();
				if (array_key_exists($code, $tmp)) {
					$arr['fc']['field_cataloguing_agency'][] = 
						array(
							"field_agency_type" => $tmp[$code], 
							"value" => $str
						);
				
				}
				break;



	
								
	}	//end switch

}// end function