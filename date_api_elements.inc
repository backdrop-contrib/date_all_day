<?php
//$Id$
/**
 * @file
 * Date API elements themes and validation.
 * This file is only included during the edit process to reduce memory usage.
 */

/**
 * Implementation of hook_elements().
 *
 * Parameters for date form elements, designed to have sane defaults so any
 * or all can be omitted.
 *
 * date_select will create a collection of form elements, with a separate
 * select or textfield for each date part. The whole collection will get
 * re-formatted into the type of date in #date_params['type'] during validation.
 *
 * date_text will create a textfield that can contain a whole
 * date or any part of a date as text. The input value will be re-formatted
 * into the type of date in #date_params['type'] date during validation.
 *
 * #date_type
 *   the type of the date value that will be stored in the database,
 *   DATE_UNIX, DATE_ISO, or DATE_DATETIME
 * #date_format
 *   a format string that describes the format and order of date parts to
 *   display in the edit form for this element, be sure to add 'A' or 'a'
 *   to get an am/pm selector, defaults to short site default format
 * #date_timezone_handling
 *   the way to determine the 'local' timezone for this date,
 *   Allows dynamic handling so dates can adapt to user timezones when needed.
 *   Allowed values are 'site', 'user', 'date', or 'none'
 * #date_timezone_db
 *   the timezone of the date/time value being passed in from the database,
 *   default is UTC.
 * #date_timezone_local
 *   Used only if timezone handling is set to 'date', otherwise ignored.
 *   The timezone to be used for date-specific timezones, the default is
 *   date site timezone, if set, or UTC. Valid timezones are
 *   standard timezone ids like US/Central, America/New_York, UTC
 * #date_granularity
 *   an array of date parts to be collected, default is (M, D, Y),
 *   Allowed values are:
 *    Y (year), M (month), D (day), H (hours), N (minutes), S (seconds)
 * #date_increment
 *   increment minutes and seconds by this amount, default is 1
 * #date_year_range
 *   the number of years to go back and forward in the year selector,
 *   default is -3:+3 (3 back and 3 forward)
 * #date_text_parts
 *   array of date parts that should use textfield instead of select
 *   like array('year') to show the year as textfield
 * #date_empty
 *   handling for empty date values:
 *     'blank' to show blank value
 *     'now' to show current time
 *     'strtotime' to use strtotime to adjust the current time
 *     'date' to insert a specific date
 * #date_empty_code
 *   if #date_empty is 'strtotime' use the value in #date_empty_code, like:
 *     [+-][##] [years|months|days|hours|minutes|seconds], i.e. '+90 days'
 *   if #date_empty is 'date' use the value in #date_empty_code as the date
 *     should be formatted as #date_type, using timezone from #date_timezone_db
 */
function _date_api_elements() {
  $date_base = array(
    '#input' => TRUE, '#tree' => TRUE,
    '#date_type' => DATE_DATETIME,
    '#date_format' => variable_get('date_format_short', 'm/d/Y - H:i'),
    '#date_timezone_handling' => variable_get('configurable_timezones', 1) ? 'user' : 'site',
    '#date_timezone_db' => 'UTC',
    '#date_timezone_local' => variable_get('date_default_timezone_name', NULL),
    '#date_granularity' => array('M', 'D', 'Y'),
    '#date_text_parts' => array(),
    '#date_increment' => 1, '#date_year_range' => '-3:+3',
    '#date_empty' => 'now',
    '#date_empty_code' => '',
    );
  $type['date_select'] = array_merge($date_base, array(
    '#process' => array('date_select_process' => array()),
    '#validate' => array('date_select_validate' => array()),
    ));
  $type['date_text'] = array_merge($date_base, array(
    '#process' => array('date_text_process' => array()),
    '#validate' => array('date_text_validate' => array()),
    ));
  return $type;
}

/**
 * Text date input form.
 *
 * Display all or part of a date in a single textfield.
 *
 * The exact parts displayed in the field are those in #date_granularity.
 * The display of each part comes from #date_format.
 * A timezone selector is added if #date_timezone_handling' == 'date'.
 */
function date_text_process($element) {
  $date =  date_default_date($element);
  $element['#date'] = $date;

  $element['#tree'] = TRUE;
  $element['date'] = array(
    '#type' => 'textfield',
    '#title' => $element['#title'],
    '#weight' => $element['#weight'],
    '#default_value' => is_object($date) ? date_format($date, date_input_format($element)) : '',
    '#attributes' => array('class' => 'date-date'),
    '#type' => 'textfield',
    '#required' => $element['#required'],
    '#theme' => 'date_textfield_element',
    );
  if ($element['#date_timezone_handling'] == 'date') {
    $element['timezone'] = date_timezone_element($element);
  }
  return $element;
}

/**
 * Flexible date/time drop-down selector.
 *
 * Splits date into a collection of date and time sub-elements, one
 * for each date part. Each sub-element can be either a textfield or a
 * select, based on the value of #date_text_fields.
 *
 * The exact parts displayed in the field are those in #date_granularity.
 * The display of each part comes from #date_format.
 * A timezone selector is added if #date_timezone_handling' == 'date'.
 */
function date_select_process($element) {
  $date =  date_default_date($element);
  $element['#tree'] = TRUE;
  $element['#date'] = $date;

  $element += date_parts_element($element, $date, $element['#date_granularity'], $element['#date_format']);
  if ($element['#date_timezone_handling'] == 'date') {
    $element['timezone'] = date_timezone_element($element);
  }
  return $element;
}

/**
 * Create form elements for one or more date parts.
 *
 * Get the order of date elements from the provided format.
 * If the format order omits any date parts in the granularity, alter the
 * granularity array to match the format, then flip the $order array
 * to get the position for each element. Then iterate through the
 * elements and create a sub-form for each part.
 *
 * @param array $element
 * @param object $date
 * @param array $granularity
 * @param string $format
 * @return array
 *   the form array for the submitted date parts
 */
function date_parts_element($element, $date, $granularity, $format) {
  $order = date_format_order($format);
  $granularity = array_intersect($granularity, $order);
  $order = array_flip($order);

  $hours_format  = strpos(strtolower($field['widget']['input_format']), 'a') ? 'g': 'G';
  $year = $year > 0 ? $year : date_format(date_now(), 'Y');
  $fields = date_granularity_parts();
  $count = 0;
  $increment = min(intval($field['widget']['increment']), 1);
  foreach ($granularity as $part) {
    $part_required = $element['#required'];
    $field = $fields[$part];
    $part_type = in_array($field, $element['#date_text_parts']) ? 'textfield' : 'select';
    $sub_element[$field] = array(
      '#title' => theme('date_select_label_'. $field, $part_type, $element),
      '#weight' => $order[$part],
      '#required' => $part_required,
      '#attributes' => array('class' => 'date-'. $field),
      );
    switch ($field) {
      case 'year':
        $sub_element[$field]['#default_value'] = is_object($date) ? date_format($date, 'Y') : '';
        $range = explode(':', $element['#date_year_range']);
        $sub_element[$field]['#options'] = drupal_map_assoc(date_years($year + $range[0], $year + $range[1], $part_required));
        break;
      case 'mon':
        $sub_element[$field]['#default_value'] = is_object($date) ? date_format($date, 'n') : '';
        $sub_element[$field]['#options'] = (date_month_names($part_required));
        break;
      case 'mday':
        $sub_element[$field]['#default_value'] = is_object($date) ? date_format($date, 'j') : '';
        $sub_element[$field]['#options'] = drupal_map_assoc(date_days($part_required));
        break;
      case 'hours':
        $sub_element[$field]['#default_value'] = is_object($date) ? date_format($date, $hours_format) : '';
        $sub_element[$field]['#options'] = drupal_map_assoc(date_hours($hours_format, $part_required));
        break;
      case 'minutes':
        $sub_element[$field]['#default_value'] = is_object($date) ? date_pad(intval(round(intval(date_format($date, 'i')) / $element['#date_increment']) * $element['#date_increment'])) : '';
        $sub_element[$field]['#options'] = drupal_map_assoc(date_minutes('i', $part_required, $element['#date_increment']));
        break;
      case 'seconds':
        $sub_element[$field]['#default_value'] = is_object($date) ? date_pad(intval(round(intval(date_format($date, 's')) / $element['#date_increment']) * $element['#date_increment'])) : '';
        $sub_element[$field]['#options'] = drupal_map_assoc(date_seconds('s', $part_required, $element['#date_increment']));
        break;
    }
    if (in_array($field, $element['#date_text_parts'])) {
      $sub_element[$field]['#type'] = 'textfield';
      $sub_element[$field]['#theme'] = 'date_textfield_element';
      $sub_element[$field]['#maxlength'] = 4;
      $sub_element[$field]['#size'] = 4;
    }
    else {
      $sub_element[$field]['#type'] = 'select';
      $sub_element[$field]['#theme'] = 'date_select_element';
    }
  }
  if (strtolower($hours_format) == 'g') {
    $sub_element['ampm'] = array(
      '#type' => 'select',
      '#theme' => 'date_select_element',
      '#default_value' => is_object($date) ? (date_format($date, 'G') < 12 ? 'am' : 'pm') : '',
      '#options' => drupal_map_assoc(date_ampm($element['#required'])),
      '#title' => theme('date_select_label_ampm', 'select', $element),
      '#weight' => 8,
      '#attributes' => array('class' => 'date-ampm'),
    );
  }
  return $sub_element;
}

/**
 * Create a timezone form element.
 *
 * @param array $element
 * @return array
 *   the timezone form element
 */
function date_timezone_element($element) {
  $sub_element = array(
    '#type' => 'select',
    '#title' => theme('date_select_label_timezone', 'select', $element),
    '#default_value' => date_element_timezone($element),
    '#options' => date_timezone_names($element['#required']),
    '#weight' => 9,
    '#required' => $element['#required'],
    '#attributes' => array('class' => 'date-timezone'),
    '#theme' => 'date_select_element',
  );
  return $sub_element;
}

/**
 *  Validation function for date selector
 *  $params = an array of the values from date_select_input.
 */
function  date_select_validate($element) {
  $field = $element['#field'];
  if ($element['#value']['ampm'] == 'pm' && $element['#value']['hours'] < 12) {
    $element['#value']['hours'] += 12;
  }
  if ($element['#required'] || !empty($element['#value']['year'])) {
    if ($element['#value']['year'] < DATE_MIN_YEAR || $element['#value']['year'] > DATE_MAX_YEAR) {
      form_set_error($fieldname, t('year must be a number between %min and %max.', array(
        '%min' => DATE_MIN_YEAR, '%max' => DATE_MAX_YEAR)));
    }
  }
  if ($element['#required'] || !empty($element['#value']['mon'])) {
    if ($element['#value']['mon'] < 1 || $element['#value']['mon'] > 12) {
      form_set_error($fieldname, t('month must be a number between 1 and 12.'));
    }
  }
  if ($element['#required'] || !empty($element['#value']['mday'])) {
    if ($element['#value']['mday'] < 1 || $element['#value']['mday'] > 31) {
      form_set_error($fieldname, t('day must be a number between 1 and 31.'));
    }
  }
  if (!form_get_errors()) {
    // Convert the array back to a value.
    $value = date_convert($element['#value'], DATE_ARRAY, DATE_DATETIME);
    $date = date_element_date($element, $value, 'local2db');

    // If it creates a valid date, store it.
    if (date_is_valid($date, DATE_OBJECT)) {
      $value = date_convert($date, DATE_OBJECT, $field['type']);
      form_set_value($element, $value);
      form_set_value($element['year'], NULL);
      form_set_value($element['mon'], NULL);
      form_set_value($element['mday'], NULL);
      form_set_value($element['hours'], NULL);
      form_set_value($element['minutes'], NULL);
      form_set_value($element['seconds'], NULL);
      form_set_value($element['ampm'], NULL);
    }
    else {
      form_set_error($element, t('A valid date cannot be constructed from %m-%d-%year.', array(
        '%m' => $element['#value']['mon'],
        '%d' => $element['#value']['mday'],
        '%y' => $element['#value']['year'],
        )));
    }
  }
}

/**
 *  Validation for text input
 *  $params = an array of the values from date_text_input.
 */
function  date_text_validate($element) {
  $input = $element['date']['#value'];
  if (!$element['#required'] && trim($input) == '') return TRUE;

  $value = date_convert_from_custom($input, $element['#date_format'], $element['#date_type']);
  $value = date_limit_value($value, $element['#date_granularity'], DATE_DATETIME);
  $date = date_element_date($element, $value, 'local2db');

  // If it creates a valid date, store it.
  if (date_is_valid($date, DATE_OBJECT)) {
    $value = date_convert($date, DATE_OBJECT, $field['type']);
    form_set_value($element['date'], NULL);
    form_set_value($element, $value);
  }
  else {
    form_set_error($element, t('A valid date cannot be created from %date', array('%date' => $value)));
  }
}

/**
 * Create a default date from the database value.
 *
 * Use #date_empty to determine what to do with empty values,
 * adjust the date to the right timezone for this element.
 */
function date_default_date($element) {
  if (empty($element['#value'])) {
    if (empty($element['#date_empty']) || $element['#date_empty'] == 'blank') {
      return NULL;
    }
    elseif ($element['#date_empty'] == 'date' && !empty($element['#date_empty_code'])) {
      $value = $element['#date_empty_code'];
    }
    else {
      $value == 'now';
    }
  }
  else {
    $value = $element['#value'];
  }
  if (!empty($value)) {
    $value = date_convert($value, $element['#date_type'], DATE_DATETIME);
    $date = date_create($value, timezone_open(date_element_timezone($element)));
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
  if (empty($element['#date_timezone_db'])) {
    $element['#date_timezone_db'] = 'UTC';
  }
  $timezone_in  = $direction == 'db2local' ? $element['#date_timezone_db'] : date_element_timezone($element);
  $timezone_out = $direction == 'db2local' ? date_element_timezone($element) : $element['#date_timezone_db'];

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
function date_element_timezone($element) {
  switch ($element['#date_timezone_handling']) {
    case('site'):
      $timezone = date_default_timezone_name(FALSE);
      break;
    case('date'):
      $timezone = !empty($element['#date_timezone_local']) ? $element['#date_timezone_local'] :  $element['#date_timezone_db'];
      break;
    case('user'):
      $timezone = date_default_timezone_name();
      break;
    default:
      $timezone = $element['#date_timezone_db'];
  }
  return !empty($timezone) ? $timezone : 'UTC';
}

/**
 * Convert a date input in a custom format to a standard date type
 *
 * Handles conversion of translated month names (i.e. turns t('Mar') or
 * t('March') into 3). Also properly handles dates input in European style
 * short formats, like DD/MM/YYYY. Works by parsing the format string
 * to create a regex that can be used on the input value.
 *
 * The original code to do this was created by Yves Chedemois (yched).
 *
 * @param string $date
 *   a date value
 * @param string $format
 *   a date format string that describes the format of the input value
 * @param string $to_type
 *   the date type to be created from the custom input value
 *   DATE_DATETIME, DATE_ISO, DATE_UNIX, or DATE_ARRAY
 * @return mixed
 *   input value converted to $to_type
 */
function date_convert_from_custom($date, $format, $to_type = DATE_DATETIME) {
  $array = date_format_patterns();
  foreach ($array as $key => $value) {
    $patterns[] = "`(^|[^\\\\\\\\])".$key."`"; // the letter with no preceding '\'
    $repl1[] = '${1}(.)';                  // a single character
    $repl2[] = '${1}('. $value .')';       // the
  }
  $patterns[] = "`\\\\\\\\([".implode(array_keys($array))."])`";
  $repl1[] = '${1}';
  $repl2[] = '${1}';

  $format_regexp = preg_quote($format);
  // extract letters
  $regex1 = preg_replace($patterns, $repl1, $format_regexp,1);
  preg_match('`^'.$regex1.'$`', stripslashes($format), $letters);
  array_shift($letters);
  // extract values
  $regex2 = preg_replace($patterns, $repl2, $format_regexp,1);
  preg_match('`^'.$regex2.'$`', $date, $values);
  array_shift($values);
  // if we did not find all the values for the patterns in the format, abort
  if (count($letters) != count($values)) {
    return  DATE_ERROR;
  }
  $final_date = array('hours' => 0, 'minutes' => 0, 'seconds' => 0,
    'mon' => 0, 'mday' => 0, 'year' => 0);
  foreach($letters as $i => $letter) {
    $value = $values[$i];
    switch($letter) {
      case 'd':
      case 'j':
        $final_date['mday'] = intval($value);
        break;
      case 'm':
        $final_date['mon'] = intval($value);
        break;
      case 'F':
        $array_month_long = array_flip(date_month_names());
        $final_date['mon'] = $array_month_long[$value];
        break;
      case 'M':
        $array_month = array_flip(date_month_names_abbr());
        $final_date['mon'] = $array_month[$value];
        break;
      case 'Y':
      case 'y':
        $year = $value;
        // if no century, we add the current one ("06" => "2006")
        $final_date['year'] = str_pad($year, 4, substr(date("Y"), 0, 2), STR_PAD_LEFT);
        break;
      case 'a':
      case 'A':
        $am_pm = strtolower($value);
        break;
      case 'g':
      case 'h':
      case 'G':
      case 'H':
        $final_date['hours'] = intval($value);
        break;
      case 'i':
        $final_date['minutes'] = intval($value);
        break;
      case 's':
        $final_date['seconds'] = intval($value);
        break;
      case 'U':
        $final_date = date_convert($value, DATE_UNIX, DATE_ARRAY);
        break;
    }
  }
  if ($ampm == 'pm' && $final_date['hours'] != 12) {
    $final_date['hours'] += 12;
  }
  if ($to_type == DATE_ARRAY) {
    // If returning a date array, get the remaining array values for the
    // input year/month/day/hours/minutes/seconds, like the day of week.
    $obj = date_create(date_convert($final_date, DATE_ARRAY, DATE_DATETIME), timezone_open('UTC'));
    $dow = date_format($obj, 'w');
    $days = date_week_days();
    $final_date['wday'] = $dow;
    $final_date['yday'] = date_format($obj, 'z');
    $final_date['weekday'] = $days[$dow];
    $final_date['month'] = date_format($obj, 'F');
    $final_date[0] = date_format($obj, 'U');
    return $final_date;
  }
  else {
    // If converting array to another type, no need to find additional
    // array values, they aren't needed by the conversion.
    return date_convert($final_date, DATE_ARRAY, $to_type);
  }
}

/**
 * Format a date selection element.
 *
 * @param $element
 *   An associative array containing the properties of the element.
 *   Properties used: title, value, options, description, required and attributes.
 * @return
 *   A themed HTML string representing the date selection boxes.
 */
function theme_date_select($element) {
  return '<div class="container-inline-date clear-block">'. theme('form_element', $element, $element['#children']) .'</div>';
}

/**
 * Format a date text element.
 *
 * @param $element
 *   An associative array containing the properties of the element.
 *   Properties used: title, value, options, description, required and attributes.
 * @return
 *   A themed HTML string representing the date selection boxes.
 */
function theme_date_text($element) {
  return theme('form_element', $element, $element['#children']);
}

/**
 *  Themes for date input form elements
 */
function theme_date_select_element($element) {
  $part = array_pop($element['#parents']);
  return '<div class="date-'. $part .'">'. theme('select', $element, $element['#children']) .'</div>';
}

function theme_date_textfield_element($element) {
  $part = array_pop($element['#parents']);
  return '<div class="date-'. $part .'">'. theme('textfield', $element, $element['#children']) .'</div>';
}

/**
 * Format labels for each date part in a date_select.
 *
 * @param $part_type
 *   the type of field used for this part, 'textfield' or 'select'
 * @param $element
 *   An associative array containing the properties of the element.
 *   Properties used: title, value, options, description, required and attributes.
 */
function theme_date_select_label_year($part_type, $element) {
  return t('Year');
}
function theme_date_select_label_mon($part_type, $element) {
  return t('Month');
}
function theme_date_select_label_mday($part_type, $element) {
  return t('Day');
}
function theme_date_select_label_hours($part_type, $element) {
  return t('Hour');
}
function theme_date_select_label_minutes($part_type, $element) {
  return t('Minute');
}
function theme_date_select_label_seconds($part_type, $element) {
  return t('Second');
}
function theme_date_select_label_ampm($part_type, $element) {
  return t('am/pm');
}
function theme_date_select_label_timezone($part_type, $element) {
  return t('Timezone');
}