<?php
//$Id$
/**
 * @file
 * Date forms and form themes and validation.
 * This file is only included during the edit process to reduce memory usage.
 */

/**
 * Override form for the site timezone settings form.
 * Display a list of timezone names instead of offsets
 * and hide the offset value.
 */
function date_form_site(&$form) {
  $form['date_default_timezone'] = array(
    '#type' => 'hidden',
    '#value' => variable_get('date_default_timezone', 0),
  );
  $timezone = variable_get('date_default_timezone_name', NULL);
  $form['date_default_timezone_name'] = array(
    '#type' => 'select',
    '#title' => t('Default time zone'),
    '#default_value' => $timezone,
    '#options' => date_timezone_names(),
    '#description' => t('Select the default site time zone. If in doubt, choose the timezone that is closest to your location which has the same rules for daylight savings time.'),
    '#weight' => -10,
    '#submit' => array('date_form_site' => array($form['date_default_timezone_name']['#value'])),
  );
}

/**
 * Override form for the user timezone settings form.
 * Display a list of timezone names instead of offsets
 * and hide the offset value.
 */
function date_form_user(&$form) {
  $form['timezone']['timezone']['#type'] = 'hidden';
  $form['timezone']['timezone']['#value'] = strlen($edit['timezone']) ? $edit['timezone'] : variable_get('date_default_timezone', 0);
  $timezone = strlen($edit['timezone_name']) ? $edit['timezone_name'] : variable_get('date_default_timezone_name', NULL);

  $form['timezone']['timezone_name'] = array(
    '#type' => 'select',
    '#title' => t('Default time zone'),
    '#default_value' => $timezone,
    '#options' => date_timezone_names(),
    '#description' => t('Select your current local time.  If in doubt, choose the timezone that is closest to your location which has the same rules for daylight savings time. Dates and times throughout this site will be displayed using this time zone.'),
    '#submit' => array('date_form_user' => array($form['timezone']['timezone_name']['#value'], $form['uid'])),
  );
  return $form;
}

/**
 * Callback from site timezone settings form to update site timezone info.
 * When the timezone name is updated, update the offset as well.
 */
function date_form_update_site($timezone) {
  if (empty($timezone)) return;
  variable_set('date_default_timezone_name', $timezone);
  $date_time = date_create();
  date_timezone_set($date_time, timezone_open($timezone ? $timezone : 'UTC'));
  $offset = date_offset_get($date_time);
  variable_set('date_default_timezone', $offset);
}

/**
 * Callback from user timezone settings form to update user timezone info.
 * When the timezone name is updated, update the offset as well.
 */
function date_form_update_user($timezone, $uid) {
  if (empty($timezone)) return;
  $date_time = date_create();
  date_timezone_set($date_time, timezone_open($timezone ? $timezone : 'UTC'));
  $offset = date_offset_get($date_time);
  db_query("UPDATE {users} SET timezone_name = '%s', offset = '%s' WHERE uid = %d", $timezone, $offset, $uid);
}

/**
 *  Flexible Date/Time Drop-Down Selector
 *
 * @param $params
 *   an array with the values noted in date_param_keys().
 *
 */
function date_select_process($element) {
  $element['#tree'] = TRUE;

  $element['#timezone_in']   = empty($element['#timezone_in']) ? $element['#timezone_in'] : (!$element['#blank_default'] || $element['#value'] ? 'UTC' : '');
  $element['#timezone_out']  = empty($element['#timezone_out']) ? $element['#timezone_out'] : (!$element['#blank_default'] || $element['#value'] ?  variable_get('date_default_timezone_name', NULL) : '');

  // Get the order of date elements from the provided format.
  // If the format order omits any date parts in the granularity, alter the
  // granularity array to match the format, then flip the $order array
  // to get the position for each element.
  $order = date_format_order($element['#format']);
  $element['#granularity'] = array_intersect($element['#granularity'], $order);
  $order = array_flip($order);

  $hours_format  = strpos(strtolower($element['#format']), 'a') ? 'g': 'G';
  $date =  date_form_date($element);
  if ((!$element['#blank_default'] || $element['#value'] > '') && !empty($date)) {
    $year    = date_format($date, 'Y');
    $mon     = date_format($date, 'n');
    $mday    = date_format($date, 'j');
    $hours   = date_format($date, $hours_format);
    $minutes = intval(date_format($date, 'i'));
    $seconds = intval(date_format($date, 's'));
  }
  $year = $year > 0 ? $year : date_format(date_now(), 'Y');
  $labels = date_granularity_names();
  $fields = date_granularity_parts();
  foreach ($element['#granularity'] as $part) {
    $field = $fields[$part];
    $part_required = $element['#required'] || !(in_array($part, $element['#opt_fields']) && $element['#blank_default'] != 'blank');
    $element['#default_value'][$part] = $$field;
    $element[$field] = array(
      '#title' => $labels[$part],
      '#default_value' => $$field,
      '#weight' => $order[$part],
      );
    if (!in_array($part, $element['#text_fields'])) {
      switch ($part) {
        case 'D':
          $options = date_days($part_required);
          break;
        case 'M':
          $options = date_month_names($part_required);
          break;
        case 'Y':
          $options = date_years($year - $element['#years_back'], $year + $element['#years_forward']);
          break;
        case 'H':
          $options = date_hours($hours_format, $part_required);
          break;
        case 'N':
          $options = date_minutes('i', $part_required, $element['#increment']);
          break;
        case 'S':
          $options = date_seconds('s', $part_required, $element['#increment']);
          break;
      }
      $element[$field]['#type'] = 'select';
      $element[$field]['#options'] = $options;
    }
    else {
      $element[$field]['#type'] = 'textfield';
      $element[$field]['#required'] = $part_required;
      $element[$field]['#maxlength'] = $part == 'Y' ? 4 : 2;
      $element[$field]['#size'] = $part == 'Y' ? 4 : 2;
    }
  }
  if ($hours_format == 'g') {
    $part_required = $element['#required'] || !(in_array('N', $element['#opt_fields']) && $element['#blank_default'] != 'blank');
    $element['ampm'] = array(
      '#type' => 'select',
      '#default_value' => $ampm,
      '#options' => date_ampm($part_required),
      '#required' => ($element['#required'] && !in_array('hours', $element['#opt_fields'])) ? $element['#required'] : 0,
      '#title' => t('am/pm'),
      '#weight' => 8,
    );
  }
  return $element;
}

/**
 *  Text date input form, with optional jscalendar popup
 *
 * @param $params
 *   an array with the values noted in date_param_keys().
 */
function  date_text_process($element) {
  $element['#timezone_in']   = isset($element['#timezone_in']) ? $element['#timezone_in'] : (!$element['#blank_default'] || $element['#value'] ? 'UTC' : '');
  $element['#timezone_out']  = isset($element['#timezone_out']) ? $element['#timezone_out'] : (!$element['#blank_default'] || $element['#value'] ?  _date_get_site_timezone() : '');

  $date =  date_form_date($element);
  $element['#value'] = date_format($date, $element['#format']);

  // if the jscalendar is used for input, add some attributes to be passed to the js
  // also need to adjust the date format slightly to accomodate the js capability
  if ($element['#jscalendar'] && module_exists('jscalendar')) {
    $element['#attributes'] = array('class' => 'jscalendar');
    $element['#jscalendar_ifFormat'] = date_format_to_jscalendar($element['#format']);
    $element['#jscalendar_showsTime'] = date_has_time($granularity) ? 'true' : 'false';
    $element['#jscalendar_timeFormat'] = strstr(strtolower($format), 'a') ? '12' : '24';
  }
  return $element;
}

/**
 *  Validation function for date selector
 *  $params = an array of the values from date_select_input.
 */
function  date_select_validate($element) {
  if ($element['#value']['ampm'] == 'pm' && $element['#value']['hours'] < 12) {
    $element['#value']['hours'] += 12;
  }
  if (($element['#required'] || $element['#value']['year']) && !in_array($element['#value']['year'], $element['#opt_fields'])) {
    if (!date_part_is_valid($element['#value']['year'], 'year')) {
      form_set_error($fieldname, t('year must be a number between %min and %max.', array(
        '%min' => DATE_MIN_YEAR, '%max' => DATE_MAX_YEAR)));
    }
  }
  if (($element['#required'] || $element['#value']['mon']) && !in_array($element['#value']['mon'], $element['#opt_fields'])) {
    if (!date_part_is_valid($element['#value']['mon'], 'mon')) {
      form_set_error($fieldname, t('month must be a number between 1 and 12.'));
    }
  }
  if (($element['#required'] || $element['#value']['mday']) && !in_array($element['#value']['mday'], $element['#opt_fields'])) {
    if (!date_part_is_valid($element['#value']['mday'], 'mday')) {
      form_set_error($fieldname, t('day must be a number between 1 and 31.'));
    }
  }
  if (!form_get_errors()) {
    // Convert the array back to a value.
    $string = date_convert($element['#value'], DATE_ARRAY, DATE_DATETIME);
    $string = date_limit_value($string, $element['#granularity'], DATE_DATETIME);
    $date = date_create($string, timezone_open($element['#timezone_in']));
    date_timezone_set($date, timezone_open('UTC'));
    $value = date_convert(date_format($date, DATE_FORMAT_DATETIME), DATE_DATETIME, $element['#date_type']);
    // If it creates a valid date, store it.
    if (date_is_valid($date, DATE_OBJECT)) {
      form_set_value($element, $value);
      form_set_value($element['#parents']['timezone'], $element['#timezone_in']);
      form_set_value($element['#parents']['offset'], date_offset_get($date));
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
  $element['#value'] = trim($element['#value']);
  if (!$element['#required'] && $element['#value'] == '') return TRUE;
  $string = date_convert_from_custom($element['#value'], $element['#format'], DATE_DATETIME);
  $string = date_limit_value($string, $element['#granularity'], DATE_DATETIME);
  $date = date_create($string, timezone_open($element['#timezone_in']));
  date_timezone_set($date, timezone_open('UTC'));
  $value = date_convert(date_format($date, DATE_FORMAT_DATETIME), DATE_DATETIME, $element['#date_type']);
  if (date_is_valid($date, DATE_OBJECT)) {
    form_set_value($element, $value);
    form_set_value($element['#parents']['timezone'], $element['#timezone_in']);
    form_set_value($element['#parents']['offset'], date_offset_get($date));
  }
  else {
    form_set_error($element, t('A valid date cannot be created from %date', array('%date' => $element['#value'])));
  }
}

/**
 * Set value, timezone, and offset in the form element
 */
function _date_select_value(&$form) {
  if (isset($form['#default_value'])) {
    $form['#value'] = $form['#default_value'];
  }
  else {
    $form['#value'] = date_form_date($element);
  }
}

/**
 * Set value, timezone, and offset in the form element
 */
function _date_text_value(&$form) {
  if (isset($form['#default_value'])) {
    $form['#value'] = $form['#default_value'];
  }
  else {
    $form['#value'] = date_form_date($element);
  }
}

/**
 * Create a date from the database value, adjusting it to the
 * local timezone for display on the form.
 */
function  date_form_date($element) {
  $value = date_convert($element['#value'], $element['#date_type'], DATE_DATETIME);
  $date = NULL;
  if ($value !==  DATE_ERROR && !empty($value)) {
    $date = date_create($value, timezone_open($element['#timezone_in']));
    date_timezone_set($date, timezone_open($element['#timezone_out']));
  }
  elseif ($element['#blank_default'] == 'now') {
    $date = date_create('now', timezone_open($element['#timezone_in']));
    date_timezone_set($date, timezone_open($element['#timezone_out']));
  }
  elseif ($value !==  DATE_ERROR && $element['#blank_default'] != 'blank') {
    $date = date_create('now', timezone_open($element['#timezone_in']));
    date_timezone_set($date, timezone_open($element['#timezone_out']));
    date_modify($date, $element['#blank_default']);
  }
  return $date;
}

/**
 * Convert a date input in a custom format to a standard date type
 *
 * Handles conversion of translated month names (turns t('Mar') or
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
 *  Themes for date input form elements
 */

/**
 *  Date selector form
 */
function theme_date_select_input($form) {
  $form['#type']   = 'fieldset';
  return drupal_render($form);
}

/**
 *  Date selector description
 */
function theme_date_select_description($form) {
  $description = drupal_render($form);
  if ($description) {
    return '<br class="clear" /><div class="description"><div class="form-item">'. $description.'</div></div>';
  }
  return '';
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
  return theme('textfield', $element);
}
