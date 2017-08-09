<?php
/**
 * @file
 * Drush commands for module Manual configuration manager.
 */

/**
 * Implements hook_drush_command().
 */
function manual_conf_manager_drush_command() {
  $items['manual-conf-manager-update-content-type'] = array(
    'description' => 'Update a given content type or a list of given content types.',
    'aliases' => array('cfmngr-uct'),
    'arguments' => array(
      'content_type' => 'A content type machine name or a list of content types machine names separated by space or \'all\'.',
    ),
    'options' => array(
      'delete-unused-fields' => 'Whether to delete the unused fields or not. Default to true.',
    ),
    'required-arguments' => 1,
    'examples' => array(
      'drush cfmngr-uct all' => 'Update all content types.',
      'drush cfmngr-uct all --delete_unused_fields=false' => 'Update all content types but do not delete fields not present in code.',
      'drush cfmngr-uct content_type_a' => 'Update content type content_type_a.',
      'drush cfmngr-uct content_type_a content_type_b content_type_c' => 'Update the listed content types.',
    ),
  );
  $items['manual-conf-manager-delete-content-type'] = array(
    'description' => 'Delete a content type',
    'aliases' => array('cfmngr-dct'),
    'arguments' => array(
      'content_type' => 'A content type machine name',
    ),
    'required-arguments' => 1,
  );

  return $items;
}

/**
 * Callback for the manual-conf-manager-update-content-type druhs command.
 *
 * @param string $content_type
 *   The content type to update.
 */
function drush_manual_conf_manager_update_content_type($content_type = NULL) {
  $content_types = func_get_args();

  $option = drush_get_option('delete-unused-fields');
  if (isset($option) && $option != 'true' && $option != 'false') {
    return drush_log(dt('delete-unused-fields value must be either true or false.'), 'error');
  }
  elseif (isset($option)) {
    $option = ($option == 'false') ? FALSE : TRUE;
  }
  else {
    $option = TRUE;
  }

  if (empty($content_types)) {
    return drush_log(dt('There were no content types that could be updated.'), 'ok');
  }
  elseif (count($content_types) === 1 && $content_type === 'all') {
    drush_print(dt('All content types will be updated'));
    if (!drush_confirm(dt('Do you really want to continue?'))) {
      return drush_user_abort();
    }

    // Do the thing.
    manual_conf_manager_update_content_type($content_type);
  }
  else {
    drush_print(dt('The following content types will be updated: !content_types', array('!content_types' => implode(', ', $content_types))));
    if (!drush_confirm(dt('Do you really want to continue?'))) {
      return drush_user_abort();
    }

    // Do the thing.
    manual_conf_manager_update_content_type($content_types, array('delete_unused_fields' => $option));
  }
}

/**
 * Callback for the manual-conf-manager-update-content-type druhs command.
 *
 * @param string $content_type
 *   The content type to delete.
 */
function drush_manual_conf_manager_delete_content_type($content_type) {
  drush_print(dt('The content type @type will be deleted, along with all the content of this type.', array('@type' => $content_type)));
  if (!drush_confirm(dt('Do you really want to continue?'))) {
    return drush_user_abort();
  }

  if (!node_type_load($content_type)) {
    drush_print(dt('The content type @type does not exists.', array('@type' => $content_type)));
    return drush_user_abort();
  }

  manual_conf_manager_batch_delete_content_type($content_type);
}

/**
 * Delete a content type.
 *
 * Initializes and launch a batch process to delete all node of a specific
 * type and delete the content type.
 */
function manual_conf_manager_batch_delete_content_type($content_type) {
  // Get all nodes from a given content type.
  $results = db_select('node', 'n')
    ->fields('n', array('nid'))
    ->condition('type', $content_type)
    ->execute();

  $nids = array();
  foreach ($results as $result) {
    $nids[] = $result->nid;
  }

  // Break up all of our data so each process does not time out.
  $chunks = array_chunk($nids, 20);
  $operations = array();
  $count_chunks = count($chunks);

  // For every chunk, assign some method to run on that chunk of data.
  $i = 0;
  foreach ($chunks as $chunk) {
    $i++;
    $operations[] = array(
      'manual_conf_manager_batch_delete_nodes',
      array(
        $chunk,
        $content_type,
        t('(Processing chunk @chunk  of  @count)', array('@chunk ' => $i, '@count' => $count_chunks)),
      ),
    );
  }

  // Put all that information into our batch array.
  $batch = array(
    'operations' => $operations,
    'title' => t('Delete content type batch'),
    'init_message' => t('Initializing'),
    'error_message' => t('An error occurred'),
    'finished' => 'manual_conf_manager_batch_delete_content_type_finished',
  );

  // Get the batch process all ready!
  batch_set($batch);
  $batch =& batch_get();

  // Because we are doing this on the back-end, we set progressive to false.
  $batch['progressive'] = FALSE;

  // Start processing the batch operations.
  drush_backend_batch_process();
}

/**
 * Batch API callback. Deletes multiple nodes.
 *
 * @param array $chunk
 *   An array of nids.
 * @param string $content_type
 *   Content type name.
 * @param string $operation_details
 *   Information about the operation.
 * @param mixed $context
 *   Is it really used?
 */
function manual_conf_manager_batch_delete_nodes(array $chunk, $content_type, $operation_details, &$context) {
  node_delete_multiple($chunk);

  // Will show what chunk we're on.
  $context['message'] = $operation_details;

  // Set content type name for use in finished function.
  $context['results']['content_type'] = $content_type;
}

/**
 * Callback for batch API finish function.
 *
 * Delete a content type and the associated variables.
 */
function manual_conf_manager_batch_delete_content_type_finished($success, $results, $operations) {
  $type = $results['content_type'];
  if (node_type_load($type)) {
    node_type_delete($type);

    variable_del('field_bundle_settings_node_' . $type);
    variable_del('language_content_type_' . $type);
    variable_del('menu_options_' . $type);
    variable_del('menu_parent_' . $type);
    variable_del('node_options_' . $type);
    variable_del('node_preview_' . $type);
    variable_del('node_submitted_' . $type);

    node_types_rebuild();
    menu_rebuild();

    drush_print(dt('Content type @type deleted!', array('@type' => $type)));
  }
}
