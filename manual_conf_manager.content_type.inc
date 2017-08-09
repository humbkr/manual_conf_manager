<?php
/**
 * @file
 * Code for Content Types management.
 */

/**
 * Create or update a content type definition (everything but fields).
 *
 * @param string $machine_name
 *   The content type machine name.
 * @param array $definition
 *   The content type definition.
 */
function manual_conf_manager_update_content_type_definition($machine_name, array $definition) {
  $configuration = manual_conf_manager_parse_content_type_definition($definition);

  $existing_types = node_type_get_names();
  if (!array_key_exists($machine_name, $existing_types)) {
    // Create new content type.
    $type = array(
      'type' => $machine_name,
      'name' => $configuration['name'],
      'base' => 'node_content',
      'custom' => 1,
      'modified' => 1,
      'locked' => 0,
    );

    if (isset($configuration['description'])) {
      $type['description'] = $configuration['description'];
    }
    if (isset($configuration['title_label'])) {
      $type['title_label'] = $configuration['title_label'];
    }
    if (isset($configuration['help'])) {
      $type['help'] = $configuration['help'];
    }

    $type = node_type_set_defaults($type);
    node_type_save($type);

    // Save content type options.
    foreach ($configuration['variables'] as $var_name => $var_value) {
      variable_set($var_name, $var_value);
    }

    watchdog('manual_conf_manager', 'Created content type ' . $machine_name, array(), WATCHDOG_INFO);
  }
  else {
    $type = node_type_load($machine_name);
    $type->name = $configuration['name'];

    if (isset($configuration['description'])) {
      $type->description = $configuration['description'];
    }
    if (isset($configuration['title_label'])) {
      $type->title_label = $configuration['title_label'];
    }
    if (isset($configuration['help'])) {
      $type->help = $configuration['help'];
    }

    node_type_save($type);

    // Save content type options.
    foreach ($configuration['variables'] as $var_name => $var_value) {
      variable_set($var_name, $var_value);
    }

    watchdog('manual_conf_manager', 'Updated content type ' . $machine_name, array(), WATCHDOG_INFO);
  }
}

/**
 * Update content types with latest configuration from code.
 *
 * @param mixed $machine_name
 *   Either: An array of content types machine_names , an content type
 *   machine_name as a string, or "all" to update all content types.
 * @param array $options
 *   A list of options. Available options:
 *   - delete_unused_fields: is set to TRUE, delete the field instances
 *     present in database but not in code. Default to TRUE.
 */
function manual_conf_manager_update_content_type($machine_name, array $options = array()) {
  if (isset($machine_name) && $machine_name != '') {
    // Get custom content types definitions.
    $content_types = module_invoke_all('manual_conf_manager_content_type_definition');

    // Get content types to update.
    $to_update = array();
    if (is_array($machine_name)) {
      foreach ($content_types as $key => $item) {
        if (in_array($key, $machine_name)) {
          $to_update[$key] = $item;
        }
      }
    }
    elseif ($machine_name === 'all') {
      $to_update = $content_types;
    }
    elseif (isset($content_types[$machine_name])) {
      $to_update[$machine_name] = $content_types[$machine_name];
    }

    // Update the selected content types.
    foreach ($to_update as $key => $content_type) {
      // Update content type definition.
      manual_conf_manager_update_content_type_definition($key, $content_type);

      // Update fields.
      $module_path = drupal_get_path('module', $content_type['module']);

      if (file_exists($module_path . '/' . $content_type['includes_path'] . '/' . $key . '.field_base.inc')) {
        module_load_include('inc', $content_type['module'], $content_type['includes_path'] . '/' . $key . '.field_base');
        $field_bases = call_user_func($key . '_field_default_field_bases');
        manual_conf_manager_update_field_bases($field_bases);
      }
      else {
        watchdog('manual_conf_manager', 'Could not find a field base definition file for content type ' . $key . '.');
      }

      if (file_exists($module_path . '/' . $content_type['includes_path'] . '/' . $key . '.field_instance.inc')) {
        module_load_include('inc', $content_type['module'], $content_type['includes_path'] . '/' . $key . '.field_instance');
        $field_instances = call_user_func($key . '_field_default_field_instances');

        // Remove field instances that are in database but not in the code.
        if (!isset($options['delete_unused_fields']) || $options['delete_unused_fields'] === TRUE) {
          manual_conf_manager_delete_unused_field_instances($field_instances, 'node', $key);
          foreach ($content_type['related_entities'] as $entity_type => $entity_bundle) {
            manual_conf_manager_delete_unused_field_instances($field_instances, $entity_type, $entity_bundle);
          }
        }

        manual_conf_manager_update_field_instances($field_instances);
      }
      else {
        watchdog('manual_conf_manager', 'Could not find a field instance definition file for content type ' . $key . '.');
      }
    }

    cache_clear_all();
  }
}

/**
 * Parse a content type definition and add default values.
 *
 * @param array $definition
 *   The content type definition.
 *
 * @return array
 *   The parsed content type definition.
 *
 * @throws Exception
 *   If a mandatory parameter is missing.
 */
function manual_conf_manager_parse_content_type_definition(array $definition) {
  $configuration = $definition;

  // Check mandatory parameters are set.
  $required = array(
    'name',
    'module',
  );
  foreach ($required as $item) {
    if (!isset($definition[$item]) || $definition[$item] == '') {
      throw new Exception(t('Missing parameter in content type definition : @param.', array('@param' => $item)));
    }
  }

  // Set defaults.
  if (!isset($definition['includes_path'])) {
    $configuration['includes_path'] = 'includes';
  }
  if (!isset($definition['variables'])) {
    $configuration['variables'] = array();
  }
  if (!isset($definition['related_entities'])) {
    $configuration['related_entities'] = array();
  }
  if (!isset($definition['title_label'])) {
    $configuration['title_label'] = 'Title';
  }

  return $configuration;
}

/**
 * Delete field instances not present in code from a content type.
 *
 * @param array $field_instances
 *   The field instances defined in the code.
 * @param string $type
 *   The entity type these instances are related to.
 * @param string $bundle
 *   The entity bundle these instances are related to.
 * @param bool $clean_fields
 *   If TRUE, also delete the field bases if not used anymore. Default to FALSE.
 */
function manual_conf_manager_delete_unused_field_instances(array $field_instances, $type, $bundle, $clean_fields = FALSE) {
  $field_instances_in_db = field_info_instances($type, $bundle);

  foreach ($field_instances as $field_instance) {
    if ($field_instance['entity_type'] === $type && $field_instance['bundle'] === $bundle) {
      if (isset($field_instances_in_db[$field_instance['field_name']])) {
        unset($field_instances_in_db[$field_instance['field_name']]);
      }
    }
  }

  // We now have in $field_instances_in_db only fields that are not in code.
  foreach ($field_instances_in_db as $field_instance_in_db) {
    // Delete instance without deleting base.
    field_delete_instance($field_instance_in_db, $clean_fields);

    watchdog('manual_conf_manager', 'Instance of field ' . $field_instance_in_db['field_name'] .
      ' in ' . $field_instance_in_db['entity_type'] . '-' . $field_instance_in_db['bundle'] . ' has been deleted.', array(), WATCHDOG_INFO);
  }
}
