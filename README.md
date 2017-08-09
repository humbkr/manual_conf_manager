# Manual Configuration manager
A Drupal 7 module to manage configuration without Features.

This module provides utility functions and hooks to help manage a Drupal website
configuration without using Features / Strongarm intensively.

Drush commands are available to update the configuration without reinstalling
the site.

First, have a look at manual_conf_manager.api.php and
manual_conf_manager.drush.inc.


A. Manage content types Configuration

1. General process:
- Export content type and fields definition with Features (or another tool, or
  you can write the code from scratch, but if Features is good at one thing its
  exporting configuration, so why not use it)
- Create a custom module and put the generated code in custom files and custom
  hooks
- Use custom functions and Drush commands to manage the configuration

Pros:
- Total control over the installation and update processes allowing other
  operations to be performed automatically
- Updates can be managed very specifically in custom code, no need to revert
  an entire feature with possible side effects because the configuration has
  been changed by the client on the production
- Less mistakes when exporting / reverting a Feature and realising it didn't
  export all the fields
- No magic, allowing a better understanding of the Drupal API
- Better management of shared field bases

Cons:
- Slightly slower to manually export the configuration (but the following
  maintenance is easier)
- No interface to see if the configuration in database is different from the
  configuration in the code (but it shouldn't anyway, right?)


2. Exporting configuration
- Create a custom module with the following configuration:
    - my_module
      \- includes
        \- my_content_type.field_base.inc
        \- my_content_type.field_instance.inc
      \- my_module.info
      \- my_module.install
      \- my_module.module
- Generate what you want to export with Features interface
- Reorganise the code in your own module:

Features (w/ Strongarm) generates the following files:
  - xxx_features_field_base.inc : contains field bases definition
  - xxx_features_field_instance.inc : contains field instances definition
  - xxx.features.inc : contains the content type definition
  - xxx.strongarm.inc : contains exported variables

xxx_features_field_base.inc:
In this file, rename the function into
my_content_type_field_default_field_bases() and put it in your module's
my_content_type.field_base.inc file

xxx_features_field_instance.inc:
In this file, rename the function into
my_content_type_field_default_field_instances() and put it in your module's
my_content_type.field_instance.inc file

xxx.features.inc:
We will not use the Drupal hook, but our own, to be able to manage the content
type more precisely.
Implements hook_manual_conf_manager_content_type_definition() in your module's
.module file and report the configuration found in the my_module_node_info()
hook.
See manual_conf_manager.api.php for more information.

xxx.strongarm.inc:
Haha. Six lines to export one variable when a variable_set is enough.
In your hook_manual_conf_manager_content_type_definition(),
locate the 'variables' array and fill it with the variables name and value.
The manual_conf_manager API will do the variable_set for you.

- Implement hook_install() in your module's .install file
function my_module_install() {
  maual_conf_manager_update_content_type('my_content_type');
}

And that's all it takes.


When the configuration needs to be updated, you can export only the modified
fields / variables with Features and quickly update your custom module's code
and implement hook_update() in the same way you implemented hook_install().




--------------------------------------------------------------------------------

Tested and functional:
- manual_conf_manager_place_blocks()
- manual_conf_manager_update_field_bases()
- manual_conf_manager_update_field_instances()
- manual_conf_manager_views_update_load_views()
- manual_conf_manager_update_content_type()


Works but need more testing:
- manual_conf_manager_update_content_type_definition()



TODO:
- Add function to delete a content type with all the associated content.
- Add functions to manage vocabularies and other entities.