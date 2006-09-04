<?php
// $Id$
/**
 * Views field filters
 */
function date_views_filters($field) {
  return array(
   'default' => array(
     'operator' => array(
       '=' => t('is equal to'),
       '!=' => t('is not equal to'),
       '>' => t('greater than'),
       '>=' => t('greater than or equal to'),
       '<' => t('less than'),
       '<=' => t('less than or equal to'),
       'LIKE' => t('matches the pattern'),
       ),
     ),
   );
}

function date_views_help() {
  return t('<h2>Date Views</h2>
        <h3>Date Range Argument</h3>
        <p>
        A flexible date range argument is provided in views. 
        The date range argument is based on the ISO 8601 week, duration, and time interval standards. 
        The argument expects a value like 2006-01-01T12:30:00--2006-01-15, or 2006-W24, or @P1W.
        Separate from and to dates with a separator, a double hyphen (--). 
        The from and to dates in argument are ISO dates, but parts can be omitted,  
        and will be assumed to be the first possible (for the from date)
        or the last possible (for the to date) value in that time period. 
        The \'to\' portion of the argument can be eliminated if it is the same as the \'from\' portion. 
        Use @ instead of a date to substitute in the current date and time. 
        </p><p>
        Several shortcuts are available:
        </p><ul>
        <li>Use @P1H, @P1D, @P1W, @P1M, @P1Y to get next hour/day/week/month/year from now</li>
        <li>Use date before P sign to get next hour/day/week/month/year from that date</li>
        <li>Periods can be combined, i.e. P1Y2M4D sets a date range of 1 year, 2 months, and 4 days.</li>
        <li>Use format like 2006-W24 to find ISO week number 24 in year 2006</li>
        </ul>
        <p>Examples of views date range arguments:</p>
        <table>
        <tr><th>Argument</th><th>Resulting Query Range</th></tr>
        <tr><td>2006-W24</td><td>24th ISO week in 2006</td></tr>
        <tr><td>2006</td><td>the whole year of 2006</td></tr>
        <tr><td>2006-03</td><td>the whole month of Mar 2006</td></tr>
        <tr><td>2006-02--2007-03-15</td><td>Feb 1 2006 to Mar 15 2006</td></tr>
        <tr><td>2006-08-31T14:00:00P2H</td><td>the 14th to 16th hours of Aug 8 2006</td></tr>
        <tr><td>@--2006-12-31</td><td>NOW to 2006-12-31T23:59:59</td></tr>
        <tr><td>@P3H</td><td>NOW to three hours from now</td></tr>
        <tr><td>@P1Y90D</td><td>NOW to 1 year and 90 days from now</td></tr>
        <tr><td>2006-03-05P1W</td><td>the 1 week period starting Mar 5 2006</td></tr>
        <tr><td>2006-01P3M</td><td>the 3 month period starting Jan 2006</td></tr>
        </table><p>See <a href="http://en.wikipedia.org/wiki/ISO_8601">http://en.wikipedia.org/wiki/ISO_8601</a> 
        for more information abou ISO dates, ISO weeks, and ISO duration and time intervals.');
}
 
/**
 * Implementation of hook_views_arguments().
 */
function date_views_arguments() {
   
  // must create an argument with the name content: $field['field_name'] to override 
  // the one created automatically for all fields by content.module (which will NOT work correctly)
  // sending both to the same handler ultimately, using a wrapper function to identify
  // whether the date is an iso or timestamp date
  
  $field_types = _content_field_types();
  $arguments = array();

  foreach (content_fields() as $field) {
    $db_info = content_database_info($field);
    switch ($field['type']) {
    case 'date':
      $argument = array();
      $argument['name'] = $field_types[$field['type']]['label'] .' '. $field['widget']['label'] .' ('. $field['field_name'] .')';
      $argument['handler'] = 'date_views_argument_range_handler';
      $argument['help'] = t("Defines an argument to filter for dates within a range, in the format 'YYYY-MM-DD--YYYY-MM-DD'. Many other options can be used in arguments. See %link for other examples.", array('%link' => l(t('help'), 'admin/help/date')));
      $argument['option'] = 'date_range_arg_options';
      $arguments['content: '. $field['field_name']] = $argument;
      break;
    
    case 'datestamp':
      $argument = array();
      $argument['name'] = $field_types[$field['type']]['label'] .' '. $field['widget']['label'] .' ('. $field['field_name'] .')';
      $argument['handler'] = 'date_views_timestamp_argument_range_handler';
      $argument['help'] = t("Defines an argument to filter for dates within a range, in the format 'YYYY-MM-DD--YYYY-MM-DD'. Many other options can be used in arguments. See %link for other examples.", array('%link' => l(t('help'), 'admin/help/date')));
      $argument['option'] = 'date_range_arg_options';
      $arguments['content: '. $field['field_name']] = $argument;
      break;
    }
  }
  return $arguments;
}

/**
 * 
 * Flexible date range argument handler
 * 
 * Argument is based on ISO 8601 date duration and time interval standards
 * 
 * See http://en.wikipedia.org/wiki/ISO_8601#Week_dates for definitions of ISO weeks
 * See http://en.wikipedia.org/wiki/ISO_8601#Duration for definitions of ISO duration and time interval
 * 
 * Argument expects a value like 2006-01-01--2006-01-15, or 2006-W24, or @P1W
 * Separate from and to dates or date and period with a double hyphen (--)
 * 
 * From and to dates in argument are ISO dates, but can be shortened and missing parts will be added
 * Omitted parts of ISO dates will be assumed to be the first possible (for the from date)
 * or the last possible (for the to date) value in that time period
 * 
 * The 'to' portion of the argument can be eliminated if it is the same as the 'from' portion
 * Use @ instead of a date to substitute in the current date and time.
 * 
 * Shortcuts are available:
 * 
 * Use periods (P1H, P1D, P1W, P1M, P1Y) to get next hour/day/week/month/year from now
 * Use date before P sign to get next hour/day/week/month/year from that date
 * The ISO standard calls for a separator (--) between a date and the P, 
 * but the separator is optional between a start date and a period in this argument 
 * to make the result easier to read.
 * 
 * This module does not currently handle the option of using a period with an end date,
 * only a start date followed by a period.
 * 
 * Use format like 2006-W24 to find ISO week number 24 in year 2006
 * 
 * Examples:
 * 
 * --------- ARGUMENT --------    >> -------- RESULTING QUERY RANGE --------
 * 2006-W24                       >> 24th ISO week in 2006
 * 2006                           >> the whole year of 2006
 * 2006-03                        >> the whole month of Mar 2006
 * 2006-02--2007-03               >> Feb 1 2006 to Mar 31 2006
 * 2006-08-31T14--2006-08-31T16   >> the 14th to 16th hours of Aug 8 2006
 * @--2006-12-31                  >> NOW to 2006-12-31T23:59:59
 * @P3H                           >> NOW to three hours from now
 * @P1Y90D                        >> NOW to 1 year and 90 days from now
 * 2006-03-05P1W                  >> the week starting Mar 5 2006
 * 2006-01P3M                     >> 3 months starting Jan 2006
 * 
 * The groupby selector values are used only if a summary view of the argument is requested
 * possible values are by year, by month, by week, by day, and by hour
 * 
 * if summaries are used, navigating to the view with no argument will display subtotals for the query, 
 * grouped by the selected range, with a link to the complete query for each range
 * 
 */
function date_views_timestamp_argument_range_handler($op, &$query, $argtype, $arg = '') {
  // this is just a wrapper function that sets the date type 
  return date_views_argument_range_handler($op, $query, $argtype, $arg, 'int');
}
function date_views_argument_range_handler($op, &$query, $argtype, $arg = '', $field_type = 'iso') {
  static $format;
  
  include_once(drupal_get_path('module', 'date') .'/date.inc');
  
  $name = explode(':', is_array($argtype) ? $argtype['type'] : $argtype);
  $field_name = trim($name[1]);
  $field      = content_fields($field_name);
  $db_info    = content_database_info($field);
  $value      = $db_info['columns']['value']['column'];
  $timezone   = $db_info['columns']['timezone']['column'];
  $table      = 'node_data_'. $field['field_name']; // uses views alias table name
  
  // this needs work, date-specific timezone handling will not work right in views now
  switch ($field['tz_handling']) {
  case ('date'):
  case ('site'):
    $offset = variable_get('date_default_timezone', 0);
    break;
  default:
    $offset = '';
    break;
  }
  
  switch ($op) {
    case 'summary':
      
      // in the summary operation, the arg contains the selected option
      $groupby = $arg;
      
      switch ($groupby) {
        case ('year'):
          $format = 'Y';
          $fieldinfo['field'] = date_sql_concat(array(
              date_sql('YEAR', $table .'.'. $value, $field_type, $offset), 
              ));
          break;
        case ('month'):
          $format = 'F Y';
          $fieldinfo['field'] = date_sql_concat(array(
              date_sql('YEAR', $table .'.'. $value, $field_type), "'-'", 
              date_sql_pad(date_sql('MONTH', $table .'.'. $value, $field_type, $offset)),
              ));
          break;
        case ('day'):
          $format = 'F j Y';
          $fieldinfo['field'] = date_sql_concat(array(
              date_sql('YEAR', $table .'.'. $value, $field_type), "'-'", 
              date_sql_pad(date_sql('MONTH', $table .'.'. $value, $field_type, $offset)), "'-'",
              date_sql_pad(date_sql('DAY', $table .'.'. $value, $field_type, $offset)),
              ));
          break;
        case ('hour'):
          $format = 'F j Y - H';
          $fieldinfo['field'] = date_sql_concat(array(
              date_sql('YEAR', $table .'.'. $value, $field_type), "'-'", 
              date_sql_pad(date_sql('MONTH', $table .'.'. $value, $field_type, $offset)), "'-'",
              date_sql_pad(date_sql('DAY', $table .'.'. $value, $field_type, $offset)), "'T'",
              date_sql_pad(date_sql('HOUR', $table .'.'. $value, $field_type, $offset)),
              ));
          break;
        case ('week'):
          $format = 'F j Y (W)';
          $fieldinfo['field'] = date_sql_concat(array(
              date_sql('YEAR', $table .'.'. $value, $field_type, $offset), "'-W'", 
              date_sql('WEEK', $table .'.'. $value, $field_type, $offset),
              ));
          break;
      }
      $fieldinfo['fieldname'] = 'range';
      
      $query->ensure_table($table);
      $query->add_field($value, $table);
      $query->add_field($timezone, $table);
      return $fieldinfo;
      break;

    case 'link':
      // links are created from date range groupings
      $time = $query->$value > '' ? ($field_type == 'iso' ? date_iso2unix($query->$value) : $query->$value) : '';
      return l(date_date($format, $time), $arg .'/'. $query->range);

    case 'sort':
      break;

    case 'filter':
      
      if (stristr($arg, 'P')) {
        // for a date plus value, get the min and max values
        $range = date_plus_period($arg);
        $min_date = $range[0];
        $max_date = $range[1];
      }
      elseif (stristr($arg, '-W') && !stristr($arg, '--')) {
        // for a specified week, get the min and max values
        $range = date_week_value($arg);
        $min_date = $range[0];
        $max_date = $range[1];
      }
      else {
        // for all other get the date range from the supplied argument
        $range = (array) explode('--', $arg);
        $min_date = date_range_value($range[0], 'min');
        $max_date = date_range_value(($range[1] ? $range[1] : $range[0]), 'max');
      }
      
      $query->ensure_table($table);
      $query->add_field($value, $table);
      $query->add_field($timezone, $table);
      $query->add_where(date_sql('DATE', $table .'.'. $value, $field_type, $offset) .">='". str_replace('T', ' ', $min_date) ."'");
      $query->add_where(date_sql('DATE', $table .'.'. $value, $field_type, $offset) ."<='". str_replace('T', ' ', $max_date) ."'");
      break;
  
    case 'title':
      $item = array(key($db_info['columns']) => $query);
      return content_format($field, $item, 'default');
  }
}

/**
 *  Validate and pad date range argument element
 * 
 *  @param $value - a full or partial ISO date from an argument
 *  @param $value_type - min or max, whether it is the from or the to part of the range
 *  @return complete, validated ISO date
 */
function date_range_value($value, $value_type = 'min') {
  
  include_once(drupal_get_path('module', 'date') .'/date.inc');

  $now   = date_date(DATE_STRING_ISO, time());
  if (trim($value) == '@' || trim($value) == '') return $now;

  switch (strlen($value)) {
  case (4):
    $return = ($value_type == 'min' ? $value .'-01-01T00:00:00' : $value .'-12-31T23:59:59');
    break;
  case (7):
    $return = ($value_type == 'min' ? $value .'-01T00:00:00' : $value .'-31T23:59:59');
    break;
  case (10):
    $return = ($value_type == 'min' ? $value .'T00:00:00' : $value .'T23:59:59');
    break;
  case (13):
    $return = ($value_type == 'min' ? $value .':00:00' : $value .':59:59');
    break;
  case (16):
    $return = ($value_type == 'min' ? $value .':00' : $value .':59');
    break;
  case (19):
    $return = $value;
    break;
  default:
    $return = $now;
  }
  // use date_preg to test for validity of constructed date
  return (date_preg($return) ? $return : $now);
}

/** 
 *  Compute min and max dates for a week
 * 
 *  based on ISO weeks, which start counting on the first Monday in a week that 
 *  has at least 4 days in the current year
 * 
 *  @value - an argument in the format 2006-W20 (year + -W + week number)
 *  @return an array of ISO dates representing the first and last day in the week
 */
function date_week_value($value) {
  
  include_once(drupal_get_path('module', 'date') .'/date.inc');

  $parts = explode('-W', $value);
  $year = $parts[0];
  // subtract 1 from week number so we don't double-count the final week
  $weeks = intval($parts[1] - 1);
  // get a unix value for the first day of the year
  $first_day_of_year = date_iso2unix($year .'-01-01T00:00:00');
  
  // get to the day of week of the first day of the year, 0 is Sunday
  $dow   = date_gmdate('w', $first_day_of_year);
  // ISO week counts actual first week only if it has at least 4 days in it
  if ($dow > 2) $weeks += 1;
  // calc adjustment from first day of year dow back or forward to Monday
  $shift = intval((1 - $dow) * 86400);
  // the day we want is $weeks away from first day of year, adjusted to the Monday of that week by $shift
  $first_day_of_week = $first_day_of_year + ($weeks * 604800) + $shift;
  $last_day_of_week  = $first_day_of_week + 604800 - 1;
  // convert the unix dates back to iso
  return array(date_unix2iso($first_day_of_week), date_unix2iso($last_day_of_week));

}

/**
 *  Compute min and max dates for a P value
 * 
 *  @value = an argument in the format (start date)P#(period type)
 *     where (period type) can be Y (year), M (month), D (day), W (week), H (hour)
 *     i.e. P1Y or P90D or P1Y3M2D4H
 *  @return an array of ISO dates representing the first and last day in the range
 */
function date_plus_period($value) {
  
  include_once(drupal_get_path('module', 'date') .'/date.inc');

  // min date is whatever is to the left of the period sign, defaults to current date
  $value = str_replace('--P', 'P', $value);
  $range = explode('P', $value);
  $min_date = date_range_value($range[0], 'min');

  // create a date object to use for the max_date
  $max_date = date_make_date($min_date, 'GMT', 'db', DATE_ISO);
  
  // iterate through the requested period, adding values as needed to the date object
  $remaining = $range[1];
  if ($years = strpos($remaining, 'Y')) {
    $sub       = explode('Y', $remaining);
    $remaining = $sub[1];
    $count     = intval($sub[0]);
    $max_iso   = intval(substr($max_date->db->iso, 0, 4) + $count) . substr($max_date->db->iso, 4, 15);
    date_set_date($max_date, $max_iso, 'GMT', 'db', DATE_ISO, TRUE);
  }
  if ($months = strpos($remaining, 'M')) {
    $sub       = explode('M', $remaining);
    $remaining = $sub[1];
    $count     = intval($sub[0]);
    $cur_mon   = intval(substr($max_date->db->iso, 6, 2));
    $cur_year  = intval(substr($max_date->db->iso, 0, 4));
    $max_iso   = (intval($cur_mon + $count) < 12 ? $cur_year : intval($cur_year + 1)) . '-'. sprintf('%02d', (intval($cur_mon + $count) < 12 ? intval($cur_mon + $count) : 1)) . substr($min_date, 7, 12);
    date_set_date($max_date, $max_iso, 'GMT', 'db', DATE_ISO, TRUE);
  }
  if (stristr($range[1], 'W')) {
    $sub       = explode('W', $remaining);
    $remaining = $sub[1];
    $count     = intval($sub[0]); 
    $max_unix  = intval($max_date->db->timestamp + (604800 * $count));
    date_set_date($max_date, $max_unix, 'GMT', 'db', DATE_UNIX, TRUE);
  }
  if ($days = strpos($remaining, 'D')) {
    $sub       = explode('D', $remaining);
    $remaining = $sub[1];
    $count     = intval($sub[0]); 
    $max_unix  = intval($max_date->db->timestamp + (86400 * $count));
    date_set_date($max_date, $max_unix, 'GMT', 'db', DATE_UNIX, TRUE);
  }
  if ($hours = strpos($remaining, 'H')) {
    $sub       = explode('H', $remaining);
    $remaining = $sub[1];
    $count     = intval($sub[0]); 
    $max_unix  = intval($max_date->db->timestamp + (3600 * $count));
    date_set_date($max_date, $max_unix, 'GMT', 'db', DATE_UNIX, TRUE);
  }
  // slice 1 second off max date to stop it just before end of period
  // needed because we are using <= as the operator
  $date->db->unix = intval($max_date->db->timestamp - 1);
  date_set_date($max_date, $date->db->unix, 'GMT', 'db', DATE_UNIX, TRUE);
  return array($min_date, $max_date->db->iso);

}

/**
 *  Define groupby options for date range summaries
 */
function date_range_arg_options() {
  return array(
    'year'  => t('summarize by year'), 
    'month' => t('summarize by month'), 
    'day'   => t('summarize by day'), 
    'week'  => t('summarize by week'), 
    'hour'  => t('summarize by hour')
    );
}