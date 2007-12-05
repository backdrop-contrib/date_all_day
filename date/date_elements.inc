<?php
//$Id$
/**
 * @file
 * Date forms and form themes and validation.
 * This file is only included during the edit process to reduce memory usage.
 */

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
    '#columns' => array('value', 'value2', 'timezone', 'offset'),
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

  $timezone = date_element_timezone($element, $edit);
  $from_date = date_create($element['#value'][$from_field], timezone_open($timezone));
  $to_date = date_create($element['#value'][$to_field], timezone_open($timezone));

  $element[$from_field] = array(
    '#field'         => $field,
    '#title'         => t($field['widget']['label']),
    '#weight'        => $field['widget']['weight'],
    '#description'   => t($field['widget']['description']),
    '#required'      => ($field['required'] && $delta == 0) ? 1 : 0,
    '#field'         => $field,
    '#delta'         => $delta,
    '#default_value' => $from_date,
    '#date_type'        => $field['type'],
    '#date_format'      => date_input_format($element),
    '#date_granularity' => date_granularity($field),
    '#date_text_parts'  => (array) $field['widget']['text_parts'],
    '#date_increment'   => $field['widget']['increment'],
    '#date_year_range'  => $field['widget']['year_range'],
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
    if ($element['#date_settings']['timezone_handling']  == 'date') {
      $element[$to_field]['#date_settings']['timezone_element'] = $element['#date_settings']['timezone_element'];
    }
    if ($field['widget']['default_value2'] != 'same') {
      $element[$to_field]['#date_empty'] = $field['widget']['default_value2'];
      $element[$to_field]['#date_empty_code'] = $field['widget']['default_value_code2'];
    }
  }

  return $element;
}

/**
 * Validate and update a combo element.
 * Don't try this if there were errors before reaching this point.
 */
function date_combo_validate($element) {
  if (!form_get_errors()) {
    $field = $element['#field'];
    $delta = $element['#delta'];
    $from_field = $element['#columns'][0];
    $to_field = $element['#columns'][1];
    $tz_field = $element['#columns'][2];
    $offset_field = $element['#columns'][3];
    if ($field['multiple']) {
      switch ($field['widget']['type']) {
        case 'date_select':
          if (empty($element['#value'][$from_field]['year']) && ($delta > 0 || !$field['required'])) {
            form_set_value($element, NULL);
          }
          break;
        default:
          if (empty($element['#value'][$from_field]) && ($delta > 0 || !$field['required'])) {
            form_set_value($element, NULL);
          }
          break;
      }
    }
    if (empty($element['#value'][$to_field])) {
      $element['#value'][$to_field] = $element['#value'][$from_field];
    }
    $element['#value'][$tz_field] = timezone_name_get($element['#date']);
    $element['#value'][$offset_field] = date_offset_get($element['#date']);
    form_set_value($element, $element['#value']);
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
  );
  return theme('fieldset', $fieldset);
}

/**
 * Create a default date from the database value.
 *
 * Use ['#date_settings']['empty'] to determine what to do with empty values,
 * adjust the date to the right timezone for this element.
 */
function date_default_date($element, $edit) {
  if (empty($element['#value'])) {
    if (empty($element['#date_empty']) || $element['#date_empty'] == 'blank') {
      return NULL;
    }
    elseif ($element['#date_empty'] == 'date' && !empty($element['#date_empty_code'])) {
      $value = $element['#date_empty_code'];
    }
    else {
      $value = 'now';
    }
  }
  else {
    $value = $element['#value'];
  }
  if (!empty($value)) {
    if ($value != 'now') {
      $value = date_convert($value, $element['#date_type'], DATE_DATETIME);
    }
    $date = date_create($value, timezone_open(date_element_timezone($element, $edit)));
  }
  if ($date === DATE_ERROR) {
    return NULL;
  }
  if (empty($element['#value']) && $element['#date_empty'] == 'strtotime' && !empty($element['#date_empty_code'])) {
    date_modify($date, $element['#date_empty_code']);
  }
  return $date;
}

/**
 * Helper function to create a date object for an element.
 *
 * @param $element
 *   the date element
 * @param $value
 *   the value to set the date to
 * @param $direction
 *   'db2local' or 'local2db'
 * @return
 *   a valid date object, adjusted to the right timezone
 */
function date_element_date($element, $value, $direction = 'db2local') {
  if (empty($element['#date_settings']['timezone_db'])) {
    $element['#date_settings']['timezone_db'] = 'UTC';
  }
  $timezone_in  = $direction == 'db2local' ? $element['#date_settings']['timezone_db'] : date_element_timezone($element);
  $timezone_out = $direction == 'db2local' ? date_element_timezone($element) : $element['#date_settings']['timezone_db'];

  $date = date_create($value, timezone_open($timezone_in));
  if (!empty($timezone_out) && $timezone_out != $timezone_in) {
    date_timezone_set($date, timezone_open($timezone_out));
  }
  return $date;
}

/**
 *  Helper function to figure out which local timezone applies
 *  to a date element and select it
 */
function date_element_timezone($element, $edit) {
  switch ($element['#field']['tz_handling']) {
    case('site'):
      $timezone = date_default_timezone_name(FALSE);
      break;
    case('date'):
      $timezone = !empty($element['#value']['timezone'][0]) ? $element['#value']['timezone'][0] : date_default_timezone_name();
      break;
    case('gmt'):
      $timezone = 'UTC';
      break;
    default:
      $timezone = date_default_timezone_name();
  }
  return !empty($timezone) ? $timezone : date_default_timezone_name();
}

