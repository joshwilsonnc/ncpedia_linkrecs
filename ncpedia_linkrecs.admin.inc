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
    '#options' => drupal_map_assoc(range(1, 100)),
    '#default_value' => variable_get('ncpedia_linkrecs_max', 50),
    '#description' => t('Probably not worthwhile to offer editors thousands of suggestions, you know?'),
    '#required' => TRUE,
  );
  
  return system_settings_form($form);
}