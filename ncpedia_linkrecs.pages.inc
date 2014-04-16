<?php
// $Id$

/**
 * @file
 * Page callback and associated functions for the link recommender module
 */

/**
 * Link suggestions tab
 */
function ncpedia_linkrecs_page($form, &$form_state) {
    
  $form = array();
  if (preg_match('/node\/([0-9]+)\/linkrecs(\/page\/([0-9]+))?/', current_path(), $matches)) {

    //parsing current path generates node ID and page number, if included
    $this_nid = $matches[1];    
    if (array_key_exists(3, $matches)) {
      $page = intval($matches[3]);
    }
    else {
      $page = 0;
    }

    //Populate linking candidates array with a call to _check_for_links
    //Pager information can help determine which set of candidates to provide from a long list
    $titles = array();
    if (is_numeric($this_nid)) {
      $titles = ncpedia_linkrecs_check_for_links($this_nid, $page);
    }

    //if there are possible matches, create a form to review them
    if (!empty($titles)) {

      //Build explanatory header text
      $linkrecs_header = '<p>' . t('Suggested links to ');
      $linkrecs_header .= l(ncpedia_linkrecs_get_title($this_nid), 'node/'.$this_nid, array('attributes' => array('target' => '_blank')));
      $linkrecs_header .= t(" below. Review the suggestions (all links open in a new window) and 
        select which you'd like linked, then click 'Create Links' to update. External links
        are replaced.");
      $linkrecs_header .= '<em>' . t('You should review all updates once they are completed.') . '</em></p>';

      //Extract title variations; used for plural checking
      $title_list = array();
      foreach($titles as $title_variation => $link_candidates) {
        $title_list[] = $title_variation;
      }

      //Add explanatory header text to the form
      $form['linkrecs-header'] = array(
        '#type' => 'markup',
        '#markup' => $linkrecs_header,
      );

      //if a pager is needed, create markup and unset that element from the array before proceeding
      //we'll append actual pager at the bottom of the form
      $pager_markup = '';
      if (isset($titles['linkrecs-page-end'])) {        
        $pager_markup .= '<p>';
        if ($titles['linkrecs-page-end']['current-page'] > 0) {
          $page_track = $titles['linkrecs-page-end']['current-page'] - 1;
          if ($page_track == 0) {
            $pager_markup .= l(t('<< Previous page'), 'node/' . $this_nid . '/linkrecs');
          }
          else {
            $pager_markup .= l(t('<< Previous page'), 'node/' . $this_nid . '/linkrecs/page/' . $page_track);
          }
          $pager_markup .= $titles['linkrecs-page-end']['more-available'] ? ' | ' : '';
        }        
        if ($titles['linkrecs-page-end']['more-available'] === TRUE) {
          $page_track = $titles['linkrecs-page-end']['current-page'] + 1;
          $pager_markup .=  l(t('Next page >>'), 'node/' . $this_nid . '/linkrecs/page/' . $page_track);
        }
        $pager_markup .= '</p>';

        unset($titles['linkrecs-page-end']);
      }
      //Checkbox toggle
      $form['linkrecs-checkboxes-toggle'] = array(
        '#title' => t('Check/Uncheck All'), 
        '#type' => 'checkbox',
        '#default_value' => 0,
      );
      //Build main form
      foreach($titles as $title_variation => $link_candidates) {        
        //Collapse experimental plural variations by default. These have the potential to be
        //weird or annoying.
        if (in_array($title_variation . 's', $title_list)) {
          $form['links'][$title_variation] = array(
            '#title' => t('Experimental plural variation (may not make sense): ' . $title_variation),
            '#type' => 'fieldset',
            '#collapsible' => TRUE,
            '#collapsed' => TRUE,
          );
        }
        else {
          $form['links'][$title_variation] = array(
            '#title' => t('Matches for: ' . $title_variation),
            '#type' => 'fieldset',
            '#collapsible' => TRUE,
            '#collapsed' => FALSE,
          );       
        }
        //Main form-building loop. Places linking candidates into the form as checkbox options
        foreach($link_candidates as $candidate) {
          $form['links'][$title_variation][$candidate['nid']] = array(
            '#title' => $candidate['link'],
            '#description' => $candidate['context'],
            '#link_type' => $candidate['link-type'],
            '#type' => 'checkbox',
            '#default_value' => 0,
            '#states' => array(
              'unchecked' => array (
                ':input[name="linkrecs-checkboxes-toggle"]' => array('unchecked' => TRUE),
              ),
            )
          );
        }
      }
      $form['linkrecs-node'] = array('#type' => 'hidden', '#value' => $this_nid);
      $form['linkrecs-submit'] = array(
        '#type' => 'submit',
        '#value' => t('Create Links'),
      );   
      if ($pager_markup !== '') {
        $form['linkrecs-pager'] = array(
          '#type' => 'markup',
          '#markup' => $pager_markup,
        );
      }
    }
    else {
      $options = '<p>' . t('No suggested links found.') . '</p>';
      $options .= '<p>' . l(t('Back to ') . ncpedia_linkrecs_get_title($this_nid), 'node/'.$this_nid) . '</p>';
      $form['information'] = array(
        '#type' => 'markup',
        '#markup' => $options,
      );
    }
  }
  else {
    $options = '<p>' . t('Could not parse node ID.') . '</p>';
    $form['information'] = array(
      '#type' => 'markup',
      '#markup' => $options,
    );
  }
  
  return $form;
}

function ncpedia_linkrecs_page_submit($form, &$form_state) {
  
  $link_to_node_id = $form_state['values']['linkrecs-node'];

  //Amend $link_to_node_id in the event that it has a pager suffix
  if (!is_numeric($link_to_node_id)) {
    $page_info = explode('page', $link_to_node_id);
    if (count($page_info)===2 && is_numeric($page_info[0]) && is_numeric($page_info[1])) {
      $link_to_node_id = $page_info[0];
    }
  }

  foreach ($form_state['complete form']['links'] as $linked_term => $linked_term_array) {
    if (is_array($linked_term_array)) {
      foreach ($linked_term_array as $node_to_update => $match_array) {
        if ((is_array($match_array)) && (isset($match_array['#value'])) && ($match_array['#value'] === 1)) {
          //Get selected entry body from database, update, save
          $n = node_load($node_to_update); 
          $temp_body = ncpedia_linkrecs_update_links($linked_term, $link_to_node_id, $n->body['und'][0]['value'], $match_array['#link_type']);
          if ($temp_body !== '') {
            drupal_set_message($temp_body);
            $n->body['und'][0]['value'] = $temp_body;  
            node_save($n);
            $report_success = l($n->title, 'node/'. $n->nid, array('attributes' => array('target' => '_blank')));
            $report_success .= t(" updated.");
            drupal_set_message($report_success);
          }
          else {
            drupal_set_message("Sorry, there was an error with the update. No changes were made.");
          }
        }
      }
    }
  }
}

/**
 * Searches database for title matches
 * 
 * @param integer $nid
 * @param integer $page
 * @return array
 */
function ncpedia_linkrecs_check_for_links($nid,$page=0) {
  
  $max_links_suggested = variable_get('ncpedia_linkrecs_max', 30);
  $context_string_length = variable_get('ncpedia_linkrecs_context_length', 60);
  $search_style = variable_get('ncpedia_linkrecs_search_style', 'default');
  
  $title = ncpedia_linkrecs_parse_title(ncpedia_linkrecs_get_title($nid));  
 
  $links = array();
  //links_count and boundary variables to isolate a particular set of candidates
  //for linking among a long list
  $links_count = 0;
  $links_min = $page * $max_links_suggested + 1;
  $links_max = $page * $max_links_suggested + $max_links_suggested;
      
  //keep track of all link suggestions to avoid duplication  
  $master_list = array();
 
  foreach ($title as $nt) {
    
    if ($links_count > $links_max) {
      break;
    }    
    
    //Pull titles of matches for count & approval
    //Query does not do any complex searching, just looks for matching text
    //We will do more thorough checks below      
    
    switch ($search_style) {
      case "fulltext":
        //THIS VERSION uses a full-text indexed table
        //Much faster than above! But requires a pre-built table with FULLTEXT indexing
        //Still not as efficient as an external search (e.g., Solr)
        $result = db_query('SELECT n.title AS title, n.nid AS nid, lh.body_value AS body_value
          FROM {ncpedia_linkrecs_helper} lh INNER JOIN {node} n ON n.nid = lh.entity_id 
          WHERE MATCH (lh.body_value) AGAINST (:search IN BOOLEAN MODE) ORDER BY title ASC',
                array(':search' => '+"'.$nt.'"'));
        break;
      default:
        //THIS VERSION uses Drupal's default field_data_body table
        //Not very efficient, doesn't scale. Does a full table scan. But, requires
        //no additional tables or indexing.
        $result = db_query("SELECT n.title AS title, n.nid AS nid, fdb.body_value AS body_value 
          FROM {field_data_body} fdb INNER JOIN {node} n ON n.nid = fdb.entity_id 
          WHERE (fdb.body_value LIKE :search) ORDER BY title ASC",
                array(':search' => "%".$nt."%")); 
    }
    
    foreach ($result as $row) {
      //Skip self-matches.
      //Check master list, if not there, add it and proceed
      if (($row->nid !== $nid) && (!in_array($row->nid, $master_list))) {

        //The query above merely matched text. However, there are instances when the match
        //is not suitable:
        //1. If the matching text is contained within an H1 tag (i.e., it's the title)
        //2. If the matching text is contained within an image's alt/title text
        //3. If the matching text is already linked
        //
        //In cases 1 & 2, we assume there are non-title, non-image date mataches as well,
        //so we'll just strip out the H1 and IMGs and look elsewhere. In case 3, if the text
        //is already INTERNALLY linked (i.e., to another page on the site), we will
        //disregard it as a candidate. Though if the text is EXTERNALLY linked, we can 
        //consider updating the link to the new internal candidate.

        //get node body value  
        //stripping image and h1 tags from original - we don't want to consider
        //text present in alt/title image attributes, nor titles 
        switch ($search_style) {
          case "fulltext":
            $r = db_query("SELECT fdb.body_value AS body_value 
                FROM {field_data_body} fdb WHERE fdb.entity_id=:nid",
                array(':nid' => $row->nid)); 
            $body_col = $r->fetchCol();
            $body = ncpedia_linkrecs_get_tempbody($body_col[0]);
            break;
          default:
            $body = ncpedia_linkrecs_get_tempbody($row->body_value);
        }

        //Pass title & body to link reviewer
        //Per that function documentation, one of three statuses is returned:
        //'unlinked' - the text was found in the body, it is not linked
        //'external' - the text was found in the body, linked externally
        //'internal' - the text was found in the body, linked internally
        //'not found' - the text was not found at all in the body (this is possible; it may
        //              be a false positive - the match might have been a title or img match
        //              and has been stripped out)
        //
        //We will proceed if unlinked or external        
        $existing_link_check = ncpedia_linkrecs_review_links($nt, $body);

        //Proceed with creating a link and contextual text, skipping
        //articles that already have a matching internal link
        if (($existing_link_check == 'unlinked') || ($existing_link_check == 'external')) {

          $master_list[] = $row->nid;

          $links_count++;

          //If we're over the max, that implies we've already filled up the return array
          //and there are additional suggestions, so this signals the form builder that
          //we need a "Next" link in the pager
          if ($links_count > $links_max) {
            $links['linkrecs-page-end']['more-available'] = TRUE;
            break;
          }

          if ($links_count >= $links_min) {
            //We've met the minimum, and the above test implies we're under the max, so
            //we can add this item to the array.
            //Earlier version did a range query to speed things up but this lead to 
            //problems. The query is imprecise and will retrieve already matched items and
            //such that need to be filtered post-query. It's something of a performance
            //penalty, but the whole set needs to be retrieved and counted manually as
            //candidates are found.

            //Find the first match. This is generally the only link we'd add.
            $location = stripos($body, $nt);
            //Context snippet start point should be the location of the needle minus the 
            //context length value. Exception is when needle appears very close to the 
            //beginning of the haystack, in which case, just
            //start the snippet at the beginning of the haystack.
            $start_context = $location > $context_string_length ? $location - $context_string_length : 0;
            //Total context snippet length should be twice the defined length value, plus 
            //the length of the needle. Alter this if the needle appears to close to the 
            //end of the haystack. In that case, truncate
            //the snippet to end at the end of the haystack.
            $context_length = $location + strlen($nt) + $context_string_length < strlen($body) ? 
              strlen($nt)+$context_string_length*2 : strlen($body) - $start_context;

            //Set context snippet and append ellipses or appropriate notice
            $context = strip_tags(substr($body, $start_context, $context_length));
            $context = $start_context !== 0 ? '...' . $context : '[START OF BODY]' . $context;
            $context = $context_length > $context_string_length * 2 ? $context . '...' : $context . '[END OF BODY]';   

            $links[$nt][$row->title]['nid'] = $row->nid;
            $links[$nt][$row->title]['link'] = l($row->title, "node/" . $row->nid, array('attributes' => array('target' => '_blank')));
            $links[$nt][$row->title]['context'] = $existing_link_check === 'external' ?
                    'EXTERNAL LINK FOUND. UPDATING WILL REPLACE: ' . $context : $context;
            $links[$nt][$row->title]['link-type'] = $existing_link_check;

            //When right at the max point (or if you're simply not on the initial page,
            //set the page-end elements in the return array.
            //We continue to process though, to see if there are more available. Could
            //be that this is the last one anyway.
            if (($links_count === $links_max) || ($page != 0)) {
              $links['linkrecs-page-end']['current-page'] = $page;
              $links['linkrecs-page-end']['more-available'] = FALSE;
            }    
          }
        }
      }
    }
  }

  return $links;
}

/**
 * Returns a node's title when given a node id
 * 
 * Lighter-weight over a full node load, since we only need title
 * 
 * @param integer $nid
 * @return string
 */
function ncpedia_linkrecs_get_title($nid) {
  $result = db_query('SELECT n.title FROM {node} n WHERE n.nid = :nid', array(':nid' => $nid));
  $init_title = $result->fetchCol();
  return $init_title[0];
}

/**
 * Takes a string as a full title, returns array of possible titles 
 * 
 * This can't be perfect, but some general rules are applied:
 * *Strip out or don't count common phrases that won't help with finding unique matches
 * *Anything in the format [word],[otherword] is assumed to be [a last name],[a first name].
 * In which case, be sure to search for the name as-is, as well as [first name] [last name]
 * *If the title has three or more words, try variations that use combinations of the words
 * But don't bother searching for one word of the three. Probably too vague to be useful.
 * 
 * @param type $title
 */
function ncpedia_linkrecs_parse_title($full_title) {
  
  //Running list of title strings to ignore, keep as a regex
  //e.g., super-common phrases that will generate far too many matches to be useful
  $verboten_list = '/north|carolina|tar|heel|junior|historian|research|branch/i';

  //Title strings that should be stripped from the full title
  //e.g., if you have a suffix on one batch of titles and you wouldn't want them 
  //to be matched as suggestions
  $title_removes_list = array (
    ' (from Tar Heel Junior Historian)',
  );
  $title_replacements_list = array (
    '',
  );
  
  $title[] = trim(str_replace($title_removes_list, $title_replacements_list, $full_title));

  //Check for cases of node titles being lastname, firstname(s)
  //if found, rearrange and add variations to array. Keep original in array, as well
  if (substr_count($title[0], ',') > 1) { //true for name suffixes, e.g. Fox, Joe, Jr.
    $variations = preg_split('/,\s*/i', $title[0]);
    $title[] = $variations[0] . ', ' . $variations[1];
    $title[] = $variations[1] . ' ' . $variations[0];
    $title[] = $variations[1] . ' ' . $variations[0] . ', ' . $variations[2];
  }  
  elseif (substr_count($title[0], ',') === 1) {
    $title[] =  ltrim(strstr($title[0], ','), ', ') . ' ' . strstr($title[0], ',', TRUE);
  }
  else { 
    //inferring it's not a name lastname, firstname; do a few other small things:

    //de-pluralize - can't reliably do this but at least try a simple -s removal
    if (preg_match('/s\Z/i', $title[0])) {
      $title[] = substr($title[0], 0, strlen($title[0])-1);
    }  
    //check for multi-part entries, strip "Part:" text if found
    //no need to bother trying to match "Part:" text, original can be overwritten
    if (preg_match('/\s-\sPart\s[1-9]:\s/i', $title[0])) {
      $temp_title = preg_replace('/\s-\sPart\s[1-9]:\s/i', '***', $title[0]);
      $title = explode('***', $temp_title);    
    }
  }
  //Additionally populate array word clusters up to two words
  //don't count clusters where all words are 3 chars or less
  foreach ($title as $t) {
    $all_the_words = str_word_count($t, 1);
    for ($i=count($all_the_words)-1; $i>1; $i--) {
      for ($j=0; $j<=count($all_the_words)-$i; $j++) {
        $str = '';
        $no_short_words = TRUE;
        for ($k=0; $k<$i; $k++) {
          if (strlen($all_the_words[$j+$k]) <= 3) {
            $no_short_words = FALSE;
            break;
          }
          else {
            $str .= $all_the_words[$j+$k] . ' ';          
          }
        }
        //skip anything with short words or common phrases
        $str = rtrim($str);
        if (($no_short_words) && (!preg_match($verboten_list, $str)) && (!in_array($str, $title))) { 
          $title[] = $str;
        }
      }
    }
  }
  return $title;  
}

/**
 * Finds matching text in a node body and determines its link status. Returns one of
 * the following statuses:
 * 
 *'unlinked' - the text was found in the body, it is not linked
 *'external' - the text was found in the body, linked externally
 *'internal' - the text was found in the body, linked internally
 *'not found' - the text was not found at all in the body (this is possible; it may
 *              be a false positive - the match might have been a title or img match
 *              and has been stripped out)
 * 
 * @param string $nt is the 'node title', a string of text to search for
 * @param string $body
 * @return string
 */
function ncpedia_linkrecs_review_links($nt, $body) {
  
  $status = '';
  
  //Review all the links in the body for the intended match
  //matches[0] = all link tags
  //matches[1] = URL for each tag
  //matches[2] = linked text
  if (preg_match_all('/\<a\shref=\"(.+?)\"\>(.+?)\<\/a\>/is', $body, $matches)) {
    for ($i=0; $i<count($matches[0]); $i++) {
      //check all linked text for a match
      if (stripos($matches[2][$i], $nt) !== FALSE) {  
        //if you find it, assume URL that starts with a / is internal
        if (preg_match('/^\//is', $matches[1][$i])) { 
          $status = 'internal';
          break;
        }
        else {
          $status = 'external';
          break;
        }
      }
    }
  }
  if ($status === '') {
    $status = stripos($body, $nt) !== FALSE ? 'unlinked' : 'not found';
  }

  return $status;
}

/**
 * 
 * @param string $nt The title string
 * @param string $update_path The node ID to link to
 * @param string $body
 * @param string $link_type 'external' or 'unlinked' considered; assumed 'unlinked' unless specified
 * @return type
 */
function ncpedia_linkrecs_update_links ($nt, $update_path, $body, $link_type) {
  
  //Get all the images, store temporarily in $img_matches[0]
  $img_count = preg_match_all('/\<img.+?\/\>/is', $body, $img_matches);
  $h1_count = preg_match_all('/\<h1\>.+?\<?\/h1\>/is', $body, $h1_matches);
  $temp_body = ncpedia_linkrecs_get_tempbody($body);
  
  $updated_body = '';
  
  if ($link_type === 'external') {
    if (preg_match_all('/\<a\shref=\"(.+?)\"\>(.+?)\<\/a\>/is', $temp_body, $matches)) {
      for ($i=0; $i<count($matches[0]); $i++) {
        if (stripos($matches[2][$i],$nt)!==FALSE) {
          $updated_body = str_replace($matches[0][$i], '<a href="/node/' . $update_path . '">' . $matches[2][$i] . '</a>', $temp_body);
          break;
        }
      }
    }
  }
  else {
    //preg_replace used so I can limit to one replacement only
    $updated_body = preg_replace('/(' . $nt . ')/i', '<a href="/node/'. $update_path .'">$1</a>', $temp_body, 1);
  }
  
  //Replace images
  for ($i=0; $i<$img_count; $i++) {
    $updated_body = preg_replace('/!!!IMG_REMOVED!!!/', $img_matches[0][$i], $updated_body, 1);
  }  
  for ($i=0; $i<$h1_count; $i++) {
    $updated_body = preg_replace('/!!!H1_REMOVED!!!/', $h1_matches[0][$i], $updated_body, 1);
  }  
  
  return $updated_body;
}

function ncpedia_linkrecs_get_tempbody($body) {
  
  $search_array = array (
    '/\<img.+?\/\>/is',
    '/\<h1\>.+?\<?\/h1\>/is',
  );
  $replace_array = array (
    '!!!IMG_REMOVED!!!',
    '!!!H1_REMOVED!!!',
  );
  
  return preg_replace($search_array, $replace_array, $body);
}