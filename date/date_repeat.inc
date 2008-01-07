<?php
//$Id$
/**
 * @file
 * Implementation of Date Repeat API calculations for the CCK Date field.
 */

/**
 * Theme the human-readable description for a Date Repeat rule.
 *
 * This is an interim solution, we won't want to repeat this rule on
 * every repeation of the date, we'll want to optionally display
 * the rule instead of the whole list of repeating dates. No easy
 * way to do this in D5, it will work better in D6.
 */
function theme_date_repeat_display($field, $item, $node = NULL) {
  $output = date_repeat_rrule_description($item['rrule']);
  $output = '<div>Repeats '. $output .'</div>';
  return $output;
}

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
  $rrule = $form_values[$field_name]['rrule'];

  // If the rule, the start date, or the end date have changed, re-calculate
  // the repeating dates.
  if (!empty($rrule) && ($rrule != $form_values[$field_name][0]['#prev_rrule']
    || $form_values[$field_name][0]['value'] != $element['#prev_value']
    || $form_values[$field_name][0]['value2'] != $element['#prev_value2'])) {
    $parsed = date_ical_parse_rrule($field, $rrule);
    $item = $form_values[$field_name][0];
    $timezone = date_get_timezone($field['tz_handling'], $item[0]['timezone']);
    $start = date_convert($item['value'], $field['type'], DATE_OBJECT);
    $start_datetime = date_format($start, DATE_FORMAT_DATETIME);
    if (!empty($item['value2']) && $new_field['value2'] != $item['value']) {
      $end = date_convert($item['value2'], $field['type'], DATE_OBJECT);
      $duration = date_diff($start, $end);
    }
    else {
      $end = $start;
      $duration = 0;
    }

    // TODO - figure out a better way to handle dates with no UNTIL date
    // and how far out to set outside date. For now, set a final date for
    // the repeat for dates with no UNTIL to 1 year from the end date.
    date_modify($end, '+1 year');
    $end_datetime = date_format($end, DATE_FORMAT_DATETIME);

    $exceptions = explode(',', $parsed['EXDATE']);
    $dates = date_repeat_calc($rrule, $start_datetime, $end_datetime, $exceptions);
    foreach ($dates as $delta => $date) {
      $date_start = date_make_date($date, $timezone, $field['type']);
      $date_end = drupal_clone($date_start);
      date_modify($date_end, '+'. $duration .' seconds');
      $value[$field_name][$delta] = array(
        'value' => date_format($date_start, date_type_format($field['type'])),
        'value2' => date_format($date_end, date_type_format($field['type'])),
        'offset' => date_offset_get($date_start),
        'offset2' => date_offset_get($date_end),
        'timezone' => $timezone,
        'rrule' => $delta == 0 ? $rrule : '',
        );
    }
    form_set_value($element, $value[$field_name]);
  }
  else {
    form_set_value(array('#parents' => array($field_name, 0, 'rrule')), $rrule);
    form_set_value($element['rrule'], NULL);
  }
}

/**
 * Clean up Date Repeat column information.
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