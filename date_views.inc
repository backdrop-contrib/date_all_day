<?php
// ?Id?
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
        <p>
        A flexible date range argument is available in views. 
        The argument expects a value like 2006-01-01::2006-01-15, or 2006W24, or @1W.
        The from and to dates in argument are ISO dates, but can be shortened and missing parts will be added. 
        Omitted parts of ISO dates will be assumed to be the first possible (for the from date)
        or the last possible (for the to date) value in that time period. 
        The \'to\' portion of the argument can be eliminated if it is the same as the \'from\' portion. 
        If the \'from\' portion is eliminated, the current date and time is assumed as the from date. 
        </p><p>
        Several shortcuts are available:
        </p><ul>
        <li>Use @1H, @1D, @1W, @1M, @1Y to get next hour/day/week/month/year from now</li>
        <li>Use date before @ sign to get next hour/day/week/month/year from that date</li>
        <li>Use format like 2006W24 to find ISO week number 24 in year 2006</li>
        </ul>
        <p>Examples of views date range arguments:</p>
        <table>
        <tr><th>Argument</th><th>Resulting Query Range</th></tr>
        <tr><td>2006W24</td><td>24th ISO week in 2006</td></tr>
        <tr><td>2006</td><td>the whole year of 2006</td></tr>
        <tr><td>2006-03</td><td>the whole month of Mar 2006</td></tr>
        <tr><td>2006-02::2007-03</td><td>Feb 1 2006 to Mar 31 2006</td></tr>
        <tr><td>2006-08-31T14::2006-08-31T16</td><td>the 14th to 16th hours of Aug 8 2006</td></tr>
        <tr><td>::2006-12-31</td><td>NOW to 2006-12-31T23:59:59</td></tr>
        <tr><td>@3H</td><td>NOW to three hours from now</td></tr>
        <tr><td>@90D</td><td>NOW to 90 days from now</td></tr>
        <tr><td>@1Y</td><td>NOW to one year from now</td></tr>
        <tr><td>2006-03-05@1W</td><td>the week starting Mar 5 2006</td></tr>
        <tr><td>2006-01@3M</td><td>3 months starting Jan 2006</td></tr>
        </table>');
}
 
/**
 * Implementation of hook_views_arguments().
 */
function date_views_arguments() {
   
  $field_types = _content_field_types();
  $arguments = array();

  foreach (content_fields() as $field) {
    $db_info = content_database_info($field);
    switch ($field['type']) {
    case 'date':
    case 'datestamp':
      
      $argument = array();
      $argument['name'] = $field_types[$field['type']]['label'] .t(' by date range: '). $field['widget']['label'] .' ('. $field['field_name'] .')';
      $argument['handler'] = 'date_views_argument_range_handler';
      $argument['help'] = t("Defines an argument to filter for dates within a range, in the format 'YYYY-MM-DD::YYYY-MM-DD'. Many other options can be used in arguments. See %link for other examples.", array('%link' => l(t('help'), 'admin/help/date')));
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
 * Argument expects a value like 2006-01-01::2006-01-15, or 2006W24, or @1W
 * 
 * From and to dates in argument are ISO dates, but can be shortened and missing parts will be added
 * Omitted parts of ISO dates will be assumed to be the first possible (for the from date)
 * or the last possible (for the to date) value in that time period
 * 
 * The 'to' portion of the argument can be eliminated if it is the same as the 'from' portion
 * If the 'from' portion is eliminated, the current date and time is assumed
 * 
 * Shortcuts are available:
 * 
 * Use @1H, @1D, @1W, @1M, @1Y to get next hour/day/week/month/year from now
 * Use date before @ sign to get next hour/day/week/month/year from that date
 * 
 * Use format like 2006W24 to find ISO week number 24 in year 2006
 * 
 * Examples:
 * 
 * --------- ARGUMENT --------  >> -------- RESULTING QUERY RANGE --------
 * 2006W24                      >> 24th ISO week in 2006
 * 2006                         >> the whole year of 2006
 * 2006-03                      >> the whole month of Mar 2006
 * 2006-02::2007-03             >> Feb 1 2006 to Mar 31 2006
 * 2006-08-31T14::2006-08-31T16 >> the 14th to 16th hours of Aug 8 2006
 * ::2006-12-31                 >> NOW to 2006-12-31T23:59:59
 * @3H                          >> NOW to three hours from now
 * @90D                         >> NOW to 90 days from now
 * @1Y                          >> NOW to one year from now
 * 2006-03-05@1W                >> the week starting Mar 5 2006
 * 2006-01@3M                   >> 3 months starting Jan 2006
 * 
 * The groupby selector values are used only if a summary view of the argument is requested
 * possible values are by year, by month, by week, by day, and by hour
 * 
 * if summaries are used, navigating to the view with no argument will display subtotals for the query, 
 * grouped by the selected range, with a link to the complete query for each range
 * 
 */
function date_views_argument_range_handler($op, &$query, $argtype, $arg = '') {
  static $format;

  include_once(drupal_get_path('module', 'date') .'/date.inc');
  
  $name = explode(':', is_array($argtype) ? $argtype['type'] : $argtype);
  $type = explode('-', $name[0]);
  $field_type = ($type[0] == 'date' ? 'iso' : 'int');
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
              date_sql('YEAR', $value, $field_type, $offset), 
              ));
          break;
        case ('month'):
          $format = 'F Y';
          $fieldinfo['field'] = date_sql_concat(array(
              date_sql('YEAR', $value, $field_type), "'-'", 
              date_sql_pad(date_sql('MONTH', $value, $field_type, $offset)),
              ));
          break;
        case ('day'):
          $format = 'F j Y';
          $fieldinfo['field'] = date_sql_concat(array(
              date_sql('YEAR', $value, $field_type), "'-'", 
              date_sql_pad(date_sql('MONTH', $value, $field_type, $offset)), "'-'",
              date_sql_pad(date_sql('DAY', $value, $field_type, $offset)),
              ));
          break;
        case ('hour'):
          $format = 'F j Y - H';
          $fieldinfo['field'] = date_sql_concat(array(
              date_sql('YEAR', $value, $field_type), "'-'", 
              date_sql_pad(date_sql('MONTH', $value, $field_type, $offset)), "'-'",
              date_sql_pad(date_sql('DAY', $value, $field_type, $offset)), "'T'",
              date_sql_pad(date_sql('HOUR', $value, $field_type, $offset)),
              ));
          break;
        case ('week'):
          $format = 'F j Y (W)';
          $fieldinfo['field'] = date_sql_concat(array(
              date_sql('YEAR', $value, $field_type, $offset), "'W'", 
              date_sql('WEEK', $value, $field_type, $offset),
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
      
      if (stristr($arg, '@')) {
        // for a date plus value, get the min and max values
        $range = date_plus_value($arg);
        $min_date = $range[0];
        $max_date = $range[1];
      }
      elseif (stristr($arg, 'W')) {
        // for a specified week, get the min and max values
        $range = date_week_value($arg);
        $min_date = $range[0];
        $max_date = $range[1];
      }
      else {
        // for all other get the date range from the supplied argument
        $range = (array) explode('::', $arg);
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
 *  similar to above function but altered to return a week range
 *  @value - an argument in the format 2006W20 (year + W + week number)
 *  @return an array of ISO dates representing the first and last day in the week
 */
function date_week_value($value) {
  
  include_once(drupal_get_path('module', 'date') .'/date.inc');

  $parts = explode('W', $value);
  $year = $parts[0];
  $weeks = intval($parts[1]);
  // get a unix value for the first day of the year
  $first_day_of_year = date_iso2unix($year .'-01-01T00:00:00');
  
  // need to get to the monday of the week $weeks after the first day of the year
  $dow   = date_date('w', $first_day_of_year);
  $shift = intval((1 - $dow) * 86400);
  $first_day_of_week = $first_day_of_year + ($weeks * 604800) + $shift;
  $last_day_of_week  = $first_day_of_week + 604800 - 1;
  // convert the unix dates back to iso
  return array(date_unix2iso($first_day_of_week), date_unix2iso($last_day_of_week));

}

/**
 *  Compute min and max dates for a @ value
 * 
 *  @value = an argument in the format @1W (a date or no date + a count + a type
 *  @return an array of ISO dates representing the first and last day in the range
 */
function date_plus_value($value) {
  
  include_once(drupal_get_path('module', 'date') .'/date.inc');

  // min date is whatever is to the left of the plus sign, defaults to current date
  $range = explode('@', $value);
  $min_date = date_range_value($range[0], 'min');
  
  // convert min date to unix to do date math
  $min_unix = date_iso2unix($min_date);
  
  // for hour, day, and week add seconds to unix date 
  // for month and year, manipulating iso date is easier
  if (stristr($range[1], 'H')) {
    $count    = intval(str_replace('H', '', $range[1]));
    $max_unix = intval($min_unix + (3600 * $count) -1);
  }
  elseif (stristr($range[1], 'D')) {
    $count    = intval(str_replace('D', '', $range[1]));
    $max_unix = intval($min_unix + (86400 * $count) -1);
  }
  elseif (stristr($range[1], 'W')) {
    $count    = intval(str_replace('W', '', $range[1]));
    $max_unix = intval($min_unix + (604800 * $count) -1);
  }
  elseif (stristr($range[1], 'M')) {
    $count    = intval(str_replace('M', '', $range[1]));
    $cur_mon  = intval(substr($min_date, 6, 2));
    $cur_year = intval(substr($min_date, 0, 4));
    $max_date = (intval($cur_mon + $count) < 12 ? $cur_year : intval($cur_year + 1)) . '-'. sprintf('%02d', (intval($cur_mon + $count) < 12 ? intval($cur_mon + $count) : 1)) . substr($min_date, 7, 11);
  }
  elseif (stristr($range[1], 'Y')) {
    $count = intval(str_replace('Y', '', $range[1]));
    $max_date = intval(substr($min_date, 0, 4) + $count) . substr($min_date, 5, 15);
    break;
  }
  $max_date = $max_unix ? date_unix2iso($max_unix) : $max_date;
  return array($min_date, $max_date);

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