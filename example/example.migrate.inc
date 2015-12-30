<?php

/**
 * Implements hook_migrate_api().
 */
function cfx_example_migrate_api() {
  $api = array(
    'api' => 2,
    'groups' => array(
      'cfx_example' => array(
        'title' => t('Content Fixtures Example Migrations'),
      ),
    ),
    'migrations' => array(
      'cfx_nodetype' => array(
        'class_name' => 'ContentFixturesNodeNodetype',
        'group_name' => 'cfx_example',
      ),
      'cfx_other_nodetype' => array(
        'class_name' => 'ContentFixturesNodeOtherNodetype',
        'group_name' => 'cfx_example',
        'dependencies' => array(
          'cfx_nodetype',
        ),
      ),
      'cfx_menu' => array(
        'class_name' => 'ContentFixturesMenuExample',
        'group_name' => 'cfx_example',
      ),
    ),
  );
  return $api;
}