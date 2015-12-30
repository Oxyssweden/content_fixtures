<?php
/**
 * @file
 * Provide Drush integration for content fixtures.
 */


/**
 * Implements hook_drush_help().
 */
function content_fixtures_drush_help($section) {
  switch ($section) {
    case 'drush:content-fixtures-generate':
      return dt('Generate content fixtures migrations.');
  }
}

/**
 * Implements hook_drush_command().
 */
function content_fixtures_drush_command() {
  $items = array();

  $items['content-fixtures-generate'] = array(
    'description' => 'Placeholder *Not yet implemented*',
    'callback' => 'drush_content_fixtures_generate',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap.
    'options' => array(
      'create' => 'Create new module',
    ),
    'examples' => array(
      'drush cfx-gen example_module node bundle' => 'Generate the nesessary files in module "example_module" for creating "bundle" type "node" entities.',
    ),
    'aliases' => array('cfx-gen'),
  );

  return $items;
}

/**
 * Generate content fixtures.
 */
function drush_content_fixtures_generate() {
  if (!drush_bootstrap_to_phase(DRUSH_BOOTSTRAP_DRUPAL_DATABASE)) {
    return drush_set_error('DRUPAL_SITE_NOT_FOUND', dt('You need to specify an alias or run this command within a drupal site.'));
  }

  if (drush_drupal_major_version() != 7) {
    return drush_set_error('ONLY DRUPAL 7 SUPPORTED', dt('Wrong version detected.'));
  }

  // Everything done.
  drush_log(dt('Did nothing. @todo implement'), 'success');
}
