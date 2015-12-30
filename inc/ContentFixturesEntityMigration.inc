<?php

abstract class ContentFixturesEntityMigration extends Migration {

  private $cfx_field_instances = NULL;

  private $subfieldSeparator = '__';

  public function __construct($arguments = array()) {
    parent::__construct($arguments);
    $this->description = t('Import @entity from CSV file.', array('@entity' => static::$cfx_entity_type . '-' . static::$cfx_bundle));

    // Create a map object for tracking the relationships between source rows
    $this->createMap();

    // Create a MigrateSource object.
    $path = $this->getSourceFilesPath() . static::$cfx_entity_type . '-' . static::$cfx_bundle . '.csv';
    $this->source = new MigrateSourceCSV($path, array(), $this->sourceFileOptions(), $this->fields());

    switch(static::$cfx_entity_type) {
      case 'taxonomy_term':
        $this->destination = new MigrateDestinationTerm(static::$cfx_bundle);
        break;
      default:
        $this->destination = new MigrateDestinationEntityAPI(static::$cfx_entity_type, static::$cfx_bundle);
        break;
    }

    // Field Mappings

    foreach($this->getFieldInstances() as $field) {
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
        // @todo: entityreference fields
        //case 'entityreference':
          // Link with breweries, based on the ID
          //$this->addFieldMapping('field_trappist_brewery', 'brewery')
          //->sourceMigration('MZBrewery');
         // break;
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

    // Author, default to admin
    $this->addFieldMapping('uid')->defaultValue(1);
  }

  /**
   * Create a map object for tracking the relationships between source rows.
   *
   * @return string
   */
  function createMap() {
    $map_class = 'entity';
    switch(static::$cfx_entity_type) {
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
        //$map_class = 'node';
        break;
      default:
        $source_key = array(
          'id' => array(
            'type' => 'int',
            'not null' => TRUE,
          ));
        //$source_key = MigrateDestinationEntityAPI::getKeySchema(static::$cfx_entity_type, static::$cfx_bundle);
    }

    if ($source_key) {
      switch($map_class) {
        case 'entity':
          $this->map = new MigrateSQLMap($this->machineName, $source_key,
            MigrateDestinationEntityAPI::getKeySchema(static::$cfx_entity_type, static::$cfx_bundle)
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

  function fields() {
    $fields = array();
    foreach($this->getFieldInstances() as $field) {
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
   * Returns CSV column definitions.
   *
   * @return mixed
   */
  function csvcolumns() {
    $i = 0;
    $columns = array();
    foreach($this->getFieldInstances() as $field) {
      $columns[$i] = array($field['field_name'], $field['field_name']);
      $i++;
    }
    return $columns;
  }

  /**
   * Add fields that are not picked up automatically
   *
   * @return array
   */
  function getExtraFields() {
    $extra = array();

    // Mock field for label
    // @todo: generalize for other entity types.
    // $types = entity_get_info(static::$cfx_entity_type);
    // $fields = field_info_extra_fields(static::$cfx_entity_type, static::$cfx_bundle, 'form');


    switch(static::$cfx_entity_type) {
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
  function getFieldInstances() {
    if($this->cfx_field_instances == NULL) {
      $this->cfx_field_instances = $this->getExtraFields();
      $fields = field_info_instances(static::$cfx_entity_type, static::$cfx_bundle);
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
    //$row->product_images = explode(', ', $row->images);
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
