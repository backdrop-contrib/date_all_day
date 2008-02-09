<?php
// $Id$

/*
 * Implementation of hook_pathauto_node().
 *
 * Allows users to use the textual representation of the parts of a date field
 * as a component of paths.
 */
function _date_pathauto_node($op, $node = NULL) {
  switch ($op) {
    case 'placeholders':
      $placeholders = array();
      foreach (content_fields() as $field) {
        if (($field['type'] == 'date' || $field['type'] == 'datestamp') && is_array($field['granularity'])) {
          if (array_key_exists('year', $field['granularity'])) {
            $placeholders['['. $field['field_name'] .'_y]'] = t($field['widget']['label'] .' - year');
          }
          if (array_key_exists('month', $field['granularity'])) {
            $placeholders['['. $field['field_name'] .'_m]'] = t($field['widget']['label'] .' - month');
          }
          if (array_key_exists('day', $field['granularity'])) {
           $placeholders['['. $field['field_name'] .'_d]'] = t($field['widget']['label'] .' - day');
          }
          if (array_key_exists('hour', $field['granularity'])) {
            $placeholders['['. $field['field_name'] .'_h]'] = t($field['widget']['label'] .' - hours');
          }
          if (array_key_exists('minute', $field['granularity'])) {
            $placeholders['['. $field['field_name'] .'_n]'] = t($field['widget']['label'] .' - minutes');
          }
          if (array_key_exists('second', $field['granularity'])) {
            $placeholders['['. $field['field_name'] .'_s]'] = t($field['widget']['label'] .' - seconds');
          }
        }
      }
 return $placeholders;
      break;

    case 'values':
      // all this stuff is from content_pathauto.inc
      $results = array();

      // Get node output (filtered and with module-specific fields).
      if (node_hook($node, 'view')) {
        node_invoke($node, 'view', false, false);
      }
      else {
        $node = node_prepare($node, false);
      }
      // Allow modules to change $node->body before viewing.
      node_invoke_nodeapi($node, 'view', false, false);
      // Get node output (filtered and with module-specific fields).
      if (node_hook($node, 'view')) {
        node_invoke($node, 'view', false, false);
      }
      else {
        $node = node_prepare($node, false);
      }
      // Allow modules to change $node->body before viewing.
      node_invoke_nodeapi($node, 'view', false, false);

      foreach (content_fields() as $field) {
        if ($field['type'] == 'date' || $field['type'] == 'datestamp' && is_array($field['granularity'])) {
          // this could probably be done better
          $fieldname = $field['field_name'];
          $datefield = $node->$fieldname;
          $datevalue = $datefield[0]['value'];
          $date = date_make_date($datevalue, 'UTC', $field['type']);

          // from date.module
          if ($field['tz_handling'] == 'none') {
            // if no timezone handling was elected, create a date object with the database value
            $iso = date_format($date, DATE_FORMAT_ISO);
          }
          else {
            date_timezone_set($date, timezone_open(date_get_timezone($field['tz_handling'], $item['timezone'])));
            $iso = date_format($date, DATE_FORMAT_ISO);
          }
          if (is_array($field['granularity'])) {
            // we were using the date_iso_field calls, but we'd prefer 01 rather than 1 for january
            if (array_key_exists('year', $field['granularity'])) {
              $results['['. $field['field_name'] .'_y]'] = pathauto_cleanstring(substr($iso, 0, 4));
            }
            if (array_key_exists('month', $field['granularity'])) {
              $results['['. $field['field_name'] .'_m]'] = pathauto_cleanstring(substr($iso, 5, 2));
            }
            if (array_key_exists('day', $field['granularity'])) {
              $results['['. $field['field_name'] .'_d]'] = pathauto_cleanstring(substr($iso, 8, 2));
            }
            if (array_key_exists('hour', $field['granularity'])) {
              $results['['. $field['field_name'] .'_h]'] = pathauto_cleanstring(substr($iso, 11, 2));
            }
            if (array_key_exists('minute', $field['granularity'])) {
              $results['['. $field['field_name'] .'_n]'] = pathauto_cleanstring(substr($iso, 14, 2));
            }
            if (array_key_exists('second', $field['granularity'])) {
              $results['['. $field['field_name'] .'_s]'] = pathauto_cleanstring(substr($iso, 17, 2));
            }
          }
        }
      }
      return $results;
 break;

    default:
 break;
  }
}