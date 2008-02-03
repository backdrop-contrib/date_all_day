<?php
//$Id$
/**
 * @file
 * Implementation of Date Repeat API calculations for the CCK Date field.
 *
 * Consolidated here so the code isn't parsed if repeats aren't being used
 * or processed, and to make it easier to maintain.
 *
 * The current implementation adds a repeat form to the date field so the user
 * can select the repeat rules. That selection is built into an RRULE
 * which is stored in the zero position of the field values. During widget
 * validation, the rule is parsed to see what dates it will create,
 * and multiple values are added to the field, one for each repeat date.
 * That update only happens when the rule, the from date, or the to date
 * change, no need to waste processing cycles for other changes to the node
 * values.
 *
 * Lots of possible TODOs, the biggest one is figuring out the best
 * way to handle dates with no UNTIL date since we can't add an infinite
 * number of values to the field. For now, we require the UNTIL date.
 */

/**
 * Widget processing for date repeat form element.
 *
 * Create the RRULE as a top-level element rather than a delta level
 * element, we'll compute the repeat sequence in the widget validation
 * to create the element delta values.
 */
function _date_repeat_widget(&$element, &$node, $field, &$items) {
  $element[$field['field_name']]['rrule'] = array(
    '#type' => 'date_repeat_rrule',
    '#weight' => ($field['widget']['weight'] + .3),
    '#default_value' => $items[0]['rrule'],
    '#date_timezone' => $element[$field['field_name']][0]['timezone'],
    '#prev_value' => $items[0]['value'],
    '#prev_value2' => $items[0]['value2'],
    '#prev_rrule' => $items[0]['rrule'],
    );
}
/**
 * Validation for date repeat form element.
 *
 * Create multiple values from the RRULE results.
 * Lots more work needed here.
 */
function _date_repeat_widget_validate($element) {
  global $form_values;

  $field_name = $element['#parents'][0];
  $field = $element[0]['#field'];

  $values = date_repeat_merge($element['#post'][$field_name]['rrule']);

  // If no repeats are set, clean up the form and return.
  if ($values['FREQ'] == 'NONE') {
    form_set_value(array('#parents' => array($field_name, 0, 'rrule')), NULL);
    form_set_value($element['rrule'], NULL);
    return;
  }

  // Require the UNTIL date for now.
  // The RRULE has already been created by this point, so go back
  // to the posted values to see if this was filled out.
  $error_field = implode('][', $element['#parents']) .'][rrule][UNTIL][datetime][date';
  if (empty($values['UNTIL']['datetime'])) {
    form_set_error($error_field, t('The UNTIL value is required for repeating dates.'));
  }
  if (form_get_errors()) {
    return;
  }

  // If the rule, the start date, or the end date have changed, re-calculate
  // the repeating dates, wipe out the previous values, and populate the
  // field with the new values.

  // TODO
  // Is it right to not do anything unless there are changes? Will that
  // confuse anyone? Commenting that out for now...
  $rrule = $form_values[$field_name]['rrule'];
  if (!empty($rrule)
    //&& ($rrule != $element['rrule']['#prev_rrule']
    //|| $form_values[$field_name][0]['value'] != $element['rrule']['#prev_value']
    //|| $form_values[$field_name][0]['value2'] != $element['rrule']['#prev_value2'])
    ) {

    $item = $form_values[$field_name][0];
    $timezone = date_get_timezone($field['tz_handling'], $item[0]['timezone']);
    $start = date_make_date($item['value'], $timezone, $field['type']);
    $start_datetime = date_format($start, DATE_FORMAT_DATETIME);
    if (!empty($item['value2']) && $new_field['value2'] != $item['value']) {
      $end = date_make_date($item['value2'], $timezone, $field['type']);
      $duration = date_diff($start, $end);
    }
    else {
      $end = $start;
      $duration = 0;
    }

    $end = date_ical_date($values['UNTIL'], $timezone);
    $end_datetime = date_format($end, DATE_FORMAT_DATETIME);

    $parts = date_repeat_split_rrule($rrule);
    $parsed = $parts[0];
    $parsed_exceptions = (array) $parts[1];
    $exceptions = array();
    foreach ($parsed_exceptions as $exception) {
      $date = date_ical_date($exception);
      $exceptions[] = date_format($date, 'Y-m-d');
    }

    $dates = date_repeat_calc($rrule, $start_datetime, $end_datetime, $exceptions);
    foreach ($dates as $delta => $date) {
      $date_start = date_make_date($date, $timezone, $field['type']);
      $date_end = drupal_clone($date_start);
      date_modify($date_end, '+'. $duration .' seconds');
      $value[$field_name][$delta] = array(
        'value' => date_format($date_start, date_type_format($field['type'])),
        'value2' => date_format($date_end, date_type_format($field['type'])),
        'timezone' => $timezone,
        'rrule' => $delta == 0 ? $rrule : '',
        );
    }
    form_set_value($element, $value[$field_name]);
  }
  else {
    // If no changes are needed, move the RRULE back to the zero value
    // item of the field.
    form_set_value(array('#parents' => array($field_name, 0, 'rrule')), $rrule);
    form_set_value($element['rrule'], NULL);
  }
}

/**
 * Clean up Date Repeat column information.
 *
 * To be used for uninstalls, not currently implemented anywhere.
 */
function _date_repeat_remove() {
  $types = content_types();
  foreach ($types as $type_name => $type) {
    foreach ($type['fields'] as $field_name => $field) {
      if ($field['multiple'] == 99) {
        $db_info = content_database_info($field);
        $table = $db_info['table'];
        $column = $db_info['columns']['rrule']['column'];
        db_query("ALTER TABLE {". $table ."} DROP $column");
        db_query("UPDATE {node_field} set multiple = 1 WHERE field_name = '". $field['field_name'] ."'");
      }
    }
  }
}