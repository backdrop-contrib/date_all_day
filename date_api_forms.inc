<?php
//$Id$
/**
 * @file
 * Date forms and form themes and validation.
 * This file is only included during the edit process to reduce memory usage.
 */

/**
 * Parameters for date input fields, designed to have sane defaults so any
 * or all can be omitted.
 *
 * @param value
 *   the date/time to be processed, default is the current date and time
 * @param type
 *   the type of the date value, DATE_UNIX, DATE_ISO, DATE_DATETIME, etc
 * @param label
 *   the label for the form item, default is 'Date'
 * @param field_name
 *   the field name for the form item, default is 'value'
 * @param weight
 *   the weight of the date element, default is 0
 * @param delta
 *   a delta value for the item, to accomodate multiple date fields,
 *   default is 0
 * @param timezone_in
 *   the timezone of the date/time value being passed in, default is UTC
 * @param timezone_out
 *   the timezone to be used when displaying the date, default is date
 *   site timezone, if set, or UTC. Valid timezones are standard timezone
 *   ids like US/Central, America/New_York, UTC
 * @param am/pm
 *   0 to use 24 hour format, 1 to use 12 hour format, default is 0
 * @param granularity
 *   an array of date parts to be allowed, default is array(M, D, Y),
 *   Y - year, M - month, D - day, H - hours, N - minutes, S - seconds
 * @param format
 *   a format string that describes the type and order of input elements,
 *   defaults to short site default format
 * @param blank_default
 *   how to handle empty dates:
 *   1 to show an empty date field with blank values,
 *   0 to fill with current date, default is 0
 * @param required
 *   1 if the field must be filled out, default is 1
 * @param description
 *   text to be used as a description for the fieldset
 *
 * // Date select options
 * @param increment
 *   increment minutes and seconds by increment amount, default is 1
 * @param opt_fields
 *   an array of fields that need not be filled out, default is empty array
 * @param years_back
 *   the number of years to go back on the year selector
 * @param years_forward
 *   the number of years to go forward on the year selector
 *
 * // jscalendar options
 * @param jscalendar
 *   1 use if available, 0 do not use
 *
 * @return array
 */
function date_param_keys() {
  return array(
    'label',       'value',         'timezone_in',  'timezone_out',
    'type',        'format',        'am/pm/',       'weight',
    'delta',       'required',      'increment',    'granularity',
    'description', 'blank_default', 'jscalendar',   'opt_fields',
    'years_back',  'years_forward', 'field_name',   'jscalendar',
    );
}

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
 *  Timezone input form
 *
 * @param $params
 *   an array with the values noted in date_param_keys().
 */
function  date_timezone_input($params) {
  // Set some default values for $params that are not supplied.
  $required      = isset($required) ? $required : 1;
  $blank_default = isset($blank_default) ? $blank_default : 'UTC';
  $timezone_out  = isset($timezone_out) ? $timezone_out : (!$blank_default || $value ?  variable_get('date_default_timezone_name', NULL) : '');
  $granularity   = is_array($granularity) ? $granularity : array('T');
  $label         = $label ? $label .' ' : '';

  if (in_array('T', $granularity)) {
    $form['timezone'] = array(
      '#title' =>  $label . t('timezone'),
      '#type' => 'select',
      '#default_value' => $timezone_out ? $timezone_out : $blank_default,
      '#options' => $required ? date_timezone_names() : array('' => '') + date_timezone_options(),
      '#required' => ($required && !in_array('timezone', $opt_fields)) ? $required : 0,
      );
  }
  else {
    $form['timezone'] = array(
      '#type' => 'hidden',
      '#value' => $timezone_out,
    );
  }
  $form['#theme'] = 'date_form_timezone';
  $form['#weight'] = $weight;
  return $form;
}

/**
 *  Flexible Date/Time Drop-Down Selector
 *
 * @param $params
 *   an array with the values noted in date_param_keys().
 *
 */
function  date_select_input($params) {
  // Set some default values for $params that are not supplied,
  extract($params);
  $type          = isset($type) ? $type : DATE_DATETIME;
  $label         = $label ? $label : t('Date');
  $delta         = isset($delta) ? $delta : 0;
  $granularity   = is_array($granularity) ? $granularity : array('M', 'D', 'Y');
  $increment     = isset($increment) ? $increment : 1;
  $required      = isset($required) ? $required : 1;
  $type          = isset($type) ? $type : DATE_ISO;
  $weight        = isset($weight) ? $weight : 0;
  $opt_fields    = is_array($opt_fields) ? $opt_fields : array();
  $blank_default = isset($blank_default) ? $blank_default : 0;
  $timezone_in   = isset($timezone_in) ? $timezone_in : (!$blank_default || $value ? 'UTC' : '');
  $timezone_out  = isset($timezone_out) ? $timezone_out : (!$blank_default || $value ?  _date_get_site_timezone() : '');
  $hours_format  = !empty($formats['input']['am_pm']) ? 'g': 'G';
  $field_name    = $field_name ? $field_name : 'value';

  // Get the order to be used for the date parts from the format.
  $format = isset($format) ? $format : variable_get('date_format_short', 'm/d/Y - H:i');
  $format = strtr($format, array('M' => 'm', 'j' => 'd'));
  $order = array(
    'M' => strpos($format, 'm'),
    'D' => strpos($format, 'd'),
    'Y' => strpos($format, 'Y'));

  // Recreate $params.
  foreach (date_param_keys() as $key) {
    $params[$key] = $$key;
  }

  $date =  date_form_date($type, $value, $timezone_in, $timezone_out, $blank_default);
  if ((!$blank_default || $value > '') && !empty($date)) {
    $year    = date_format($date, 'Y');
    $mon     = date_format($date, 'n');
    $mday    = date_format($date, 'j');
    $hours   = date_format($date, $hours_format);
    $minutes = intval(date_format($date, 'i'));
    $seconds = intval(date_format($date, 's'));
    $ampm    = strtolower(date_format($date, 'a'));
  }

  // create the form values
  $form['#title']  = $label;
  $form['#weight'] = intval($weight);
  $form['#theme']  = 'date_form_select';
  $form['#attributes'] = array('class' => 'container-inline-date clear-block');
  $form['#validate'] = array('date_select_validate' => $params);
  if (in_array('D', $granularity)) {
    $form['mday'] = array(
      '#default_value' => $mday,
      '#title' => t('day'),
      '#required' => ($required && !in_array('mday', $opt_fields)) ? $required : 0,
      '#weight' => $order['D'],
    );
    if ($select_day) {
      $days_array = date_days();
      if (!$required || in_array('mday', $opt_fields) || $blank_default) array_unshift($days_array, '');
      $form['mday']['#type'] = 'select';
      $form['mday']['#options'] = $days_array;
    }
    else {
      $form['mday']['#type'] = 'textfield';
      $form['mday']['#maxlength'] = 2;
      $form['mday']['#size'] = 2;
    }
  }
  if (in_array('M', $granularity)) {
    $form['mon'] = array(
      '#default_value' => $mon,
      '#title' => t('month'),
      '#required' => ($required && !in_array('mon', $opt_fields)) ? $required : 0,
      '#weight' => $order['M'],
    );
    if ($select_month) {
      $months_array = date_month_names();
      if (!$required || in_array('mon', $opt_fields) || $blank_default) array_unshift($months_array, '');
      $form['mon']['#type'] = 'select';
      $form['mon']['#options'] = $months_array;
    }
    else {
      $form['mon']['#type'] = 'textfield';
      $form['mon']['#maxlength'] = 2;
      $form['mon']['#size'] = 2;
    }
  }
  if (in_array('Y', $granularity)) {
    $form['year'] = array(
      '#default_value' => $year,
      '#title' => t('year'),
      '#required' => ($required && !in_array('year', $opt_fields)) ? $required : 0,
      '#weight' => $order['Y'],
    );
    if ($select_year) {
      $year = $year > 0 ? $year : date('Y', date_time());
      $years_array = date_years(intval($year - $years_back), intval($year + $years_forward));
      // array_unshift converts the assoc array to a numeric one, can't use it here
      if (!$required || in_array('year', $opt_fields) || $blank_default) $years_array = array(0 => '') + $years_array;
      $form['year']['#type'] = 'select';
      $form['year']['#options'] = $years_array;
    }
    else {
      $form['year']['#type'] = 'textfield';
      $form['year']['#maxlength'] = 4;
      $form['year']['#size'] = 4;
    }
  }
  if (in_array('H', $granularity)) {
    $hours_array = date_hours($hours_format);
    $form['hours'] = array(
      '#type' => 'select',
      '#default_value' => $hours,
      '#options' => $hours_array,
      '#required' => ($required && !in_array('hours', $opt_fields)) ? $required : 0,
      '#title' => t('hour'),
      '#weight' => 4,
    );
  }
  if (in_array('N', $granularity)) {
    $form['minutes'] = array(
      '#type' => 'select',
      '#default_value' => $minutes,
      '#options' => date_minutes('m', $increment),
      '#required' => ($required && !in_array('minutes', $opt_fields)) ? $required : 0,
      '#title' => t('minute'),
      '#weight' => 5,
    );
  }
  if (in_array('S', $granularity)) {
    $form['seconds'] = array(
      '#type' => 'select',
      '#default_value' => $seconds,
      '#options' => date_seconds('s', $increment),
      '#required' => ($required && !in_array('seconds', $opt_fields)) ? $required : 0,
      '#title' => t('second'),
      '#weight' => 6,
    );
  }
  if ($formats['input']['am_pm']) {
    if (!$required || in_array('hours', $opt_fields) || $blank_default) array_unshift($options, '');
    $form['ampm'] = array(
      '#type' => 'select',
      '#default_value' => $ampm,
      '#options' => date_ampm(),
      '#required' => ($required && !in_array('hours', $opt_fields)) ? $required : 0,
      '#title' => t('am/pm'),
      '#weight' => 8,
    );
  }
  $form[] = array(
    '#theme' => 'date_form_select_description',
    '#weight' => 11,
    'description' => array(
      '#value' =>  $description,
     ),
  );
  return $form;
}

/**
 *  Text date input form, with optional jscalendar popup
 *
 * @param $params
 *   an array with the values noted in date_param_keys().
 */
function  date_text_input($params) {
  // Set some default values for $params that are not supplied,
  extract($params);
  $type          = isset($type) ? $type : DATE_DATETIME;
  $label         = $label ? $label : t('Date');
  $delta         = isset($delta) ? $delta : 0;
  $granularity   = is_array($granularity) ? $granularity : array('M', 'D', 'Y');
  $required      = isset($required) ? $required : 1;
  $blank_default = isset($blank_default) ? $blank_default : 0;
  $weight        = isset($weight) ? $weight : 0;
  $timezone_in   = isset($timezone_in) ? $timezone_in : (!$blank_default || $value ? 'UTC' : '');
  $timezone_out  = isset($timezone_out) ? $timezone_out : (!$blank_default || $value ?  _date_get_site_timezone() : '');
  $jscalendar    = $params['jscalendar'] ? 1 : 0;
  $opt_fields    = is_array($opt_fields) ? $opt_fields : array();
  $field_name    = $field_name ? $field_name : 'value';
  $format = isset($format) ? $format : variable_get('date_format_short', 'm/d/Y - H:i');
  // Recreate $params.
  foreach (date_param_keys() as $key) {
    $params[$key] = $$key;
  }
  $date =  date_form_date($type, $value, $timezone_in, $timezone_out, $blank_default);
  $value = date_format($date, $format);

  $form[$field_name] = array(
    '#type' => 'textfield',
    '#title' => $label,
    '#default_value' => $value,
    '#required' => ($delta == 0) ? $required : 0,
    '#description' => $description,
    '#weight' => $weight,
    '#validate' => array('date_text_validate' => $params),
  );
  // if the jscalendar is used for input, add some attributes to be passed to the js
  // also need to adjust the date format slightly to accomodate the js capability
  if ($jscalendar && module_exists('jscalendar')) {
    $form['#theme'] = 'date_form_jscalendar';
    $form[$field_name]['#attributes'] = array('class' => 'jscalendar');
    $form[$field_name]['#jscalendar_ifFormat'] = $formats['input']['jscal'];
    $form[$field_name]['#jscalendar_showsTime'] = date_has_time($granularity) ? 'true' : 'false';
    $form[$field_name]['#jscalendar_timeFormat'] = $formats['input']['am_pm'] ? '12' : '24';
  }
  else {
    $form['#theme'] = 'date_form_text';
  }
  return $form;
}

/**
 *  Validation function for date selector
 *  $params = an array of the values from date_select_input.
 */
function  date_select_validate($form, $params = array()) {
  extract($params);

  $type        = $type > '' ? $type : DATE_DATETIME;
  $required    = isset($required) ? $required : 1;
  $opt_fields  = is_array($opt_fields) ? $opt_fields : array();
  $granularity = $granularity ? $granularity : array('M', 'D', 'Y');

  if ($value['ampm'] == 'pm' && $value['hours'] < 12) $value['hours'] += 12;
  // check for invalid numbers in fields that were submitted using textfields
  if (($required || $value['year']) && !in_array($value['year'], $opt_fields)) {
    if (!date_part_is_valid($value['year'], 'year')) form_set_error($fieldname, t('year must be a number between %min and %max.', array('%min' => DATE_MIN_YEAR, '%max' => DATE_MAX_YEAR)));
  }
  if (($required || $value['mon']) && !in_array($value['mon'], $opt_fields)) {
    if (!date_part_is_valid($value['mon'], 'mon')) form_set_error($fieldname, t('month must be a number between 1 and 12.'));
  }
  if (($required || $value['mday']) && !in_array($value['mday'], $opt_fields)) {
    if (!date_part_is_valid($value['mday'], 'mday')) form_set_error($fieldname, t('day must be a number between 1 and 31.'));
  }
  // adjust back to 24 hours time if 12 hours input was collected
  if ($array['ampm'] == 'pm' && $array['hours'] < 12) $array['hours'] += 12;

  if (!form_get_errors()) {
    // Convert the array back to a value.
    $string = date_granularity_value(date_convert($array, DATE_ARRAY, $type), $type);
    $date = date_create($string, timezone_open($timezone_in));
    date_timezone_set($date, timezone_open('UTC'));
    $date = date_convert(date_format($date, DATE_DATETIME), DATE_DATETIME, $type);
    // If it creates a valid date, store it.
    if (date_is_valid($date, $type)) {
      form_set_value($fieldname, $date);
    }
    else {
      form_set_error($fieldname, t('A valid date cannot be constructed from %m-%d-%year.', array('%m' => $value['mon'], array('%d' => $value['mday'], '%y' => $value['year']))));
    }
  }
}

/**
 *  Validation for text input
 *  $params = an array of the values from date_text_input.
 */
function  date_text_validate($form, $params) {
  extract($params);
  $value = trim($value);
  if (!$required && $value == '') return TRUE;
  if ($string = date_convert_from_custom($value, $format, $type)) {
    $date = date_create($string, timezone_open($timezone_in));
    if (date_is_valid($date, $type)) {
      form_set_value($fieldname, $date);
    }
    else {
      form_set_error($field_name, t('A valid date cannot be created from %date', array('%date' => $value)));
    }
  }
}

/**
 * Create a date for the form from the input value,
 * adjusting it to the right timezone.
 */
function  date_form_date($type, $value_in, $timezone_in, $timezone_out, $blank_default) {
  $value = date_convert($value_in, $type, DATE_DATETIME);
  $date = NULL;
  if ($value !=  DATE_ERROR && !empty($value)) {
    $date = date_create($value, $timezone_in);
    date_timezone_set($date, timezone_open($timezone_out));
  }
  else {
    if (!$blank_default && $value_in == '') {
      $date = date_create('now', timezone_open($timezone_in));
      date_timezone_set($date, timezone_open($timezone_out));
    }
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
function theme_date_form_select($form) {
  $form['#type']   = 'fieldset';
  return drupal_render($form);
}

/**
 *  Date selector description
 */
function theme_date_form_select_description($form) {
  $description = drupal_render($form);
  if ($description) {
    return '<br class="clear" /><div class="description"><div class="form-item">'. $description.'</div></div>';
  }
  return '';
}

/**
 *  Text input form
 */
function theme_date_form_text($form) {
  return drupal_render($form);
}

/**
 *  jsCalendar input form
 */
function theme_date_form_jscalendar($form) {
  return drupal_render($form);
}

/**
 *  Timezone input form
 */
function theme_date_form_timezone($form) {
  return drupal_render($form);
}