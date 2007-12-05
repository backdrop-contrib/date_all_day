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
 * Fill the element #default_value with a date object adjusted to the
 * proper timezone.
 *
 * date_select will create a collection of form elements, with a separate
 * select or textfield for each date part. The whole collection will get
 * re-formatted back into a date object during validation.
 *
 * date_text will create a textfield that can contain a whole
 * date or any part of a date as text. The user input value will be re-formatted
 * back into a date object during validation.
 *
 * #date
 *   Pass in a date object set to the timezone used in the form display,
 *   generally the site or user timezone.
 *
 * #date_type
 *   The type of date to be created from this element, could be
 *   DATE_UNIX, DATE_DATETIME, or DATE_ISO
 *
 * #date_format
 *   A format string that describes the format and order of date parts to
 *   display in the edit form for this element, be sure to add 'A' or 'a'
 *   to get an am/pm selector, defaults to short site default format.
 *
 * #date_granularity
 *   An array of date parts to be collected, default is ('month', 'day', 'year'),
 *   Allowed values are: year, month, day, hour, inute, second.
 *
 * #date_increment
 *   Increment minutes and seconds by this amount, default is 1.
 *
 * #date_year_range
 *   The number of years to go back and forward in the year selector,
 *   default is -3:+3 (3 back and 3 forward).
 *
 * #date_text_parts
 *   Array of date parts that should use textfield instead of select
 *   i.e. array('year') will format the year as a textfield and other
 *   date parts as drop-down selects.
 *
 * #date_empty
 *   handling for empty date values, can be 'blank' or 'now'.
 */
function _date_api_elements() {
  $date_base = array(
    '#input' => TRUE, '#tree' => TRUE, '#date' => NULL,
    '#date_type' => DATE_UNIX,
    '#date_format' => variable_get('date_format_short', 'm/d/Y - H:i'),
    '#date_granularity' => array('month', 'day', 'year'),
    '#date_text_parts' => array(),
    '#date_increment' => 1,
    '#date_year_range' => '-3:+3',
    '#date_empty' => 'now',
    );
  $type['date_select'] = array_merge($date_base, array(
    '#process' => array('date_select_process' => array()),
    '#validate' => array('date_select_validate' => array()),
    ));
  $type['date_text'] = array_merge($date_base, array(
    '#process' => array('date_text_process' => array()),
    '#validate' => array('date_text_validate' => array()),
    ));
  $type['date_timezone'] = array(
    '#input' => TRUE, '#tree' => TRUE,
    '#process' => array('date_timezone_process' => array()),
    '#validate' => array('date_timezone_validate' => array()),
    );
  return $type;
}

/**
 * Create a timezone form element.
 *
 * @param array $element
 * @return array
 *   the timezone form element
 */
function date_timezone_process($element, $edit = NULL) {
  $element['timezone'] = array(
    '#type' => 'select',
    '#title' => theme('date_select_label_timezone', 'select', $element),
    '#default_value' => $element['#value'],
    '#options' => date_timezone_names($element['#required']),
    '#weight' => $element['#weight'],
    '#required' => $element['#required'],
    '#attributes' => array('class' => 'date-timezone'),
    '#theme' => 'date_select_element',
  );
  return $element;
}

/**
 * Text date input form.
 *
 * Display all or part of a date in a single textfield.
 *
 * The exact parts displayed in the field are those in #date_granularity.
 * The display of each part comes from #date_format.
 */
function date_text_process($element, $edit = NULL) {
  if (empty($element['#date']) && $element['#date_empty'] == 'now') {
    $element['#date'] = date_now();
  }

  // Don't overwrite values already added to $element['date'] in case
  // using something like jscalendar that needs to set custom values.
  $element['#tree'] = TRUE;
  $element['date']['#type'] = 'textfield';
  $element['date']['#weight'] = !empty($element['date']['#weight']) ? $element['date']['#weight'] : $element['#weight'];
  $element['date']['#default_value'] = is_object($element['#date'] ) ? date_format($element['#date'] , date_input_format($element)) : '';
  $element['date']['#attributes']['class'] = $element['date']['#attributes']['class'] . ' date-date';
  $element['date']['#required'] = $element['#required'];
  $element['date']['#theme'] = 'date_textfield_element';
  return $element;
}

/**
 * Flexible date/time drop-down selector.
 *
 * Splits date into a collection of date and time sub-elements, one
 * for each date part. Each sub-element can be either a textfield or a
 * select, based on the value of ['#date_settings']['text_fields'].
 *
 * The exact parts displayed in the field are those in #date_granularity.
 * The display of each part comes from ['#date_settings']['format'].
 */
function date_select_process($element, $edit = NULL) {
  if (empty($element['#date']) && $element['#date_empty'] == 'now') {
    $element['#date'] = date_now();
  }
  $element['#tree'] = TRUE;
  $element += date_parts_element($element, $element['#date'], $element['#date_granularity'], $element['#date_format']);
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
  $count = 0;
  $increment = min(intval($field['widget']['increment']), 1);
  foreach ($granularity as $field) {
    $part_required = $element['#required'];
    $part_type = in_array($field, $element['#date_text_parts']) ? 'textfield' : 'select';
    $sub_element[$field] = array(
      '#title' => theme('date_select_label_'. $field, $part_type, $element),
      '#weight' => $order[$part],
      '#required' => $part_required,
      '#attributes' => array('class' => $element['#attributes']['class'] . ' date-'. $field),
      );
    switch ($field) {
      case 'year':
        $sub_element[$field]['#default_value'] = is_object($date) ? date_format($date, 'Y') : '';
        $range = explode(':', $element['#date_year_range']);
        $sub_element[$field]['#options'] = drupal_map_assoc(date_years($year + $range[0], $year + $range[1], $part_required));
        break;
      case 'month':
        $sub_element[$field]['#default_value'] = is_object($date) ? date_format($date, 'n') : '';
        $sub_element[$field]['#options'] = (date_month_names($part_required));
        break;
      case 'day':
        $sub_element[$field]['#default_value'] = is_object($date) ? date_format($date, 'j') : '';
        $sub_element[$field]['#options'] = drupal_map_assoc(date_days($part_required));
        break;
      case 'hour':
        $sub_element[$field]['#default_value'] = is_object($date) ? date_format($date, $hours_format) : '';
        $sub_element[$field]['#options'] = drupal_map_assoc(date_hours($hours_format, $part_required));
        break;
      case 'minute':
        $sub_element[$field]['#default_value'] = is_object($date) ? date_pad(intval(round(intval(date_format($date, 'i')) / $element['#date_increment']) * $element['#date_increment'])) : '';
        $sub_element[$field]['#options'] = drupal_map_assoc(date_minutes('i', $part_required, $element['#date_increment']));
        break;
      case 'second':
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
  if (strtolower($hours_format) == 'g' && date_has_time($granularity)) {
    $sub_element['ampm'] = array(
      '#type' => 'radios',
      '#return_value' => is_object($date) ? (date_format($date, 'G') < 12 ? 'am' : 'pm') : '',
      '#options' => drupal_map_assoc(date_ampm(TRUE)),
      '#weight' => 8,
      '#attributes' => array('class' => 'date-ampm'),
    );
  }
  return $sub_element;
}

/**
 *  Validation function for date selector
 *  $params = an array of the values from date_select_input.
 */
function date_select_validate($element) {
  $field = $element['#field'];
  if ($element['#value']['ampm'] == 'pm' && $element['#value']['hour'] < 12) {
    $element['#value']['hour'] += 12;
  }
  if ($element['#required'] || !empty($element['#value']['year'])) {
    if ($element['#value']['year'] < variable_get('date_min_year', 1) || $element['#value']['year'] > variable_get('date_max_year', 4000)) {
      form_set_error($fieldname, t('year must be a number between %min and %max.', array(
        '%min' => variable_get('date_min_year', 1), '%max' => variable_get('date_max_year', 4000))));
    }
  }
  if ($element['#required'] || !empty($element['#value']['month'])) {
    if ($element['#value']['month'] < 1 || $element['#value']['month'] > 12) {
      form_set_error($fieldname, t('month must be a number between 1 and 12.'));
    }
  }
  if ($element['#required'] || !empty($element['#value']['day'])) {
    if ($element['#value']['day'] < 1 || $element['#value']['day'] > 31) {
      form_set_error($fieldname, t('day must be a number between 1 and 31.'));
    }
  }
  if (!form_get_errors()) {
    // Convert the array back to a value.
    $value = date_convert($element['#value'], DATE_ARRAY, DATE_DATETIME);

    // If it creates a valid date, store it.
    if (date_is_valid($value, DATE_DATETIME)) {
      $timezone = date_timezone_get($element['#date']);
      $date = date_create($value, $timezone);
      $value = date_convert($date, DATE_OBJECT, $element['#date_type']);
      form_set_value($element, $value);
    }
    else {
      form_set_error($element, t('A valid date cannot be constructed from %m-%d-%year.', array(
        '%m' => $element['#value']['month'],
        '%d' => $element['#value']['day'],
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

  $value = date_convert_from_custom($input, $element['#date_format']);

  // If it creates a valid date, store it.
  if (date_is_valid($value, DATE_DATETIME)) {
    $timezone = date_timezone_get($element['#date']);
    $date = date_create($value, $timezone);
    $value = date_convert($date, DATE_OBJECT, $element['#date_type']);
    form_set_value($element, $value);
  }
  else {
    form_set_error($element, t('A valid date cannot be created from %date', array('%date' => $value)));
  }
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
 * @return mixed
 *   input value converted to a DATE_DATETIME value
 */
function date_convert_from_custom($date, $format) {
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
  $final_date = array('hour' => 0, 'minute' => 0, 'second' => 0,
    'month' => 0, 'day' => 0, 'year' => 0);
  foreach($letters as $i => $letter) {
    $value = $values[$i];
    switch($letter) {
      case 'd':
      case 'j':
        $final_date['day'] = intval($value);
        break;
      case 'n':
      case 'm':
        $final_date['month'] = intval($value);
        break;
      case 'F':
        $array_month_long = array_flip(date_month_names());
        $final_date['month'] = $array_month_long[$value];
        break;
      case 'M':
        $array_month = array_flip(date_month_names_abbr());
        $final_date['month'] = $array_month[$value];
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
        $final_date['hour'] = intval($value);
        break;
      case 'i':
        $final_date['minute'] = intval($value);
        break;
      case 's':
        $final_date['second'] = intval($value);
        break;
      case 'U':
        return date_convert($value, DATE_UNIX, DATE_DATETIME);
        break;
    }
  }
  if ($ampm == 'pm' && $final_date['hour'] != 12) {
    $final_date['hour'] += 12;
  }
  return date_convert($final_date, DATE_ARRAY, DATE_DATETIME);
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
function theme_date_select_label_month($part_type, $element) {
  return t('Month');
}
function theme_date_select_label_day($part_type, $element) {
  return t('Day');
}
function theme_date_select_label_hour($part_type, $element) {
  return t('Hour');
}
function theme_date_select_label_minute($part_type, $element) {
  return t('Minute');
}
function theme_date_select_label_second($part_type, $element) {
  return t('Second');
}
function theme_date_select_label_ampm($part_type, $element) {
  return t('am/pm');
}
function theme_date_select_label_timezone($part_type, $element) {
  return t('Timezone');
}