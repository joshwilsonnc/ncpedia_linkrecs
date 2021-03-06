<?php
// $Id$

/**
 * @file
 * Admin page callback for the link recommender module.
 */

/**
 * Builds and returns the link recommender settings form.
 */
function ncpedia_linkrecs_admin_settings() {
  
  $form = array();
  $form['ncpedia_linkrecs_max'] = array(
    '#type' => 'select',
    '#title' => t('Maximum number of link suggestions offered to user'),
    '#options' => drupal_map_assoc(array(10, 20, 30, 50, 75, 100, 150, 200)),
    '#default_value' => variable_get('ncpedia_linkrecs_max', 30),
    '#description' => t('Probably not worthwhile to offer editors thousands of suggestions, you know?'),
    '#required' => TRUE,
  );
  $form['ncpedia_linkrecs_context_length'] = array(
    '#type' => 'select',
    '#title' => t('Length of context string provided'),
    '#options' => drupal_map_assoc(array(20, 40, 60, 80, 100, 125, 150, 175, 200)),
    '#default_value' => variable_get('ncpedia_linkrecs_context_length', 60),
    '#description' => t('The longer the context string, the better understanding editors have of the link
      placement. A longer string could make the page ugly or slow down the page load, however.'),
    '#required' => TRUE,
  );  
  $form['ncpedia_linkrecs_search_style'] = array(
    '#type' => 'select',
    '#title' => t('Type of database search performed'),
    '#options' => drupal_map_assoc(array('default','fulltext')),
    '#default_value' => variable_get('ncpedia_linkrecs_search_style', 'default'),
    '#description' => t('Default search can use Drupal core tables, but is slow. Fulltext search requires a separate table (see documentation), but is much faster.'),
    '#required' => TRUE,
  );
  //
  
  
  return system_settings_form($form);
}