<?php
// $Id$

/**
 * @file
 * Interface between date.module and pathauto.module.
 */

/*
 * Implementation of hook_pathauto_node().
 *
 * Allows users to use the textual representation of the parts of a date field
 * as a component of paths.
 */
function date_pathauto_node($op, $node = NULL) {
  include_once(drupal_get_path('module', 'date') .'/date.inc');
  switch ($op) {
    case 'placeholders':
      $placeholders = array();
      foreach (content_fields() as $field) {
		if ($field['type'] == 'date') {
		  if (in_array('D', $field['granularity'])) {
      	    $placeholders['['. $field['field_name'] . '_d]'] = t($field['widget']['label'] . ' - day');
		  }
		  if (in_array('M', $field['granularity'])) {
    	    $placeholders['['. $field['field_name'] . '_m]'] = t($field['widget']['label'] . ' - month');
		  }
		  if (in_array('Y', $field['granularity'])) {
    	    $placeholders['['. $field['field_name'] . '_y]'] = t($field['widget']['label'] . ' - year');
		  }
		  if (in_array('H', $field['granularity'])) {
    	    $placeholders['['. $field['field_name'] . '_h]'] = t($field['widget']['label'] . ' - hours');
		  }
		  if (in_array('N', $field['granularity'])) {
    	    $placeholders['['. $field['field_name'] . '_n]'] = t($field['widget']['label'] . ' - minutes');
		  }
		  if (in_array('S', $field['granularity'])) {
    	    $placeholders['['. $field['field_name'] . '_s]'] = t($field['widget']['label'] . ' - seconds');
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
		if ($field['type'] == 'date') {
		  // this could probably be done better
		  $fieldname = $field['field_name'];
		  $datefield = $node->$fieldname;
		  $datevalue = $datefield[0]['value'];

		  // from date.module
          if ($field['tz_handling'] == 'none') {
            // if no timezone handling was elected, create a date object with the database value
            $date = date_make_date(trim($datevalue), 'none', 'local', $field['type']);
          }
		  else {
            // create a date object with a gmt timezone from the database value
            $date = date_make_date(trim($datevalue), 'GMT', 'db', $field['type']);
            // convert the date object to the proper timezone, depending on the field's tz_handling value
            date_convert_timezone($date, 'GMT', date_get_timezone($field['tz_handling'], $item['timezone']), 'local');
          }
		  $iso = $date->local->iso;

		  if (in_array('D', $field['granularity'])) {
            $results['['. $field['field_name'] . '_d]'] = pathauto_cleanstring(date_iso_day($iso));
		  }
		  if (in_array('M', $field['granularity'])) {
            $results['['. $field['field_name'] . '_m]'] = pathauto_cleanstring(date_iso_mon($iso));
		  }
		  if (in_array('Y', $field['granularity'])) {
            $results['['. $field['field_name'] . '_y]'] = pathauto_cleanstring(date_iso_year($iso));
		  }
		  if (in_array('H', $field['granularity'])) {
            $results['['. $field['field_name'] . '_h]'] = pathauto_cleanstring(date_iso_hours($iso));
		  }
		  if (in_array('N', $field['granularity'])) {
            $results['['. $field['field_name'] . '_n]'] = pathauto_cleanstring(date_iso_minutes($iso));
		  }
		  if (in_array('S', $field['granularity'])) {
            $results['['. $field['field_name'] . '_s]'] = pathauto_cleanstring(date_iso_seconds($iso));
		  }
		}
      }

      return $results;
	  break;
	
	default:
	  break;
	
  }
}
