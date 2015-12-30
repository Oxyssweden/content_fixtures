<?php

abstract class ContentFixturesEntityMigration extends Migration {

  private $cfx_field_instances = NULL;

  private $subfieldSeparator = '__';

  public function __construct($arguments = array()) {
    $bundle = static::$cfx_bundle;
    $entity_type = static::$cfx_entity_type;

    parent::__construct($arguments);
    $this->description = t('Import @entity from CSV file.', array('@entity' => $entity_type . '-' . $bundle));

    // Create a map object for tracking the relationships between source rows
    $this->createMap($entity_type, $bundle);

    // Create a MigrateSource object.
    $path = $this->getSourceFilesPath() . $entity_type . '-' . $bundle . '.csv';
    $this->source = new MigrateSourceCSV($path, array(), $this->sourceFileOptions(), $this->fields($entity_type, $bundle));

    // Set destination migration class
    $this->destination = $this->getDestinationByType($entity_type, $bundle);

    // Field Mappings
    $this->fieldInstanceMap($this->getFieldInstances($entity_type, $bundle));

    // Author, default to admin
    $this->addFieldMapping('uid')->defaultValue(1);
  }

  /**
   * Get destination migration class
   *
   * @param $type
   */
  function getDestinationByType($entity_type, $bundle) {
    switch ($entity_type) {
      case 'taxonomy_term':
        return new MigrateDestinationTerm($bundle);
        break;
      default:
        return new MigrateDestinationEntityAPI($entity_type, $bundle);
        break;
    }
  }

  /**
   * Add field mappings.
   *
   * @param $fields
   */
  function fieldInstanceMap($fields) {
    // @todo: entityreference fields
    foreach ($fields as $field) {
      $name = $field['field_name'];
      switch($field['base_field_info']['type']) {
        case 'file':
        case 'image':
          // Image
          $this->addFieldMapping($name, $name);
          $this->addFieldMapping($name . ':file_replace')
            ->defaultValue(FILE_EXISTS_REPLACE);
          $this->addFieldMapping($name . ':source_dir')
            ->defaultValue($this->getSourceFilesPath() . '/files');
          break;
        case 'taxonomy_term_reference':
          // Taxonomy Term. Allow multiple.
          $this->addFieldMapping($name, $name)
            ->separator(',');
          $this->addFieldMapping($name . ':create_term')
            ->defaultValue(TRUE);
          break;
        case 'datetime':
        case 'date':
          $this->addFieldMapping($name, $name . $this->subfieldSeparator .'from' );
          $this->addFieldMapping($name . ':to', $name . $this->subfieldSeparator .'to');
          break;
        case 'link_field':
          $this->addFieldMapping($name, $name . $this->subfieldSeparator .'url' );
          $this->addFieldMapping($name . ':title', $name . $this->subfieldSeparator .'title');
          break;
        case 'text_with_summary':
          // Generic field
          $this->addFieldMapping($name, $name);
          $this->addFieldMapping($name . ':format')->defaultValue('panopoly_wysiwyg_text');
          break;
        case 'commerce_price':
          $this->addFieldMapping($name, $name);
          $this->addFieldMapping($name . ':currency_code')->defaultValue(commerce_default_currency());
          break;
        default:
          // Generic field
          $this->addFieldMapping($name, $name);
      }
    }
  }

  /**
   * Create a map object for tracking the relationships between source rows.
   *
   * @return string
   */
  function createMap($entity_type, $bundle) {
    $map_class = 'entity';
    switch($entity_type) {
      case 'commerce_product':
        $source_key = array(
          'sku' => array(
            'type' => 'varchar',
            'length' => 32,
            'not null' => TRUE,
          ));
        break;
      case 'node':
        $source_key = array(
          'id' => array(
            'type' => 'int',
            'not null' => TRUE,
          ));
        break;
      default:
        $source_key = array(
          'id' => array(
            'type' => 'int',
            'not null' => TRUE,
          ));
    }

    if ($source_key) {
      switch($map_class) {
        case 'entity':
          $this->map = new MigrateSQLMap($this->machineName, $source_key,
            MigrateDestinationEntityAPI::getKeySchema($entity_type, $bundle)
          );
          break;
        case 'node':
          $this->map = new MigrateSQLMap($this->machineName,
            $source_key,
            MigrateDestinationNode::getKeySchema()
          );
          break;
        default:
      }

    }
  }

  function fields($entity_type, $bundle) {
    $fields = array();
    foreach($this->getFieldInstances($entity_type, $bundle) as $field) {
      $fields[] = array($field['field_name'] =>
        isset($field['label']) ? $field['label'] : $field['field_name']);
    }
    return $fields;
  }

  /**
   * Get the migration files path.
   *
   * @return string
   */
  function getSourceFilesPath() {
    return drupal_get_path('module', static::$cfx_module_name) . '/migrate/import/';
  }

  /**
   * Add fields that are not picked up automatically
   *
   * @return array
   */
  function getExtraFields($entity_type, $bundle) {
    $extra = array();

    // @todo: generalize for other entity types.
    switch($entity_type) {
      case 'commerce_product':
        $extra[] = array(
          'field_name' => 'sku',
          'base_field_info' => array('type' => 'text')
        );
        break;
      case 'node':
        $extra['title'] = array(
          'field_name' => 'title',
          'base_field_info' => array('type' => 'text')
        );
        break;
      case 'taxonomy_term':
        $extra['name'] = array(
          'field_name' => 'name',
          'base_field_info' => array('type' => 'text')
        );
        break;
      default:
    }

    // $extra['name'] = field_info_instance($entity_type, $field_name, $bundle_name);
    return $extra;
  }

  /**
   * Fetch field instances defined in this module.
   *
   * @return array|null
   */
  function getFieldInstances($entity_type, $bundle) {
    if($this->cfx_field_instances == NULL) {
      $this->cfx_field_instances = $this->getExtraFields($entity_type, $bundle);
      $fields = field_info_instances($entity_type, $bundle);
      // Filter out the fields we want.
      foreach($fields as $field) {
        $field['base_field_info'] = field_info_field($field['field_name']);
        $this->cfx_field_instances[] = $field;
      }
    }
    return $this->cfx_field_instances;
  }

  public function prepareRow($row) {
    foreach ($row as $field => $value) {
      $compound_field = FALSE;

      // Check if the field is json encoded.
      if (is_string($value) && substr($value, 0, 1) == '{') {
        $field_items = json_decode($value);
        // Normalize into array for single value fields
        if (is_object($field_items)) {
          $compound_field = isset($field_items->items) ? $field_items->items : array($field_items);
        }
      }
      if ($compound_field) {
        foreach ($compound_field as $index => $compound_field_item) {
          // Add each json attribute as subfield.
          foreach ($compound_field_item as $subfield => $subvalue) {
            $subfield_id = $field . $this->subfieldSeparator . $subfield;
            $row->{$subfield_id}[$index] = $subvalue;
          }
        }
      }
    }
    $row->language = LANGUAGE_NONE;
    return parent::prepareRow($row);
  }

  protected function sourceFileOptions() {
    return array(
      'header_rows' => 1,
      'delimiter' => ",",
      'enclosure' => '"',
      'embedded_newlines' => 1
    );
  }
}
