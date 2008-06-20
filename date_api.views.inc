<?php
//$Id$
/**
 * @file
 * Defines date-related Views data and plugins:
 * 
 * Date argument:
 *   A generic date argument that has an option to select one or more 
 *   Views date fields to filter on, automatically adds them to the view, 
 *   and then filters the view by the value of the selected field(s). 
 *   The flexible argument will accept and evaluate most ISO date
 *   and period formats, like 2009-05-01, 2008-W25, P1W. 
 * 
 * Date filter:
 *   A generic date argument that has an option to select a 
 *   Views date field to filter on, and provides a place to select
 *   a value and an adjustment to use for the filtering. The value
 *   can be a date or 'now', and the adjustment can be something like
 *   '+90 days'. 
 * 
 * Current date argument default
 *   Adds a default option to set the argument to the current date 
 *   when the argument is empty.
 * 
 * Date navigation attachment
 *   Navigation that can be attached to any display to create back/next 
 *   links by date, requires the date argument and uses the current
 *   date argument default to set a starting point for the view.
 */
//views_include_handlers();
/**
 * Implementation of hook_views_data()
 */
function date_api_views_data() {
  $data = array();
  
  // The flexible date argument.
  $data['node']['date_argument'] = array(
    'group' => t('Date'),
    'title' => t('Date'),
    'help' => t('Filter any Views date field by a date argument, using any common ISO date/period format (i.e. YYYY, YYYY-MM, YYYY-MM-DD, YYYY-W99, YYYY-MM-DD--P3M, P90D, etc).'),
    'argument' => array(
      'handler' => 'date_api_argument_handler',
      'empty name field' => t('Undated'),
    ),
  );  
  // The flexible date fliter.
  $data['node']['date_filter'] = array(
    'group' => t('Date'),
    'title' => t('Date'),
    'help' => t('Filter any Views date field.'),
    'filter' => array(
      'handler' => 'date_api_filter_handler',
      'empty name field' => t('Undated'),
    ),
  );  
  return $data;
}

/**
 * Implementation of hook_views_plugins
 */
function date_api_views_plugins() {
  $path = drupal_get_path('module', 'date_api');
  $base = array(
    'file' => 'theme.inc',
    'path' => "$path/theme",
  );
  return array(
    'module' => 'date_api', // This just tells our themes are elsewhere.
    'display' => array(
      // Display plugin for date navigation.
      'date_nav' => $base + array(
        'title' => t('Date browser'),
        'help' => t('Date back/next navigation to attach to other displays. Requires the Date argument.'),
        'handler' => 'date_plugin_display_attachment',
        'theme' => 'views_view',
        'use ajax' => TRUE,
        'admin' => t('Date browser'),
        'help topic' => 'display-date_navigation',
      ),
    ),
    'style' => array(
      // Style plugin for the navigation display.
      'date_nav' => $base + array(
        'title' => t('Date browser style'),
        'help' => t('Creates back/next navigation.'),
        'handler' => 'date_navigation_plugin_style',
        'theme' => 'date_navigation',
        'uses row plugin' => FALSE,
        'uses fields' => FALSE,
        'uses options' => TRUE,
        'type' => 'date_nav', 
        'even empty' => TRUE,
      ),
    ),
  );
}

/**
 * Date API argument handler.
 */
class date_api_argument_handler extends views_handler_argument_date {
  function construct() {
    parent::construct();
    include_once( drupal_get_path('module', 'date_api') .'/date_api_sql.inc');
    $this->date_handler = new date_sql_handler();
    $this->date_handler->construct();
  }

  /**
   * Get granularity and use it to create the formula and a format
   * for the results.
   */
  function init(&$view, $options) {
    parent::init($view, $options);
    $date_handler = $this->date_handler;
    $date_handler->granularity = $this->options['granularity'];
    $this->format = $date_handler->views_formats($date_handler->granularity, 'display');
    $this->sql_format = $date_handler->views_formats($date_handler->granularity, 'sql');
  }

  /**
   * Default value for the date_fields option.
   */
  function options(&$options) {
    parent::options($options);
    $options['date_fields'] = array();
    $options['date_method'] = 'OR';
    $options['granularity'] = 'month';
  }

  /**
   * Add a form element to select date_fields for this argument.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $options = $this->date_handler->date_parts();
    unset($options['second'], $options['minute']);
    $options += array('week' => t('Week'));
    $form['granularity'] = array(
      '#title' => t('Granularity'),
      '#type' => 'radios',
      '#options' => $options,
      '#default_value' => $this->options['granularity'],
      '#multiple' => TRUE,
      '#description' => t('Select the type of date value to be used in defaults, summaries, and navigation. For example, a granularity of \'month\' will set the default date to the current month, summarize by month in summary views, and link to the next and previous month when using date navigation.'),
    );

    $fields = date_api_fields();
    $options = array();
    foreach ($fields['name'] as $name => $field) {
      $options[$name] = $field['label'];
    }
    $form['date_fields'] = array(
      '#title' => t('Date field(s)'),
      '#type' => 'checkboxes',
      '#options' => $options,
      '#default_value' => $this->options['date_fields'],
      '#multiple' => TRUE,
      '#description' => t('Select one or more date fields to filter with this argument.'),
    );
    $form['date_method'] = array(
      '#title' => t('Method'),
      '#type' => 'radios',
      '#options' => array('OR' => t('OR'), 'AND' => t('AND')),
      '#default_value' => $this->options['date_method'],
      '#description' => t('Method of handling multiple date fields in the same query. Return items that have any matching date field (date = field_1 OR field_2), or only those with matches in all selected date fields (date = field_1 AND field_2).'),
      );
    
  }
    
  function options_validate($form, &$form_state) {
    if (empty($form_state['values']['options']['date_fields'])) {
      form_error($form, t('You must select at least one date field for this argument.'));
    }
  }
  
  function options_submit($form, &$form_state) {
    $form_state['values']['options']['date_fields'] = array_filter($form_state['values']['options']['date_fields']);
  }
  
  // Update the summary values to show selected granularity.
  function admin_summary() {
    if (!empty($this->options['date_fields'])) {
      return ' ('. implode(', ', $this->options['date_fields']) .')';
    }
    else {
      return parent::admin_summary();
    }
  }

  /**
   * Set the empty argument value to the current date, 
   * formatted appropriately for this argument.
   */
  function get_default_argument($raw = FALSE) {
    if (!$raw && $this->options['default_argument_type'] == 'date') {
      return date($this->format(), time());
    }
    else {
      return parent::get_default_argument($raw);
    }
  }
    
  function format() {
    if (!empty($this->options['granularity'])) {
      $date_handler = new date_sql_handler();
      return $date_handler->views_formats($this->options['granularity']);
    }
    else {
      return !empty($this->options[$this->option_name]) ? $this->options[$this->option_name] : 'Y-m';
    }
  }
    
  /**
   * Provide a link to the next level of the view from the summary.
   */
  function summary_name($data) {
    $format = $this->date_handler->views_formats($this->options['granularity'], 'display');
    $created = $data->{$this->name_alias};
    return format_date(strtotime($created), 'custom', $format, 0);
  }

  /**
   * Need to override the basic link since base_alias is now a formula.
   */
  function summary_link($data, $url) {
    $value = $data->{$this->name_alias};
    return url("$url/$value");
  }

  /**
   * Provide a link to the next level of the view from the argument.
   */
  function title() {
    $format = $this->date_handler->views_formats($this->options['granularity'], 'display');
    return format_date(strtotime($this->argument), 'custom', $format, 0);
  }

  /**
   * Create a summary query that matches the granularity.
   *
   * Needed or Views will do a groupby on the complete date instead
   * of only the part of the date actually used in the argument.
   */
  function summary_query() {
    $this->get_query_fields();
    
    // No way to do summaries on more than one field at a time.
    if (count($this->query_fields) > 1) {
      return;
    }
    
    $field = $this->query_fields[0]['field'];
    $date_handler = $this->query_fields[0]['date_handler'];
      
    // Get the SQL format for this granularity, like Y-m,
    // and use that as the grouping value.
    $format = $date_handler->views_formats($this->options['granularity'], 'sql');
    $this->formula = $date_handler->sql_format($format, $date_handler->sql_field($field['fullname']));
    
    $this->ensure_my_table();
    
    // Make sure this field is added to the query so we have all necessary tables.
    $this->query->add_field($field['table_name'], $field['field_name']);
      
    // Add the computed field.
    $this->base_alias = $this->name_alias = $this->query->add_field(NULL, $this->formula, $field['queryname']);
    $this->query->set_count_field(NULL, $this->formula, $field['queryname']);    

    return $this->summary_basics(FALSE);
  }

  function get_query_fields() {
    $fields = date_api_fields();
    $fields = $fields['name'];
    $min_date = isset($this->min_date) ? $this->min_date : NULL;
    $min_utc = isset($this->min_utc) ? $this->min_utc : NULL;
    $max_date = isset($this->max_date) ? $this->max_date : NULL;
    $max_utc = isset($this->max_utc) ? $this->max_utc : NULL;
    $this->query_fields = array();
    foreach ($this->options['date_fields'] as $delta => $name) {
      if (array_key_exists($name, $fields) && $field = $fields[$name]) {
        $date_handler = new date_sql_handler();
        $date_handler->construct($field['sql_type'], date_default_timezone_name());
        $date_handler->granularity = $this->options['granularity'];
        $tz_handling  = $field['tz_handling'];
        switch ($tz_handling) {
          case 'date' :
            $date_handler->db_timezone = 'UTC';
            $date_handler->local_timezone_field = $field['timezone_field'];
            $date_handler->local_offset_field = $field['offset_field'];
            $date_handler->min_date = $min_date;
            $date_handler->max_date = $max_date;
            break;
          case 'none':
            $date_handler->db_timezone = date_default_timezone_name();
            $date_handler->local_timezone = date_default_timezone_name();
            $date_handler->min_date = $min_date;
            $date_handler->max_date = $max_date;
            break;
          case 'utc':
            $date_handler->db_timezone = 'UTC';
            $date_handler->local_timezone = 'UTC';
            $date_handler->min_date = $min_utc;
            $date_handler->max_date = $max_utc;
            break;
          default :
            $date_handler->db_timezone = 'UTC';
            $date_handler->local_timezone = 'UTC';
            $date_handler->min_date = $min_utc;
            $date_handler->max_date = $max_utc;
            break;
        }
        $this->query_fields[] = array('field' => $field, 'date_handler' => $date_handler);
      }
    }
  }
 
  /**
   * Set up the query for this argument.
   *
   * The argument sent may be found at $this->argument.
   */
  function query() { 
    $parts = $this->date_handler->arg_parts($this->argument);
    foreach ($parts[0]['date'] as $key => $part) {
      // The last part evaluated is the one that will 'stick'
      // as the date type.
      $this->granularity = $key;
      $this->$key = $part;
    }
    $range = $this->date_handler->arg_range($this->argument);
    $min_date = $range[0];
    $max_date = $range[1];
    
    // Create min and max dates in both local and UTC time.
    // We'll compare fields to the UTC date whenever possible
    // to avoid the need to do timezone conversions. When that
    // isn't possible (the date is not stored in UTC or needs to
    // be converted back to a time that may be different than
    // the local timezone) we will have to do tz conversions in
    // the database.
    $this->min_date = $min_date;
    $this->min_utc = drupal_clone($min_date);
    date_timezone_set($this->min_utc, timezone_open('UTC'));
    $this->max_date = $max_date;
    $this->max_utc = drupal_clone($max_date);
    date_timezone_set($this->max_utc, timezone_open('UTC'));
        
    $this->get_query_fields();
    if (!empty($this->query_fields)) {
      // Use set_where_group() with the selected date_method 
      // of 'AND' or 'OR' to create the where clause.
      $this->query->set_where_group($this->options['date_method'], 'date');
      $this->ensure_my_table();
      foreach ($this->query_fields as $query_field) {
        $field = $query_field['field'];
        $date_handler = $query_field['date_handler'];
      
        // Make sure this field is added to the query.
        $this->query->add_field($field['table_name'], $field['field_name']);
        foreach ($field['related_fields'] as $related) {
          $bits = explode('.', $related);
          if ($bits[1] != $field['field_name']) {
            $this->query->add_field($field['table_name'], $bits[1]);
          }
        }
        $from = $date_handler->sql_where_date('DATE', $field['fullname'], '>=', date_format($date_handler->min_date, DATE_FORMAT_DATETIME));
        $to   = $date_handler->sql_where_date('DATE', $field['fullname'], '<=', date_format($date_handler->max_date, DATE_FORMAT_DATETIME));
        $sql = str_replace('***table***', $this->table_alias, "($from AND $to)");
        if ($sql) {
          $this->query->add_where('date', $sql);
        }
      }
    }
  }
}
  
/**
 * The plugin that handles date navigation attachments.
 * 
 * Creates a special attachment for this purpose only. 
 */
class date_plugin_display_attachment extends views_plugin_display_attachment {
  
  // Require the date_nav style. That style has a date_nav type
  // so it won't show up as a style option on any other display.
  function get_style_type() { return 'date_nav'; }
  
  // No options to set style, force it to the right value.
  function defaultable_sections($section = NULL) {
    if (in_array($section, array('row_options', 'row_plugin', 'items_per_page'))) {
      return FALSE;
    }
    return parent::defaultable_sections($section);
  }

  function options(&$display) {
    parent::options($display);
    $display->display_options['style_plugin'] = 'date_nav';
    $display->display_options['items_per_page'] = 0;
    $display->display_options['row_plugin'] = '';
    //$display->display_options['defaults']['style_plugin'] = FALSE;
    $display->display_options['defaults']['style_options'] = FALSE;
    $display->display_options['defaults']['items_per_page'] = FALSE;
    $display->display_options['defaults']['row_plugin'] = FALSE;
    $display->display_options['defaults']['row_options'] = FALSE;
  } 
}

/**
 * Style plugin to create date back/next navigation.
 * 
 * The style plugin passes some argument values to the theme, and
 * ensures that the date argument is present and that the default
 * value is set to the current date.
 */
class date_navigation_plugin_style extends views_plugin_style {
  
  /**
   * Style validation.
   */
  function validate() {
    $errors = parent::validate();
    
    $arguments = $this->display->handler->get_option('arguments');
    if (!in_array('date_argument', array_keys($arguments))) {
      $errors[] = t('The @style requires the Calendar: Date argument.', array('@style' => $this->definition['title']));      
    }
    else {
      if ($arguments['date_argument']['default_argument_type'] != 'date') {
        $errors[] = t('The @style requires the Calendar: Date argument to provide a default argument set to default to the current date.', array('@style' => $this->definition['title']));      
      }
    }
    return $errors;
  }
  
  function query() {
    include_once(drupal_get_path('module', 'date_api') .'/date_api_sql.inc');
        
    // Bring the argument information into the view so our theme can access it.
    $i = 0;    
    foreach ($this->view->argument as $id => $argument) {
      if ($id == 'date_argument') {
        $this->view->granularity = $argument->granularity;
        $this->view->date_arg = $argument->argument;
        $this->view->date_arg_pos = $i;
        $this->view->year = isset($argument->year) ? $argument->year : NULL;
        $this->view->month = isset($argument->month) ? $argument->month: NULL;
        $this->view->day = isset($argument->day) ? $argument->day : NULL;
        $this->view->week = isset($argument->week) ? $argument->week : NULL;
        $this->view->min_date = $argument->min_date;
        $this->view->max_date = $argument->max_date;
      }
      $i++;
    }
     
    // bring the node type into the query so we can use it in the theme
    $this->view->query->add_field('node', 'type');
    parent::query();
  }
  
  /**
   * Render the calendar navigation style.
   */
  function render() {
    return theme($this->theme_functions(), $this->view, $this->options, array());
  }
}

/**
 * A flexible, configurable date filter.
 *
 * This filter allows you to select a granularity of date parts to filter on,
 * such as year, month, day, etc.
 *
 * Each part can be set to blank to show all values; 'now' to filter for
 * the current value of that part, or a specific value.
 *
 * An adjustment field is provided that will adjust the selected filter
 * value by something like '+90 days' or '-1 month';
 */
class date_api_filter_handler extends views_handler_filter_numeric {
  var $date_handler = NULL;

  // Add a date handler to the filter.
  function construct() {
    parent::construct();
    include_once( drupal_get_path('module', 'date_api') .'/date_api_sql.inc');
    $this->date_handler = new date_sql_handler();
    $this->date_handler->construct();
    $this->date_handler->granularity = $this->options['granularity'];
  }

  function init(&$view, $options) {
    parent::init($view, $options);
    $handler = $this->date_handler;
    $handler->granularity = isset($options['granularity']) ? $options['granularity'] : 'day';
    $handler->adjustment_field = isset($options['adjustment_field']) ? $options['adjustment_field'] : 0;
  }

  // Set default values for the date filter.
  function options(&$options) {
    parent::options($options);
    $options['date_fields'] = array();
    $options['granularity'] = 'day';
    $options['adjustment_field'] = 0;

    // We use different values than the parent form, so we must
    // construct our own value options.
    $options['value'] = array();
    foreach (array('value', 'min', 'max') as $prefix) {
      foreach (array('date', 'adjustment') as $key => $part) {
        $options['value'][$prefix . $key] = '';
      }
    }
  }

  /**
   * Set the granularity of the date parts to use in the filter.
    */
  function has_extra_options() { return TRUE; }

  function extra_options_form(&$form, &$form_state) {
    $form['adjustment_field'] = array(
      '#type' => 'radios',
      '#title' => t('Filter type'),
      '#default_value' => $this->options['adjustment_field'],
      '#options' => array(
        0 => t('Date only'), 
        1 => t('Both date and adjustment'), 
        2 => t('Adjustment only'),
        ),
      '#description' => t('Choose a date to filter on, or use an adjustment field for a value like \'+1 day\'. When you use both date and adjustment, the adjustment will be added to the date. When the adjustment field is used with no date field, the adjustment will be made to the current date. The date can be a specific date, or \'now\'.'),
      );
    $form['granularity'] = $this->date_handler->granularity_form($this->options['granularity']);
    $form['granularity']['#description'] = '<p>'. t('Select a granularity for the date filter. For instance, selecting \'day\' will create a filter where you can select the year, month, and day as YYYY-MM-DD.') .'</p>';
    if (!$this->date_handler->db_tz_support()) {
      $form['granularity']['#description'] .= '<p>'. t('This database does not appear to have native timezone support. Filtering using hour, minute, or second granularity is likely to return incorrect results at least some of the time on systems without native timezone support, so it is recommended to set the granularity to no more than \'day\'.') .'</p>';
    }
    $fields = date_api_fields();
    $options = array();
    foreach ($fields['name'] as $name => $field) {
      $options[$name] = $field['label'];
    }
    $form['date_fields'] = array(
      '#title' => t('Date field(s)'),
      '#type' => 'radios',
      '#options' => $options,
      '#default_value' => $this->options['date_fields'],
      '#multiple' => FALSE,
      '#description' => t('Select date field(s) to filter with this argument.'),
    );
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
    $format = $handler->views_formats($handler->granularity, 'sql');
    $format_help = $handler->format_help($format);
    $form['value']['description'] = array(
      '#prefix' => '<div class=""><div class="form-item"><div class="description">',
      '#suffix' => '</div></div></div>',
      '#value' => t('Date format @format. Leave date blank to do no filtering, use \'now\' to filter for the current date.', array('@format' => $format_help)),
      );
    if ($this->options['adjustment_field'] == 1) {
      $form['value']['description']['#value'] .= t(' \'Adjustment\' filters for an offset like \'+1 day\' from the date value, most useful when used with \'now\'.');
    }
    elseif ($this->options['adjustment_field'] == 2) {
      $form['value']['description']['#value'] = t('\'Adjustment\' filters for an offset like \'+1 day\' from the current time.');
    }
  }

  function date_filter_parts() {
    switch ($this->options['adjustment_field']) {
      case 1:
        return array('date' => t('Date'), 'adjustment' => t('Adjustment'));
      case 2:
        return array('adjustment' => t('Adjustment'));
      default:
        return array('date' => t('Date'));
    }
  }
  
  /**
   * A form element to select date part values.
   *
   * @param string $prefix
   *   A prefix for the date values, 'value', 'min', or 'max'.
   * @param string $source
   *   The operator for this element.
   * @param string $which
   *   Which element to provide, 'all', 'value', or 'minmax'.
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
    $min = $handler->part_info('min');
    $max = $handler->part_info('max');
    $limit = $handler->granularity;
    switch ($this->options['adjustment_field']) {
      case 1:
        $parts = array('date' => t('Date'), 'adjustment' => t('Adjustment'));
        $first_item = 'date';
        $last_item = 'adjustment';
        break;
      case 2:
        $parts = array('adjustment' => t('Adjustment'));
        $first_item = 'adjustment';
        $last_item = 'adjustment';
        break;
      default:
        $parts = array('date' => t('Date'));
        $first_item = 'date';
        $last_item = 'date';
        break;
    }
    foreach ($parts as $key => $name) {
      $type = 'textfield';
      $form[$prefix . $key] = array(
        '#title' => t('@type @value', array('@type' => $prefixname, '@value' => $name)),
        '#type' => $type,
        '#size' => 20,
        '#default_value' => !empty($this->value[$prefix . $key]) ? $this->value[$prefix . $key] : '',
        '#prefix' => '<div class="views-exposed-date-filter">',
        '#suffix' => '</div>',
      );
      if ($which == 'all') {
        $dependency = array(
          '#process' => array('views_process_dependency'),
          '#dependency' => array($source => $operator_values),
        );
        $form[$prefix . $key] += $dependency;
      }
      // Add wrappers to force each date grouping to a separate line.
      if ($key == $first_item) {
        $form[$prefix . $key]['#prefix'] = '<div class="clear-block"><div class="views-left-75">' .
          $form[$prefix . $key]['#prefix'];
      }
      if ($key == $last_item) {
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
    $parts = $this->date_filter_parts();
    $min = $handler->part_info('min');
    $max = $handler->part_info('max');
    $values = $form_state['values']['options']['value'];
    // Validate date values.
    unset($values['offset']);
    foreach ($values as $name => $value) {
      $part = str_replace(array('min', 'max', 'value'), '', $value);
      if (!empty($part) && $value != '' && $value != 'now' &&
        ($value < $min[$part] || $value > $max[$part])) {
        form_error($form['value'][$name], t('@value is invalid.', array('@value' => $parts[$part])));
      }
    }
  }
  
  // Update the summary values to provide
  // meaningful information for each option.
  function admin_summary() {
    if (!empty($this->options['date_fields'])) {
      $field = $this->options['date_fields'];
    }
    
    $handler = $this->date_handler;
    $output = "($field) ". check_plain($this->operator) . ' ';
    $parts = $handler->date_parts();
    
    // If the filter is exposed, display the granularity.
    if ($this->options['exposed']) {
      return t('(@field) <strong>Exposed</strong> Granularity: @format', array('@field' => $field, '@format' => $parts[$handler->granularity]));
    }
    // If the filter is not exposed, display the selected values.
    // Check both empty and is_numeric to show all non-blank values,
    // including zero values.
    $min = '';
    $max = '';
    $handler = $this->date_handler;
    $separators = $handler->part_info('sep');
    if (in_array($this->operator, $this->operator_values(2))) {
      foreach ($this->date_filter_parts() as $key => $part) {
        if (!empty($this->value['min'. $key]) || !empty($this->value['max'. $key])
        || is_numeric($this->value['min'. $key]) || is_numeric($this->value['max'. $key])) {
          $min .= $separators[$key] . check_plain($this->value['min'. $key]);
          $max .= $separators[$key] . check_plain($this->value['max'. $key]);
        }
      }
      $output .= t('@min and @max', array('@min' => $min, '@max' => $max));
    }
    else {
      foreach ($this->date_filter_parts() as $key => $part) {
        if (!empty($this->value['value'. $key]) || is_numeric($this->value['value'. $key])) {
          $min .= $separators[$key] . check_plain($this->value['value'. $key]);
        }
      }
      $output .= $min;
    }
    return $output;
  }
 
  function op_between($field) {
    $this->date_filter('min', $field, '>=');
    $this->date_filter('max', $field, '<=');
    return;
  }

  function op_simple($field) {
    $this->date_filter('value', $field, $this->operator);
    return;
  }

  function date_filter($prefix, $field, $operator) {
    if (empty($this->value[$prefix .'adjustment']) && empty($this->value[$prefix .'date'])) {
      return;
    }
    $date_handler = $this->date_handler;
    $granularity = $this->options['granularity'];
    $filter_parts = $this->date_filter_parts();
    $adjustment = 0;
    if (!empty($this->value[$prefix .'adjustment'])) {
      $adjustment = strtotime($this->value[$prefix .'adjustment'], 0);
    }
    if ($this->value[$prefix .'date'] == 'now') {
      $date = date_now();
    }
    else {
      $selected = $date_handler->arg_parts($this->value[$prefix .'date']);
      $date = date_make_date($date_handler->complete_date($selected[0]['date']));
    }
    if (!empty($adjustment)) {
      date_modify($date, "+$adjustment seconds");
    }
    $this->format = $date_handler->views_formats($granularity, 'sql');
    $value = date_format($date, $this->format);
        
    $range = $this->date_handler->arg_range($value);
    $min_date = $range[0];
    $max_date = $range[1];
    
    // Create min and max dates in both local and UTC time.
    // We'll compare fields to the UTC date whenever possible
    // to avoid the need to do timezone conversions. 
    $this->min_date = $min_date;
    $this->min_utc = drupal_clone($min_date);
    date_timezone_set($this->min_utc, timezone_open('UTC'));
    $this->max_date = $max_date;
    $this->max_utc = drupal_clone($max_date);
    date_timezone_set($this->max_utc, timezone_open('UTC'));
    
    $this->get_query_fields();
    if (!empty($this->query_fields)) {
      // Use set_where_group() with the selected date_method 
      // of 'AND' or 'OR' to create the where clause.
      //$this->query->set_where_group($this->options['date_method'], 'date');
      $this->ensure_my_table();
      foreach ((array) $this->query_fields as $query_field) {
        $field = $query_field['field'];
        $date_handler = $query_field['date_handler'];
        $parts = $date_handler->date_parts();
    
        // Make sure this field is added to the query.
        $this->query->add_field($field['table_name'], $field['field_name']);
        foreach ($field['related_fields'] as $related) {
          $bits = explode('.', $related);
          if ($bits[1] != $field['field_name']) {
            $this->query->add_field($field['table_name'], $bits[1]);
          }
        }
        if (!empty($this->value[$prefix .'adjustment'])) {
          // See if there are any filters other than the adjustment,
          // if not, compare to NOW() and return.
          if ($this->options['adjustment_field'] == 2) {
            $sql = $date_handler->sql_field($field['fullname'], 0) . " $operator ". $date_handler->sql_field('NOW', $adjustment);
            $this->query->add_where($this->options['group'], $sql);
            continue;
          }
        }
        $value = $prefix == 'max' ? date_format($date_handler->max_date, $this->format) : date_format($date_handler->min_date, $this->format);
        $sql = $date_handler->sql_where_format($this->format, $field['fullname'], $operator, $value);
        $this->query->add_where($this->options['group'], $sql);
        continue;
      }
    }
  }
  
  function get_query_fields() {
    $fields = date_api_fields();
    $fields = $fields['name'];
    $min_date = isset($this->min_date) ? $this->min_date : NULL;
    $min_utc = isset($this->min_utc) ? $this->min_utc : NULL;
    $max_date = isset($this->max_date) ? $this->max_date : NULL;
    $max_utc = isset($this->max_utc) ? $this->max_utc : NULL;
    $this->query_fields = array();
    foreach ((array) $this->options['date_fields'] as $delta => $name) {
      if (array_key_exists($name, $fields) && $field = $fields[$name]) {
        $date_handler = new date_sql_handler();
        $date_handler->construct($field['sql_type'], date_default_timezone_name());
        $date_handler->granularity = $this->options['granularity'];
        
        $tz_handling  = $field['tz_handling'];
        switch ($tz_handling) {
          // Date-specific timezone fields must always be converted
          // before comparing them.
          case 'date' :
            $date_handler->db_timezone = 'UTC';
            $date_handler->local_timezone_field = $field['timezone_field'];
            $date_handler->local_offset_field = $field['offset_field'];
            $date_handler->min_date = $min_date;
            $date_handler->max_date = $max_date;
            break;
          // Dates with no timezones and dates that are always UTC
          // must never be converted before comparing them.  
          case 'none':
            $date_handler->db_timezone = date_default_timezone_name();
            $date_handler->local_timezone = date_default_timezone_name();
            $date_handler->min_date = $min_date;
            $date_handler->max_date = $max_date;
            break;
          case 'utc':
            $date_handler->db_timezone = 'UTC';
            $date_handler->local_timezone = 'UTC';
            $date_handler->min_date = $min_utc;
            $date_handler->max_date = $max_utc;
            break;
          // We're comparing partial dates in the filter, like
          // Y-m-d H, so we have to convert the db date to the local 
          // timezone to get the right results.  
          default:
            $date_handler->db_timezone = 'UTC';
            $date_handler->local_timezone = date_default_timezone_name(); 
            $date_handler->min_date = $min_date;
            $date_handler->max_date = $max_date;

        }
        $this->query_fields[] = array('field' => $field, 'date_handler' => $date_handler);
      }
    }
  }
}

/**
 *  Identify all potential date/timestamp fields and cache the data.
 */
function date_api_fields($base = 'node', $reset = FALSE) {
  static $fields;
  if (empty($fields[$base]) || $reset) {
    $cid = 'date_api_fields_'. $base;
    if (!$reset && $cached = cache_get($cid, 'cache_views')) {
      $fields[$base] = $cached->data;
    }
    else {
      $fields[$base] = _date_api_fields($base);
    }
  }
  return $fields[$base];
}

/**
 *  Identify all potential date/timestamp fields.
 *
 *  @return
 *   array with fieldname, type, and table
 */
function _date_api_fields($base = 'node') {
  $cid = 'date_api_fields_'. $base;
  cache_clear_all($cid, 'cache_views');
  
  $all_fields = date_api_views_fetch_fields($base, 'field');
  $fields = array();
  foreach ((array) $all_fields as $name => $val) {
    $fromto = array();
    $tmp = explode('.', $name);
    $field_name = $tmp[1];
    $table_name = $tmp[0];
    $alias = str_replace('.', '_', $name);
      
    $handler = views_get_handler($table_name, $field_name, 'field');
    $type = '';
    
    // For cck fields, get the date type.
    if (isset($handler->content_field)) {
      if ($handler->content_field['type'] == 'date') {
        $type = 'cck_string';
      }
      elseif ($handler->content_field['type'] == 'datestamp') {
        $type = 'cck_timestamp';
      }
      elseif ($handler->content_field['type'] == 'datetime') {
        $type = 'cck_datetime';
      }
    }
    
    // This is a core timestamp field.
    elseif (strstr($field_name, 'timestamp') || strstr($field_name, 'updated') 
      || strstr($field_name, 'created') || strstr($field_name, 'changed')) {
      $type = 'timestamp';
    }
    

    // Don't do anything if this is not a date field we can handle.
    if (!empty($type)) {

      // Handling for simple timestamp fields
      $fromto = array($alias, $alias);
      $tz_handling = 'site';
      $related_fields = array();
      $timezone_field = '';
      $offset_field = '';
      $rrule_field = '';
      $granularity = array('year', 'month', 'day', 'hour', 'minute');
       
      // dates with from and to dates need to handle both fields as one
      // add the from and to dates to the first one found and ignore the second
      $fields[$name]['table_name'] = $table_name;
      $fields[$name]['field_name'] = $field_name;
      $fields[$name]['type'] = $type;
      
      // Handling for content field dates
      if (isset($handler->content_field['tz_handling'])) {
        $tz_handling = $handler->content_field['tz_handling'];
        $db_info = content_database_info($handler->content_field);
        if ($tz_handling == 'date') {
          $offset_field = $table_name .'.'. $db_info['columns']['offset']['column'];
        }
        $related_fields = array(
          $table_name .'.'. $field_name
          );
        if (isset($db_info['columns']['value2']['column'])) {
          $related_fields = array_merge($related_fields, array($table_name .'.'. $db_info['columns']['value2']['column']));
        }
        if (isset($db_info['columns']['timezone']['column'])) {
          $related_fields = array_merge($related_fields, array($table_name .'.'. $db_info['columns']['timezone']['column']));
          $timezone_field = $table_name .'.'. $db_info['columns']['timezone']['column'];
        }
        if (isset($db_info['columns']['rrule']['column'])) {
          $related_fields = array_merge($related_fields, array($table_name .'.'. $db_info['columns']['rrule']['column']));
          $rrule_field = $table_name .'.'. $db_info['columns']['rrule']['column'];
        }
      }

      // Handling for cck fromto dates
      if (isset($handler->content_field)) {
        switch ($handler->content_field['type']) {
          case 'date':
          case 'datetime':
          case 'datestamp':
            $db_info = content_database_info($handler->content_field);
            $fromto = array(
              $table_name .'_'. $db_info['columns']['value']['column'],
              $table_name .'_'. ($handler->content_field['todate'] ? $db_info['columns']['value2']['column'] : $db_info['columns']['value']['column']),
              );
            break;
        }
        $granularity = $handler->content_field['granularity'];
      }
      
      // CCK fields append a column name to the field, others do not
      // need a real field_name with no column name appended for cck date formatters
      switch ($type) {
        case 'cck_string':
          $sql_type = DATE_ISO;
          break;
        case 'cck_datetime':
          $sql_type = DATE_DATETIME;
          break;
        default:
          $sql_type = DATE_UNIX;
          break;
      }
      $fields['name'][$name] = array(
        'type' => $type,
        'sql_type' => $sql_type,
        'label' => $val['group'] .': '. $val['title'],
        'granularity' => $granularity,
        'fullname' => $name,
        'table_name' => $table_name,
        'field_name' => $field_name,
        'query_name' => $alias,
        'fromto' => $fromto,
        'tz_handling' => $tz_handling,
        'offset_field' => $offset_field,
        'timezone_field' => $timezone_field,
        'rrule_field' => $rrule_field,
        'related_fields' => $related_fields,
      );
      $fields['alias'][$alias] = $fields['name'][$name];
    }
  }
  
  //cache_set($cid, $fields, 'cache_views');
  return $fields;
}