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
function date_combo_process($element) {
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
    '#title'         => $field['widget']['label'],
    '#weight'        => $field['widget']['weight'],
    '#description'   => $field['widget']['description'],
    '#required'      => ($field['required'] && $delta == 0) ? 1 : 0,
    '#field'         => $field,
    '#delta'         => $delta,
    '#default_value' => $element['#value'][$from_field],
    '#date_type'              => $field['type'],
    '#date_format'            => date_input_format($element),
    '#date_empty'             => $field['widget']['default_value'],
    '#date_empty_code'        => $field['widget']['default_value_code'],
    '#date_granularity'       => date_granularity($field),
    '#date_timezone_handling' => $field['tz_handling'],
    '#date_timezone_db'       => $field['timezone_db'],
    '#date_timezone_local'    => $element['#value'][$tz_field],
    '#date_text_parts'        => (array) $field['widget']['text_parts'],
    '#date_increment'         => $field['widget']['increment'],
    '#date_year_range'        => $field['widget']['year_range'],
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
    if ($field['widget']['default_value2'] != 'same') {
      $element[$to_field]['#date_empty'] = $field['widget']['default_value2'];
      $element[$to_field]['#date_empty_code'] = $field['widget']['default_value_code2'];
    }
  }

  if ($element['#date_timezone_handling']  == 'date') {
    $element[$tz_field] = date_timezone_element($element);
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
    $element['#value'][$tz_field] = date_timezone_get($element['#date']);
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