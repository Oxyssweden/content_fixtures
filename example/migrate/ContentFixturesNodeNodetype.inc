<?php
/**
 * @file
 * Contains migrations for example content
 */

class ContentFixturesNodeNodetype extends ContentFixturesEntityMigration {

  private $cfx_entity_type = 'node';
  private $cfx_bundle = 'nodetype';
  // Name of the module this migration lives in.
  private $cfx_module_name = 'example';

  public function __construct($arguments = array()) {
    parent::__construct($arguments);
  }

  function prepareRow($row) {
    parent::prepareRow($row);
  }
}
