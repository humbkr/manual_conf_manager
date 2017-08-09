<?php
/**
 * @file
 * API documentation for the Manual Configuration Manager module.
 */

/**
 * Provides information about content type(s) configuration.
 *
 * This hook allows manual_conf_manager to know about a content type definition
 * and to update / create the content type in database accordingly.
 *
 * @return array
 *   An array containing content type(s) definition. The array is keyed with the
 *   content types machine names.
 */
function hook_manual_conf_manager_content_type_definition() {
  return array(
    // Keys are the content types machine names.
    'my_content_type' => array(
      // Human readable name (mandatory).
      'name' => t('My content type'),
      // Human readable description (optional).
      'description' => t('This is my content type.'),
      // Help text displayed in admin (optional).
      'help' => t('Help text'),
      // Label of the title field.
      'title_label' => 'Title',
      // Module containing the content type definition (mandatory).
      'module' => 'my_custom_module',
      // Sub folder containing the fields definition files (optional, default
      // to 'includes'). See README.txt for more information.
      'includes_path' => 'includes',
      // Eventual related entities used by the content type's fields. Array
      // containing field names keyed by the entity types used by the fields.
      // Common use case: content type using field_collection fields.
      // (Optional).
      'related_entities' => array(
        'field_collection_item' => 'field_collection',
      ),
      // Variables to set during installation or update of the content type.
      // Array of variables values keyed by variables names. (Optional).
      'variables' => array(
        'node_options_my_content_type' => array('status'),
        'comment_my_content_type' => 0,
        'node_submitted_my_content_type' => FALSE,
      ),
    ),
  );
}
