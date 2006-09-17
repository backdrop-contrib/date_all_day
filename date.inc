<?php
//$Id$
/**
 * @file
 * Date/time API functions
 *
 * this api uses timezones rather than offsets to avoid dst problems
 * the event_timezone.inc api is used to compute the correct offset for a timezone, taking into account dst
 *
 * Usage examples:
 *  create a new date object:
 *    $date = date_make_date();
 *
 *  set local value of 2006-05-04T10:24:00 US/Eastern
 *    date_set_date($date, '2006-05-04T10:24:00', 'US/Eastern', 'local', DATE_ISO);
 *
 *  display the local value using the format string 'm/d/Y H:i'
 *    print date_show_date($date, 'm/d/Y H:i', 'local');
 *
 *  display the db values of the date object:
 *    print date_show_value($date, 'db', DATE_UNIX); // the unix db value
 *    print date_show_value($date, 'db', DATE_ISO);  // the iso db value
 *
 *  display the date object and all its parts:
 *    print_r($date);  
 */

// add here to define additional date types
define('DATE_ISO',  'date');
define('DATE_UNIX', 'datestamp');
define('DATE_STRING_ISO', "Y-m-d\TH:i:s");

/**
 *  identify the file to be used for timezone information
 * 
 *  use the event_timezones.inc file for timezone conversion
 *  if event is not enabled, use a copy of that file, stored as date_timezones.inc
 *  this large file will be included only when needed to reduce overhead
 */
if (!module_exist('event')) {
  define('DATE_TIMEZONES', drupal_get_path('module', 'date') .'/date_timezones.inc');
} else {
  define('DATE_TIMEZONES', drupal_get_path('module', 'event') .'/event_timezones.inc');
}


/**
 *  Set up optional adodb date library to handle dates outside the normal php range
 *  Could be extended later for other date libraries
 * 
 *  adodb date library instructions:
 *    get the file adodb-time.inc.php from http://phplens.com/phpeverywhere/adodb_date_library
 *    put it in the same folder with date.module
 * 
 *  external library will be used if available, otherwise api will use standard php date handling functions
 */
if (file_exists(drupal_get_path('module', 'date') .'/adodb-time.inc.php')) {

  define('DATE_LIBRARY',  'ADODB');
  define('DATE_MAX_YEAR', 3000);
  define('DATE_MIN_YEAR', 100);

  // if there is another program loading adodb-time.inc.php in another directory after this file has run
  // there will be php errors stating that the functions have already been declared
  // to fix this, alter the path to the file below to point to the path used by the program that uses the file last
  // 
  // the standard path to this file
  //     drupal_get_path('module', 'date') .'/adodb-time.inc.php';
  // an example alternate path if the gallery2 program is installed (gallery2 uses adodb, too)
  //     strtolower($_SERVER["DOCUMENT_ROOT"]) .'/gallery2/lib/adodb/adodb-time.inc.php';

  define('DATE_LIBRARY_FILE', drupal_get_path('module', 'date') .'/adodb-time.inc.php');

} else {

  define('DATE_LIBRARY',    '');
  if (strstr(PHP_OS, 'WIN') && PHP_VERSION < 5.1) {
    define('DATE_MIN_YEAR', 1970);
  } else {
    define('DATE_MIN_YEAR', 1901);
  }
  define('DATE_MAX_YEAR', 2038);

}

/**
 *  Date wrapper functions
 * 
 *  will use external library if available, otherwise native php date functions
 *  currently set up for adodb date library, could be extended to include other date libraries
 * 
 *  @param $array - array parameters are in the format created by getdate()
 *  prefix functions with @ to surpress ugly windows error messages for dates outside valid range
 */
function date_load_library() {
  if (DATE_LIBRARY == 'ADODB' && !function_exists('adodb_date_test_date') && file_exists(DATE_LIBRARY_FILE)) {
    include_once(DATE_LIBRARY_FILE);
  }
}
function date_getdate($timestamp) {
  date_load_library();
  switch (DATE_LIBRARY) {
    case ('ADODB'):
      return adodb_getdate($timestamp);
    default:
      return getdate($timestamp);
  }
}
/**
 *  We need a gmgetdate function, which does not exist
 *  because the getdate function creates an array where date parts and timestamp don't match
 *  getdate date parts are timezone-adjusted to the server zone and timestamp is not
 *  we need an array where date parts match the timestamp
 */
function date_gmgetdate($timestamp) {
  date_load_library();
  switch (DATE_LIBRARY) {
    case ('ADODB'):
      $array    = adodb_getdate(date_adj_zone($timestamp));
      $array[0] = $timestamp;
      return $array;
    default:
      $array    = getdate(date_adj_zone($timestamp));
      $array[0] = $timestamp;
      return $array;
  }
}
function date_date($format, $timestamp = FALSE) {
  date_load_library();
  switch (DATE_LIBRARY) {
  case ('ADODB'):
    return @adodb_date($format, ($timestamp));
  default:
    return date($format, $timestamp);
  }
}
function date_gmdate($format, $timestamp = FALSE) {
  date_load_library();
  switch (DATE_LIBRARY) {
    case ('ADODB'):
    return @adodb_gmdate($format, $timestamp);
  default:
    return @gmdate($format, $timestamp);
  }
}
function date_mktime($array) {
  date_load_library();
  switch (DATE_LIBRARY) {
  case ('ADODB'):
    $timestamp = @adodb_mktime($array['hours'], $array['minutes'], $array['seconds'], $array['mon'], $array['mday'], $array['year']);
    return $timestamp;
  default:
    $timestamp = @mktime($array['hours'], $array['minutes'], $array['seconds'], $array['mon'], $array['mday'], $array['year']);
    return $timestamp;
  }
}
function date_gmmktime($array) {
  date_load_library();
  switch (DATE_LIBRARY) {
    case ('ADODB'):
    $timestamp = @adodb_gmmktime($array['hours'], $array['minutes'], $array['seconds'], $array['mon'], $array['mday'], $array['year']);
    return $timestamp;
  default:
    $timestamp = @gmmktime($array['hours'], $array['minutes'], $array['seconds'], $array['mon'], $array['mday'], $array['year']);
    return $timestamp;
  }
}
function date_strftime($format, $timestamp = FALSE) {
  date_load_library();
  switch (DATE_LIBRARY) {
    case ('ADODB'):
    return @adodb_strftime($format, $timestamp);
  default:
    return @strftime($format, $timestamp);
 }  
}
function date_gmstrftime($format, $timestamp = FALSE) {
  date_load_library();
  switch (DATE_LIBRARY) {
    case ('ADODB'):
    return @adodb_gmstrftime($format, $timestamp);
  default:
    return @gmstrftime($format, $timestamp);
 }
}

/**
 *  These functions will remove server timezone adjustment from timestamps
 *  needed because some php date functions automatically adjust to server zone
 *  but the date object uses raw values for date parts and does manual timezone adj
 *  needed for ability to accomodate timezones other than server zone
 */
function date_adj_zone($timestamp) {
  date_load_library();
  switch (DATE_LIBRARY) {
    case ('ADODB'):
      return intval($timestamp - adodb_date('Z', $timestamp));
  default:
      return intval($timestamp - date('Z', $timestamp));
  }
}
function date_gmadj_zone($timestamp) {
  date_load_library();
  switch (DATE_LIBRARY) {
    case ('ADODB'):
      return intval($timestamp + adodb_date('Z', $timestamp));
  default:
      return intval($timestamp + date('Z', $timestamp));
  }
}

/**
 *  Function to create a date object
 * 
 *  All creation, manipulation, and display of the date is done using this date object
 * 
 *  @param $value - the date/time value to set
 *  @param $timezone - 'GMT', 'none', or timezone name - the timezone of this value
 *  @param $type - db or local, the part of the date object to set
 *  @param $format - DATE_UNIX or DATE_ISO, the format of the provided value
 *  @return - the date object
 */
function date_make_date($value = '', $timezone = 'GMT', $type = 'db', $format = DATE_ISO) {
  
  $date                   = new StdClass();
  $date->db               = new StdClass();
  $date->db->timestamp    = NULL;
  $date->db->iso          = NULL;
  $date->db->parts        = NULL;
  $date->local            = new StdClass();
  $date->local->timestamp = NULL;
  $date->local->iso       = NULL;
  $date->local->parts     = NULL;
  $date->local->timezone  = NULL;

  if (trim($value) == '') return $date;

  // if initialized with a date, go ahead and set that date up
  date_set_date($date, $value, $timezone, $type, $format);
  
  return $date;

}

/**
 *  Function to set local and db date parts in the date object
 * 
 *  @param $date - the date object
 *  @param $value - the date/time value to set
 *  @param $timezone - 'GMT', 'none', or timezone name - the timezone of this value
 *     - the 'none' option will do no timezone conversions, dates will be stored exactly as entered
 *  @param $type - db or local, the part of the date object to set
 *     - if you supply a local value and timezone, the function will create the related db values
 *  @param $format - DATE_UNIX or DATE_ISO, the format of the provided value
 *     - if you supply a unix timestamp, the date object will also create an iso version of the date, and vice versa
 *  @param $reset - force a reset of the provided value even if it already exists
 */
function date_set_date(&$date, $value, $timezone = 'GMT', $type = 'db', $format = DATE_ISO, $reset = FALSE, $display_errors = FALSE) {
  
  // store the starting value of the date object in case there are errors
  $old_date = $date;

  $error = array();

  if ($format == DATE_UNIX) {
    
    $date->$type->timestamp = $value;

    $parts = date_unix2array($value);
    if ($parts === 'ERROR') {
      $error[] = 'unix2array';
    } else {
      $date->$type->parts = $parts;
    }

    $iso = date_unix2iso($value);
    if ($iso === 'ERROR') {
      $error[] = 'unix2iso';
    } else {
      $date->$type->iso = $iso;
    }
  
  } elseif ($format == DATE_ISO) {
    
    $date->$type->iso = $value;

    $parts = date_iso2array($value);
    if ($parts === 'ERROR') {
      $error[] = 'iso2array';
    } else {
      $date->$type->parts = $parts;
    }
    
    $unix = date_iso2unix($value);
    if ($unix === 'ERROR') {
      $error[] = 'iso2unix';
    } else {
      $date->$type->timestamp = $unix;
    }
    
  } else {

    $error[] = $format;

  }
  
  if (!$error && $type == 'local' && (!$date->db->iso || $reset)) {
    
    $date->local->timezone = $timezone;

    // if the local value was submitted, go ahead and compute the db part of the date
    date_convert_timezone($date, $date->local->timezone, 'GMT', 'db');

  } elseif (!$error && $type == 'db' && $date->local->iso && (!$date->local->iso || $reset)) {
      
    // compute local value if the db value was submitted 
    // and the local value has not been created and there is a local timezone
    date_convert_timezone($date, 'GMT', $date->local->timezone, 'local');
  }
  
  if ($error) {
    // if unable to set date to this value, revert to previous date settings and show error message
    $date = $old_date;
    if ($display_errors) {
      drupal_set_message(t('Unable to set date. ') . implode(', ', $error));
    }
    return FALSE;
  }
  
  return TRUE;

}

/**
 *  Timezone conversion function
 * 
 *  @param $date - the date object
 *  @param $timezone_in - 'none', 'GMT', or timezone name in format 'US/Central'
 *  @param $timezone_out - 'none', 'GMT', or timezone name in format 'US/Central'
 *  @param $type - 'db' or 'local', the part of the date object to be created
 * 
 */
function date_convert_timezone(&$date, $timezone_in = 'GMT', $timezone_out = 'GMT', $type = 'db') {
  
  $fromtype = $type == 'db' ? 'local' : 'db';
  
  // skip timezone conversion if no timezone conversion was desired
  // set the local and db parts of the date object to be the same and return
  if ($timezone_in == 'none' || $timezone_out == 'none' || empty($date->$fromtype->parts[0])) {
    
    if (($date->local->iso || $date->local->iso == 0) && !$date->db->iso) {
      
      date_set_date($date, $date->local->iso, $timezone_out, 'db', DATE_ISO);
    
    } elseif (($date->db->iso || $date->db->iso == 0) && !$date->local->iso) {
      
      date_set_date($date, $date->db->iso, $timezone_out, 'local', DATE_ISO);

    }

    return;
  }
  
  if ($type == 'local') {
    
    $date->local->timezone = $timezone_out;

    // attempt conversion only when there is available data to use
    if (!$date->db->iso && !$date->db->iso == 0) return;

    // see if an offset applies, and adjust date accordingly
    if ($offset = date_offset(date_gmgetdate($date->db->timestamp), $date->local->timezone)) {
      
      $out_date = $date->db->timestamp + $offset;
      date_set_date($date, $out_date, $timezone_out, $type, DATE_UNIX);
      
    // no offset, just set other part of date object to same value
    } else {

      date_set_date($date, $date->db->iso, $timezone_out, $type, DATE_ISO);

    }

  } else {

    // attempt conversion only when there is available data to use
    if ((!$date->local->iso && !$date->local->iso == 0) || !$date->local->timezone) return;
    
    // see if an offset applies, and adjust date accordingly
    if ($offset   = date_offset(date_gmgetdate($date->local->timestamp), $date->local->timezone)) {

      $out_date = $date->local->timestamp - $offset;
      date_set_date($date, $out_date, $timezone_out, $type, DATE_UNIX);
      
    // no offset, just set other part of date object to same value
    } else {

      date_set_date($date, $date->local->iso, $timezone_out, $type, DATE_ISO);

    }

  }
  
  return;
  
}

/**
 *  A function to calculate offset using timezone.inc file
 * 
 *  @param  $date_parts - an array of date parts in the format created by getdate()
 *  @param  $timezone - the timezone to use
 *  @return $offset - the offset of $timezone from GMT on the $date_parts date
 */
function date_offset($date_parts, $timezone = 'GMT') {
  
  include_once(DATE_TIMEZONES);  

  // no adjustment needed for gmt dates
  if ($timezone == 'GMT') return 0;

  // find the timezone info for the input timezone
  $timezones = array_flip(event_zonelist());
  $zid       = $timezones[$timezone];

  // create a timestamp for the date to be evaluated
  if ($date_parts['year'] < 1970) {

    // fudge offset for dates prior to 1970 by finding the offset in 1971
    $date_parts['year'] = 1971;
    $timestamp = date_array2unix($date_parts);
  
  } else {
    
    $timestamp = $date_parts[0];

  }

  return event_get_offset($zid, $timestamp);

}

/**
 *  A function to display the database value for the date object
 * 
 *  @param $date - the date object
 *  @param $format - DATE_UNIX or DATE_ISO, the type of value to display
 *  @param $type - 'db' or 'local', the date value to display
 */
function date_show_value($date, $type = 'db', $format = DATE_ISO) {
  
  if ($format == DATE_UNIX) {
    
    return $date->$type->timestamp;
  
  } else {
    
    return $date->$type->iso;
  
  }
}

/**
 *  A function to display formatted output for the date object
 * 
 *  @param $date - the date object
 *  @param $format_string - the format string to be used for the display
 *  @param $type - 'local' or 'db', the date value to be displayed
 *  @param $zone_display - '', '0000', '00:00', 'name' - type of timezone display to append
 */
function date_show_date($date, $format_string, $type = 'local', $zone_display = '') {
  
  if ($zone_display) {

    $append = date_append_zone($date, $zone_display);

  }

  // using gmdate so php won't try to adjust value for server timezone
  // needed because date object has already made necessary timezone adjustments
  
  if ($type == 'db' || (!$date->local->timestamp && $date->local->timestamp != 0)) {
    
    return date_gmdate($format_string, date_fuzzy_stamp($date->db)) . $append;
  
  } elseif ($date->local->timestamp || $date->local->timestamp == 0) {
    
    return date_gmdate($format_string, date_fuzzy_stamp($date->local)) . $append;
  
  } 
}

/**
 *  Create a valid timestamp that can be used for date formatting
 *  even if only partial date info is available, 
 *  i.e. for an iso date with month and year only
 */
function date_fuzzy_stamp($datetype) {

  if ($datetype->timestamp) {
    
    return $datetype->timestamp;

  } else {

    return date_gmmktime(array(
      'year'   =>  ($datetype->parts['year']    ? $datetype->parts['year']    : date('Y')),
      'mon'    =>  ($datetype->parts['mon']     ? $datetype->parts['mon']     : 1),
      'mday'   =>  ($datetype->parts['mday']    ? $datetype->parts['mday']    : 1),
      'hours'   => ($datetype->parts['hours']   ? $datetype->parts['hours']   : 0),
      'minutes' => ($datetype->parts['minutes'] ? $datetype->parts['minutes'] : 0),
      'seconds' => ($datetype->parts['seconds'] ? $datetype->parts['seconds'] : 0),
      ));
  
  }
  
}

/**
 *  A function to append the zone offset or name to a display
 * 
 *  Alternative to timezone identifiers in format string 
 *  needed because format string tz identification may display server zone rather than site or date zone
 *  no option for zone abbreviation (i.e. EST) because zone abbreviations
 *  are not unique, nor are they available in the timezone.inc data
 * 
 *  @param $type - the type of display desired
 *      - '0000' will display zone offset like -0500
 *      - '00:00' will display zone offset like -05:00
 *      - 'name' will display zone name
 */
function date_append_zone($date, $type = '') {

  if (!$type) return;

  $offset  = intval($date->local->timestamp - $date->db->timestamp);
  $hours   = intval($offset / 3600);
  $minutes = intval(($offset / 60) - ($hours * 60));
  
  switch (trim($type)) {
  case ('0000'):
    return ' '. sprintf('%+05d', $hours * 100);
  case ('00:00'):
    return ' '. sprintf('%+03d', $hours) .':'. sprintf('%02d', $minutes);
  case ('name'):
    return ' '. $date->local->timezone;
  }
}

function date_append_zone_options() {
  return array(
    '' => '',
    '0000' => '+-0000',
    '00:00' => '+-00:00',
    'name' => t('zone name'),
    );
}

/**
 *  Time zone option list
 * 
 *  @return array of timezone ids (i.e. US/Eastern, US/Central)
 *  @param  $limit - an optional offset value, to limit the results to timezones that match that offset
 */
function date_timezone_options($limit = '') {
  
  include_once(DATE_TIMEZONES);

  if (!$limit) {
    $zonelist = drupal_map_assoc(event_zonelist());

  } else {
    $zones = event_get_timezones();
    foreach ($zones as $zone) {
      if ($zone['offset'] == $limit || $zone['offset_dst'] == $limit) {
        $zonelist[$zone['timezone']] = $zone['timezone'];
      }
    }
  }
  $zonelist += array('GMT' => 'GMT');
  asort($zonelist);
  return $zonelist;

}

/**
 *  Set system variable for site default timezone
 *  Needed because current system variable tracks offset rather than timezone
 */
function date_set_site_timezone($val) {

  include_once(DATE_TIMEZONES);

  $timezones = event_zonelist();

  if (!in_array($val, $timezones)) $val = 'GMT';

  variable_set('date_default_timezone_name', $val);
}

/**
 *  Get system variable setting for site default timezone
 */
function date_get_site_timezone() {

  return variable_get('date_default_timezone_name', 'GMT');

}

/**
 *  Timezone handling options
 *  omitting user option for now because we only know the user's offset at best, not their timezone
 *  come back later and enable this option if there is a way to collect and save user timezones
 * 
 *  the 'none' option will do no timezone conversions and will store and display dates exactly as entered
 *  useful in locales or situations where timezone conversions are not working reliably,
 *  for dates with no times, for historical dates where timezones are irrelevant, 
 *  or anytime conversion is unnecessary or undesirable
 */
function date_timezone_handling_options() {

  return array(
    'gmt' => 'GMT',
    'site' => t('Site\'s time zone'),
    'date' => t('Date\'s time zone'),
    //'user' => t('User\'s time zone'),
    'none' => t('No time zone conversion'),
  );
}

/**
 *  Function to figure out which timezone applies to a date and select it
 */
function date_get_timezone($handling, $timezone = '') {
  
  switch ($handling) {
    case ('site'):
      $timezone_out = date_get_site_timezone();
      break;
    case ('date'):
      $timezone_out = $timezone > '' ? $timezone : date_get_site_timezone();
      break;
    case ('user'):
      // come back here and add appropriate value if user timezones are can be collected and saved
      // until then, user option is not enabled
    case ('none'):
      $timezone_out = 'none';
      break;
    default:
      $timezone_out = 'GMT';
  }
  return $timezone_out > '' ? $timezone_out : 'GMT';
}


/**
 *  Flexible Date/Time Drop-Down Selector
 * 
 *  $params = an array of values, including
 *    label = the label for the date group, default is 'Date'
 *    value = the date/time to be processed, default is the current date and time
 *    timezone_in  = the timezone of the date/time value to be processed, default is GMT
 *    timezone_out = the timezone to be used when displaying the date, default is date site timezone, if set, or GMT
 *       valid timezones are standard timezone ids like US/Central, America/New_York, GMT
 *    format = the format of the date value, default is DATE_ISO
 *       DATE_UNIX => unix timestamp
 *       DATE_ISO => iso 8601 YYYY-MM-DDThh:mm:ss
 *    am/pm = 0 to display 24 hour format, 1 to display 12 hour format with am/pm selector, default is 0
 *    weight = the weight of the date group, default is 0
 *    delta = a delta value for the group to accomodate multiple date fields, default is 0
 *    granularity = an array of date parts to be selected, like array('Y','M','D'), default is M, D, Y
 *       Y => year, M => month, D => day, H => hours, N => minutes, S => seconds, T => timezone
 *    increment = increment minutes and seconds by increment amount, default is 1
 *    opt_fields = an array of fields that need not be filled out, default is empty array
 *    blank_default = 1 to show an empty date field with blank values, 0 to fill with current date, default is 0
 *    required = 1 if the field must contain a valid date, default is 1
 *    description = text to be used as a description for the fieldset
 */
function date_select_input($params) {
  
  theme_add_style(drupal_get_path('module', 'date') .'/date.css');
  
  // set the variables
  $label         = $params['label'] ? $params['label'] : t('Date');
  $delta         = isset($params['delta']) ? $params['delta'] : 0;
  $title         =  ($delta > 0) ? (' '.$delta) : '';
  $granularity   = is_array($params['granularity']) ? $params['granularity'] : array('M', 'D', 'Y');
  $increment     = isset($params['increment']) ? $params['increment'] : 1;
  $required      = isset($params['required']) ? $params['required'] : 1;
  $format        = isset($params['format']) ? $params['format'] : DATE_ISO;
  $formats       = $params['formats'];
  $weight        = isset($params['weight']) ? $params['weight'] : 0;
  $opt_fields    = is_array($params['opt_fields']) ? $params['opt_fields'] : array();
  $blank_default = isset($params['blank_default']) ? $params['blank_default'] : 0;
  $timezone_in   = isset($params['timezone_in']) ? $params['timezone_in'] : 'GMT';
  $timezone_out  = isset($params['timezone_out']) ? $params['timezone_out'] : date_get_site_timezone();
  $description   = $params['description'];
  $select_month  = $params['select_month'];
  $select_day    = $params['select_day'];
  $select_year   = $params['select_year'];
  $years_back    = $params['years_back'];
  $years_forward = $params['years_forward'];

  if ($formats['input']['am_pm']) {
    $hours_format = t('h');
    for ($i = 0; $i <= 12; $i++) $hours_array[$i] = $i < 10 ? "0$i" : $i;
  } 
  else {
    $hours_format = t('H');
    for ($i = 0; $i <= 23; $i++) $hours_array[$i] = $i < 10 ? "0$i" : $i;
  }
  
  // create a date object with the desired date
  $date = date_make_date();

  if ($params['value']) {
    date_set_date($date, $params['value'], $timezone_in, 'db', $format);
    date_convert_timezone($date, $timezone_in, $timezone_out, 'local');
  }
  
  // if no date provided, use current date
  if (!$blank_default && $params['value'] == '') {
    switch ($format) {
    case (DATE_UNIX):
      $now = date_gmadj_zone(time());
      break;
    default:
      $now = date_unix2iso(date_gmadj_zone(time()));
    }
    date_set_date($date, $now, $timezone_out, 'local', $format);
  }

  if (!$blank_default || $params['value'] > '') {
    $year    = date_show_date($date, 'Y');
    $mon     = date_show_date($date, 'm');
    $mday    = date_show_date($date, 'd');
    $hours   = date_show_date($date, $hours_format);
    $minutes = (string)date_show_date($date, 'i');
    $seconds = (string)date_show_date($date, 's');
    $ampm    = strtolower(date_show_date($date, 'a'));
  } 
  
  // create the form values
  $form['#type']   = 'fieldset';
  $form['#title']  = t($label) . $title;
  $form['#weight'] = intval($weight);
  $form['#prefix'] = '<div class="container-inline-date">';
  $form['#suffix'] = '</div>';
  
  if (in_array('D',$granularity)) {
    $form['mday'] = array(
      '#default_value' => $mday,
      '#title' => t('day'),
      '#required' => ($required && !in_array('mday', $opt_fields)) ? $required : 0,
      '#weight' => $formats['input']['select']['D'],
    );
    if ($select_day) {
      $days_array = drupal_map_assoc(range(1, 31));
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

  if (in_array('M',$granularity)) {
    $form['mon'] = array(
      '#default_value' => $mon,
      '#title' => t('month'),
      '#required' => ($required && !in_array('mon', $opt_fields)) ? $required : 0,
      '#weight' => $formats['input']['select']['M'],
    );
    if ($select_month) {
      $months_array = drupal_map_assoc(range(1, 12), 'map_month');
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

  if (in_array('Y',$granularity)) {
    $form['year'] = array(
      '#default_value' => $year,
      '#title' => t('year'),
      '#required' => ($required && !in_array('year', $opt_fields)) ? $required : 0,
      '#weight' => $formats['input']['select']['Y'],
    );
    if ($select_year) {
      $years_array = drupal_map_assoc(range($year - $years_back, $year + $years_forward));
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

  $form['space1'] = array(
    '#type' => 'markup',
    '#value' => ' <div class="date-space">&nbsp;</div>',
    '#weight' => 3,
    );

  if (in_array('H',$granularity)) {
    $form['hours'] = array(
      '#type' => 'select',
      '#default_value' => $hours,
      '#options' => $hours_array,
      '#required' => ($required && !in_array('hours', $opt_fields)) ? $required : 0,
      '#title' => t('hour'),
      '#weight' => 4,
    );
  }
  if (in_array('N',$granularity)) {
    for ($i = 0; $i <= 59; $i += $increment) $minutes_array[$i] = $i < 10 ? "0$i" : $i;
    $form['minutes'] = array(
      '#type' => 'select',
      '#default_value' => $minutes,
      '#options' => $minutes_array,
      '#required' => ($required && !in_array('minutes', $opt_fields)) ? $required : 0,
      '#title' => t('minute'),
      '#weight' => 5,
    );
  }
  if (in_array('S',$granularity)) {
    for ($i = 0; $i <= 59; $i += $increment) $seconds_array[$i] = $i < 10 ? "0$i" : $i;
    $form['seconds'] = array(
      '#type' => 'select',
      '#default_value' => $seconds,
      '#options' => $seconds_array,
      '#required' => ($required && !in_array('seconds', $opt_fields)) ? $required : 0,
      '#title' => t('second'),
      '#weight' => 6,
    );
  }
  if ($formats['input']['am_pm']) {
    $options = array('am' => t('am'), 'pm' => t('pm'));
    if (!$required || in_array('hours', $opt_fields) || $blank_default) array_unshift($options, '');
    $form['space2'] = array(
      '#type' => 'markup',
      '#value' => '<div class="date-space">&nbsp;</div>',
      '#weight' => 7,
    );
    $form['ampm'] = array(
      '#type' => 'select',
      '#default_value' => $ampm,
      '#options' => $options,
      '#required' => ($required && !in_array('hours', $opt_fields)) ? $required : 0,
      '#title' => t('am/pm'),
      '#weight' => 8,
    );
  }
  if (in_array('T',$granularity)) {
    $form['timezone'] = array(
      '#type' => 'select',
      '#default_value' => $timezone_out,
      '#options' => date_timezone_options(),
      '#required' => ($required && !in_array('timezone', $opt_fields)) ? $required : 0,
      '#title' => t('timezone'),
      '#prefix' => '<div class="date-timezone">',
      '#suffix' => '</div>',
      '#weight' => 10,
    );
  } else {
    $form['timezone'] = array(
      '#type' => 'hidden',
      '#value' => $timezone_out,
    );
  }
  $form[] = array(
    '#type' => 'markup',
    '#value' => '<div class="date-clearing"> </div>'. ($description > '' ? '<div class="form-item description">'.$description.'</div>' : ''),
  );
  return $form;
}

/**
 *  Text date input form, with optional jscalendar popup
 * 
 *  $params = an array of values, including
 *    label = the label for the date group, default is 'Date'
 *    value = the date/time to be processed, default is the current date and time
 *    timezone_in  = the timezone of the date/time value to be processed, default is GMT
 *    timezone_out = the timezone to be used when displaying the date, default is date site timezone, if set, or GMT
 *       valid timezones are standard timezone ids like US/Central, America/New_York, GMT
 *    format = the format of the date value, default is DATE_ISO
 *       DATE_UNIX => unix timestamp
 *       DATE_ISO => iso 8601 YYYY-MM-DDThh:mm:ss
 *    weight = the weight of the date group, default is 0
 *    delta = a delta value for the group to accomodate multiple date fields, default is 0
 *    granularity = an array of date parts to be selected, like array('Y','M','D'), default is M, D, Y
 *       Y => year, M => month, D => day, H => hours, N => minutes, S => seconds, T => timezone
 *    required = 1 if the field must contain a valid date, default is 1
 *    description = text to be used as a description for the fieldset
 *    blank_default = 1 to show an empty date field with blank values, 0 to fill with current date, default is 0
 *    jscalendar = 1 use if available, 0 do not use
 */
function date_text_input($params) {
  
  // set the variables
  $label         = $params['label'] ? $params['label'] : t('Date');
  $delta         = isset($params['delta']) ? $params['delta'] : 0;
  $title         =  ($delta > 0) ? (' '.$delta) : '';
  $granularity   = is_array($params['granularity']) ? $params['granularity'] : array('M', 'D', 'Y');
  $required      = isset($params['required']) ? $params['required'] : 1;
  $format        = isset($params['format']) ? $params['format'] : DATE_ISO;
  $formats       = $params['formats'];
  $weight        = isset($params['weight']) ? $params['weight'] : 0;
  $timezone_in   = isset($params['timezone_in']) ? $params['timezone_in'] : 'GMT';
  $timezone_out  = isset($params['timezone_out']) ? $params['timezone_out'] : date_get_site_timezone();
  $description   = $params['description'];
  $jscalendar    = $params['jscalendar'] ? 1 : 0;
  $blank_default = isset($params['blank_default']) ? $params['blank_default'] : 0;
  
  // create a new date object and convert it to the right timezone
          
  // create a date object with the desired date
  $date = date_make_date();

  if ($params['value']) {
    date_set_date($date, $params['value'], $timezone_in, 'db', $format);
    date_convert_timezone($date, $timezone_in, $timezone_out, 'local');
  }
  
  // if no date provided, use current date
  if (!$blank_default && $params['value'] == '') {
    switch ($format) {
    case (DATE_UNIX):
      $now = date_gmadj_zone(time());
      break;
    default:
      $now = date_unix2iso(date_gmadj_zone(time()));
    }
    date_set_date($date, $now, $timezone_out, 'local', $format);
  }
  
  // get the local iso version of the database value
  $value = date_show_value($date, 'local', DATE_ISO);
  //$value = date_show_value($date, 'local', DATE_UNIX);
  
  // if date-specific timezone handling was selected, provide a way for the user to enter the timezone
  if (in_array('T', $granularity)) {
    $form['#type'] = 'fieldset';
    $form['#title'] = $label;
    $form['#weight'] = $weight;
            
    $form['value'] = array(
      '#type' => 'textfield',
      '#title' => t('date'),
      //'#default_value' => gmdate($formats['input']['text'], $value),
      '#default_value' => $value,
      '#required' => ($delta == 0) ? $required : 0,
    );
    if (in_array('T',$granularity)) {
      $form['timezone'] = array(
        '#title' => t('timezone'),
        '#type' => 'select',
        '#default_value' => $timezone_out,
        '#options' => date_timezone_options(),
        '#required' => ($required && !in_array('timezone', $opt_fields)) ? $required : 0,
        '#title' => t('timezone'),
      );
    } else {
      $form['timezone'] = array(
        '#type' => 'hidden',
        '#value' => $timezone_out,
      );
    }

    $form[] = array(
      '#type' => 'markup',
      '#value' => '<div class="form-item"><div class="description">'.$description.'</div></div>',
    );

  } else {

    // date-specific timezone not requested, just get date
    $form['value'] = array(
      '#type' => 'textfield',
      '#title' => $label,
      //'#default_value' => gmdate($formats['input']['text'], $value),
      '#default_value' => $value,
      '#required' => ($delta == 0) ? $required : 0,
      '#description' => $description,
      '#weight' => $weight,
    );
  }

  // if the jscalendar is used for input, add some attributes to be passed to the js
  // also need to adjust the date format slightly to accomodate the js capability

  if ($jscalendar && module_exist('jscalendar')) {
    $form['value']['#default_value'] = date_iso2jscalendar($value, $granularity);
    $form['value']['#attributes'] = array('class' => 'jscalendar');
    $form['value']['#jscalendar_ifFormat']  = '%Y-%m-%d ' . (date_has_time($granularity) ? '%H:%M:%S' : '');
    //$form['value']['#jscalendar_ifFormat'] = $formats['input']['jscal'];
    $form['value']['#jscalendar_showsTime'] = date_has_time($granularity) ? 'true' : 'false';
    $form['value']['#jscalendar_timeFormat'] = $formats['input']['am_pm'] ? '12' : '24';
  }
  return $form;
}

/**
 *  Construct a value to save to the database from the date selector
 * 
 *  @param $array - an array of date parts collected by the date selector
 *  @param $format - DATE_UNIX or DATE_ISO
 *  @param $timezone_in - timezone of supplied value
 *  @param $granularity = an array of date parts to be selected, like array('Y','M','D'), default is M, D, Y
 *       Y => year, M => month, D => day, H => hours, N => minutes, S => seconds, T => timezone
 */
function date_selector_make_dbdate($array, $format = DATE_ISO, $timezone_in = 'GMT', $granularity = array('M', 'D', 'Y')) {
  
  // adjust back to 24 hours time if 12 hours input was collected
  if ($array['ampm'] == 'pm' && $array['hours'] < 12) $array['hours'] += 12;
  
  // try to construct a date from the submitted values
  $date = date_make_date();
  
  if ($format == DATE_UNIX) {
    
    if (date_set_date($date, date_array2unix($array), $timezone_in, 'local', $format)) {
      
      return date_show_value($date, 'db', $format);

    }
  } elseif ($format == DATE_ISO) {
    
    if (date_set_date($date, date_array2iso($array), $timezone_in, 'local', $format)) {
      
      return date_show_value($date, 'db', $format);

    }
  }

  return FALSE;
}

/**
 *  Construct a value to save to the database from jscalendar input
 * 
 *  @param $value - a text value created by js_calendar
 *  @param $format - DATE_UNIX or DATE_ISO
 *  @param $timezone_in - timezone of supplied value
 *  @param  $granularity = an array of date parts to be selected, like array('Y','M','D'), default is M, D, Y
 *       Y => year, M => month, D => day, H => hours, N => minutes, S => seconds, T => timezone
 */
function date_jscalendar_make_dbdate($value, $format, $timezone_in = 'GMT', $granularity = array('M', 'D', 'Y')) {
  
  switch($format) {
    case (DATE_UNIX):
      $value = date_jscalendar2unix($value, $granularity);
      break;
    case (DATE_ISO):
      $value = date_jscalendar2iso($value, $granularity);
      break;
  }

  if ($value) {

    if ($date = date_make_date($value, $timezone_in, 'local', $format)) {

      return date_show_value($date, 'db', $format);

    }
  }
  return FALSE;
}

/**
 *  Construct a value to save to the database from text input
 * 
 *  @param $value - string date value, could be iso format or text for strtotime conversion
 *  @param $format - DATE_UNIX or DATE_ISO
 *  @param $timezone_in - timezone of supplied value
 *  @param  $granularity = an array of date parts to be selected, like array('Y','M','D'), default is M, D, Y
 *       Y => year, M => month, D => day, H => hours, N => minutes, S => seconds, T => timezone
 */
function date_text_make_dbdate($value, $format, $timezone_in = 'GMT', $granularity = array('M', 'D', 'Y')) {
    
  switch ($format) {
    case (DATE_UNIX):
      $value = date_text2unix($value);
      break;
    case (DATE_ISO):
      $value = date_text2iso($value);
      break;
  }
  
  if ($value) {
    if ($date = date_make_date($value, $timezone_in, 'local', $format)) {
      return date_show_value($date, 'db', $format);
    }
  }
  return FALSE;
}

/**
 *  Validation function for date selector
 *  $params = an array of values including:
 *    required = is a valid date required, default is true
 *    opt_fields = an array of fields that need not be filled out, default is empty array
 */
function date_selector_validate($value, $fieldname, $params = array()) {
  
  $type       = $params['type'] > '' ? $params['type'] : DATE_ISO;
  $required   = isset($params['required']) ? $params['required'] : 1;
  $opt_fields = is_array($params['opt_fields']) ? $params['opt_fields'] : array();
  $granularity = $params['granularity'] ? $params['granularity'] : array('M', 'D', 'Y');

  if ($value['ampm'] == 'pm' && $value['hours'] < 12) $value['hours'] += 12;
  
  // check for invalid numbers in fields that were submitted using textfields
  if ($required && !in_array($value['year'], $opt_fields)) {

    if (!date_part_is_valid($value['year'], 'year')) form_set_error($fieldname, t('year must be a number between %min and %max.', array('%min' => DATE_MIN_YEAR, '%max' => DATE_MAX_YEAR)));

  }
  
  if ($required && !in_array($value['mday'], $opt_fields)) {

    if (!date_part_is_valid($value['mday'], 'day')) form_set_error($fieldname, t('day must be a number between 1 and 31.'));

  }

  // try to construct a date from the submitted values
  if ($required && !date_selector_make_dbdate($value, $type)) {

    form_set_error($fieldname, t('Date cannot be constructed using values %s.', array('%s' => implode(', ',$value))));
    return FALSE;

  } else {

    return TRUE;
  }
}

/**
 *  Validation for jscalendar input
 */
function date_jscalendar_validate($value, $fieldname, $format, $granularity = array('M', 'D', 'Y')) {
  
  switch ($format) {
    case (DATE_UNIX):
      if (!date_jscalendar2unix($value, $granularity)) {
        form_set_error($fieldname, t('The text \'%s\' is not a valid date.', array('%s' => $value)));
        return FALSE;
      }
      break;
    case (DATE_ISO):
      if (!date_jscalendar2iso($value, $granularity)) {
        form_set_error($fieldname, t('The text \'%s\' is not a valid date.', array('%s' => $value)));
        return FALSE;
      }
      break;
  }
  return TRUE;
}

/**
 *  Validation for text input
 */
function date_text_validate($value, $fieldname, $format, $granularity = array('M', 'D', 'Y')) {
  
  switch ($format) {
    case (DATE_UNIX):
      if (!$value = date_text2unix($value)) {
        form_set_error($fieldname, t('The text \'%s\' is not a valid date.', array('%s' => $value)));
        return FALSE;
      }

    case (DATE_ISO):
      if (!$value = date_text2iso($value)) {
        form_set_error($fieldname, t('The text \'%s\' is not a valid date.', array('%s' => $value)));
        return FALSE;
      }
  }
  
  return TRUE;
}

/**
 * Date conversion functions
 *
 *  a variety of ways to convert values are provided
 *  arrays are in the format created by getdate()
 *  return false for anything that can't produce a valid year
 *  iso date may have a year only or year and month only
 *  unix dates must have at least a year month and day
 * 
 *  these function return 'ERROR' instead of FALSE because of
 *  problems distinguishing between a valid 0 value and a FALSE value
 * 
 */
function date_array2iso($array) {
  
  if (!date_is_valid($array, 'array')) return 'ERROR';
  
  return  sprintf("%04d",intval($array['year'])) . 
    '-'. sprintf("%02d",intval($array['mon'])) .  
    '-'. sprintf("%02d",intval($array['mday'])) . 
    'T'. sprintf("%02d",intval($array['hours'])) . 
    ':'. sprintf("%02d",intval($array['minutes'])) . 
    ':'. sprintf("%02d",intval($array['seconds']));
}

function date_array2unix($array) {

  if (!date_is_valid($array, 'array')) return 'ERROR';

  // a date without a month and day cannot be made into a unix timestamp
  if (!$array['mon'] || !$array['mday']) {
    return NULL;
  }

  // using gmmktime instead of mktime so no server timezone adjustment is made
  return date_gmmktime($array);

}

function date_iso2array($iso) {
  
  if (!date_preg($iso)) return 'ERROR';
  
  if (!date_is_valid($iso, DATE_ISO)) return 'ERROR';

  // a unix date requires a year, month, and day
  if (date_iso_year($iso) && date_iso_mon($iso) && date_iso_day($iso)) {

    $unix = date_iso2unix($iso);
  
    if ($unix === 'ERROR') {
    
      return 'ERROR';

    } else {
    
      return date_unix2array($unix);
    }
  
  // an iso date is valid even without a month and/or a day
  } elseif (date_iso_year($iso)) {

    return array(
      'year' => date_iso_year($iso),
      'mon' => date_iso_mon($iso),
      'mday' => date_iso_day($iso),
      'hours' => date_iso_hours($iso),
      'minutes' => date_iso_minutes($iso),
      'seconds' => date_iso_seconds($iso),
      0 => NULL,
      );

  } else {

      return 'ERROR';

  }
  
}

function date_unix2array($unix) {

  if (!date_is_valid($unix, DATE_UNIX)) return 'ERROR';
  
  // using gmgetdate instead of getdate so no server timezone adjustment is made
  return date_gmgetdate($unix);

}

function date_unix2iso($unix) {

  if (!date_is_valid($unix, DATE_UNIX)) return 'ERROR';

  // using gmdate instead of date so no server timezone adjustment is made
  return date_gmdate(DATE_STRING_ISO, $unix);

}

function date_iso2unix($iso) {
  
  if (!date_preg($iso)) return 'ERROR';
  
  if (!date_is_valid($iso, DATE_ISO)) return 'ERROR';
  
  // a unix date requires a year, month, and day
  if (date_iso_year($iso) && date_iso_mon($iso) && date_iso_day($iso)) {
  
    $array = array(
      'hours' => date_iso_hours($iso), 
      'minutes' => date_iso_minutes($iso), 
      'seconds' => date_iso_seconds($iso), 
      'mon' => date_iso_mon($iso), 
      'mday' => date_iso_day($iso), 
      'year' => date_iso_year($iso)
      );
  
    // using gmmktime instead of mktime so no server timezone adjustment is made
    $unix = date_gmmktime($array);
  
    // php versions earlier than 5.1 return -1 for a false response, convert that to 'ERROR'
    if ($unix == -1) return 'ERROR';
  
    return $unix;

  } else {

    return NULL;

  }

}

/**
 *  The JS calendar needs a date in a format like MM-DD-YY HH:MM:SS
 *  Replace the 'T' in the iso date, also cut off the time string if no time is desired
 */
function date_iso2jscalendar($iso, $granularity = array('M', 'D', 'Y')) {
  
  if (!date_preg($iso)) return 'ERROR';
  
  if (date_has_time($granularity)) {

    return str_replace('T', ' ', $iso);

  } else {

    return substr($iso, 0, 10);

  }
}

function date_unix2jscalendar($unix, $granularity = array('M', 'D', 'Y')) {

  if ($iso = date_unix2iso($unix)) {

    if ($iso = date_iso2jscalendar($iso, $granularity = array('M', 'D', 'Y'))) {

      return $iso;

    }
  }
  return 'ERROR';
}

/**
 *  When saving the JS calendar date to the database, put the 'T' back in
 *  and append a blank time string if time is not required
 */
function date_jscalendar2iso($text, $granularity = array('M', 'D', 'Y')) {

  if (strstr($text, ' ')) {

     $iso = str_replace(' ', 'T', $text);

  } elseif ($text > '') {

     $iso = $text .'T00:00:00';

  } else {

     return 'ERROR';
  }
  
  if (date_preg($iso)) {
    
    return $iso;

  } else {

    return 'ERROR';

  }
}

function date_jscalendar2unix($text, $granularity = array('M', 'D', 'Y')) {
  
  if ($iso = date_jscalendar2iso($text, $granularity = array('M', 'D', 'Y'))) {
    
    if ($unix = date_iso2unix($iso)) {
      
      return $unix;
    }
  }
  return 'ERROR';
}

/**
 *  Use stringtotime function to create an iso date out of text
 */
function date_text2iso($text) {
  
  // if date supplied in iso format, use it
  if (date_preg($text) && date_part_is_valid(date_iso_year($text), 'year')) return $text;
  
  // if not iso date, try strtotime conversion
  $iso = date_gmdate(DATE_STRING_ISO, strtotime($text));
  
  if (date_part_is_valid(date_iso_year($iso), 'year')) {
    
    return $iso;

  } else {

    return 'ERROR';

  }
}

function date_text2unix($text) {
  
  if ($iso = date_text2iso($text)) {
    
    if ($unix = date_iso2unix($iso)) {
      
      return $unix;

    }
  }
  return 'ERROR';
}

function date_iso_year($iso) {
  return (integer) substr($iso, 0, 4);
}
function date_iso_mon($iso){
  return (integer) substr($iso, 5, 2);
}
function date_iso_day($iso) {
  return (integer) substr($iso, 8, 2);
}
function date_iso_hours($iso) {
  return (integer) substr($iso, 11, 2);
}
function date_iso_minutes($iso) {
  return (integer) substr($iso, 14, 2);
}
function date_iso_seconds($iso) {
  return (integer) substr($iso, 17, 2);
}
// using gmdate instead of date so no server timezone adjustment is made to provided values
function date_unix_year($unix) {
  return (integer) date_gmdate('Y', $unix);
}
function date_unix_mon($unix) {
  return (integer) date_gmdate('m', $unix);
}
function date_unix_day($unix) {
  return (integer) date_gmdate('d', $unix);
}
function date_unix_hours($unix) {
  return (integer) date_gmdate('H', $unix);
}
function date_unix_minutes($unix) {
  return (integer) date_gmdate('i', $unix);
}
function date_unix_seconds($unix) {
  return (integer) date_gmdate('s', $unix);
}

/**
 *  Functions to test the validity of various date parts
 * 
 *  @param $format could be 'array', DATE_UNIX, or DATE_ISO
 */
function date_is_valid($value, $format) {

  switch ($format) {
  case ('array'):
    if (!date_part_is_valid($value['year'], 'year') || !date_part_is_valid($value['mon'], 'mon') || !date_part_is_valid($value['mday'], 'mday')) return FALSE;
    return TRUE;

  case (DATE_UNIX):
    if (!date_part_is_valid(date_unix_year($value), 'year') || !date_part_is_valid(date_unix_mon($value), 'mon', 1) || !date_part_is_valid(date_unix_day($value), 'mday', 1)) return FALSE;
    return TRUE;

  default:
    if (!date_part_is_valid(date_iso_year($value), 'year') || !date_part_is_valid(date_iso_mon($value), 'mon') || !date_part_is_valid(date_iso_day($value), 'mday')) return FALSE;
    return TRUE;
  }
}

/**
 *  Function to test validity of specific date part
 * 
 *  @param $value - the value to test
 *  @param $part - the type of date part provided, coult be 'year', 'mon', or 'mday'
 *  @parma $min either 0 or 1, use to set min for mon and mday
 *      can be 0 for iso dates, set to 1 for unix timestamps that require a complete date
 */
function date_part_is_valid($value, $part, $min = 0) {
  
  switch ($part) {
  case ('year'):
    if ($value < DATE_MIN_YEAR || $value > DATE_MAX_YEAR) return FALSE;
    break;

  case ('mon'):
    if ($value < $min || $value > 12) return FALSE;
    break;

  case ('mday'):
    if ($value < $min || $value > 31) return FALSE;
    break;

  case ('week'):
    if ($value < 0 || $value > 53) return FALSE;
    break;
  }
  return TRUE;
}

/**
 *  Regex validation for iso date
 */
function date_preg($item) {
  if (preg_match('/([0-9]{4})-([0-9]{2})-([0-9]{2})T([0-9]{2}):([0-9]{2}):([0-9]{2})/', $item)) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
 *  a function to figure out if any time data is to be collected or displayed
 */
function date_has_time($granularity) {
  return sizeof(array_intersect($granularity, array('H', 'N', 'S'))) > 0 ? TRUE : FALSE;
}

/**
 *  Function to create an option list for input formats
 *
 *  @return array of date format strings
 *    with a key of the format string and a value of the current date displayed in that format
 *
 */
function date_input_options() {
  $formats = array(
    'Y-m-d H:i' => 'Y-m-d H:i:s',
    'm/d/Y - H:i' => 'm/d/Y - H:i:s',
    'd/m/Y - H:i' => 'd/m/Y - H:i:s',
    'Y/m/d - H:i' => 'Y/m/d - H:i:s',
    'd.m.Y - H:i' => 'd.m.Y - H:i:s',
    'm/d/Y - g:ia' => 'm/d/Y - g:i:sa',
    'd/m/Y - g:ia' => 'd/m/Y - g:i:sa',
    'Y/m/d - g:ia' => 'Y/m/d - g:i:sa',
    'M j Y - H:i' => 'M j Y - H:i:s',
    'j M Y - H:i' => 'j M Y - H:i:s',
    'Y M j - H:i' => 'Y M j - H:i:s',
    'M j Y - g:ia' => 'M j Y - g:i:sa',
    'j M Y - g:ia' => 'j M Y - g:i:sa',
    'Y M j - g:ia' => 'Y M j - g:i:sa'
  );
  $options = array();
  $options['site-wide'] = t('default') .' ('. format_date(time(), 'custom', $formats[variable_get('date_format_short', 'm/d/Y - H:i')]) . ')';
  foreach (array_values($formats) as $f) {
    $options[$f] = format_date(time(), 'custom', $f);
  }
  return $options;
}

/**
 *  Function to create an option list that will display various date and time formats
 * 
 *  @param $type - 'date', 'time', or 'both'
 *  @param $tz_handling - default is 'site'
 *  @param $illustration_date - an iso date to use for illustration of the date options
 *    default date for illustration: August 1 YYYY 2:05:10 PM
 *      - single digit months and days to see leading zeros
 *      - a month number different than the day number to see month and day positions
 *      - an afternoon time to illustrate 24 hours vs am/pm formats
 * 
 *  @return array of date format strings for all date_date_options() and/or date_time_options()
 *    with a key of the format string and a value of the illustration date displayed in that format
 * 
 */
function date_output_options($type = 'date', $tz_handling = 'site', $illustration_date = NULL) {

  $illustration_date = $illustration_date ? $illustration_date : date_gmdate('Y', time()) .'-08-01T14:05:10';
  
  switch ($type) {
  case ('date'):
    $options = date_date_options();
    break;
   
  case ('time'): 
    $options = date_time_options();
    break;

  default:
    $options = date_date_options() + date_time_options();
    break;
  }

  $date = date_make_date($illustration_date, date_get_timezone($tz_handling), 'local', DATE_ISO);

  $view = array();

  foreach ($options as $opt) {

    $view[$opt] = date_show_date($date, $opt);

  }

  return $view;
}

/**
 *  Options for date view formats
 */
function date_date_options() {
  return array(
    ' ', 'Y', 'M', 'F', 'Y M', 'M Y', 'Y F', 'F Y', 
    'Y/m/d', 'm/d/Y', 'd/m/Y', 'Y/m/d', 
    'Y.m.d', 'm.d.Y', 'd.m.Y', 'Y.m.d', 
    'Y-m-d', 'm-d-Y', 'd-m-Y', 'Y-m-d', 
    'M j Y', 'j M Y', 'Y M j',
    'D, m/d/Y', 'D, d/m/Y', 'D, Y/m/d', 
    'D, m.d.Y', 'D, d.m.Y', 'D, Y.m.d', 
    'D, m-d-Y', 'D, d-m-Y', 'D, Y-m-d', 
    'F j, Y', 'j F, Y', 'Y, F j', 'F j, Y', 'j F Y',
    'Y, F j', 'l, F j, Y', 'l, j F, Y', 'l, Y,  F j', 'l, F j, Y', 'l, j F Y', 'l, Y,  F j',
    );
}

/**
 * Options for time view formats
 */
function date_time_options() {
  return array(
    ' ',
    'H:i',
    'h:i a',
    'h:i A',
    );
}

function date_formats($format, $granularity) {

  //split date part and time part
  preg_match("/([djDlzwWmnFMYy].*[djDlzwWmnFMYy])/", $format, $matches, PREG_OFFSET_CAPTURE);
  $format_date = $matches[1][0];
  $format_date_pos = $matches[1][1];
  preg_match("/([gGhHisaA].*[gGhHisaA])/", $format, $matches, PREG_OFFSET_CAPTURE);
  $format_time = $matches[1][0];
  $format_time_pos = $matches[1][1];
  $format_separator = substr($format, $format_date_pos + strlen($format_date), $format_time_pos - ($format_date_pos + strlen($format_date)));

  $jscal_format_date = $jscal_format_time = array();
  $text_format_date = $text_format_time = array();
  $select_format_date = array();
  $weight_index = 0;

  $format_to_jscalendar = array(
    'd' => '%d', 'j' => '%e', 'D' => '%a', 'l' => '%A', 'z' => '%j', 'w' => '%w',
    'W' => '%U',
    'm' => '%m', 'n' => '%m', 'F' => '%B', 'M' => '%b',
    'Y' => '%Y', 'y' => '%y',
    'g' => '%l', 'G' => '%k', 'h' => '%I', 'H' => '%H',
    'i' => '%M',
    's' => '%S',
    'a' => '%P', 'A' => '%p',
  );
  foreach (preg_split('//', $format_date, -1, PREG_SPLIT_NO_EMPTY) as $c) {
    switch ($c) {
      case 'd':
      case 'j':
        if (in_array('D',$granularity)) {
          $jscal_format_date[] = $format_to_jscalendar[$c];
          $text_format_date[] = $c;
          $select_format_date['D'] = $weight_index++;
        }
        break;

      case 'F':
      case 'M':
      case 'm':
      case 'n':
        if (in_array('M',$granularity)) {
          $jscal_format_date[] = $format_to_jscalendar[$c];
          $text_format_date[] = $c;
          $select_format_date['M'] = $weight_index++;
        }
        break;

      case 'Y':
      case 'y':
        if (in_array('Y',$granularity)) {
          $jscal_format_date[] = $format_to_jscalendar[$c];
          $text_format_date[] = $c;
          $select_format_date['Y'] = $weight_index++;
        }
        break;

      default:
        if (!preg_match('/[a-zA-Z]/', $c) && !isset($date_separator)) {
          $date_separator = $c;
        }
        break;
    }
  }

  if (date_has_time($granularity)) {
    foreach (preg_split('//', $format_time, -1, PREG_SPLIT_NO_EMPTY) as $c) {
      switch ($c) {
        case 'g':
        case 'G':
        case 'h':
        case 'H':
          if (in_array('H',$granularity)) {
            $jscal_format_time[] = $format_to_jscalendar[$c];
            $text_format_time[] = $c;
          }
          break;

        case 'i':
          if (in_array('N',$granularity)) {
            $jscal_format_time[] = $format_to_jscalendar[$c];
            $text_format_time[] = $c;
          }
          break;

        case 's':
          if (in_array('S',$granularity)) {
            $jscal_format_time[] = $format_to_jscalendar[$c];
            $text_format_time[] = $c;
          }
          break;

        case 'a':
        case 'A':
          if (date_has_time($granularity)) {
            $jscal_format_time[] = $format_to_jscalendar[$c];
            $text_format_time[] = $c;
            $am_pm = true;
          }
          break;

        default:
          if (!preg_match('/[a-zA-Z]/', $c) & !isset($time_separator)) {
            $time_separator = $c;
          }
          break;
      }
    }
  }

  // we store the current site-wide value, in order to be able to detect its changes
  if ($format == 'site-wide') {
    $formats['input']['site-wide'] = $format;
  }

  $formats['input']['jscal'] = implode($date_separator, $jscal_format_date);
  $formats['input']['jscal'] .= date_has_time($granularity) ? $format_separator . implode($time_separator, $jscal_format_time) : '';

  $formats['input']['text'] = implode($date_separator, $text_format_date);
  $formats['input']['text'] .= date_has_time($granularity) ? $format_separator . implode($time_separator, $text_format_time) : '';

  $formats['input']['select'] = $select_format_date;
  $formats['input']['am_pm'] = $am_pm;

  return $formats;
}
/**
 *  Convert date() to strftime() format strings
 * 
 *  @param $format - a format string for date()
 *  @return corresponding format string for strftime()
 */
function date_date2strftime($format) {
  return strtr($format, date_date2strftime_replace());
}

/** 
 *  Date() to strftime() replacement array
 * 
 *  Adjusted for differences between unix and windows strftime()
 * 
 *  Not all date formats have corresponding strftime formats
 *  Trying for best match or blank for no match
 */
function date_date2strftime_replace() {
  static $replace = array();

  if (empty($replace)) {
    if (stristr(PHP_OS, 'WIN')) {
      $type = 'windows';
    }
    else {
      $type = 'unix';
    }

    $replace = array(
    'd' => '%d', // day of month, leading zero
    'j' => ($type == 'unix' ? '%e' : '%#d'), // day of month, no leading zero
    'F' => '%B', // full month name
    'M' => '%b', // month abbreviation
    'm' => '%m', // month number, leading zero
    'n' => ($type == 'unix' ? '%m' : '%#m'), // month number, no leading zero
    'Y' => '%Y', // year, 4 digit
    'y' => '%y', // year, 2 digit
    'l' => '%A', // full day name
    'D' => '%a', // abbreviated day name
    'w' => '%w', // dow, 0 for sunday
    'N' => '%u', // dow, 7 for sunday
    'H' => '%H', // 24 hour hour, leading zero
    'h' => '%I', // 12 hour hour, leading zero
    'G' => (type == 'unix' ? '%H' : '%#H'), // 24 hour hour, no leading zero
    'g' => (type == 'unix' ? '%I' : '%#I'), // 12 hour hour, no leading zero
    'i' => '%M', // minutes, leading zero
    's' => '%S', // seconds, leading zero
    'a' => '%p', // am/pm lowercase (strftime windows is only upppercase)
    'A' => '%p', // am/pm capitalized (striftime unix is only lowercase)
    'W' => '%V', // ISO week number of year, starting on first Monday
    'o' => '%G', // ISO year that matches ISO week
    'T' => '%Z', // server timezone name
    'r' => ($type == 'unix' ? '%c' : '%#c'), // formatted complete date and time (Thu, 21 Dec 2000 16:01:07)

    /* date formats with no good strftime match */
    'z' => '', // day of year (0-365), strftime equivalent %j uses 1-365
    'S' => '', // day of month ordinal like st, nd, rd
    't' => '', // number of days in month, 28-31
    'L' => '', // leap year, 0 or 1
    'I' => '', // daylight savings time, 0 or 1
    'B' => '', // Swatch internet time, 000-999
    'e' => '', // server timezone identifier like US/Eastern
    'O' => '', // server timezone to gmt offset (+0200)
    'P' => '', // server timezone to gmt offset (+02:00)
    'Z' => '', // server timezone to gmt offset in seconds
    'U' => '', // seconds since Unix Epoch
    'c' => '', // complete ISO date
    );
  }
  return $replace;
}

/**
 *  Cross-database date SQL wrapper function
 *  allows use of normalized native date functions in both mysql and postgres
 *  designed to be extensible to other databases
 * 
 *  @param $result_type - NOW, DATE, YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, DOW, DOY, WEEK
 *  @param $field - the name of the date field to be analyzed
 *  @param $date_type - the type of date field being analyzed, int or iso
 *  @param $offset - timezone offset in seconds, either a value or fieldname
 *  @param $offset_op - the operation to perform on the offset, + or -
 *  @return a SQL statement appropriate for the $db_type
 * 
 *  example:          date_sql('WEEK', 'MYFIELD', 'int', 'MYOFFSETFIELD', '+')
 *  mysql returns:    WEEK(FROM_UNIXTIME(MYFIELD) + INTERVAL MYOFFSETFIELD SECOND, 3)
 *  postgres returns: EXTRACT(WEEK FROM(TIMESTAMP(MYFIELD::ABSTIME::INT4) + INTERVAL MYOFFSETFIELD SECONDS))
 */
function date_sql($result_type, $field, $date_type = 'int', $offset = '', $offset_op = '+') {
  global $db_type;

  if ($date_type == 'NOW') {
    // set up an initial value for now
    $field = 'NOW()';
  } 
  elseif ($date_type == 'int' && $field) {
    // convert integer field value to native date format
    switch ($db_type) {
     case ('mysql'):
     case ('mysqli'):
       $field = "FROM_UNIXTIME($field)";
       break;
     case ('postgres'):
       $field = "TIMESTAMP($field::ABSTIME::INT4)";
       break;
     }
  }
  else {
     // get rid of the 'T' in ISO dates to match native date field
     $field = " REPLACE($field,'T',' ')";
  }
  // adjust date for timezone offset
  if ($offset) {
    switch ($db_type) {
    case ('mysql'):
    case ('mysqli'):
      $field = "$field $offset_op INTERVAL $offset SECOND";
      break;
    case ('postgres'):
      $field = "$field $offset_op INTERVAL $offset SECONDS";
      break;
    }
  }
  // return requested sql
  // note there is no space after FROM to avoid db_rewrite problems
  // see http://drupal.org/node/79904
  switch ($result_type) {
  case ('NOW'):
  case ('DATE'):
    return $field;
  case ('YEAR'):
    return "EXTRACT(YEAR FROM($field))";
  case ('MONTH'):
    return "EXTRACT(MONTH FROM($field))";
  case ('DAY'):
    return "EXTRACT(DAY FROM($field))";
  case ('HOUR'):
    return "EXTRACT(HOUR FROM($field))";
  case ('MINUTE'):
    return "EXTRACT(MINUTE FROM($field))";
  case ('SECOND'):
    return "EXTRACT(SECOND FROM($field))";
  case ('WEEK'):  // ISO week number for date
    switch ($db_type) {
      case ('mysql'):
      case ('mysqli'):
        // WEEK using arg 3 in mysql should return the same value as postgres EXTRACT
        return "WEEK($field, 3)";
      case ('postgres'):
        return "EXTRACT(WEEK FROM($field))";
    }
  case ('DOW'):
    switch ($db_type) {
      case ('mysql'):
      case ('mysqli'):
        // mysql returns 1 for Sunday through 7 for Saturday
        // php date functions and postgres use 0 for Sunday and 6 for Saturday
        return "INTEGER(DAYOFWEEK($field) - 1)";
      case ('postgres'):
        return "DOW($field)";
    }
  case ('DOY'):
    switch ($db_type) {
      case ('mysql'):
      case ('mysqli'):
        return "DAYOFYEAR($field)";
      case ('postgres'):
        return "DOY($field)";
    }
  }
}

/**
 *  A helper function to do cross-database concatation of date parts
 * 
 *  @param $array - an array of values to be concatonated in sql
 *  @return - correct sql string for database type
 */
function date_sql_concat($array) {
  global $db_type;

  switch ($db_type) {
    case ('mysql'):
    case ('mysqli'):
      return "CONCAT(". implode(",", $array) .")";
    case ('postgres'):
      return implode(" || ", $array);
  }
}

/**
 *  A helper function to do cross-database padding of date parts
 * 
 *  @param $str - a string to apply padding to
 *  @param $size - the size the final string should be
 *  @param $pad - the value to pad the string with
 *  @param $side - the side of the string to pad
 */
function date_sql_pad($str, $size = 2, $pad = '0', $side = 'l') {
  switch ($side) {
  case ('r'):
    return "RPAD($str, $size, '$pad')";
  default:
    return "LPAD($str, $size, '$pad')";
  }
}