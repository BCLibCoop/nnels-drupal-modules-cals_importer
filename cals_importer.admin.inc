<?php
/**
 * @file
 * Define admin settings form
 */

/**
 * Implements module settings.
 *
 * Includes general settings and those specified in the selected driver.
 */
function calsauthen_admin_settings() {
  $targets = calsauthen_list_targets();
  $form['authentication_targets'] = array(
      '#prefix' => '<div>',
      '#markup' => $targets,
      '#suffix' => '</div>',
      '#weight' => -20,
  );

  $form['calsauthen_enable_logging'] = array(
    '#type' => 'checkbox',
    '#title' => t('Additional logging'),
    '#description' => t("Adds more detailed logging of ILS authentication's account management activities.  Disabled (default) logs erros only."),
    '#default_value' => variable_get('calsauthen_enable_logging', 0),
  );

  $enable_logging = variable_get('calsauthen_enable_logging', 0);
  if ($enable_logging) {
    $form['logging_warning'] = array(
      '#prefix' => '<div class="messages warning">',
      '#markup' => t('Warning: UIDs for CALS Authentication accounts are being logged. Turn off <em>Enable external account logging</em>, above, unless you are testing a driver.'),
      '#suffix' => '</div>',
    );
  }

  $form['calsauthen_show_no_mail_warning'] = array(
    '#type' => 'checkbox',
    '#title' => t("Warn user if they haven't set an email address"),
    '#description' => t("Drupal requires users to register an email addresse in their profile. If a driver is unable to get the user's email address from the external authentication source, enabling this option will display a warning until the user adds their email address to their profile."),
    '#default_value' => variable_get('calsauthen_show_no_mail_warning', 1),
    '#weight' => -20,
  );

  $form['roles'] = array(
    '#type' => 'fieldset',
    '#title' => t('Default roles for external accounts'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $roles = user_roles(TRUE);
  // Remove 'authenticated user' (rid 2), which is always the second element in
  // the roles array. We are only left with roles created by the site
  // administrator. We do this since Drupal will always add a new user to the
  // authenticated user role.
  unset($roles[2]);
  $form['roles']['calsauthen_default_roles'] = array(
    '#title' => t('Default roles'),
    '#type' => 'checkboxes',
    '#options' => $roles,
    '#default_value' => variable_get('calsauthen_default_roles', array()),
    '#description' => t('The roles that accounts created using external authentication are placed in. Does not apply to local Drupal accounts.'),
  );

/*
  $drivers = calsauthen_get_drivers();
  $form['calsauthen_driver'] = array(
    '#type' => 'select',
    '#title' => t('Driver'),
    '#default_value' => variable_get('calsauthen_driver', 'sample'),
    '#options' => $drivers,
    '#description' => t('Driver-specific settings will appear below after you have saved this form. Settings for drivers that are not selected are retained.'),
    '#weight' => -12,
  );
*/
  $form['node_types'] = array(
    '#type' => 'fieldset',
    '#title' => t('Node types'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  /*
	$form['node_types']['calsauthen_default_node_types'] = array(
		'#title' => t('node types'),
		'#type' => 'select',
		'#options' => node_type_get_types(),
		'#default_value' => variable_get('calsauthen_default_node_types', array()),
	);
	*/
  return system_settings_form($form);
}

/**
 * Get list of all drivers.
 */
function calsauthen_get_drivers() {
  $drivers = array();
  $module_dir = drupal_get_path('module', 'calsauthen');
  // Ignore *.class.inc.
  $file_options = array('nomask' => '/(\.\.?|CVS|\.class\.inc)$/');
  $driver_files = file_scan_directory($module_dir . '/drivers', '/.*\.inc$/', $file_options);

  foreach ($driver_files as $driver) {
    $drivers[$driver->name] = $driver->name;
  }
  return $drivers;
}

/**
 * limit the node types the form should show up on.
 */
function calsauthen_get_node_types() {
  $types = node_type_get_types();
  
  return $types;
}
