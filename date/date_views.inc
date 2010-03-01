<?php
// $Id$

/**
 * Implementation of hook_views_data().
 */
function _date_views_data($field) {
  $data = content_views_field_views_data($field);
  $db_info = content_database_info($field);
  $table_alias = content_views_tablename($field);

  // Swap the filter and argument handlers to the date handlers.
  $data[$table_alias][$field['field_name'] . '_value']['filter']['handler'] = 'date_views_filter_handler';
  $data[$table_alias][$field['field_name'] . '_value']['argument']['handler'] = 'date_views_argument_handler';

  // Set the date type in the handlers.
  $date_type = $field['type'] == 'date' ? 'iso' : 'int';
  $data[$table_alias][$field['field_name'] . '_value']['field']['date_type'] = $date_type;
  $data[$table_alias][$field['field_name'] . '_value']['filter']['date_type'] = $date_type;
  $data[$table_alias][$field['field_name'] . '_value']['argument']['date_type'] = $date_type;
  $data[$table_alias][$field['field_name'] . '_value']['sort']['date_type'] = $date_type;

  // Add in another set of fields, filters, and aguments for the To date.
  if ($field['settings']['todate']) {
    $data[$table_alias][$field['field_name'] . '_value2'] = $data[$table_alias][$field['field_name'] . '_value'];
    $data[$table_alias][$field['field_name'] . '_value']['title'] = t('!label (!field value)', array('!label' => t($field['label']), '!field' => t($field['field_name'])));
    $data[$table_alias][$field['field_name'] . '_value2']['title'] = t('!label (!field value2)', array('!label' => t($field['label']), '!field' => t($field['field_name'])));
  }

  // Create another argument for the date browser, using the configuration already created.
  $data[$table_alias][$field['field_name'] . '_browser'] = $data[$table_alias][$field['field_name'] . '_value'];
  $data[$table_alias][$field['field_name'] . '_browser']['title'] = t('Date Browser (!field value)', array('!field' => t($field['field_name'])));
  $data[$table_alias][$field['field_name'] . '_browser']['argument']['handler'] = 'date_views_browser_argument_handler';
  unset($data[$table_alias][$field['field_name'] . '_browser']['field']);
  unset($data[$table_alias][$field['field_name'] . '_browser']['filter']);
  unset($data[$table_alias][$field['field_name'] . '_browser']['sort']);
  if ($field['settings']['todate']) {
    $data[$table_alias][$field['field_name'] . '_browser2'] = $data[$table_alias][$field['field_name'] . '_value2'];
    $data[$table_alias][$field['field_name'] . '_browser2']['title'] = t('Date Browser (!field value2)', array('!field' => t($field['field_name'])));
    $data[$table_alias][$field['field_name'] . '_browser2']['argument']['handler'] = 'date_views_browser_argument_handler';
    unset($data[$table_alias][$field['field_name'] . '_browser2']['field']);
    unset($data[$table_alias][$field['field_name'] . '_browser2']['filter']);
    unset($data[$table_alias][$field['field_name'] . '_browser2']['sort']);
  }

  return $data;
}

/**
 * The subclass simply adds properties,
 * for field-specific subclasses to use if they need to.
 */
class date_views_argument_handler extends views_handler_argument_date {
  var $content_field;

  function construct() {
    parent::construct();

    $this->content_field = content_fields($this->definition['content_field_name']);
    $this->additional_fields = $this->definition['additional fields'];
  }
}

/**
 * The subclass simply adds properties,
 * for field-specific subclasses to use if they need to.
 */
class date_views_filter_handler extends views_handler_filter_custom_date {
  var $content_field;

  function construct() {
    parent::construct();

    $this->content_field = content_fields($this->definition['content_field_name']);
    $this->additional_fields = $this->definition['additional fields'];
  }
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
/**
 * A flexible, configurable date argument.
 *
 * This argument allows you to set one or more date parts to filter on,
 * such as year, month, and day; month only; a complete date, etc.
 *
 * @ingroup views_argument_handlers
 */
class date_views_browser_argument_handler extends views_handler_argument_formula {
  var $content_field;
  var $date_handler = NULL;

  /**
   * Add date handler to the argument.
   */
  function construct() {
    parent::construct();
    $this->content_field = content_fields($this->definition['content_field_name']);
    $this->additional_fields = $this->definition['additional fields'];
    $this->date_handler = new views_date_handler();
    $this->date_handler->construct();
    if (isset($this->definition['date_type'])) {
      $this->date_handler->date_type = $this->definition['date_type'];
    }
  }

  /**
   * Get granularity and use it to create the formula and a format
   * for the results.
   */
  function init(&$view, $options) {
    parent::init(&$view, $options);
    $handler = $this->date_handler;
    $handler->granularity = $handler->granularity_keys($options['granularity']);
    switch ($handler->granularity[0]) {
      case ('year'):
        $this->format = 'Y';
        $this->sql_format = 'Y';
        break;
      case ('month'):
        $this->format = 'F Y';
        $this->sql_format = 'Y-m';
        break;
      case ('day'):
        $this->format = 'F j Y';
        $this->sql_format = 'Y-m-d';
        break;
      case ('hour'):
        $this->format = 'F j Y - H';
        $this->sql_format = 'Y-m-d\TH';
        break;
      case ('week'):
        $this->format = 'F j Y (W)';
        $this->sql_format = 'Y-\WW';
        break;
    }
  }

  /**
   * Default value for the granularity option.
   */
  function options(&$options) {
    parent::options($options);
    $parts = $this->date_handler->date_parts();
    unset($parts['adjustment']);
    $options['granularity'] = 'month';
  }

  /**
   * Add a form element to select granularity.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    // Select the granularity of the date parts to use in the argument.
    $handler = $this->date_handler;
    $form['granularity'] = $handler->granularity_form($this->options['granularity']);
    $form['granularity']['#description'] = t('Time range for each page, i.e. if you use a \'Month\' date range, the page will show the selected month and the back/next links will take you to the next and previous month.');
    $form['granularity']['#type'] = 'select';
    unset($form['granularity']['#options']['adjustment']);
    unset($form['granularity']['#options']['second']);
    unset($form['granularity']['#options']['minute']);
  }

  /**
   * Re-work the form values array into a proper granularity array.
   */
  function options_validate($form, &$form_state) {
    $values = $form_state['values'];
    $handler = $this->date_handler;
    form_set_value($form['granularity'], $handler->granularity_keys($values['options']['granularity']), $form_state);
  }

  // Update the summary values to show selected granularity.
  function admin_summary() {
    $handler = $this->date_handler;
    $granularity = array_values($handler->date_parts($handler->granularity));
    return t('<br />Grouped by: @granularity', array('@granularity' => $granularity[0]));
  }

  /**
   * Provide a link to the next level of the view
   */
  function summary_name($data) {
    $created = $data->{$this->name_alias};
    return format_date(strtotime($created), 'custom', $this->format, 0);
  }

  /**
   * Provide a link to the next level of the view
   */
  function title() {
    return format_date(strtotime($this->argument), 'custom', $this->format, 0);
  }

  /**
   * Create a summary query that matches the granularity.
   *
   * Needed or Views will do a groupby on the complete date instead
   * of only the part of the date actually used in the argument.
   */
  function summary_query() {
    $this->ensure_my_table();
    $handler = $this->date_handler;

    // We need to alter the base alias so the groupby clause will group by
    // the relevant part of the date instead of a complete date.
    $this->formula = $handler->sql_format($this->sql_format, "$this->table_alias.$this->real_field");

    $alias = $this->name_alias = $this->table_alias . "_" . $this->real_field;
    $this->query->add_field(NULL, $this->get_formula(), $alias);
    $this->base_alias = $this->get_formula();

    return $this->summary_basics();
  }

  /**
   * Need to override the basic link since base_alias is now a formula.
   */
  function summary_link($data, $url) {
    $value = $data->{$this->name_alias};
    return url("$url/$value");
  }

  /**
   * Create a query that matches the argument.
   *
   * Move through the arg and pick out date values to add to the query.
   */
  function query() {
    $this->ensure_my_table();
    $range = date_views_date_range($this->argument, $this->content_field);
    $handler = $this->date_handler;
    $this->query->add_where(0, $handler->sql_field("$this->table_alias.$this->real_field") . ">='" . str_replace('T', ' ', $range[0]) . "'");
    $this->query->add_where(0, $handler->sql_field("$this->table_alias.$this->real_field") . "<='" . str_replace('T', ' ', $range[1]) . "'");
  }
}

/**
 * Deconstruct a Date Browser argument into a date range.
 *
 * @param unknown_type $arg
 * @param unknown_type $field
 * @return unknown
 */
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
    $max_date = date_range_value((isset($range[1]) ? $range[1] : $range[0]), 'max');
  }
  if (!empty($field)) {
    $min_date = date_limit_value($min_date, date_granularity($field), $field['type']);
    $max_date = date_limit_value($max_date, date_granularity($field), $field['type']);
  }
  return array($min_date, $max_date);
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
  $date = date_make_date($year . '-01-04 00:00:00', 'UTC');
  date_modify($date, '+' . $week . ' weeks');
  date_modify($date, '-1 Monday');
  $min_date = date_format($date, DATE_FORMAT_ISO);
  date_modify($date, '+1 week');
  date_modify($date, '-1 second');
  $max_date = date_format($date, DATE_FORMAT_ISO);
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
  include_once(' ./' . drupal_get_path('module', 'date_api') . '/date_api_ical.inc');
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
    'DATA' => 'P' . $range[1]),
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
  case (4):
    $return = ($value_type == 'min' ? $value . '-01-01T00:00:00' : $value . '-12-31T23:59:59');
    break;
  case (7):
    $return = ($value_type == 'min' ? $value . '-01T00:00:00' : $value . '-31T23:59:59');
    break;
  case (10):
    $return = ($value_type == 'min' ? $value . 'T00:00:00' : $value . 'T23:59:59');
    break;
  case (13):
    $return = ($value_type == 'min' ? $value . ':00:00' : $value . ':59:59');
    break;
  case (16):
    $return = ($value_type == 'min' ? $value . ':00' : $value . ':59');
    break;
  case (19):
    $return = $value;
    break;
  default:
    $return = $now;
  }
  // use regex to test for validity of constructed date
  return (preg_match(DATE_REGEX_ISO, $return) ? $return : $now);
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
    'needs_fields' => TRUE,
    'needs_table_header' => TRUE,
    'validate' => 'date_browser_validate',
    'even_empty' => TRUE,
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
    form_error($form["$type-info"][$type . '_type'], t('The Date Browser requires at least one field.'));
  }
  // Make sure all arguments are set to 'Display all values'
  // and that a date argument has been provided.
  $found = FALSE;
  $options = array_keys(date_range_arg_options());
  foreach ($view['argument'] as $delta => $argument) {
    if (in_array($argument['options'], $options)) {
      $found = TRUE;
      if (is_numeric($delta) && $argument['argdefault'] != 2) {
        form_error($form['argument'][$delta]['argdefault'], t('Date Browser arguments must be set to \'Display All Values\' .'));
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
  include_once(' ./' . drupal_get_path('module', 'date_api') . '/date_api_sql.inc');
  $date_views_browser_views = date_views_browser_get_views();
  if (in_array($view->name, array_keys($date_views_browser_views))) {
    $name       = explode(':', $view->argument[0]['type']);
    $field_name = trim($name[1]);
    $field_name = substr($field_name, 0, 3) == 'to|' ? substr($field_name, 3) : $field_name;
    $field      = content_fields($field_name);
    $field_type = $field['type'] == DATE_UNIX ? 'int' : 'iso';
    $db_info    = content_database_info($field);
    $table      = 'node_data_' . $field['field_name'];

    // Add a combo FROM|TO date field to the query
    $value      = $db_info['columns']['value']['column'];
    $value2     = !empty($db_info['columns']['value2']['column']) ? $db_info['columns']['value2']['column'] : $db_info['columns']['value']['column'];
    $combo = date_sql_concat(array($table . ' . ' . $value, "'|'", $table . ' . ' . $value2)) . ' AS date_combo ';
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
        $query->add_where(date_sql('DATE', $table . ' . ' . $value, $field_type, $offset) . ">='" . str_replace('T', ' ', $range[0]) . "'");
        $query->add_where(date_sql('DATE', $table . ' . ' . $value, $field_type, $offset) . "<='" . str_replace('T', ' ', $range[1]) . "'");
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
      $date_views_browser_views = $cached->data;
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
      $argument_list = "'" . implode("','", array_keys($arguments)) . "'";
      if (!$argument_list) {
        return array();
      }
      $result = db_query("SELECT arg.*, view.name FROM {view_argument} arg INNER JOIN {view_view} view ON arg.vid=view.vid WHERE arg.type IN ($argument_list) AND view.page_type=:type", array(
      ':type' => 'date_views_browser',
      ));
      foreach ($result as $view) {
        $date_views_browser_views[$view->name] = $view;
      }
      cache_set($cid, $date_views_browser_views, 'cache_views');
    }
  }
  return $date_views_browser_views;
}

/**
 *  Return the correct period for the date range argument
 */
function date_views_browser_period($period = 'month') {
  switch ($period) {
  case ('year'):
    return 'P1Y';
  case ('week'):
    return 'P1W';
  case ('day'):
    return 'P1D';
  case ('hour'):
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
  date_modify($date, '-1' . $period);
  $prev = $view->url . '/' .
    date_format_date($date, 'custom', date_views_browser_period_format($period)) .
    date_views_browser_period($period);
  // Move forward 2 periods (back to the current period and to the next period)
  // and create a link to that date.
  date_modify($date, '+2 ' . $period);
  $next = $view->url . '/' .
    date_format_date($date, 'custom', date_views_browser_period_format($period)) .
    date_views_browser_period($period);

  $label = date_views_browser_period_label($arg, $period);
  return theme('date_views_browser_navigation', $label, $period, $prev, $next, $view);
}
/**
 *  Return field or value for a timezone offset.
 */
function date_views_offset($field) {
  switch ($field['settings']['tz_handling']) {
  case (DATE_ISO):
    $offset = 'node_data_' . $field['field_name'] . ' . ' . $field['field_name'] . '_offset';
    break;
  case ('site'):
    $offset = variable_get('date_default_timezone', 0);
    break;
  default:
    $offset = '';
    break;
  }
  return $offset;
}

//********************************************************************************//
//********************************************************************************//
//********************************************************************************//

// TODO Remove the code below if this gets added to the Views module in http://drupal.org/node/241759.
/**
 * A class to manipulate dates.
 */
class views_date_handler {
  var $granularity = array();
  var $db_type = 'mysql';
  var $date_type = 'int';

  /**
   * Make sure granularity has a sane default value.
   */
  function construct($date_type = 'int') {
    $this->granularity = $this->granularity_keys();
    $this->db_type = $GLOBALS['db_type'];
    $this->date_type = $date_type;
  }

  /**
   * An array of all date parts,
   * optionally limited to an array of allowed parts.
   */
  function date_parts($limit = array()) {
    $parts =  array(
      'year' => t('Year'), 'month' => t('Month'), 'day' => t('Day'),
      'hour' => t('Hour'), 'minute' => t('Minute'), 'second' => t('Second'),
      'adjustment' => t('Adjustment'),
      );
    if (!empty($limit)) {
      foreach ($parts as $key => $part) {
        if (!in_array($key, $limit)) {
          unset($parts[$key]);
        }
      }
    }
    return $parts;
  }

  /**
   * The minimum valid value for a date part, or an array of all minimums.
   */
  function part_min($part = NULL) {
    $min = array(
      'year' => 100, 'month' => 1, 'day' => 1,
      'hour' => 0, 'minute' => 0, 'second' => 0);
    if (!empty($part) && array_key_exists($part, $min)) {
      return $min[$part];
    }
    return $min;
  }

  /**
   * The maximum valid value for a date part, or an array of all maximums.
   */
  function part_max($part = NULL) {
    $max = array(
      'year' => 4000, 'month' => 12, 'day' => 31,
      'hour' => 23, 'minute' => 59, 'second' => 59);
    if (!empty($part) && array_key_exists($part, $max)) {
      return $max[$part];
    }
    return $max;
  }

  /**
   * The SQL format for a date part, or an array of all formats.
   */
  function part_format($part = NULL) {
    $formats = array(
      'year' => 'Y', 'month' => 'm', 'day' => 'd',
      'hour' => 'H', 'minute' => 'i', 'second' => 's');
    if (!empty($part) && array_key_exists($part, $formats)) {
      return $formats[$part];
    }
    return $formats;
  }

  /**
   * Convert a format string into help text,
   * i.e. 'Y-m-d' becomes 'YYYY-MM-DD' .
   *
   * @param unknown_type $format
   * @return unknown
   */
  function format_help($format) {
    $replace = array(
      'Y' => 'YYYY', 'm' => 'MM', 'd' => 'DD',
      'H' => 'HH', 'i' => 'MM', 's' => 'SS', '\T' => 'T');
    return strtr($format, $replace);
  }

  /**
   * Rewrite a format string so it only inludes elements from a
   * specified granularity array.
   *
   * Example:
   *   date_limit_format('F j, Y - H:i', array('year', 'month', 'day'));
   *   returns 'F j, Y'
   *
   * @param $format
   *   a format string
   * @param $granularity
   *   an array of allowed date parts, all others will be removed
   *   array('year', 'month', 'day', 'hour', 'minute', 'second');
   * @return
   *   a format string with all other elements removed
   */
  function limit_format($format, $granularity) {
    // Strip out timezone formatting.
    $regex = array('([OZPe])');
    // Get rid of dash separating date and time if either is missing.
    if (!date_has_time($granularity)
      || sizeof(array_intersect($granularity, array('year', 'month', 'day')) == 0)) {
      $regex[] = '( -)';
    }
    if (!date_has_time($granularity)) {
        $regex[] = '(a|A)';
        $regex[] = '(\\\T)';
    }
    // Create regular expressions to remove selected values from string.
    $nongranularity = array_diff(array('year', 'month', 'day', 'hour', 'minute', 'second'), $granularity);
    foreach ($nongranularity as $element) {
      switch ($element) {
        case 'year':
          $regex[] = '([\-/\.]?[Yy][\-/\.,]?)';
          break;
        case 'day':
          $regex[] = '([\-/\.]?[lDdj][\-/\.,]?)';
          break;
        case 'month':
          $regex[] = '([\-/\.]?[FMmn][\-/\.,]?)';
          break;
        case 'hour':
          $regex[] = '([HhGg][:]?)';
          break;
        case 'minute':
          $regex[] = '([:]?[i])';
          break;
        case 'second':
          $regex[] = '([:]?[s])';
          break;
      }
    }
    // Remove selected values from string.
    // Don't leave any trailing punctuation behind.
    $format = trim(preg_replace($regex, array(), $format));
    return preg_replace('([\-/\.,]$)', '', $format);
  }

  /**
   * Normalized, validated granularity key array.
   *
   * Produce a simple key array from a variety of
   * possible granularity inputs:
   *   date_parts() array format:
   *       $key => $label,
   *       $key2 => $label2,
   *       $key3 => $label3,
   *   FAPI checkboxes $form_values format:
   *       $key => 0,
   *       $key2 => $key2,
   *       $key3 => $key3,
   *   Normal format:
   *       0 => $key,
   *       1 => $key2,
   *       2 => $key3,
   *
   * @param array $granularity
   *   The granularity array to analyze, all possible keys will
   *   be returned if left empty.
   */
  function granularity_keys($granularity = array()) {
    $keys = array_keys($this->date_parts());
    $labels = array_values($this->date_parts());
    $values = array();
    foreach ((array) $granularity as $key => $value) {
      if (is_numeric($key) && in_array($value, $keys)) {
        $values[] = $value;
      }
      elseif (in_array($key, $keys) && $value !== 0) {
        $values[] = $key;
      }
    }
    return !empty($values) ? $values : $keys;
  }

  /**
   * A form element to select the granularity.
   */
  function granularity_form($granularity) {
    $form = array(
      '#title' => t('Granularity'),
      '#type' => 'checkboxes',
      '#default_value' => $this->granularity_keys($granularity),
      '#options' => $this->date_parts(),
     );
     return $form;
  }

  /**
   * Helper function to create cross-database SQL dates.
   *
   * @param $field
   *   The real table and field name, like 'tablename.fieldname' .
   * @param $set_offset
   *   The name of a field that holds the timezone offset or a fixed timezone
   *   offset value. If not provided, the normal Drupal timezone handling
   *   will be used, i.e. $set_offset = 0 will make no timezone adjustment.
   * @return
   *   An appropriate SQL string for the db type and field type.
   */
  function sql_field($field, $set_offset = NULL) {
    if ($field == 'NOW()') {
      return $field;
    }
    $offset = $set_offset !== NULL ? $set_offset : views_get_timezone();
    switch ($this->db_type) {
      case 'mysql':
      case 'mysqli':
        switch ($this->date_type) {
          case 'int':
            $field = "FROM_UNIXTIME($field)";
            break;
          case 'iso':
            $field = "STR_TO_DATE($field, '%Y-%m-%%dT%T')";
            break;
          case 'datetime':
            break;
        }
        if (!empty($offset)) {
          $field = "($field + INTERVAL $offset SECOND)";
        }
        return $field;
      case 'pgsql':
        switch ($this->date_type) {
          case 'int':
            $field = "$field::ABSTIME";
            break;
          case 'iso':
            $field = "TO_DATE($field, 'FMYYYY-FMMM-FMDDTFMHH:FMMI:FMSS')";
            break;
          case 'datetime':
            break;
        }
        if (!empty($offset)) {
          $field = "($field + 'INTERVAL $offset SECONDS')";
        }
        return $field;
    }
  }

  /**
   * Helper function to create cross-database SQL date formatting.
   *
   * @param $format
   *   A format string for the result, like 'Y-m-d H:i:s' .
   * @param $field
   *   The real table and field name, like 'tablename.fieldname' .
   * @param $set_offset
   *   The name of a field that holds the timezone offset or a fixed timezone
   *   offset value. If not provided, the normal Drupal timezone handling
   *   will be used, i.e. $set_offset = 0 will make no timezone adjustment.
   * @return
   *   An appropriate SQL string for the db type and field type.
   */
  function sql_format($format, $field, $set_offset = NULL) {
    $field = $this->sql_field($field, $set_offset);
    switch ($this->db_type) {
      case 'mysql':
      case 'mysqli':
        $replace = array(
          'Y' => '%Y',
          'm' => '%m',
          'd' => '%%d',
          'H' => '%H',
          'i' => '%i',
          's' => '%s',
          );
        $format = strtr($format, $replace);
        return "DATE_FORMAT($field, '$format')";
      case 'pgsql':
        $replace = array(
          'Y' => 'YY',
          'm' => 'MM',
          'd' => 'DD',
          'H' => 'HH24',
          'i' => 'MI',
          's' => 'SS',
          );
        $format = strtr($format, $replace);
        return "TO_CHAR($field, '$format')";
    }
  }

  /**
   * Helper function to create cross-database SQL date extraction.
   *
   * @param $extract_type
   *   The type of value to extract from the date, like 'MONTH' .
   * @param $field
   *   The real table and field name, like 'tablename.fieldname' .
   * @param $set_offset
   *   The name of a field that holds the timezone offset or a fixed timezone
   *   offset value. If not provided, the normal Drupal timezone handling
   *   will be used, i.e. $set_offset = 0 will make no timezone adjustment.
   * @return
   *   An appropriate SQL string for the db type and field type.
   */
  function sql_extract($extract_type, $field, $set_offset = NULL) {
    $field = $this->sql_field($field, $set_offset);

    // Note there is no space after FROM to avoid db_rewrite problems
    // see http://drupal.org/node/79904.
    switch ($extract_type) {
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
      switch ($this->db_type) {
        case ('mysql'):
        case ('mysqli'):
          // WEEK using arg 3 in mysql should return the same value as postgres EXTRACT
          return "WEEK($field, 3)";
        case ('pgsql'):
          return "EXTRACT(WEEK FROM($field))";
      }
    case ('DOW'):
      switch ($this->db_type) {
        case ('mysql'):
        case ('mysqli'):
          // mysql returns 1 for Sunday through 7 for Saturday
          // php date functions and postgres use 0 for Sunday and 6 for Saturday
          return "INTEGER(DAYOFWEEK($field) - 1)";
        case ('pgsql'):
          return "EXTRACT(DOW FROM($field))";
      }
    case ('DOY'):
      switch ($this->db_type) {
        case ('mysql'):
        case ('mysqli'):
          return "DAYOFYEAR($field)";
        case ('pgsql'):
          return "EXTRACT(DOY FROM($field))";
      }
    }
  }
}

/**
 * A flexible, configurable date filter.
 *
 * This filter allows you to select one or more date parts to filter on,
 * such as year, month, and day; month only; a complete date, etc.
 *
 * Each part can be set to blank to show all values; 'now' to filter for
 * the current value of that part, or a specific value.
 *
 * An adjustment field is provided that will adjust the selected filter
 * value by something like '+90 days' or '-1 month';
 */
class views_handler_filter_custom_date extends views_handler_filter_numeric {
  var $date_handler = NULL;

  // Add a date handler to the filter.
  function construct() {
    parent::construct();
    $this->date_handler = new views_date_handler();
    $this->date_handler->construct();
    if (isset($this->definition['date_type'])) {
      $this->date_handler->date_type = $this->definition['date_type'];
    }
  }

  function init(&$view, $options) {
    parent::init(&$view, $options);
    $handler = $this->date_handler;
    $handler->granularity = $handler->granularity_keys($options['granularity']);
  }

  // Set default values for the date filter.
  function options(&$options) {
    parent::options($options);
    $options['granularity'] = $this->date_handler->granularity_keys();

    // We use different values than the parent form, so we must
    // construct our own value options.
    $options['value'] = array();
    foreach (array('value', 'min', 'max') as $prefix) {
      foreach ($this->date_handler->date_parts() as $key => $part) {
        $options['value'][$prefix . $key] = '';
      }
    }
  }

  /**
   * Set the granularity of the date parts to use in the filter.
    */
  function has_extra_options() { return TRUE; }

  function extra_options_form(&$form, &$form_state) {
    $form['granularity'] = $this->date_handler->granularity_form($this->options['granularity']);
    $form['granularity']['#description'] = t('Limit the filter to use only the selected date parts.');
  }

  function extra_options_validate($form, &$form_state) {
    $values = $form_state['values'];
    $handler = $this->date_handler;
    form_set_value($form['granularity'], $handler->granularity_keys($values['granularity']), $form_state);
  }

  /**
   * Add the selectors to the value form using the date handler.
   */
  function value_form(&$form, &$form_state) {
    // We use different values than the parent form, so we must
    // construct our own form element.
    $form['value'] = array();
    $form['value']['#tree'] = TRUE;
    $which = 'all';
    if (!empty($form['operator'])) {
      $source = ($form['operator']['#type'] == 'radios') ? 'radio:options[operator]' : 'edit-options-operator';
    }

    if (!empty($form_state['exposed'])) {
      if (empty($this->options['expose']['operator'])) {
        // exposed and locked.
        $which = in_array($this->operator, $this->operator_values(2)) ? 'minmax' : 'value';
      }
      else {
        $source = 'edit-' . form_clean_id($this->options['expose']['operator']);
      }
    }

    $handler = $this->date_handler;
    if ($which == 'all' || $which == 'value') {
      $form['value'] += $this->date_parts_form('value', $source, $which, $this->operator_values(1));
    }

    if ($which == 'all' || $which == 'minmax') {
      $form['value'] += $this->date_parts_form('min', $source, $which, $this->operator_values(2));
      $form['value'] += $this->date_parts_form('max', $source, $which, $this->operator_values(2));
    }
    $form['value']['description'] = array(
      '#prefix' => '<div class=""><div class="form-item"><div class="description">',
      '#suffix' => '</div></div></div>',
      '#value' => t('Blank values do no filtering, \'now\' filters for the current value.'),
      );
    if (in_array('adjustment', $handler->granularity)) {
      $form['value']['description']['#value'] .= t('\'Adjustment\' adds a value like \'+1 day\' to the other values.');
    }

  }

  /**
   * The first date part used in this instance, to know when to start
   * a new sub-grouping.
   */
  function first_part() {
    $parts = (array) $this->date_handler->granularity;
    return array_shift($parts);
  }

  /**
   * The last date part used in this instance, to know when to end
   * a sub-grouping.
   */
  function last_part() {
    $parts = (array) $this->date_handler->granularity;
    return array_pop($parts);
  }

  /**
   * A form element to select date part values.
   *
   * @param string $prefix
   *   A prefix for the date values, 'value', 'min', or 'max' .
   * @param string $source
   *   The operator for this element.
   * @param string $which
   *   Which element to provide, 'all', 'value', or 'minmax' .
   * @param array $operator_values
   *   An array of the allowed operators for this element.
   * @param array $limit
   *   An array of date parts to limit this element to.
   *
   * @return
   *   The form date part element for this instance.
   */
  function date_parts_form($prefix, $source, $which, $operator_values) {
    $prefixname = $prefix == 'value' ? '' : ($prefix == 'min' ? t('From') : t('To'));
    $handler = $this->date_handler;
    $min = $handler->part_min();
    $max = $handler->part_max();
    $limit = $handler->granularity;
    foreach ($handler->date_parts($limit) as $key => $name) {
      $options = array('' => '', 'now' => 'now');
      $type = 'select';
      if ($key == 'year' || $key == 'adjustment') {
        $type = 'textfield';
      }
      $form[$prefix . $key] = array(
        '#title' => t('@value', array('@type' => $prefixname, '@value' => $name)),
        '#type' => $type,
        '#size' => $key == 'adjustment' ? 20 : ($key == 'year' ? 6 : 1),
        //'#options' => $options,
        '#default_value' => !empty($this->value[$prefix . $key]) ? $this->value[$prefix . $key] : '',
        '#prefix' => '<div class="views-exposed-date-filter">',
        '#suffix' => '</div>',
      );
      switch ($key) {
        case 'year':
        case 'adjustment':
          break;
        case 'month':
          $form[$prefix . $key]['#options'] = $options + drupal_map_assoc(range(1, 12), 'map_month');
          break;
        default:
          $form[$prefix . $key]['#options'] = $options + drupal_map_assoc(range($min[$key], $max[$key]));
          break;
      }
      if ($type == 'textfield') {
        unset($form[$prefix . $key]['#options']);
      }
      if ($which == 'all') {
        $dependency = array(
          '#process' => array('views_process_dependency'),
          '#dependency' => array($source => $operator_values),
        );
        $form[$prefix . $key] += $dependency;
      }
      // Add wrappers to force each date grouping to a separate line.
      if ($key == $this->first_part()) {
        $form[$prefix . $key]['#prefix'] = '<div class="clear-block"><div class="views-left-75">' .
          $form[$prefix . $key]['#prefix'];
      }
      if ($key == $this->last_part()) {
        $form[$prefix . $key]['#suffix'] .= '</div></div>';
      }
    }
    return $form;
  }

  // User the date handler to validate the form.
  function options_validate(&$form, &$form_state) {
    if (!isset($form_state['values']['options']['value'])) {
      return;
    }
    $handler = $this->date_handler;
    $min = $handler->part_min();
    $max = $handler->part_max();
    $values = $form_state['values']['options']['value'];

    // Validate date values.
    unset($values['offset']);
    foreach ($values as $name => $value) {
      $pos = 3;
      if (substr($name, 0, 5) == 'value') {
        $pos = 5;
      }
      $part = substr($name, $pos, strlen($name));
      if (!empty($part) && $value != '' && $value != 'now' &&
        ($value < $min[$part] || $value > $max[$part])) {
        form_error($form['value'][$name], t('@value is invalid.', array('@value' => $parts[$part])));
      }
    }
  }

  // Update the summary values to provide
  // meaningful information for each option.
  function admin_summary() {
    $handler = $this->date_handler;
    $output = check_plain($this->operator) . ' ';

    // If the filter is exposed, display the granularity.
    if ($this->options['exposed']) {
      return t('<strong>Exposed</strong> @format', array('@format' => implode(', ', ($handler->date_parts($handler->granularity)))));
    }
    // If the filter is not exposed, display the selected values.
    // Check both empty and is_numeric to show all non-blank values,
    // including zero values.
    $min = array();
    $max = array();
    $handler = $this->date_handler;
    if (in_array($this->operator, $this->operator_values(2))) {
      foreach ($handler->date_parts($this->value['granularity']) as $key => $part) {
        if (!empty($this->value['min' . $key]) || !empty($this->value['max' . $key])
        || is_numeric($this->value['min' . $key]) || is_numeric($this->value['max' . $key])) {
          $min[] = $part . '=' . check_plain($this->value['min' . $key]);
          $max[] = $part . '=' . check_plain($this->value['max' . $key]);
        }
      }
      $output .= t('@min and @max', array('@min' => implode(': ', $min), '@max' => implode(': ', $max)));
    }
    else {
      foreach ($handler->date_parts($handler->granularity) as $key => $part) {
        if (!empty($this->value['value' . $key]) || is_numeric($this->value['value' . $key])) {
          $min[]= $part . '=' . check_plain($this->value['value' . $key]);
        }
      }
      $output .= implode(': ', $min);
    }
    return $output;
  }

  function op_between($field) {
    $value = $this->date_filter('min', $field, '>=');
    $value = $this->date_filter('max', $field, '<=');
    return;
  }

  function op_simple($field) {
    $value = $this->date_filter('value', $field, $this->operator);
    return;
  }

  function date_filter($prefix, $field, $operator) {
    $handler = $this->date_handler;
    $granularity = $handler->granularity;
    $parts = $handler->date_parts();
    unset($parts['adjustment']);
    if (!empty($this->value[$prefix . 'adjustment'])) {
      $adjustment = views_get_timezone() + strtotime($this->value[$prefix . 'adjustment']) - REQUEST_TIME;
    }
    foreach ($parts as $key => $label) {
      if (!empty($adjustment)) {
          $extract = $handler->sql_extract(strtoupper($key), $field, $adjustment);
        }
        else {
          $extract = $handler->sql_extract(strtoupper($key), $field);
      }
      if (!in_array($key, $granularity) || empty($this->value[$prefix . $key])) {
        // Skip this value
      }
      elseif ($this->value[$prefix . $key] == 'now') {
        $value = $handler->sql_extract(strtoupper($key), "NOW()", 0);
        $this->query->add_where($this->options['group'], $extract . " $operator %s", $value);
      }
      else {
        $value = $this->value[$prefix . $key];
        $this->query->add_where($this->options['group'], $extract . " $operator %s", $value);
      }
    }
  }
}

/**
 * A flexible, configurable date argument.
 *
 * This argument allows you to set one or more date parts to filter on,
 * such as year, month, and day; month only; a complete date, etc.
 *
 * @ingroup views_argument_handlers
 */
class views_handler_argument_date extends views_handler_argument_formula {
  var $date_handler = NULL;

  /**
   * Add date handler to the argument.
   */
  function construct() {
    parent::construct();
    $this->date_handler = new views_date_handler();
    $this->date_handler->construct();
    if (isset($this->definition['date_type'])) {
      $this->date_handler->date_type = $this->definition['date_type'];
    }
  }

  /**
   * Get granularity and use it to create the formula and a format
   * for the results.
   */
  function init(&$view, $options) {
    parent::init(&$view, $options);
    $handler = $this->date_handler;
    $handler->granularity = $handler->granularity_keys($options['granularity']);
    $sql_format = $handler->limit_format('Y-m-d\TH:i:s', $handler->granularity);
    $this->formula = $handler->sql_format($sql_format, "***table***.$this->real_field");
    $this->format = $handler->limit_format(variable_get('date_format_long', 'l, F j, Y - H:i'), $handler->granularity);
  }

  /**
   * Default value for the granularity option.
   */
  function options(&$options) {
    parent::options($options);
    $parts = $this->date_handler->date_parts();
    unset($parts['adjustment']);
    $options['granularity'] = $this->date_handler->granularity_keys($parts);
  }

  /**
   * Add a form element to select granularity.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    // Select the granularity of the date parts to use in the argument.
    $handler = $this->date_handler;
    $form['granularity'] = $handler->granularity_form($this->options['granularity']);
    $form['granularity']['#description'] = t('Limit the argument to use only the selected date parts.');
    unset($form['granularity']['#options']['adjustment']);
  }

  /**
   * Re-work the form values array into a proper granularity array.
   */
  function options_validate($form, &$form_state) {
    $values = $form_state['values'];
    $handler = $this->date_handler;
    form_set_value($form['granularity'], $handler->granularity_keys($values['options']['granularity']), $form_state);
  }

  // Update the summary values to show selected granularity.
  function admin_summary() {
    $handler = $this->date_handler;
    $format = $handler->format_help($handler->limit_format('Y-m-d\TH:i:s', $handler->granularity));
    return t('<br />Argument format: @format', array('@format' => $format));
  }

  /**
   * Provide a link to the next level of the view
   */
  function summary_name($data) {
    $created = $data->{$this->name_alias};
    return format_date(strtotime($created), 'custom', $this->format, 0);
  }

  /**
   * Provide a link to the next level of the view
   */
  function title() {
    return format_date(strtotime($this->argument), 'custom', $this->format, 0);
  }

  /**
   * Create a summary query that matches the granularity.
   *
   * Needed or Views will do a groupby on the complete date instead
   * of only the part of the date actually used in the argument.
   */
  function summary_query() {
    $this->ensure_my_table();
    $handler = $this->date_handler;

    // We need to alter the base alias so the groupby clause will group by
    // the relevant part of the date instead of a complete date.
    $alias = $this->name_alias = $this->table_alias . "_" . $this->real_field;
    $this->query->add_field(NULL, $this->get_formula(), $alias);
    $this->base_alias = $this->get_formula();

    return $this->summary_basics();
  }

  /**
   * Need to override the basic link since base_alias is now a formula.
   */
  function summary_link($data, $url) {
    $value = $data->{$this->name_alias};
    return url("$url/$value");
  }

  /**
   * Create a query that matches the argument.
   *
   * Move through the arg and pick out date values to add to the query.
   */
  function query() {
    $this->ensure_my_table();
    $handler = $this->date_handler;
    $granularity = $handler->granularity;
    $parts = $handler->date_parts();
    unset($parts['adjustment']);
    $i = 0;
    foreach ($parts as $key => $label) {
      if (!in_array($key, $granularity)) {
        // Skip values not in the requested granularity.
      }
      else {
        // Find the next date part in the arg to evaluate
        $arg = intval(substr($this->argument, $i, $key == 'year' ? 4 : 2));
        $extract = $handler->sql_extract(strtoupper($key), "$this->table_alias.$this->real_field");
        $this->query->add_where(0, $extract . " = %s", $arg);
      }
      $i += $key == 'year' ? 5 : 3;
    }
  }

}