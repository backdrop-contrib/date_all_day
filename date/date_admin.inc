<?php
//$Id$
/**
 * @file
 * Date administration code.
 * Moved to separate file since there is a lot of code here that is not needed often.
 */

/**
 * Implementation of hook_widget_info().
 */
function _date_widget_info() {
  $info = array(
    'date_select' => array('label' => t('Select List'),
      'field types' => array('date', 'datestamp'),
    ),
    'date_text' => array('label' => t('Text Field with strtotime validation'),
      'field types' => array('date', 'datestamp'),
    ),
  );
  if (module_exists('jscalendar')) {
    $info['date_js'] = array('label' => t('Text Field with javascript pop-up calendar'),
       'field types' => array('date', 'datestamp'),
     );
  }
  if (module_exists('date_popup')) {
    $info['date_popup'] = array('label' => t('Text Field with jquery pop-up calendar'),
       'field types' => array('date', 'datestamp'),
     );
  }
  return $info;
}

/**
 * Implementation of hook_widget_settings().
 */
function _date_widget_settings($op, $widget) {
  switch ($op) {
    case 'callbacks':
      return array('default value' => CONTENT_CALLBACK_CUSTOM);
    case 'form':
      include_once(drupal_get_path('module', 'date') .'/date_admin.inc');
      return date_widget_form($widget);

    case 'save':
      return array('default_value', 'default_value_custom', 'input_format', 'input_format_custom', 'increment', 'text_parts', 'years_back', 'years_forward');

    case 'validate':
      if ($field['default_value'] == 'custom') {
        $is_strtotime = @strtotime($field['default_value_custom']);
        if (!$is_strtotime) {
          form_set_error('default_value_custom', t('The custom default value is invalid.'));
        }
      }
      if (!form_get_errors()) {
        $text_parts = array();
        if (empty($widget['select_year'])) {
          $text_parts[] = 'year';
        }
        if (empty($widget['select_month'])) {
          $text_parts[] = 'mon';
        }
        if (empty($widget['select_day'])) {
          $text_parts[] = 'mday';
        }
        form_set_value(array('#parents' => array('text_parts')), $text_parts);
      }
      break;

  }
}

function date_widget_form($widget) {
  $form = array();
  $form['default_value'] = array(
    '#type' => 'select', '#title' => t('Default value'),
    '#default_value' => isset($widget['default_value']) ? $widget['default_value'] : 'blank',
    '#options' => array('blank' => t('Blank'), 'now' => t('Now'), 'custom' => t('Custom')),
    '#description' => t("A default value to use for this field. If you select 'custom', add details to the box below."),
    );
  $form['default'] = array(
    '#type' => 'fieldset', '#title' => t('Customize default value'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    );
  $form['default']['default_value_custom'] = array(
    '#type' => 'textfield', '#title' => t('Custom default value'),
    '#default_value' => isset($widget['default_value_custom']) ? $widget['default_value_custom'] : '',
    '#description' => t("A custom default value should be something that describes a time by reference to the current day using strtotime, like '+90 days' (90 days from the day the field is created) or '+1 Saturday' (the next Saturday). See !strtotime for more details.", array('!strtotime' => l(t('strtotime'), 'http://www.php.net/manual/en/function.strtotime.php'))),
    );

  $options = array();
  if ($widget['type'] == 'date_popup') {
    $formats = date_popup_formats();
  }
  else {
    $formats = drupal_map_assoc(date_short_formats());
    $options['site-wide'] = t('Site default');
  }
  foreach ($formats as $f) {
    $options[$f] = format_date(time(), 'custom', $f);
  }
  $form['input']['input_format'] = array(
    '#type' => 'select', '#title' => t('Input format'),
    '#default_value' => $widget['input_format'] ? $widget['input_format'] : 'site_format',
    '#options' => $options,
    '#description' => t('Set an order and format for the date parts in the input form. The format will be adapted to remove values not in the granularity for this field.'),
  );
  $form['input']['format'] = array(
    '#type' => 'fieldset', '#title' => t('Customize input format'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    );
  $form['input']['format']['input_format_custom'] = array(
    '#type' => 'textfield',  '#title' => t('*Custom input format'),
    '#default_value' => $widget['input_format_custom'] ? $widget['input_format_custom'] : '',
    '#description' => t('The custom format, if provided, will override the input format selected above. See more about custom date formats below.'),
  );

  if ($widget['type'] == 'date_select') {
    if (!is_array($widget['text_parts'])) {
      $widget['text_parts'] = array();
    }
    $form['input']['advanced'] = array(
      '#type' => 'fieldset', '#title' => t('Customize select options'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#weight' => -1,
      );
    $form['text_parts'] = array(
      '#type' => 'hidden',
      '#value' => $widget['text_parts'],
      );
    $form['input']['advanced']['select_day'] = array(
      '#type' => 'select', '#title' => t('Days'),
      '#default_value' => in_array('mday', $widget['text_parts']) ? 0 : 1,
      '#options' => array(0 => t('text field'), 1 => t('select list')),
      '#description' => t('Type of form to use for day selection.'),
      );
    $form['input']['advanced']['select_month'] = array(
      '#type' => 'select', '#title' => t('Months'),
      '#default_value' => in_array('mon', $widget['text_parts']) ? 0 : 1,
      '#options' => array(0 => t('text field'), 1 => t('select list')),
      '#description' => t('Type of form to use for month selection.'),
      );
    $form['input']['advanced']['select_year'] = array(
      '#type' => 'select', '#title' => t('Years'),
      '#default_value' => in_array('year', $widget['text_parts']) ? 0 : 1,
      '#options' => array(0 => t('text field'), 1 => t('select list')),
      '#description' => t('Type of form to use for year selection.'),
      );
    $form['input']['advanced']['years_back'] = array(
      '#type' => 'textfield',
      '#title' => t('Years back'),
      '#default_value' => isset($widget['years_back']) ? $widget['years_back'] : 3,
      '#size' => 2,
      '#maxsize' => 2,
      '#description' => t('Number of years to go back when using a selection list.'),
      );
    $form['input']['advanced']['years_forward'] = array(
      '#type' => 'textfield', '#title' => t('Years forward'),
      '#default_value' => isset($widget['years_forward']) ? $widget['years_forward'] : 3,
      '#size' => 2,
      '#maxsize' => 2,
      '#description' => t('Number of years to go forward when using a selection list.'),
      );
    $form['input']['advanced']['increment'] = array(
      '#type' => 'select', '#title' => t('Time increment'),
      '#default_value' => isset($widget['increment']) ? $widget['increment'] : 1,
      '#options' => array(1 => 1, 5 => 5, 10 => 10, 15 => 15, 30 => 30),
      '#description' => t('Increment the minute and second fields by this amount.'),
    );
    foreach ($form['input']['advanced'] as $key => $element) {
      if (!strstr($key, '#')) {
        $current[] = $element['#title'] .' = '. (isset($element['#options']) ? $element['#options'][$element['#default_value']] : $element['#default_value']);
      }
    }
    $form['input']['summary'] = array(
      '#value' => theme('form_element', array('#title' => t('Select options')), implode(', ', $current)),
      '#weight' => -2,
      );
  }
  return $form;
}

/**
 * Implementation of hook_field_info().
 */
function _date_field_info() {
  return array(
    'date' => array('label' => 'Date'),
    'datestamp' => array('label' => 'Datestamp'),
  );
}

/**
 * Implementation of hook_field_settings().
 */
function _date_field_settings($op, $field) {
  switch ($op) {
    case 'form':
      return date_field_form($field);

    case 'validate':
      if (!in_array('Y', $field['granularity'])) {
        form_set_error('granularity', t('Input granularity must include a year.'));
      }
      if ($field['tz_handling'] != 'none' && !in_array('H', array_filter($field['granularity']))) {
        form_set_error('tz_handling', t('Dates without hours granularity must not use any timezone handling.'));
      }
      break;

    case 'save':

      $options = array('granularity', 'field_timezone', 'tz_handling', 'todate');
      for ($i = 0; $i <= 3; $i++) {
        switch ($i) {
        case(1):
          $name = 'long';
          break;
        case(2):
          $name = 'medium';
          break;
        case(3):
          $name = 'short';
          break;
        default:
          $name = 'default';
        }
        $append = ($i > 0 ? '_'. $name : '');
        $options[] = 'output_format_date'. $append;
        $options[] = 'output_format_zone'. $append;
        $options[] = 'output_format_custom'. $append;
      }
      return $options;

    case 'database columns':
      return date_columns($field);

    case 'arguments':
      return date_views_arguments($field);

    case 'filters':
      return date_views_filters($field);
      break;
  }
}

/**
 *  Callback for field columns.
 */
function date_columns($field) {
  if ($field['type'] == 'date') {
    $db_columns['value'] = array('type' => 'varchar', 'length' => 20, 'not null' => FALSE, 'default' => NULL, 'sortable' => TRUE);
  }
  elseif ($field['type'] == 'datestamp') {
    $db_columns['value'] = array('type' => 'integer', 'length' => 11, 'not null' => FALSE, 'default' => NULL, 'sortable' => TRUE);
  }
  // If a second date is needed for 'To date', just make a copy of the first one.
  if ($field['todate']) {
    $db_columns['value2'] = $db_columns['value'];
  }
  // timezone and offset columns are used only if date-specific dates are chosen.
  if ($field['tz_handling'] == 'date' && ($field['type'] == 'date' || $field['type'] == 'datestamp')) {
    $db_columns['timezone'] = array('type' => 'varchar', 'length' => 50, 'not null' => FALSE, 'default' => NULL, 'sortable' => TRUE);
    $db_columns['offset'] = array('type' => 'integer', 'length' => 10, 'not null' => FALSE, 'default' => NULL, 'sortable' => TRUE);
  }
  return $db_columns;
}

function date_field_form($field) {
  $form = array();
  $tz_handling = $field['tz_handling'] ? $field['tz_handling'] : 'site';
  $form['input']['todate'] = array(
    '#type' => 'select', '#title' => t('To Date'),
    '#options' => array('' => t('Never'), 'optional' => t('Optional'), 'required' => t('Required')),
    '#description' => t("Display a matching second date field as a 'To date'. If marked 'Optional' field will be presented but not required. If marked 'Required' the 'To date' will be required if the 'From date' is required or filled in."),
    '#default_value' => $field['todate'] ? $field['todate'] : '',
    );
  $form['input']['granularity'] = array(
    '#type' => 'select', '#title' => t('Granularity'),
    '#default_value' => $field['granularity'] ? $field['granularity'] : array('Y', 'M', 'D'),
    '#options' => date_granularity_names(),
    '#multiple' => TRUE,
    '#size' => min(count($options), 6),
    '#description' => t('Set the date elements to be collected on input (at least a year is required).'),
    );

  $default_formatter = date_formatter_setup_form($field, 0);
  $form['output'][0] = $default_formatter;
  $form['output'][0]['#type'] = '';
  $form['output']['advanced'] = array(
    '#type' => 'fieldset', '#title' => t('Customize display options'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#description' => t('Define alternate formatting for the date display. Options other than the default are made available to views and themes. Possible formats are default, long, medium, and short.'),
  );
  for ($i = 1; $i <= 3; $i++) {
    $form['output']['advanced'][$i] = date_formatter_setup_form($field, $i);
  }
  $form['timezone']['tz_handling'] = array(
    '#type' => 'select',
    '#title' => t('Time zone handling'),
    '#default_value' => $tz_handling,
    '#options' => date_timezone_handling_options(),
    '#description' => t('Select the timezone handling method to be used for this date field'),
  );
  $form['#suffix'] = t('<div class="form-item"><div class="description">* The custom format, if provided, will override the selected display or input options. Define a php date format string like \'m-d-Y H:i\' (see !link for more details). Do not add time zone formatting to the custom format, it will not work correctly. Use the zone display option instead.</div></div>', array('!link' => l('http://php.net/date', 'http://php.net/date')));

  return $form;
}