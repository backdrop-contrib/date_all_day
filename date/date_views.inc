<?php
// $Id$

/**
 * Views field filters
 */
function _date_views_filters($field) {
  switch ($field['type']) {
    case(DATE_ISO):
      $handler = 'date_views_filter_handler';
      $ymd_handler = 'date_views_handler_filter_ymd';
      break;
    case(DATE_UNIX):
      $handler = 'date_views_timestamp_filter_handler';
      $ymd_handler = 'date_views_timestamp_handler_filter_ymd';
      break;
  }
  $format = $field['widget']['input_format'];

  // use this to default to current time
  $current = array('' => t('<all>'), 'now' => t('now'));
  $months  = ($current + drupal_map_assoc(range(1, 12), 'map_month'));
  $days    = ($current + drupal_map_assoc(range(1, 31)));

  $operator = array(
        '=' => t('is equal to'),
        '<>' => t('is not equal to'),
        '>' => t('greater than'),
        '>=' => t('greater than or equal to'),
        '<' => t('less than'),
        '<=' => t('less than or equal to'),
        );

  $filters = array(
    'default' => array(
      'name' => t('Date'),
      'operator' => $operator,
      'value' => date_views_handler_filter_date_value_form($field),
      'option' => 'string',
      'handler' => $handler,
      'type' => 'DATE',
      'extra' => array('column' => 'value', 'field' => $field),
      'help' => t('This filter allows events to be filtered by their date. Enter dates in the format: %format. Enter \'now\' to use the current time. You may enter a delta (in seconds) to the option that will be added to the time; this is most useful when combined with now. ', array('%format' => $format)),
      ),
    'year' => array(
      'name' => t('Year'),
      'operator' => $operator,
      'handler' => $handler,
      'option' => 'string',
      'type' => 'YEAR',
      'extra' => array('column' => 'value', 'field' => $field),
      'help' => t('Filter by year. Enter \'now\' to use the current year.  You may enter a delta (in seconds) to the option that will be added to the time; this is most useful when combined with now. '),
      ),
    'month' => array(
      'name' => t('Month'),
      'operator' => $operator,
      'list' => $months,
      'list-type' => 'select',
      'handler' => $handler,
      'option' => 'string',
      'type' => 'MONTH',
      'extra' => array('column' => 'value', 'field' => $field),
      'help' => t('Filter by month. Enter \'now\' to use the current month.  You may enter a delta (in seconds) to the option that will be added to the time; this is most useful when combined with now. '),
      ),
    'day' => array(
      'name' => t('Day'),
      'operator' => $operator,
      'list' => $days,
      'list-type' => 'select',
      'handler' => $handler,
      'option' => 'string',
      'type' => 'DAY',
      'extra' => array('column' => 'value', 'field' => $field),
      'help' => t('Filter by day. Enter \'now\' to use the current day.  You may enter a delta (in seconds) to the option that will be added to the time; this is most useful when combined with now. '),
      ),
    );

    // Differentiate from and to dates with a prefix that is not likely to ever be used normally.
    if ($field['todate']) {
      $filters2 = array(
        'to|default' => array(
          'name' => t('To Date'),
          'operator' => $operator,
          'value' => date_views_handler_filter_date_value_form($field),
          'option' => 'string',
          'handler' => $handler,
          'type' => 'DATE',
          'extra' => array('column' => 'value2', 'field' => $field),
          'help' => t('This filter allows events to be filtered by their date. Enter dates in the format: %format. Enter \'now\' to use the current time. You may enter a delta (in seconds) to the option that will be added to the time; this is most useful when combined with now. If you have the Date Popup module installed, you can use a popup date picker here.', array('%format' => $format)),
          ),
        'to|year' => array(
          'name' => t('To Year'),
          'operator' => $operator,
          'handler' => $handler,
          'option' => 'string',
          'type' => 'YEAR',
          'extra' => array('column' => 'value2', 'field' => $field),
          'help' => t('Filter by year. Enter \'now\' to use the current year.  You may enter a delta (in seconds) to the option that will be added to the time; this is most useful when combined with now. '),
          ),
        'to|month' => array(
          'name' => t('To Month'),
          'operator' => $operator,
          'list' => $months,
          'list-type' => 'select',
          'handler' => $handler,
          'option' => 'string',
          'type' => 'MONTH',
          'extra' => array('column' => 'value2', 'field' => $field),
          'help' => t('Filter by month. Enter \'now\' to use the current month.  You may enter a delta (in seconds) to the option that will be added to the time; this is most useful when combined with now. '),
          ),
        'to|day' => array(
          'name' => t('To Day'),
          'operator' => $operator,
          'list' => $days,
          'list-type' => 'select',
          'handler' => $handler,
          'option' => 'string',
          'type' => 'DAY',
          'extra' => array('column' => 'value2', 'field' => $field),
          'help' => t('Filter by day. Enter \'now\' to use the current day.  You may enter a delta (in seconds) to the option that will be added to the time; this is most useful when combined with now. '),
          ),
       );
       $filters += $filters2;
    }
    return $filters;
}

/**
 *  Handler for date filter.
 */

function _date_views_timestamp_filter_handler($op, $filter, $filterinfo, &$query) {
  // this is just a wrapper function that sets the date type
  return _date_views_filter_handler($op, $filter, $filterinfo, $query, 'int');
}
function _date_views_filter_handler($op, $filter, $filterinfo, &$query, $field_type = 'iso') {
  include_once('./'. drupal_get_path('module', 'date_api') .'/date_api_sql.inc');

  $value = isset($filter['value']['date']) ? $filter['value']['date'] : $filter['value'];
  $type = $filterinfo['type'];

  // Do some validation of input values before creating any SQL.
  switch ($type) {
    case 'YEAR':
    case 'MONTH':
    case 'DAY':
      if (empty($value) || !($value == 'now' || is_numeric($value))) {
        return;
      }
      break;
    default:
      if ($field_type == 'int' && (empty($value) || !($value == 'now' || date_is_valid($value, DATE_UNIX)))) {
        return;
      }
      elseif (empty($value) || !($value == 'now' || date_is_valid($value, DATE_ISO))) {
        return;
      }
      break;
  }

  $field   = $filterinfo['extra']['field'];
  $column = $filterinfo['extra']['column'];
  $db_info = $filterinfo['content_db_info'];
  $table   = 'node_data_'. $field['field_name'];
  $table_field   = $db_info['columns'][$column]['column'];

  // When using NOW(), must adjust out the server zone adj which may not match
  // the offset we really want, and add back the right offset.
  // This will be necessary any time the server timezone does not match the site or date zone
  // because the server is going to adjust the value of NOW() for the server timezone.
  switch ($type) {
    case('DATE'):
      $date = $value == 'now' ? date_sql('NOW', 'NOW()', $field_type, intval(date_views_offset($field) + $filter['options'])) : "'$value'";
      break;
    default:
      $date = $value == 'now' ? date_sql($type, "NOW()", $field_type, intval(date_views_offset($field) + $filter['options'])) : $value;
      break;
  }
  $query->ensure_table($table);
  $query->add_where(date_sql($type, $table .".". $table_field, $field_type, date_views_offset($field)) .' '. $filter['operator'] .' '. $date);
}

/**
 * Provide a form gadget for dates.
 */
function _date_views_handler_filter_date_value_form($field) {
  include_once(drupal_get_path('module', 'date_api') .'/date_api_elements.inc');
  $form = array(
    '#type' => module_exists('date_popup') ? 'date_popup' : 'date_text',
    '#date_timezone' => date_default_timezone_name(),
    '#date_type'   => $field['type'],
    '#date_format' => 'Y-m-d',
    '#size' => 20,
  );
  return $form;
}

/**
 *  Return field or value for a timezone offset.
 */
function date_views_offset($field) {
  switch ($field['tz_handling']) {
  case(DATE_ISO):
    $offset = 'node_data_'. $field['field_name'] .'.'. $field['field_name'] .'_offset';
    break;
  case('site'):
    $offset = variable_get('date_default_timezone', 0);
    break;
  default:
    $offset = '';
    break;
  }
  return $offset;
}

/**
 * Views arguments for Date fields.
 */
function _date_views_arguments($field) {
  $field_types = _content_field_types();
  $arguments = array();
  $argument = array();
  $argument['name'] = $field_types[$field['type']]['label'] . ($field['todate'] ? t(': From ') : ': ') . t($field['widget']['label']) .' ('. $field['field_name'] .')';
  $argument['handler'] = $field['type'] == 'date' ? 'date_views_argument_range_handler' : 'date_views_timestamp_argument_range_handler';
  $argument['help'] = t("Defines an argument to filter for dates within a range, in the format 'YYYY-MM-DD--YYYY-MM-DD'. Many other options can be used in arguments. See !link for other examples.", array('!link' => l(t('help'), 'admin/help/date')));
  $argument['option'] = 'date_range_arg_options';
  $arguments['content: '. $field['field_name']] = $argument;

  if ($field['todate']) {
    $argument['name'] = $field_types[$field['type']]['label'] . t(': To ') . t($field['widget']['label']) .' ('. $field['field_name'] .')';
    $arguments['content: to|'. $field['field_name']] = $argument;
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
 * Use periods (P1H, P1D, P1W, P1M, P1Y) to get next hour/day/week/month/year from now
 * Use date before P sign to get next hour/day/week/month/year from that date
 *
 * This module does not currently handle the option of using a period with an end date,
 * only a start date followed by a period.
 *
 * The groupby selector values are used only if a summary view of the argument is requested
 * possible values are by year, by month, by week, by day, and by hour
 *
 * if summaries are used, navigating to the view with no argument will display subtotals for the query,
 * grouped by the selected range, with a link to the complete query for each range
 *
 */
function _date_views_timestamp_argument_range_handler($op, &$query, $argtype, $arg = '') {
  // this is just a wrapper function that sets the date type
  return _date_views_argument_range_handler($op, $query, $argtype, $arg, 'int');
}
function _date_views_argument_range_handler($op, &$query, $argtype, $arg = '', $field_type = 'iso') {
  static $format;

  include_once('./'. drupal_get_path('module', 'date_api') .'/date_api_sql.inc');

  $name = explode(':', is_array($argtype) ? $argtype['type'] : $argtype);
  $tofield_name = trim($name[1]);
  $field_name = substr($tofield_name, 0, 3) == 'to|' ? substr($tofield_name, 3) : $tofield_name;
  if ($field_name == $tofield_name) {
    $value = 'value';
  }
  else {
    $value = 'value2';
  }

  $field      = content_fields($field_name);
  $db_info    = content_database_info($field);
  $value      = $db_info['columns'][$value]['column'];
  $timezone   = $db_info['columns']['timezone']['column'];
  $table      = 'node_data_'. $field['field_name'];

  $offset = date_views_offset($field);

  switch ($op) {
    case 'summary':

      // in the summary operation, the arg contains the selected option
      $groupby = $arg;

      switch ($groupby) {
        case('year'):
          $format = 'Y';
          $fieldinfo['field'] = date_sql_concat(array(
              date_sql('YEAR', $table .'.'. $value, $field_type, $offset),
              ));
          break;
        case('month'):
          $format = 'F Y';
          $fieldinfo['field'] = date_sql_concat(array(
              date_sql('YEAR', $table .'.'. $value, $field_type, $offset), "'-'",
              date_sql_pad(date_sql('MONTH', $table .'.'. $value, $field_type, $offset)),
              ));
          break;
        case('day'):
          $format = 'F j Y';
          $fieldinfo['field'] = date_sql_concat(array(
              date_sql('YEAR', $table .'.'. $value, $field_type, $offset), "'-'",
              date_sql_pad(date_sql('MONTH', $table .'.'. $value, $field_type, $offset)), "'-'",
              date_sql_pad(date_sql('DAY', $table .'.'. $value, $field_type, $offset)),
              ));
          break;
        case('hour'):
          $format = 'F j Y - H';
          $fieldinfo['field'] = date_sql_concat(array(
              date_sql('YEAR', $table .'.'. $value, $field_type, $offset), "'-'",
              date_sql_pad(date_sql('MONTH', $table .'.'. $value, $field_type, $offset)), "'-'",
              date_sql_pad(date_sql('DAY', $table .'.'. $value, $field_type, $offset)), "'T'",
              date_sql_pad(date_sql('HOUR', $table .'.'. $value, $field_type, $offset)),
              ));
          break;
        case('week'):
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
      if ($field['tz_handling'] == 'date') {
        $query->add_field($timezone, $table);
      }
      return $fieldinfo;
      break;

    case 'link':
      // links are created from date range groupings
      if (empty($query->$value)) return;
      if ($field['tz_handling'] == 'date') {
        $tz_name = $query->$timezone;
      }
      else {
        $tz_name = date_default_timezone_name();
      }
      $date_type = $field_type == 'iso' ? DATE_ISO : DATE_UNIX;
      $date = date_make_date($query->$value, 'UTC', $date_type);
      date_timezone_set($date, timezone_open($tz_name));
      return l(date_format_date($date, 'custom', $format), $arg .'/'. $query->range);

    case 'sort':
      break;

    case 'filter':
      $range = date_views_date_range($arg, $field);
      $query->ensure_table($table);
      $query->add_field($value, $table);
      if ($field['tz_handling'] == 'date') {
        $query->add_field($timezone, $table);
      }
      $query->add_where(date_sql('DATE', $table .'.'. $value, $field_type, $offset) .">='". str_replace('T', ' ', $range[0]) ."'");
      $query->add_where(date_sql('DATE', $table .'.'. $value, $field_type, $offset) ."<='". str_replace('T', ' ', $range[1]) ."'");
      break;

    case 'title':
      $value = explode('P', $query);
      $item = array(key($db_info['columns']) => date_range_value($value[0]));
      return content_format($field, $item, 'default');
  }
}

function date_views_date_range($arg, $field = NULL) {
  if (stristr($arg, 'P')) {
    // for a date plus value, get the min and max values
    $range = date_plus_period_range($arg);
    $min_date = $range[0];
    $max_date = $range[1];
  }
  elseif (stristr($arg, '-W') && !stristr($arg, '--')) {
    // for a specified week, get the min and max values
    $range = date_iso_week_range($arg);
    $min_date = $range[0];
    $max_date = $range[1];
  }
  else {
    // for all other get the date range from the supplied argument
    $range = (array) explode('--', $arg);
    $min_date = date_range_value($range[0], 'min');
    $max_date = date_range_value(($range[1] ? $range[1] : $range[0]), 'max');
  }
  if (!empty($field)) {
    $min_date = date_limit_value($min_date, date_granularity($field), $field['type']);
    $max_date = date_limit_value($max_date, date_granularity($field), $field['type']);
  }
  return array($min_date, $max_date);
}
/**
 *  Validate and pad date range argument element
 *
 *  @param $value - a full or partial ISO date from an argument
 *  @param $value_type - min or max, whether it is the from or the to part of the range
 *  @return complete, validated ISO date
 */
function date_range_value($value, $value_type = 'min') {
  $now   = date_format(date_now(), DATE_FORMAT_ISO);
  if (trim($value) == '@' || trim($value) == '') return $now;

  switch (strlen($value)) {
  case(4):
    $return = ($value_type == 'min' ? $value .'-01-01T00:00:00' : $value .'-12-31T23:59:59');
    break;
  case(7):
    $return = ($value_type == 'min' ? $value .'-01T00:00:00' : $value .'-31T23:59:59');
    break;
  case(10):
    $return = ($value_type == 'min' ? $value .'T00:00:00' : $value .'T23:59:59');
    break;
  case(13):
    $return = ($value_type == 'min' ? $value .':00:00' : $value .':59:59');
    break;
  case(16):
    $return = ($value_type == 'min' ? $value .':00' : $value .':59');
    break;
  case(19):
    $return = $value;
    break;
  default:
    $return = $now;
  }
  // use regex to test for validity of constructed date
  return (preg_match(DATE_REGEX_ISO, $return) ? $return : $now);
}

/**
 *  Compute min and max dates for a week
 *
 *  based on ISO weeks, which start counting on the first Monday in a week that
 *  has at least 4 days in the current year
 *
 *  January 4 is always in the first ISO week of the year. Move ahead by the
 *  number of ISO weeks to find a date in the ISO week. Find the Monday of the
 *  ISO week for the first second of the week, move ahead 1 week and back
 *  1 second to find last second of the week.
 *
 *  @value - an argument in the format 2006-W20 (year + -W + week number)
 *  @return an array of ISO dates representing the first and last day in the week
 */
function date_iso_week_range($value) {
  $parts = explode('-W', $value);
  $year = $parts[0];
  $week = $parts[1];
  $date = date_make_date($year .'-01-04 00:00:00', 'UTC');
  date_modify($date, '+'. $week .' weeks');
  date_modify($date, '-1 Monday');
  $min_date = date_format($date, DATE_FORMAT_ISO);
  date_modify($date, '+1 week');
  date_modify($date, '-1 second');
  $max_date = date_foramt($date, DATE_FORMAT_ISO);
  return array($min_date, $max_date);
}

/**
 *  Compute min and max dates for a P value
 *
 *  Min date is whatever is to the left of the period sign, defaults to
 *  current date. Use ical module to parse the period. Set end date to
 *  1 second before the end of the period, since we use <= operator.
 *
 *  @value = an argument in the format (start date)P#(period type)
 *     where (period type) can be Y (year), M (month), D (day), W (week), H (hour)
 *     i.e. P1Y or P90D or P1Y3M2D4H
 *  @return an array of ISO dates representing the first and last day in the range
 */
function date_plus_period_range($value) {
  include_once('./'. drupal_get_path('module', 'date_api') .'/date_api_ical.inc');
  $value = str_replace('--P', 'P', $value);
  $range = explode('P', $value);

  $period = substr($range[1], -1);
  switch ($period) {
    case 'Y':
      $granularity = array('year');
      break;
    case 'M':
      $granularity = array('year', 'month');
      break;
    case 'D':
      $granularity = array('year', 'month', 'day');
      break;
    case 'W':
      $granularity = array('year');
      break;
    case 'H':
      $granularity = array('year', 'month', 'day', 'hour');
  }

  $min_date = date_range_value($range[0], 'min');
  $parsed = array('DURATION' => array(
    'DATA' => 'P'. $range[1]),
    'DTSTART' => array(
      'datetime' => date_convert($min_date, DATE_ISO, DATE_DATETIME),
      'tz' => 'UTC',
      'granularity' => $granularity,
      ),
    );
  date_ical_parse_duration($parsed);

  $max_date = date_convert($parsed['DTEND']['datetime'], DATE_DATETIME, DATE_ISO);
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

//============================== Date Browser ================================//

/**
 * Works only with views that use the date range argument
 * Adds this/next period navigation links to a date argument range view
 * Adds 'week of XXX', 'month of XXX' headings to views and blocks
 * Defaults blocks and views w/out arguments to current period to start paging
 * Choose period increments by selecting the option value of date range argument
 *  (year, month, week, day, hour)
 */

/**
 * Implementation of hook_views_style_plugins()
 */
function _date_views_style_plugins() {

  $items = array();
  $items['date_views_browser'] = array(
    'name' => t('Date: Date Browser'),
    'theme' => 'date_views_browser_full_view',
    'summary_theme' => 'date_views_browser_summary_view',
    'needs_fields' => true,
    'needs_table_header' => true,
    'validate' => 'date_browser_validate',
    'even_empty' => true,
  );
  return $items;
}

/**
 * Validate a view.
 */
function date_browser_validate($type, $view, $form) {
  // list (and table) modes require there to be at least 1 field active.
  if (is_array($view['field'])) {
    $fields = array_filter(array_keys($view['field']), 'is_numeric');
  }
  if (!$fields) {
    form_error($form["$type-info"][$type .'_type'], t('The Date Browser requires at least one field.'));
  }
  // Make sure all arguments are set to 'Display all values'
  // and that a date argument has been provided.
  $found = FALSE;
  $options = array_keys(date_range_arg_options());
  foreach ($view['argument'] as $delta => $argument) {
    if (in_array($argument['options'], $options)) {
      $found = TRUE;
      if (is_numeric($delta) && $argument['argdefault'] != 2) {
        form_error($form['argument'][$delta]['argdefault'], t('Date Browser arguments must be set to \'Display All Values\'.'));
      }
    }
  }
  if (!$found) {
    form_error($form['argument'], t('A date argument must be added to a Date Browser view.'));
  }
}

/**
 *  Implementation of hook_views_query()
 *  Used to make sure view defaults to current date if no date selected
 */
function _date_views_query_alter(&$query, &$view) {
  include_once('./'. drupal_get_path('module', 'date_api') .'/date_api_sql.inc');
  $date_views_browser_views = date_views_browser_get_views();
  if (in_array($view->name, array_keys($date_views_browser_views))) {
    $name       = explode(':', $view->argument[0]['type']);
    $field_name = trim($name[1]);
    $field      = content_fields($field_name);
    $field_type = $field['type'] == DATE_UNIX ? 'int' : 'iso';
    $db_info    = content_database_info($field);
    $table      = 'node_data_'. $field['field_name'];

    // Add a combo FROM|TO date field to the query
    $value      = $db_info['columns']['value']['column'];
    $value2     = $db_info['columns']['value2']['column'];
    $combo = date_sql_concat(array($table .'.'. $value, "'|'", $table .'.'. $value2)) .' AS date_combo ';
    $query->add_field($combo, NULL);

    $path = explode('/', $view->url);
    $pos = sizeof($path);
    if ($view->build_type == 'block' || arg($pos) == '') {
      $arg = NULL;
    }
    else {
      $arg = arg($pos);
    }
    if ($arg == NULL) {
      // if no argument specified, add the current date range to the query
      $arg  = date_views_browser_period_arg($arg, $view->argument[0]['options']);
      if ($range  = date_views_date_range($arg)) {
        $query->ensure_table($table);
        $query->add_field('nid', 'node');
        $query->add_field($value, $table);
        $query->add_where(date_sql('DATE', $table .'.'. $value, $field_type, $offset) .">='". str_replace('T', ' ', $range[0]) ."'");
        $query->add_where(date_sql('DATE', $table .'.'. $value, $field_type, $offset) ."<='". str_replace('T', ' ', $range[1]) ."'");
      }
    }
  }
}

/**
 * Find all the views that qualify for date browser treatment
 *
 * @param $view
 *   if called from Views UI validation, $edit_view will be populated,
 *   otherwise it will be empty. Use that to tell if the list
 *   needs to be refreshed.
 */
function date_views_browser_get_views($reset = FALSE) {
  static $date_views_browser_views;
  if (empty($date_views_browser_views) || $reset) {
    $cid = 'date_browser_views';
    if (!$reset && $cached = cache_get($cid, 'cache_views')) {
      $date_views_browser_views = unserialize($cached->data);
    }
    else {
      $date_views_browser_views = array();
      $arguments = array();
      $fields = content_fields();
      foreach ($fields as $field) {
        if ($field['type'] == DATE_UNIX || $field['type'] == DATE_ISO) {
          $arguments = array_merge($arguments, _date_views_arguments($field));
        }
      }
      $argument_list = "'". implode("','", array_keys($arguments)) ."'";
      if (!$argument_list) {
        return array();
      }
      $result = db_query("SELECT arg.*, view.name FROM {view_argument} arg INNER JOIN {view_view} view ON arg.vid=view.vid WHERE arg.type IN ($argument_list) AND view.page_type='date_views_browser'");
      while ($view = db_fetch_object($result)) {
        $date_views_browser_views[$view->name] = $view;
      }
      cache_set($cid, 'cache_views', serialize($date_views_browser_views));
    }
  }
  return $date_views_browser_views;
}

/**
 *  Return the correct period for the date range argument
 */
function date_views_browser_period($period = 'month') {
  switch ($period) {
  case('year'):
    return 'P1Y';
  case('week'):
    return 'P1W';
  case('day'):
    return 'P1D';
  case('hour'):
    return 'P1H';
  default:
    return 'P1M';
  }
}

function date_views_browser_period_format($period) {
    switch ($period) {
    case 'year':
      $format = 'Y';
      break;
    case 'month':
      $format = 'Y-m';
      break;
    case 'week':
      $format = 'Y-W';
      break;
    case 'day':
      $format = 'Y-m-d';
      break;
    case 'hour':
      $format = 'Y-m-d\TH';
      break;
  }
  return $format;
}

/**
 *  Format an argument for the date range
 */
function date_views_browser_period_arg($arg = NULL, $period = 'month') {
  if (empty($arg)) {
    $now = date_now();
    $arg = date_format($now, date_views_browser_period_format($period));
  }
  $range = date_views_date_range($arg);
  $date = date_make_date($range[0], 'UTC');
  return date_format_date($date, 'custom', date_views_browser_period_format($period)) .
    date_views_browser_period($period);
}

/**
 *  Return label for current date range
 */
function date_views_browser_period_label($arg = NULL, $period = 'month') {
  $range = date_views_date_range($arg);
  $date = date_make_date($range[0], 'UTC');
  return theme('date_views_browser_period_label', $period, $date);
}

/**
 *  Navigation links for the full view
 */
function date_views_browser_navigation($view, $period) {
  $path = explode('/', $view->url);
  $pos = sizeof($path);
  if (arg($pos) == '') {
    $arg = NULL;
  }
  else {
    $arg = arg($pos);
  }
  // Create a date object for the beginning of the current range.
  $range = date_views_date_range($arg);
  $date = date_make_date($range[0], 'UTC');
  // Move backwards 1 period and create a link to that date.
  date_modify($date, '-1'. $period);
  $prev = $view->url .'/'.
    date_format_date($date, 'custom', date_views_browser_period_format($period)) .
    date_views_browser_period($period);
  // Move forward 2 periods (back to the current period and to the next period)
  // and create a link to that date.
  date_modify($date, '+2 '. $period);
  $next = $view->url .'/'.
    date_format_date($date, 'custom', date_views_browser_period_format($period)) .
    date_views_browser_period($period);

  $label = date_views_browser_period_label($arg, $period);
  return theme('date_views_browser_navigation', $label, $period, $prev, $next, $view);
}