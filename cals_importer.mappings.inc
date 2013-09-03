<?php

function _cals_get_marc_mappings() {
	$arr = array();

	$arr[260] = array(
		array("c", "date", "issued", "_cals_get_iso_date",),
	);


}



function _cals_get_array_from_xmlx($record) {
	$ns = "http://www.loc.gov/MARC21/slim";
	$arr = array(
			"leader" => (string) $record->children($ns)->leader,
	);
	//printPre($record);
	foreach($record->children($ns)->datafield as $datafield) {
		$tag = (string) $datafield->attributes()->tag;
		$ind1 = (string) $datafield->attributes()->ind1;
		$ind2 = (string) $datafield->attributes()->ind2;
	
		switch($tag) {
		
			case 260:
				foreach($datafield as $subfield) {
					$code = (string) $subfield->attributes()->code;
	
					if( $code == 'c' ) {
						$arr['date'][] = array("issued" => _cals_get_iso_date($subfield));
					}
					
				}
			break;	
			
			case 264:
				foreach($datafield as $subfield) {
					$code = (string) $subfield->attributes()->code;
					if ($code == 'a' || $code == 'b') $arr['publisher'][] = (string) $subfield;
					
				}
				break;
		
			//creators
			case 100:
			
				$arr['creator'] = (string) $datafield->subfield[0];
				break;
		
		
			//titles			
			case 245:
	
				$arr['title'] = (string) $datafield->subfield[0];
				if(strlen((string) $datafield->subfield[1])) $arr['title'] .=  " " . (string) $datafield->subfield[1];
				foreach($datafield->subfield as $subfield) {
					//printPre((string) $subfield);
					
				}
				
				break;
			//uniform titles: 130 0# $a, 730  0# $a	
			case 130:
			case 730:
				if(strlen((string) $datafield->subfield[0])) $arr['uniform_title'][] = (string) $datafield->subfield[0];
				break;					
			
			/**
			* alt titles: 
			*   240 ind 10 $a	
			*   246 ind 13 $a
			*   210?
			*   242?
			*/	
			case 240:
			
			case 246:
				if(strlen((string) $datafield->subfield[0])) $arr['alt_title'][] = (string) $datafield->subfield[0];
				break;
	
			/**
			* TYPES (inc. Genre): 
			*   655 0 = lcsh; 7 = Specified; 4 = not specified
			*   leader 06/07
			*/					
	
			/*
					<marc:datafield tag="655" ind1=" " ind2="4"><marc:subfield code="a">Autobiography.</marc:subfield></marc:datafield>
	
			
			*/
			case 655:
				foreach($datafield as $subfield) {
					$code = (string) $subfield->attributes()->code;
					$str = (string) $subfield;
					if( ($ind1 == 0 || $ind2 == 0) && $code == 'a' ) {
						$arr['type'][] = array("type" => "genre", "scheme" => "lcsh", "value" => (string) $subfield);
					}
				}
			break;	
	
	
			
			/**
			* SUBJECTS: 
			*   600 10 $a	
			*   610 20 $a	
			*   630 00 $alx	
			*   650 #0 $azvxy: indicator 0 = lcsh; 6 = Répertoire de vedettes-matièr
			*/					
			//subjects
			
			case 600:
				foreach($datafield as $subfield) {
					$code = (string) $subfield->attributes()->code;
					if($code == 'a') $arr['subject'][] =_cals_get_subject_value($subfield, $ind2);
				}
			
				break;
			
			case 650:
				/**
				<marc:datafield tag="650" ind1=" " ind2="0">
					<marc:subfield code="a">Ukrainian Canadians</marc:subfield>
					<marc:subfield code="v">Fiction.</marc:subfield>
				</marc:datafield>					
				
				*/
				foreach($datafield as $subfield) {
					$code = (string) $subfield->attributes()->code;
					$codes = array("a", "x", "v", "y", "z");
					if (in_array($code, $codes)) $arr['subject'][] =_cals_get_subject_value($subfield, $ind2);
				}
				break;
	
	
	
			/**
			* COVERAGE.SPATIAL: 
			*   651 #0 $a: Geo Name indicator 0 = lcsh; 6 = Répertoire de vedettes-matièr
			*   043 Geographic Code iso 3166
			*   044 Country of publishing
			*   008/15-17 prov country of publishing : http://www.loc.gov/marc/countries/
			*/					
			case "043":
				$code = (string) $subfield->attributes()->code;
				$arr['coverage.spatial']['geo_code'][] = (string) $subfield;
			break;	
			case "044":
				$code = (string) $subfield->attributes()->code;
				$arr['coverage.spatial']['country_of_publishing'][] = (string) $subfield;
			break;	
			
			case 651:
				foreach($datafield as $subfield) {
					$code = (string) $subfield->attributes()->code;
					if($code == 'a') $arr['coverage.spatial']['geo_name'][] = (string) $subfield;
				}
			break;	
			case 651:
				foreach($datafield as $subfield) {
					$code = (string) $subfield->attributes()->code;
					if($code == 'a') $arr['coverage.spatial']['geo_name'][] = (string) $subfield;
				}
			break;	
	
			/**
			* RIGHTS: 
			*   506 $ad	= access_rights
			*   540 $ad	= access_rights
			*   542 $d	= rights_holder
			*/					
			case 540:					
			case 506:
				foreach($datafield as $subfield) {
					$code = (string) $subfield->attributes()->code;
					if($code == 'a' || $code == 'd' ) $arr['rights']['access_rights'][] = (string) $subfield;
				}
			
				break;
			case 542:					
				foreach($datafield as $subfield) {
					$code = (string) $subfield->attributes()->code;
					if($code == 'd' ) $arr['rights']['rights_holder'][] = (string) $subfield;
				}
			
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
			case "028":
				foreach($datafield as $subfield) {
					$code = (string) $subfield->attributes()->code;
					if($code == 'a') $arr['identifier']['publisher_number'] = (string) $subfield;
					if($code == 'b') $arr['identifier']['producer'] = (string) $subfield;
				}
			
				break;	
				
			case "035":
				foreach($datafield as $subfield) {
					$code = (string) $subfield->attributes()->code;
					if($code == 'a') $arr['identifier']['system_control_number'] = (string) $subfield;
				}
			
				break;	
				
			case 534:
				
				foreach($datafield as $subfield) {
					$code = (string) $subfield->attributes()->code;
					if($code == 'z') $arr['identifier']['isbn'] = (string) $subfield;
				}
				break;
			case 856:
				foreach($datafield as $subfield) {
					printAndDie($subfield);
					$code = (string) $subfield->attributes()->code;
					if($ind1 == '4') $arr['repos']['field_uri'] = (string) $subfield;
				}
				break;
			/**
			* DESCRIPTIONS: 
			*   028 $a	= publisher number
			*   028 $b (producer)
			*   035	## $a system control number
			*   035 ## $a dcterms;Oclc 035 $a(OCoLC)153910628
			*   856 4# $u .URI - use for primary URL, alt URLS, DOI	
			*   534 ## $z ISBN	
			*/	
				/*
				<marc:datafield tag="500" ind1=" " ind2=" ">
					<marc:subfield code="1"> sound disc [Daisy book]:</marc:subfield>
				</marc:datafield>
				*/
				case 500:
				case 504: 
				case 505: 
				case 511:
				case 516:
				case 520:
				case 533:
				case 538:
				case 586:
				case 595:
				case 598:
				case 599:
				case 541:
					foreach($datafield as $subfield) {
						$val = trim((string) $subfield);
						$arr['description'][] = array(
							"type" => _cals_get_description_type($tag), 
							"value" => $val,
							"num_char" => strlen($val),
						);
					}
					break;
	
	
			
				
			case 800: case 830:
			
				$arr['relation']['part_of'][] = (string) $datafield->subfield[0];
				break;		
				
				
			
			/**
			* CATALOGUING AGENCY: 
			*   040 $a	= Original Cataloguing agency (NR)
			*   040 $b = cataloguing_language
			*   040 $c = transcribing_agency
			*   040 $d = modifying_agency
			*/					
			case "040":
				$tmp = _cals_importer_get_array_cataloging_agency();
				foreach($datafield as $subfield) {
					$code = (string) $subfield->attributes()->code;
					if (array_key_exists($code, $tmp)) 
						$arr['cat_agency'][] = array("refinement" => $tmp[$code], "value" => (string) $subfield);
				}
				break;
				
										
		
		}
	}

	return $arr;

}