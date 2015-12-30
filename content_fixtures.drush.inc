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
    'description' => 'Generates a content fixture migrate class',
    'callback' => 'drush_content_fixtures_generate',
    'options' => array(
      'destination' => 'defaults to sites/all/modules when creating a new module',
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
  $args = drush_get_arguments();

  if (count($args) < 4) {
    drush_die(dt('Please provide arguments: module entitytype bundle'));
  }
  list($command, $module_name, $entity_type, $bundle) = $args;

  if (!drush_bootstrap_to_phase(DRUSH_BOOTSTRAP_DRUPAL_DATABASE)) {
    return drush_set_error('DRUPAL_SITE_NOT_FOUND', dt('You need to specify an alias or run this command within a drupal site.'));
  }

  if (drush_drupal_major_version() != 7) {
    return drush_set_error('ONLY DRUPAL 7 SUPPORTED', dt('Wrong version detected.'));
  }

  $root = drush_get_option(array('r', 'root'), drush_locate_root());
  if ($root) {
    $directory = drupal_get_path('module', $module_name);
    if (empty($directory)) {
      $destination = drush_get_option(array('destination'), 'sites/all/modules');
      $directory = $destination . '/' . $module_name;
    }
    $directory = $root . '/' . $directory;
    if (is_dir($directory)) {
      drush_print(dt('Module appears to already exist in !dir', array('!dir' => $directory)));
      if (!drush_confirm(dt('Do you really want to continue?'))) {
        drush_die('Aborting.');
      }
    }
    else {
      // Create module stub
      drush_op('mkdir', $directory);
      drush_content_fixtures_module_stub($directory);
    }
    if (is_dir($directory)) {

      // Prepare directories
      if (!is_dir("{$directory}/migrate")) {
        drush_op('mkdir', "{$directory}/migrate");
      }
      if (!is_dir("{$directory}/migrate/import")) {
        drush_op('mkdir', "{$directory}/migrate/import");
      }

      // Get file contents
      $files = drush_content_fixtures_render($directory);

      // Write files
      foreach ($files as $name => $file_contents) {
        drush_op('file_put_contents', "{$directory}/$name", $file_contents);
      }

      drush_log(dt("Created module: !module in !directory", array('!module' => $module_name, '!directory' => $directory)), 'ok');
      drush_cache_clear_all();
      drush_log(dt("Cleared caches"), 'ok');
    }
    else {
      drush_die(dt('Couldn\'t create directory !directory', array('!directory' => $directory)));
    }
  }
  else {
    drush_die(dt('Couldn\'t locate site root'));
  }
}

/**
 * Write files
 */
function drush_content_fixtures_render($directory) {
  $files = array();
  list($command, $module_name, $entity_type, $bundle) = drush_get_arguments();
  $entity_type_uc = ucwords($entity_type);
  $bundle_uc = ucwords($bundle);
  $class_name = "ContentFixtures{$entity_type_uc}{$bundle_uc}";
  // .info file
  $files["{$module_name}.info"] = file_get_contents("{$directory}/{$module_name}.info");
  $info_line = "\nfiles[] = migrate/{$class_name}.inc\n";
  if (!strpos($files["{$module_name}.info"], $info_line)) {
    $files["{$module_name}.info"] .= $info_line;
  }

  // Migrate file
  $files["{$module_name}.migrate.inc"] = "<?php

/**
 * Implements hook_migrate_api().
 */
function {$module_name}_migrate_api() {
  return array(
    'api' => 2,
    'migrations' => array(
      '{$module_name}_{$entity_type}_{$bundle}' => array(
        'class_name' => '$class_name',
        'group_name' => 'content_fixtures',
      ),
    ),
  );
}";

  // Migration class
  $files["migrate/{$class_name}.inc"] = "<?php
/**
 * @file
 * Contains migrations for $bundle $entity_type
 */

class $class_name extends ContentFixturesEntityMigration {

  private \$cfx_entity_type = '$entity_type';
  private \$cfx_bundle = '$bundle';
  // Name of the module this migration lives in.
  private \$cfx_module_name = '$module_name';

  public function __construct(\$arguments = array()) {
    parent::__construct(\$arguments);
  }

  function prepareRow(\$row) {
    parent::prepareRow(\$row);
  }
}";

  // csv file
  $info = field_info_instances($entity_type, $bundle);
  $columns = join(',', array_keys($info));
  if ($entity_type == 'node') {
    $columns = 'id,title,' . $columns;
  }
  $files["migrate/import/{$entity_type}-{$bundle}.csv"] = $columns . "\n";

  return $files;
}

/**
 * Generate module stub
 */
function drush_content_fixtures_module_stub($directory) {
  list($command, $module_name, $entity_type, $bundle) = drush_get_arguments();
  $files = array(
    $module_name . '.info' => "name = $module_name
description = Content Fixtures for $entity_type type $bundle
core = 7.x
package = Content Fixtures\n",
    $module_name . '.module' => "<?php\n");
  foreach ($files as $name => $file_contents) {
    drush_op('file_put_contents', "{$directory}/$name", $file_contents);
  }
}
