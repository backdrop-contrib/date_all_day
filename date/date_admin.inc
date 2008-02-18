<?php
//$Id$
/**
 * @file
 * Date administration code.
 * Moved to separate file since there is a lot of code here that is not needed often.
 */
/**
 * Implementation of hook_widget_settings().
 */
function _date_widget_settings($op, &$field) {
  switch ($op) {
    case 'callbacks':
      return array('default value' => CONTENT_CALLBACK_CUSTOM);
    case 'form':
      return date_widget_settings_form($field);

    case 'save':
      return array('default_value', 'default_value_code', 'default_value2', 'default_value_code2', 'input_format', 'input_format_custom', 'increment', 'text_parts', 'year_range', 'label_position');

    case 'validate':
      if ($field['default_value'] == 'strtotime') {
        $is_strtotime = @strtotime($field['default_value_code']);
        if (!$is_strtotime) {
          form_set_error('default_value_code', t('The Strtotime default value is invalid.'));
        }
      }
      if ($field['default_value2'] == 'strtotime') {
        $is_strtotime = @strtotime($field['default_value_code2']);
        if (!$is_strtotime) {
          form_set_error('default_value_code2', t('The Strtotime default value for the To Date is invalid.'));
        }
      }
      if ($field['widget']['type'] == 'date_select' && !preg_match('@\-[0-9]*:[\+|\-][0-9]*@', $field['year_range'])) {
        form_set_error('year_range', t('Years back and forward must be in the format -9:+9.'));
      }
      if (isset($field['advanced']['label_position'])) {
        form_set_value(array('#parents' => array('label_position')), $field['advanced']['label_position']);
      }
      // Munge the table display for text parts back into an array of text parts.
      if (is_array($field['advanced']['text_parts'])) {
        form_set_value(array('#parents' => array('text_parts')), array_keys(array_filter($field['advanced']['text_parts'])));
      }
      break;

  }
}

function date_widget_settings_form($widget) {
  $form = array();
  $form['input']['default_value'] = array(
    '#type' => 'radios', '#title' => t('Default value'),
    '#default_value' => isset($widget['default_value']) ? $widget['default_value'] : 'blank',
    '#options' => array('blank' => t('Blank'), 'now' => t('Now'), 'strtotime' => t('Relative')),
    '#description' => t("A default value to use for this field. If you select 'Relative', add details below."),
    );
  $form['input']['default'] = array(
    '#type' => 'fieldset',
    '#title' => t('Customize Default Value'),
    '#description' => t("<p>The custom value for a Relative default should be something that describes a time by reference to the current day using strtotime, like '+90 days' (90 days from the day the field is created) or '+1 Saturday' (the next Saturday). See !strtotime for more details.</p>", array('!strtotime' => l(t('strtotime'), 'http://www.php.net/manual/en/function.strtotime.php'))),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    );
  $form['input']['default']['default_value_code'] = array(
    '#type' => 'textfield', '#title' => t('Custom value for From date'),
    '#default_value' => isset($widget['default_value_code']) ? $widget['default_value_code'] : '',
    );

  $form['input']['default']['default_value2'] = array(
    '#type' => 'radios', '#title' => t('Default value for To date'),
    '#default_value' => isset($widget['default_value2']) ? $widget['default_value2'] : 'same',
    '#options' => array('same' => t('Same as From date'), 'blank' => t('Blank'), 'now' => t('Now'), 'strtotime' => t('Relative')),
    '#description' => t("A default value to use for this field. If you select 'Relative', add details below."),
    );
  $form['input']['default']['default_value_code2'] = array(
    '#type' => 'textfield', '#title' => t('Custom value for To date'),
    '#default_value' => isset($widget['default_value_code2']) ? $widget['default_value_code2'] : '',
    );

  $options = array();
  if ($widget['type'] == 'date_popup' && module_exists('date_popup')) {
    $formats = date_popup_formats();
  }
  else {
    // example input formats must show all possible date parts, so add seconds.
    $formats = str_replace('i', 'i:s', date_short_formats());
    $formats = drupal_map_assoc($formats);
  }
  $now = date_example_date();
  foreach ($formats as $f) {
    $options[$f] = date_format_date($now, 'custom', $f);
  }
  $form['input']['input_format'] = array(
    '#type' => 'select', '#title' => t('Input format'),
    '#default_value' => $widget['input_format'],
    '#options' => $options,
    '#description' => t('Set the order and format for the date parts in the input form. The format will be adapted to remove values not in the granularity for this field.'),
  );

  $form['input']['format']['input_format_custom'] = array(
    '#type' => 'textfield',  '#title' => t('*Custom input format'),
    '#default_value' => $widget['input_format_custom'] ? $widget['input_format_custom'] : '',
    '#description' => t('The custom format, if provided, will override the input format selected above. See more about custom date formats below.'),
  );
  if ($widget['type'] == 'date_select' || $widget['type'] == 'date_popup') {
    $form['input']['year_range'] = array(
      '#type' => 'textfield',
      '#title' => t('Years back and forward'),
      '#default_value' => isset($widget['year_range']) ? $widget['year_range'] : '-3:+3',
      '#size' => 10,
      '#maxsize' => 10,
      '#description' => t('Number of years to go back and forward in the year selection list, default is -3:+3.'),
      );
    $form['input']['increment'] = array(
      '#type' => 'select', '#title' => t('Time increment'),
      '#default_value' => isset($widget['increment']) ? $widget['increment'] : 1,
      '#options' => array(1 => 1, 5 => 5, 10 => 10, 15 => 15, 30 => 30),
      '#description' => t('Increment the minute and second fields by this amount.'),
    );
  }
  else {
    $form['increment'] = array(
      '#type' => 'hidden',
      '#value' => isset($widget['increment']) ? $widget['increment'] : '-3:+3',
      );
    $form['year_range'] = array(
      '#type' => 'hidden',
      '#value' => isset($widget['year_range']) ? $widget['year_range'] : 1,
      );
  }

  $form['input']['advanced'] = array(
    '#tree' => TRUE,
    '#type' => 'fieldset',
    '#title' => t('Customize Date Parts'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    );
  $form['input']['advanced']['label_position'] = array(
    '#type' => 'radios',
    '#options' => array('above' => t('Above'), 'within' => t('Within'), 'none' => t('None')),
    '#default_value' => $widget['label_position'],
    '#title' => t('Position of date part labels'),
    '#description' => t('The location of date part labels, like \'Year\', \'Month\', or \'Day\'. \'Above\' will display them as titles above each date part. \'Within\' will insert the label as the first option in the select list and in blank textfields. \'None\' will not label any of the date parts. The exact text in the label is controlled by themes like \'date_part_label_year\' and \'date_part_label_month\'.'),
    );
  $form['input']['advanced']['text_parts'] = array(
    '#theme' => $widget['type'] == 'date_select' ? 'date_text_parts_theme' : '',
    );
  foreach (date_granularity_names() as $key => $value) {
    if ($widget['type'] == 'date_select') {
      $form['input']['advanced']['text_parts'][$key] = array(
        '#type' => 'radios',
        '#default_value' => in_array($key, (array) $widget['text_parts']) ? 1 : 0,
        '#options' => array(0 => '', 1 => ''),
        );
    }
    else {
      $form['input']['advanced']['text_parts'][$key] = array(
        '#type' => 'hidden',
        '#value' => in_array($key, (array) $widget['text_parts']) ? 1 : 0,
        );
    }
  }
  return $form;
}

/**
 * Display the text/select options for date parts in a table
 * for easier readability.
 */
function theme_date_text_parts_theme($element) {
  $names = date_granularity_names();
  $rows = array();
  foreach ($names as $key => $part) {
    if ($element[$key]['#type'] == 'hidden') {
      $rows[] = drupal_render($element[$key]);
    }
    else {
      $rows[] = array($names[$key], drupal_render($element[$key][0]), drupal_render($element[$key][1]));
    }
  }
  if ($element['year']['#type'] == 'hidden') {
    return implode($rows);
  }
  else {
    $header = array(t('Input Type'), t('Select list'), t('Text field'));
    return theme('table', $header, $rows);
  }
}

/**
 * Implementation of hook_field_settings().
 */
function _date_field_settings($op, $field) {
  switch ($op) {
    case 'form':
      return date_field_settings_form($field);

    case 'validate':
      if (!in_array('year', $field['granularity'])) {
        form_set_error('granularity', t('Input granularity must include a year.'));
      }
      if ($field['tz_handling'] != 'none' && !in_array('hour', array_filter($field['granularity']))) {
        form_set_error('tz_handling', t('Dates without hours granularity must not use any timezone handling.'));
      }
      break;

    case 'save':

      $options = array('granularity', 'timezone_db', 'tz_handling', 'todate');
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
        $options[] = 'output_format_custom'. $append;
      }
      return $options;

    case 'database columns':
      return date_columns($field);

    case 'arguments':
      include_once('./'. drupal_get_path('module', 'date') .'/date_views.inc');
      return _date_views_arguments($field);

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
    $db_columns['value'] = array('type' => 'varchar', 'length' => 20, 'not null' => FALSE, 'sortable' => TRUE);
  }
  elseif ($field['type'] == 'datestamp') {
    $db_columns['value'] = array('type' => 'integer', 'length' => 11, 'not null' => FALSE, 'sortable' => TRUE);
  }
  // If a second date is needed for 'To date', just make a copy of the first one.
  if ($field['todate']) {
    $db_columns['value2'] = $db_columns['value'];
  }
  // timezone and offset columns are used only if date-specific dates are chosen.
  if ($field['tz_handling'] == 'date') {
    $db_columns['timezone'] = array('type' => 'varchar', 'length' => 50, 'not null' => FALSE, 'sortable' => TRUE);
    $db_columns['offset'] = array('type' => 'integer', 'length' => 10, 'not null' => FALSE, 'sortable' => TRUE);
    if ($field['todate']) $db_columns['offset2'] = array('type' => 'integer', 'length' => 10, 'not null' => FALSE, 'sortable' => TRUE);
  }
  if ($field['multiple'] == 99) {
    $db_columns['rrule'] = array('type' => 'text', 'not null' => FALSE, 'sortable' => FALSE);
  }
  return $db_columns;
}

function date_field_settings_form($field) {
  $form = array();
  $tz_handling = $field['tz_handling'] ? $field['tz_handling'] : (date_has_time($field['granularity']) ? 'site' : 'none');

  // Override the normal multiple checkbox when using date repeat.
  $options = array(0 => t('Never'), 1 => t('Unlimited'));
  $description = t('Choose an option for handling multiple values. Unlimited will allow the user to manually create unlimited multiple dates one date at a time.');

  if (module_exists('date_repeat')) {
    $options += array(99 => t('Repeating'));
    $description .= t(' Repeating dates display options to allow the user to select when and how often the date will repeat.');
  }
  $form['multiple'] = array(
    '#type' => 'radios',
    '#title' => t('Multiple'),
    '#options' => $options,
    '#default_value' => $field['multiple'],
    '#description' => $description,
  );

  $form['input']['todate'] = array(
    '#type' => 'radios', '#title' => t('To Date'),
    '#options' => array('' => t('Never'), 'optional' => t('Optional'), 'required' => t('Required')),
    '#description' => t("Display a matching second date field as a 'To date'. If marked 'Optional' field will be presented but not required. If marked 'Required' the 'To date' will be required if the 'From date' is required or filled in."),
    '#default_value' => $field['todate'] ? $field['todate'] : '',
    );
  $form['input']['granularity'] = array(
    '#type' => 'checkboxes', '#title' => t('Granularity'),
    '#default_value' => $field['granularity'] ? $field['granularity'] : array('year', 'month', 'day', 'hour', 'minute'),
    '#options' => date_granularity_names(),
    '#multiple' => TRUE,
    '#size' => min(count($options), 6),
    '#description' => t('Set the date elements to be stored (at least a year is required).'),
    );

  $form['output']['simple'] = date_formatter_setup_form($field, 0);
  $form['output']['simple']['#title'] = t('Default Display');

  $form['output']['advanced'] = array(
    '#type' => 'fieldset', '#title' => t('Additional Display Settings'),
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
    '#description' => t('Select the timezone handling method to be used for this date field.'),
  );
  // Force this value to hidden because we don't want to allow it to be changed right now,
  // but allow it to be a variable if needed.
  $form['timezone']['timezone_db'] = array(
    '#type' => 'hidden',
    '#value' => 'UTC',
    );

  $form['#suffix'] = t('<div class="form-item"><div class="description">* The custom format, if provided, will override the selected display or input options. Define a php date format string like \'m-d-Y H:i\' (see !link for more details).</div></div>', array('!link' => l('http://php.net/date', 'http://php.net/date')));

  return $form;
}

/**
 *  A form to create a date formatter option
 */
function date_formatter_setup_form($field, $delta) {

  switch ($delta) {
  case(1):
    $name = 'long';
    $label = t('Long');
    $default = variable_get('date_format_long',  'l, F j, Y - H:i');
    break;
  case(2):
    $name = 'medium';
    $label = t('Medium');
    $default = variable_get('date_format_medium',  'D, m/d/Y - H:i');
    break;
  case(3):
    $name = 'short';
    $label = t('Short');
    $default = variable_get('date_format_short', 'm/d/Y - H:i');
    break;
  default:
    $name = 'default';
    $label = t('Default');
    $default = variable_get('date_format_short', 'm/d/Y - H:i');
  }
  $append = ($delta > 0 ? '_'. $name : '');
  $form = array(
    '#type' => 'fieldset', '#title' => $label,
    );

  $form['output_format_date'. $append] = array(
    '#type' => 'select', '#title' => t('Date display'),
    '#default_value' => $field['output_format_date'. $append] ? $field['output_format_date'. $append] : $default,
    '#options' => date_format_options(),
    '#multiple' => false,
  );
  $form['output_format_custom'. $append] = array(
    '#type' => 'textfield', '#title' => t('*Custom display format'),
    '#default_value' => $field['output_format_custom'. $append] ? $field['output_format_custom'. $append] : '',
  );
  return $form;
}

/**
 * Store personalized format options for each user.
 *
 * Store to avoid work of re-creating it over and over,
 * personalize for each user to preserve translation.
 *
 * @return array
 */
function date_format_options() {
  global $user;
  $cached = cache_get('date_format_options:'. $user->uid);
  $options = unserialize($cached->data);
  if (empty($options)) {
    $formats = array_merge(date_short_formats(), date_medium_formats(), date_long_formats());
    $options = array();
    $now = date_example_date();
    foreach ($formats as $format) {
      // Create an option that shows date only without time, along with the
      // default string which has both date and time.
      $no_time = date_limit_format($format, array('month', 'day', 'year'));
      $zones = array('', 'O', 'P', 'e');
      foreach ($zones as $zone) {
        $time_format = !empty($zone) ? $format .' '. $zone : $format;
        $options[$no_time] = date_format_date($now, 'custom', $no_time);
        $options[$time_format] = date_format_date($now, 'custom', $time_format);
      }
    }
    asort($options);
    cache_set('date_format_options:'. $user->uid, 'cache', serialize($options), CACHE_TEMPORARY);
  }
  return $options;
}

/**
 *  Timezone handling options
 *
 *  the 'none' option will do no timezone conversions and will store and display dates exactly as entered
 *  useful in locales or situations where timezone conversions are not working reliably,
 *  for dates with no times, for historical dates where timezones are irrelevant,
 *  or anytime conversion is unnecessary or undesirable
 */
function date_timezone_handling_options() {
  return array(
    'site' => t('Site\'s time zone'),
    'date' => t('Date\'s time zone'),
    'user' => t('User\'s time zone'),
    'gmt' => 'UTC',
    'none' => t('No time zone conversion'),
    );
}

/**
 * Get an example date and make sure the difference between
 * month and day and 12 and 24 hours will be clear.
 */
function date_example_date() {
  $now = date_now();
  if (date_format($now, 'm') == date_format($now, 'd')) {
    date_modify($now, '+1 day');
  }
  if (date_format($now, 'H') == date_format($now, 'h')) {
    date_modify($now, '+12 hours');
  }
  return $now;
}