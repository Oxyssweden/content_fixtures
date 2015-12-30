<?php

/**
 * Class ContentFixturesMenusMigration
 *
 * copied from https://gist.github.com/abulte/6898511 with alterations.
 */
abstract class ContentFixturesMenuLinkMigration extends Migration {
  public function __construct($arguments = array()) {
    parent::__construct($arguments);
    $this->description = t('Creates sample menu links.');

    // the csv file contains the header.
    $path = $this->getSourceFilesPath() . 'menu.csv';
    $this->source = new MigrateSourceCSV($path, array(), array('header_rows' => 1));

    $this->destination = new MigrateDestinationMenuLinks();

    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'mlid' => array(
          'type' => 'int',
          'unsigned' => TRUE,
          'not null' => TRUE,
          'description' => 'ID of destination link',
        ),
      ),
      MigrateDestinationMenuLinks::getKeySchema()
    );


    $this->addFieldMapping('menu_name', 'menu')->defaultValue('main-menu'); // Always Main menu
    $this->addFieldMapping('plid', 'ref_parent')->sourceMigration($this->getMachineName());
    $this->addFieldMapping('link_path', 'path');          // path of the link
    $this->addFieldMapping('router_path')->defaultValue('node/%');
    $this->addFieldMapping('link_title', 'title');        // Title of the menu item
    $this->addFieldMapping('external')->defaultValue('0'); // Internal
    $this->addFieldMapping('expanded')->defaultValue('1');
    $this->addFieldMapping('weight','weight');            // weight
    $this->addFieldMapping('customized')->defaultValue('1'); // not customized
    $this->addFieldMapping('has_children')->defaultValue('0');  // Will be overridden automatically
    $this->addFieldMapping('depth')->defaultValue('1'); // Will be overridden automatically

    $this->addUnmigratedDestinations(array('module', 'hidden','options','p1', 'p2', 'p3', 'p4', 'p5', 'p6', 'p7', 'p8', 'p9', 'updated'));

  }

  /**
   * Get the migration files path.
   *
   * @return string
   */
  function getSourceFilesPath() {
    return drupal_get_path('module', static::$cfx_module_name) . '/migrate/import/';
  }

  public function prepareRow($current_row) {
    // Always start your prepareRow implementation with this clause. You need to
    // be sure your parent classes have their chance at the row, and that if
    // they return FALSE (indicating the row should be skipped) you pass that
    // on.
    if (parent::prepareRow($current_row) === FALSE) {
      return FALSE;
    }

    $current_row->path = drupal_get_normal_path($current_row->path);

    return TRUE;
  }

/**
   * <a href="http://drupal.org/node/1403044#comment-5790748
" title="http://drupal.org/node/1403044#comment-5790748
">http://drupal.org/node/1403044#comment-5790748
</a>   * Creates a stub menu link, for when a child is imported before its parent
   *
   * @param $migration
   *  The source migration
   * @return
   *  int $mlid on success
   *  FALSE on failure
   */
  protected function createStub($migration) {
    // if ref_parent is 0, that means it has no parent, so don't create a stub
    if (!$migration->sourceValues->ref_parent) {
      return FALSE;
    }
    $menu_link = array (
      'menu_name' => $migration->sourceValues->menu_name,
      'link_path' => 'stub-path',
      'router_path' => 'stub-path',
      'link_title' => t('Stub title'),
    );
    $mlid = menu_link_save($menu_link);
    if ($mlid) {
      return array($mlid);
    }
    else {
      return FALSE;
    }
  }
}
