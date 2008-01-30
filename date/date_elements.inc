<?php
//$Id$
/**
 * @file
 * Date forms and form themes and validation.
 *
 * All code used in form editing and processing is in this file,
 * included only during form editing.
 */

/**
 * Private implementation of hook_field validate operation.
 */
function _date_field_validate($op, &$node, $field, &$items, $teaser, $page) {
  $field_name = $field['field_name'];

  // Don't try to validate if there were any errors before this point
  // since the element won't have been munged back into a date.
  if (!form_get_errors()) {
    foreach ($items as $delta => $item) {
      $process = date_process_values($field);
      foreach ($process as $processed) {
        $error_field = $field['field_name'] .']['. $delta .']['. $processed;
        $error_field .= $field['widget']['type'] == 'date_select' ? '][year' : '';
        if ($processed == 'value' && $field['todate']
          && !date_is_valid($item['value'], $field['type'])
          && (date_is_valid($item['value2'], $field['type']))) {
          form_set_error($error_field, t("A 'From date' date is required for %field %delta", array('%delta' => $field['multiple'] ? intval($delta + 1) : '', '%field' => t($field['widget']['label']))));
        }
        if ($processed == 'value2'
          && $field['todate'] == 'required' && ($field['required']
          && date_is_valid($item['value'], $field['type'])
          && !date_is_valid($item['value2'], $field['type']))) {
          form_set_error($error_field, t("A 'To date' is required for %field %delta", array('%delta' => $field['multiple'] ? intval($delta + 1) : '', '%field' => t($field['widget']['label']))));
        }
      }
    }
  }
}

/**
 * Private implementation of hook_field update and insert operations.
 */
function _date_field_update($op, &$node, $field, &$items, $teaser, $page) {
  $field_name = $field['field_name'];

  $format = $field['type'] == DATE_ISO ? DATE_FORMAT_ISO : DATE_FORMAT_UNIX;
  $timezone = date_get_timezone($field['tz_handling'], $items[0]['timezone']);
  $db_info = content_database_info($field);
  $columns = array_keys($db_info['columns']);

  $values = $items;
  foreach ($values as $delta => $item) {
    // Don't save empty values.
    if (empty($item['value']) && $delta !== 0) {
      unset($items[$delta]);
    }
    // For convenience, we process all possible columns in the element,
    // but now we need to remove columns not used by this field.
    if (is_array($item)) {
      $keys = array_keys($item);
      foreach ($keys as $column => $value) {
        if (!in_array($column, $columns)) {
          unset($items[$delta][$column]);
        }
      }
    }
  }
  $node->$field['field_name'] = $items;
}

/**
 * Private implementation of hook_widget().
 *
 * The widget builds out a complex date element in the following way:
 *
 * - A field is pulled out of the database which is comprised of one or
 *   more collections of from/to dates.
 *
 * - The dates in this field are all converted from the UTC values stored
 *   in the database back to the local time before passing their values
 *   to FAPI.
 *
 * - If values are empty, the field settings rules are used to determine
 *   if the default_values should be empty, now, the same, or use strtotime.
 *
 * - Each from/to combination is created using the date_combo element type
 *   defined by the date module. If the timezone is date-specific, a
 *   timezone selector is added to the first combo element.
 *
 * - If repeating dates are defined, a form to create a repeat rule is
 *   added to the field element.
 *
 * - The date combo element creates two individual date elements, one each
 *   for the from and to field, using the appropriate individual Date API
 *   date elements, like selects, textfields, or popups.
 *
 * - In the individual element validation, the data supplied by the user is
 *   used to update the individual date values.
 *
 * - In the combo date validation, the timezone is updated, if necessary,
 *   then the user input date values are used with that timezone to create
 *   date objects, which are used update combo date timezone and offset values.
 *
 * - In the field's submission processing, the new date values, which are in
 *   the local timezone, are converted back to their UTC values and stored.
 *
 */
function _date_widget($op, &$node, $field, &$items) {
  switch ($op) {
    case 'default value':
      return array(0 => array('value' => NULL, 'value2' => NULL));

    case 'form':
      include_once(drupal_get_path('module', 'date_api') .'/date_api_elements.inc');
      $db_info = content_database_info($field);
      $columns = $db_info['columns'];
      if (isset($columns['rrule'])) {
        unset($columns['rrule']);
      }
      $columns = array_keys($columns);
      $timezone = date_get_timezone($field['tz_handling'], $items[0]['timezone']);

      // Convert UTC dates to their local values, and adjust the
      // default values as specified in the field settings.

      // It would seem to make sense to do this conversion when the data
      // is loaded instead of when the form is created, but the loaded
      // field data is cached and we can't cache dates that have been converted
      // to the timezone of an individual user, so we cache the UTC values
      // instead and do our conversion to local dates in the form and
      // in the formatters.
      $process = date_process_values($field);
      foreach ($items as $delta => $item) {
        foreach ($process as $processed) {
          $date = date_local_date($delta, $item, $timezone, $field, $processed);
          $items[$delta][$processed] = is_object($date) ? date_format($date, date_type_format($field['type'])) : '';
        }
      }

      // Iterate through the available items, adding as many new ones as needed,
      // to build out the complete multi-valued field form element.
      $element = array(
        $field['field_name'] => array(
          '#tree' => TRUE,
          '#weight' => $field['widget']['weight'],
          '#validate' => array('date_widget_validate' => array()),
          ));

      $max = $field['multiple'] == 1 ? 2 + sizeof($items) : 0;
      foreach (range(0, $max) as $delta) {

        $element[$field['field_name']][$delta] = array(
          '#type' => 'date_combo',
          '#field' => $field,
          '#columns' => $columns,
          '#delta' => $delta,
          '#weight' => $delta,
          '#default_value' => isset($items[$delta]) ? $items[$delta] : '',
          '#date_timezone' => $timezone,
        );
        if ($field['tz_handling'] == 'date') {
          $element[$field['field_name']][$delta]['timezone'] = array(
            '#type' => $delta == 0 ? 'date_timezone' : 'hidden',
            '#field' => $field,
            '#columns' => $columns,
            '#delta' => $delta,
            '#default_value' => $timezone,
            '#weight' => $delta + .2,
            );
        }
      }

      // Add a date repeat form element, if needed.
      if (module_exists('date_repeat') && $field['multiple'] == 99) {
        include_once(drupal_get_path('module', 'date') .'/date_repeat.inc');
        _date_repeat_widget($element, $node, $field, $items);
      }
      return $element;
  }
}

/**
 * Create local date object.
 *
 * Create a date object set to local time from the field and
 * widget settings and item values, using field settings to
 * determine what to do with empty values.
 */
function date_local_date($delta, $item, $timezone, $field, $part = 'value') {

  if ($part == 'value') {
    $default_value = $field['widget']['default_value'];
    $default_value_code = $field['widget']['default_value_code'];
  }
  else {
    $default_value = $field['widget']['default_value2'];
    $default_value_code = $field['widget']['default_value_code2'];
  }

  if (empty($item[$part])) {
    if (empty($default_value) || $default_value == 'blank' || $delta > 0) {
      return NULL;
    }
    elseif ($part == 'value2' && $default_value == 'same') {
      $date = date_make_date($item['value'], $timezone);
    }
    // Special case for 'now' when using dates with no timezone,
    // make sure 'now' isn't adjusted to UTC value of 'now'.
    elseif ($field['tz_handling'] == 'none') {
      $date = date_make_date('now', date_default_timezone_name());
    }
    else {
      $date = date_make_date('now', $timezone);
    }
  }
  else {
    $value = $item[$part];
    $value = date_convert($value, $field['type'], DATE_DATETIME);
    $date = date_make_date($value, 'UTC');
    if (date_timezone_convert($field, $item)) {
      date_timezone_set($date, timezone_open($timezone));
    }
  }
  if (empty($item[$part]) && $default_value == 'strtotime' && !empty($default_value_code)) {
    date_modify($date, $default_value_code);
  }
  return $date;
}

/**
 * Implementation of hook_elements().
 *
 * date_combo will create a 'from' and optional 'to' date, along with
 * an optional 'timezone' column for date-specific timezones. Each
 * 'from' and 'to' date will be constructed from date_select or date_text.
 */
function _date_elements() {
  $type['date_combo'] = array(
    '#input' => TRUE, '#tree' => TRUE,
    '#field' => array(), '#delta' => 0,
    '#columns' => array('value', 'value2', 'timezone', 'offset', 'offset2'),
    '#process' => array('date_combo_process' => array()),
    '#validate' => array('date_combo_validate' => array()),
    );
  return $type;
}

/**
 * Process an individual date element.
 */
function date_combo_process($element, $edit = NULL) {
  $field = $element['#field'];
  $delta = $element['#delta'];
  $from_field = $element['#columns'][0];
  $to_field = $element['#columns'][1];
  $tz_field = $element['#columns'][2];

  if ($field['todate'] != 'required' && !empty($element['#default_value'][$to_field]) && $element['#default_value'][$to_field] == $element['#default_value'][$from_field]) {
    unset($element['#default_value'][$to_field]);
  }

  $element[$from_field] = array(
    '#field'         => $field,
    '#title'         => t($field['widget']['label']),
    '#weight'        => $field['widget']['weight'],
    '#description'   => empty($field['todate']) ? t($field['widget']['description']) : '',
    '#required'      => ($field['required'] && $delta == 0) ? 1 : 0,
    '#default_value' => $element['#value'][$from_field],
    '#field'         => $field,
    '#delta'         => $delta,
    '#date_timezone' => $element['#date_timezone'],
    '#date_type'        => $field['type'],
    '#date_format'      => date_limit_format(date_input_format($element), $field['granularity']),
    '#date_text_parts'  => (array) $field['widget']['text_parts'],
    '#date_increment'   => $field['widget']['increment'],
    '#date_year_range'  => $field['widget']['year_range'],
    '#date_label_position' => $field['widget']['label_position'],
    );

  // Give this element the right type, using a Date API
  // or a Date Popup element type.

  switch ($field['widget']['type']) {
    case 'date_select':
      $element[$from_field]['#type'] = 'date_select';
      break;
    case 'date_js':
      $element[$from_field]['#type'] = 'date_text';
      $element[$from_field]['date']['#attributes'] = array('class' => ' jscalendar');
      $element[$from_field]['date']['#jscalendar_ifFormat'] = strtr($field['widget']['input_format'], date_jscalendar_replacements());
      $element[$from_field]['date']['#jscalendar_showsTime'] = 'false';
      break;
    case 'date_popup':
      $element[$from_field]['#type'] = 'date_popup';
      break;
    default:
      $element[$from_field]['#type'] = 'date_text';
      break;
  }

  // If this field uses the 'To', add matching element
  // for the 'To' date, and adapt titles to make it clear which
  // is the 'From' and which is the 'To'.

  if ($field['todate']) {
    $element[$from_field]['#title']  = t('From date');
    $element[$to_field] = $element[$from_field];
    $element[$to_field]['#title'] = t('To date');
    $element[$to_field]['#default_value'] = $element['#value'][$to_field];
    $element[$to_field]['#required'] = false;
    $element[$to_field]['#weight'] += .1;
    $element[$to_field]['#description'] = t('Empty \'To date\' values will use the \'From date\' values.');
    $element['#fieldset_description'] = t($field['widget']['description']);
  }
  return $element;
}

/**
 * Validate and update a combo element.
 * Don't try this if there were errors before reaching this point.
 */
function date_combo_validate($element) {
  global $form_values;

  $field_name = $element['#parents'][0];
  $delta = $element['#parents'][1];
  $item = $form_values[$field_name][$delta];

  $field = $element['#field'];
  $delta = $element['#delta'];
  $from_field = 'value';
  $to_field = 'value2';
  $tz_field = 'timezone';
  $offset_field = 'offset';
  $offset_field2 = 'offset2';

  // Update the timezone, if changed.
  $timezone = $element['#date_timezone'];
  if (!empty($form_values['timezone']) &&
    $form_values[$field_name]['timezone'] != $element['#date_timezone']) {
    $timezone = $form_values[$field_name]['timezone'];
  }

  if (empty($item[$from_field])) {
    $item[$from_field] = NULL;
    $item[$to_field]   = NULL;
    $item[$tz_field]   = $timezone;
    $item[$offset_field] = NULL;
    $item[$offset_field2] = NULL;
    form_set_value($element, $item);
  }
  elseif (!form_get_errors()) {

    // Check todate input for blank values and substitute in fromdate
    // values where needed, then re-compute the todate with those values.

    if ($field['todate']) {
      $merged_date = array();
      foreach ($element['#post'][$field_name][$delta][$to_field] as $part => $value) {
        $merged_date[$part] = empty($value) ? $element['#post'][$field_name][$delta][$from_field][$part] : $value;
        if ($part == 'ampm' && $merged_date['ampm'] == 'pm' && $merged_date['hour'] < 12) {
          $merged_date['hour'] += 12;
        }
        elseif ($part == 'ampm' && $merged_date['ampm'] == 'am' && $merged_date['hour'] == 12) {
          $merged_date['hour'] -= 12;
        }
      }
      $element[$to_field]['#value'] = $merged_date;

      // Call the right function to turn this altered user input into
      // a new value for the todate.
      $function = $field['widget']['type'] .'_input_value';
      $to_date = $function($element[$to_field]);
      $item[$to_field] = $to_date;
    }

    $from_date = date_make_date($item[$from_field], $timezone, $field['type']);
    $item[$offset_field] = date_offset_get($from_date);
    $to_date = date_make_date($item[$to_field], $timezone, $field['type']);
    $item[$offset_field2] = date_offset_get($to_date);
    $item[$tz_field] = $timezone;
    $item['rrule'] = $rrule;
    if ($item[$from_field] > $item[$to_field]) {
      form_set_error(implode('][', $element['#parents']), t('The From date must be greater than the To date.'));
    }
    else {
      // Convert input dates back to their UTC values.
      date_timezone_set($from_date, 'UTC');
      date_timezone_set($to_date, 'UTC');
      $item[$from_field] = date_format($from_date, date_type_format($field['type']));
      $item[$to_field] = date_format($to_date, date_type_format($field['type']));
      form_set_value($element, $item);
    }
  }
}

/**
 * Handle widget processing.
 */
function date_widget_validate($element) {
  global $form_values;
  $field_name = $element['#parents'][0];
  $field = $element[0]['#field'];

  if (module_exists('date_repeat') && array_key_exists('rrule', $form_values[$field_name])) {
    include_once(drupal_get_path('module', 'date') .'/date_repeat.inc');
    return _date_repeat_widget_validate($element);
  }
}

/**
 * Replacement array for date formats to jscalendar formats
 */
function date_jscalendar_replacements() {
  return array(
    'd' => '%d', 'j' => '%e', 'D' => '%a', 'l' => '%A', 'z' => '%j', 'w' => '%w',
    'W' => '%U', 'm' => '%m', 'n' => '%m', 'F' => '%B', 'M' => '%b',
    'Y' => '%Y', 'y' => '%y',
    'g' => '%l', 'G' => '%k', 'h' => '%I', 'H' => '%H',
    'i' => '%M', 's' => '%S', 'a' => '%P', 'A' => '%p',
  );
}

/**
 * Determine the input format for this element.
 */
function date_input_format($element) {
  $field = $element['#field'];
  if (!empty($field['widget']['input_format_custom'])) {
    return $field['widget']['input_format_custom'];
  }
  elseif (!empty($field['widget']['input_format']) && $field['widget']['input_format'] != 'site-wide') {
    return $field['widget']['input_format'];
  }
  return variable_get('date_format_short', 'm/d/Y - H:i');
}

/**
 *  Theme from/to date combination on form.
 */
function theme_date_combo($element) {
  if (!$element['#field']['todate']) {
    return $element['#children'];
  }
  // Group from/to items together in fieldset.
  $fieldset = array(
    '#title' => $element['#field']['widget']['label'] .' '. ($element['#delta'] > 0 ? intval($element['#delta'] + 1) : ''),
    '#value' => $element['#children'],
    '#collapsible' => TRUE,
    '#collapsed' => empty($element['#value']) && $element['#delta'] > 0 ? TRUE : FALSE,
    '#description' => $element['#fieldset_description'],
  );
  return theme('fieldset', $fieldset);
}