<?php
/**
 * @file
 * Date API views argument handler.
 */

/**
 * Date API argument handler.
 */
class date_views_argument_handler_simple extends views_handler_argument_date {

  /**
   * Get granularity and use it to create the formula and a format
   * for the results.
   */
  function init(&$view, &$options) {
    parent::init($view, $options);

    // Identify the type of display we're using.
    $this->display_handler = $view->display_handler->definition['handler'];

    // Add a date handler.
    module_load_include('inc', 'date_api', 'date_api_sql');
    $this->date_handler = new date_sql_handler();
    $this->date_handler->date_type = DATE_UNIX;
    if (!empty($this->definition['field_name'])) {
      $field = field_info_field($this->definition['field_name']);
      if (!empty($field) && !empty($field['type'])) {
        $this->date_handler->date_type = $field['type'];
      }
    }
    $this->date_handler->granularity = $this->options['granularity'];

    // Set up the formula for this field.
    $this->format = $this->date_handler->views_formats($this->date_handler->granularity, 'display');
    $this->arg_format = $this->date_handler->views_formats($this->date_handler->granularity, 'sql');
    $this->sql_format = $this->date_handler->views_formats($this->date_handler->granularity, 'sql');
    $this->formula = $this->date_handler->sql_format($this->sql_format, $this->date_handler->sql_field("***table***.$this->real_field"));

    $this->definition['empty field name'] = t('Undated');
  }

  /**
   * Default value for the date_fields option.
   */
  function option_definition() {
    $options = parent::option_definition();
    $options['year_range'] = array('default' => '-3:+3');
    $options['granularity'] = array('default' => 'month');
    return $options;
  }

  /**
   * Add a form element to select date_fields for this argument.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $options = $this->date_handler->date_parts();
    unset($options['second'], $options['minute']);
    $options += array('week' => t('Week', array(), array('context' => 'datetime')));
    $form['granularity'] = array(
      '#title' => t('Granularity'),
      '#type' => 'radios',
      '#options' => $options,
      '#default_value' => $this->options['granularity'],
      '#multiple' => TRUE,
      '#description' => t("Select the type of date value to be used in defaults, summaries, and navigation. For example, a granularity of 'month' will set the default date to the current month, summarize by month in summary views, and link to the next and previous month when using date navigation."),
    );

    $form['year_range'] = array(
      '#title' => t('Date year range'),
      '#type' => 'textfield',
      '#default_value' => $this->options['year_range'],
      '#description' => t("Set the allowable minimum and maximum year range for this argument, either a -X:+X offset from the current year, like '-3:+3' or an absolute minimum and maximum year, like '2005:2010' . When the argument is set to a date outside the range, the page will be returned as 'Page not found (404)' ."),
    );
  }

  function options_validate(&$form, &$form_state) {
    // It is very important to call the parent function here:
    parent::options_validate($form, $form_state);
    if (!preg_match('/^(?:\-[0-9]{1,4}|[0-9]{4}):(?:[\+|\-][0-9]{1,4}|[0-9]{4})$/', $form_state['values']['options']['year_range'])) {
      form_error($form['year_range'], t('Date year range must be in the format -9:+9, 2005:2010, -9:2010, or 2005:+9'));
    }
  }

  /**
   * Provide a link to the next level of the view from the summary.
   */
  function summary_name($data) {
    $value = $data->{$this->name_alias};
    if (empty($value) && !empty($this->definition['empty field name'])) {
      return $this->definition['empty field name'];
    }
    $format = $this->date_handler->views_formats($this->options['granularity'], 'display');
    $range = $this->date_handler->arg_range($value);
    return date_format_date($range[0], 'custom', $format);
  }





}
