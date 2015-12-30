<?php

/**
 * Implements hook_migrate_api().
 */
function content_fixtures_migrate_api() {
  $api = array(
    'api' => 2,
    'groups' => array(
      'content_fixtures' => array(
        'title' => t('Content Fixtures'),
      ),
    ),
  );
  return $api;
}