<?php
// $Id$
/**
 * @file
 * Field hooks to implement a date field.
 */

/**
 * Implements hook_field_formatter_info().
 */
function date_field_formatter_info() {
  $formatters = array(
    'date_default' => array(
      'label' => t('Default'),
      'field types' => array('date', 'datestamp', 'datetime'),
      'settings' => array(
        'show_repeat_rule' => 'show',
        'multiple_number' => '',
        'multiple_from' => '',
        'multiple_to' => '',
        'fromto' => 'both',
      ),
    ),
    'format_interval' => array(
      'label' => t('As Time Ago'),
      'field types' => array('date', 'datestamp', 'datetime'),
      'settings' => array(
      ),
    ),
  );

  $format_types = _system_date_format_types_build();
  if (!empty($format_types)) {
    foreach ($format_types as $type => $type_info) {
      $formatters[$type] = array(
        'label' => $type_info['title'],
        'field types' => array('date', 'datestamp', 'datetime'),
        'settings' => array(
          'show_repeat_rule' => 'show',
          'multiple_number' => '',
          'multiple_from' => '',
          'multiple_to' => '',
          'fromto' => 'both',
        ),
      );
    }
  }

  return $formatters;
}

/**
 * Implements hook_field_formatter_settings_form().
 */
function date_field_formatter_settings_form($field, $instance, $view_mode, $form, &$form_state) {
  $display = $instance['display'][$view_mode];
  $settings = $display['settings'];
  $formatter = $display['type'];
  $form = array();
   $form['fromto'] = array(
    '#type' => 'select',
    '#options' => array(
      'both' => t('Display both From and To dates'), 
      'value' => t('Display From date only'), 
      'value2' => t('Display To date only'),
      ),
    '#access' => $field['settings']['todate'],
    '#default_value' => $settings['fromto'],
    '#weight' => 1,
  );  

  // Make the string translatable by keeping it as a whole rather than
  // translating prefix and suffix separately.
  list($prefix, $suffix) = explode('@count', t('Show @count value(s)'));
  $form['multiple_number'] = array(
    '#type' => 'textfield',
    '#size' => 5,
    '#field_prefix' => theme('advanced_help_topic', 'date_api', 'date-display') . $prefix,
    '#field_suffix' => $suffix,
    '#default_value' => $settings['multiple_number'],
    '#weight' => 1,
    '#access' => $field['cardinality'] == FIELD_CARDINALITY_UNLIMITED ? 1 : $field['cardinality'],
    '#description' => t('Identify a specific number of values to display, or leave blank to show all values.'),
  );
  
  list($prefix, $suffix) = explode('@isodate', t('starting from @isodate'));
  $form['multiple_from'] = array(
    '#type' => 'textfield',
    '#size' => 15,
    '#field_prefix' => $prefix,
    '#field_suffix' => $suffix,
    '#default_value' => $settings['multiple_from'],
    '#weight' => 2,
    '#access' => $field['cardinality'] == FIELD_CARDINALITY_UNLIMITED ? 1 : $field['cardinality'],
  );

  list($prefix, $suffix) = explode('@isodate', t('ending with @isodate'));
  $form['multiple_to'] = array(
    '#type' => 'textfield',
    '#size' => 15,
    '#field_prefix' => $prefix,
    '#field_suffix' => $suffix,
    '#default_value' => $settings['multiple_to'],
    '#weight' => 3,
    '#access' => $field['cardinality'] == FIELD_CARDINALITY_UNLIMITED ? 1 : $field['cardinality'],
    '#description' => t('Identify specific start and/or end dates in the format YYYY-MM-DDTHH:MM:SS, or leave blank for all available dates.'),
   );
  
  $form['show_repeat_rule'] = array(
    '#type' => 'select',
    '#options' => array(
      'show' => t('Display repeat rule'), 
      'hide' => t('Hide repeat rule')),
    '#default_value' => $settings['show_repeat_rule'],
    '#access' => $field['settings']['repeat'],
  );

  return $form;
}

/**
 * Implements hook_field_formatter_settings_summary().
 */
function date_field_formatter_settings_summary($field, $instance, $view_mode) {
  $display = $instance['display'][$view_mode];
  $settings = $display['settings'];
  $formatter = $display['type'];
  $summary = array();
  $options = array(
    'both' => t('Display both From and To dates'), 
    'value' => t('Display From date only'), 
    'value2' => t('Display To date only'),
  );
  $summary[] = $options[$settings['fromto']];
  
  if (!empty($field['cardinality'])) {
    $summary[] = t('Show @count value(s) starting with @date1, ending with @date2', array(
      '@count' => !empty($settings['multiple_number']) ? $settings['multiple_number'] : t('all'), 
      '@date1' => !empty($settings['multiple_from']) ? $settings['multiple_from'] : t('earliest'), 
      '@date2' => !empty($settings['multiple_to']) ? $settings['multiple_to'] : t('latest'), 
    ));
  }
  if (!empty($field['repeat'])) {
    if (!empty($settings['show_repeat_rule'])) {
      $summary[] = t('Show repeat rule');
    }
    else {
      $summary[] = t('Do not show repeat rule');
    }
  }  

  return implode('<br />', $summary);
}


/**
 * Implements hook_field_formatter_view().
 *
 * Useful values:
 *
 *   $entity->date_id
 *     If set, this will show only an individual date on a field with
 *     multiple dates. The value should be a string that contains
 *     the following values, separated with colons:
 *     - module name of the module adding the item
 *     - node nid
 *     - field name
 *     - delta value of the field to be displayed
 *     - other information the module's custom theme might need
 *
 *     Used by the calendar module and available for other uses.
 *     example: 'date:217:field_date:3:test'
 *
 *   $entity->date_repeat_show
 *     If true, tells the theme to show all the computed values
 *     of a repeating date. If not true or not set, only the
 *     start date and the repeat rule will be displayed.
 *
 */
function date_field_formatter_view($entity_type, $entity, $field, $instance, $langcode, $items, $display) {
  $element = array();
  $settings = $display['settings'];
  $formatter = $display['type'];
  $vars = array(
    'entity' => $entity,
    'entity_type' => $entity_type,
    'field' => $field,
    'instance' => $instance,
    'langcode' => $langcode,
    'items' => $items,
    'display' => $display,
    'dates' => array(),
  );
  
  // See if we are only supposed to display a selected
  // item from multiple value date fields.
  $selected_deltas = array();
  if (!empty($entity->date_id)) {
    foreach ((array) $entity->date_id as $key => $id) {
      list($module, $nid, $field_name, $selected_delta, $other) = explode(':', $id);
      if ($field_name == $field['field_name']) {
        $selected_deltas[] = $selected_delta;
      }
    }
  }
  
  switch ($display['type']) {
    case 'format_interval':
      foreach ($items as $delta => $item) {
        if (!empty($entity->date_id) && !in_array($delta, $selected_deltas)) {
          continue;
        }
        else {
          $vars['delta'] = $delta;
          $vars['item'] = $item;
          $vars['dates'] = date_formatter_process($formatter, $entity_type, $entity, $field, $instance, $langcode, $item, $display);
          $element[$delta] = array('#markup' => theme('date_format_interval', $vars));
        }
      }
      break;
    default:
      foreach ($items as $delta => $item) {
        if (!empty($entity->date_id) && !in_array($delta, $selected_deltas)) {
          continue;
        }
        else {
          $vars['delta'] = $delta;
          $vars['item'] = $item;
          $vars['dates'] = date_formatter_process($formatter, $entity_type, $entity, $field, $instance, $langcode, $item, $display);
          $element[$delta] = array('#markup' => theme('date_display_combination', $vars));
        }
      }
      break;
  }

  return $element;
}

/**
 * Implements hook_field_is_empty().
 */
function date_field_is_empty($item, $field) {
  if (empty($item['value'])) {
    return TRUE;
  }
  elseif ($field['settings']['todate'] == 'required' && empty($item['value2'])) {
    return TRUE;
  }
  return FALSE;
}

/**
 * Implements hook_field_info().
 */
function date_field_info() {
  $settings = array(
    'settings' => array(
      'todate' => '',
      'repeat' => 0,
      'granularity' => drupal_map_assoc(array('year', 'month', 'day', 'hour', 'minute')),
      'tz_handling' => 'site',
      'timezone_db' => 'UTC',
    ),
    'instance_settings' => array(
      'default_value' => 'now',
      'default_value_code' => '',
      'default_value2' => 'blank',
      'default_value_code2' => '',
      'default_format' => 'medium',
    ),
    // Integrate with the Entity Metadata module.
    'property_type' => 'date',
    'property_callbacks' => array('date_entity_metadata_property_info_alter'),
  );
  return array(
    'date' => array(
      'label' => 'Date',
      'description' => t('Store a date in the database as an ISO date, recommended for historical or partial dates.'),
      'default_widget' => 'date_select',
      'default_formatter' => 'date_default',
      ) + $settings,
    'datestamp' => array(
      'label' => 'Datestamp',
      'description' => t('Store a date in the database as a timestamp, deprecated format to suppport legacy data.'),
      'default_widget' => 'date_select',
      'default_formatter' => 'date_default',
      ) + $settings,
    'datetime' => array(
      'label' => 'Datetime',
      'description' => t('Store a date in the database as a datetime field, recommended for complete dates and times that may need timezone conversion.'),
      'default_widget' => 'date_select',
      'default_formatter' => 'date_default',
      ) + $settings,
  );
}

/**
 * Implements hook_field_widget_info().
 */
function date_field_widget_info() {
  $settings = array(
    'settings' => array(
      'input_format' => date_default_format('date_select'),
      'input_format_custom' => '',
      'increment' => 1,
      'text_parts' => array(),
      'year_range' => '-3:+3',
      'label_position' => 'above',
      'repeat_collapsed' => 0,
    ),
    'behaviors' => array(
      'multiple values' => FIELD_BEHAVIOR_DEFAULT,
      'default value' => FIELD_BEHAVIOR_CUSTOM,
    ),
  );
  
  // Repeating dates have custom handling
  // for multiple values.
  $repeat_settings = $settings;
  $repeat_settings['behaviors']['multiple values'] = FIELD_BEHAVIOR_CUSTOM;

  $info = array(
    'date_select' => array(
      'label' =>  t('Select List'),
      'field types' => array('date', 'datestamp', 'datetime'),
    ) + $settings,
    'date_select_repeat' => array(
      'label' =>  t('Select List with Repeat options'),
      'field types' => array('date', 'datestamp', 'datetime'),
    ) + $repeat_settings,
    'date_text' => array(
      'label' =>  t('Text Field with custom input format'),
      'field types' => array('date', 'datestamp', 'datetime'),
     ) + $settings,
    'date_text_repeat' => array(
      'label' =>  t('Text Field with Repeat options'),
      'field types' => array('date', 'datestamp', 'datetime'),
    ) + $repeat_settings,
  );
  if (module_exists('date_popup')) {
    $info['date_popup'] = array(
      'label' =>  t('Text Field with Date Pop-up calendar'),
      'field types' => array('date', 'datestamp', 'datetime'),
    ) + $settings;
    $info['date_popup_repeat'] = array(
      'label' =>  t('Text Field with Date Pop-up and Repeat options'),
      'field types' => array('date', 'datestamp', 'datetime'),
    ) + $repeat_settings;
  }
  if (!module_exists('date_repeat')) {
    unset($info['date_select_repeat']);
    unset($info['date_text_repeat']);
    if (isset($info['date_popup_repeat'])) {
      unset($info['date_popup_repeat']);
    }
  }
  return $info;
}

/**
 * Implements hook_field_load().
 */
function date_field_load($entity_type, $entities, $field, $instances, $langcode, &$items, $age) {
  foreach ($entities as $id => $entity) {
    foreach ($items[$id] as $delta => $item) {
      // If the file does not exist, mark the entire item as empty.
      $timezone = isset($item['timezone']) ? $item['timezone'] : '';
      if (is_array($item)) {
        $items[$id][$delta]['timezone'] = date_get_timezone($field['settings']['tz_handling'], $timezone);
        $items[$id][$delta]['timezone_db'] = date_get_timezone_db($field['settings']['tz_handling']);
        $items[$id][$delta]['date_type'] = $field['type'];
      }
    }
  }
}

/**
 * Implements hook_field_schema().
 */
function date_field_schema($field) {
  module_load_include('inc', 'date', 'date_admin');
  return _date_field_schema($field);
}

/**
 * Wrapper functions for date administration, included only when
 * processing field settings.
 */
function date_field_instance_settings_form($field, $instance) {
  module_load_include('inc', 'date', 'date_admin');
  return _date_field_instance_settings_form($field, $instance);
}

function date_field_widget_settings_form($field, $instance) {
  module_load_include('inc', 'date', 'date_admin');
  return _date_field_widget_settings_form($field, $instance);
}

function date_field_settings_form($field, $instance, $has_data) {
  module_load_include('inc', 'date', 'date_admin');
  return _date_field_settings_form($field, $instance, $has_data);
}

function date_formatter_settings($form_state = NULL, $field, $options = array(), $views_form = FALSE) {
  module_load_include('inc', 'date', 'date_admin');
  return _date_formatter_settings($form_state, $field, $options, $views_form);
}

/**
 * Implements hook_field_validate().
 */
function date_field_validate($entity_type, $entity, $field, $instance, $langcode, &$items, &$errors) {
  module_load_include('inc', 'date', 'date_elements');
  return _date_field_validate($entity_type, $entity, $field, $instance, $langcode, $items);
}

/**
 * Implements hook_field_presave().
 */
function date_field_presave($entity_type, $entity, $field, $instance, $langcode, &$items) {
  module_load_include('inc', 'date', 'date_elements');
  return _date_field_update($entity_type, $entity, $field, $instance, $langcode, $items);
}

/**
 * Implements hook_field_update().
 */
function date_field_update($entity_type, $entity, $field, $instance, $langcode, &$items) {
  module_load_include('inc', 'date', 'date_elements');
  return _date_field_update($entity_type, $entity, $field, $instance, $langcode, $items);
}

/**
 * Implements hook_field_insert().
 */
function date_field_insert($entity_type, $entity, $field, $instance, $langcode, &$items) {
  module_load_include('inc', 'date', 'date_elements');
  return _date_field_update($entity_type, $entity, $field, $instance, $langcode, $items);
}

/**
 * Implements hook_widget().
 *
 * This code and all the processes it uses are in a separate file,
 * included only when processing forms.
 */
function date_field_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $base) {
  module_load_include('inc', 'date', 'date_elements');
  return _date_field_widget_form($form, $form_state, $field, $instance, $langcode, $items, $delta, $base);
}
