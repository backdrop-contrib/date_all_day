<?php
/**
 * @file
 * Date pager. Works with a Date argument, the argument filters the view and the pager provides back/next navigation.
 *
 * @TODO Figure out how to get the pager to display on the top of the page.
 */

/**
 * Example plugin to handle paging by month.
 */
class date_views_plugin_pager extends views_plugin_pager {

  /**
   * This kind of pager does not need to count the number of records.
   */
  function use_count_query() {
    return FALSE;
  }

  /**
   * Because we don't know how many pages there are, we never believe there are more records.
   */
  function has_more_records() {
    return FALSE;
  }

  /*
   * Tell Views what this pager's setting is.
   */
  function summary_title() {
    return t("Position: @position, format: @format.", array('@position' => $this->options['pager_position'], '@format' => $this->options['link_format']));
  }

  /**
   * Tell Views what options this plugin can store.
   */
  function option_definition() {
    $options = parent::option_definition();
    $options['date_id'] = array('default' => 'date');
    $options['pager_position'] = array('default' => 'bottom');
    $options['link_format'] = array('default' => 'pager');
    $options['date_argument'] = array('default' => 'Unknown');
    $options['granularity'] = array('default' => 'Unknown');
    return $options;
  }

  /*
   * Provide the form for setting options.
   */
  function options_form(&$form, &$form_state) {
    $form['markup']['#markup'] = t('This pager works together with a Date or Content date field contextual filter. If a Date filter has been added to the view, this pager will provide back/next paging to match the granularity of that filter (i.e. paging by year, month, week, or day). If there is no Date contextual filter on this view, the pager will not appear.');
    $form['date_id'] = array(
      '#title' => t('Date identifier'),
      '#type' => 'textfield',
      '#description' => t('The query identifier to use when fetching date data from in the URL.'),
      '#default_value' => $this->options['date_id'],
      '#required' => TRUE,
    );
    $form['pager_position'] = array(
      '#title' => t('Pager position'),
      '#type' => 'select',
      '#options' => array('bottom' => t('Bottom'), 'top' => t('Top'), 'both' => t('Both')),
      '#description' => t('Where to place the date pager, on the top, bottom, or both top and bottom of the content.'),
      '#default_value' => $this->options['pager_position'],
      '#required' => TRUE,
    );
    $form['link_format'] = array(
      '#title' => t('Link format'),
      '#type' => 'select',
      '#options' => array('pager' => t('Pager'), 'clean' => t('Clean')),
      '#description' => t("The format for pager link urls. With the Pager format, the links look like 'calendar/?date=2020-05'. The Clean format links look like 'calendar/2020-05'. The Clean format links look nicer but the Pager format links are likely to work better if the calendar is used in blocks or panels."),
      '#default_value' => $this->options['link_format'],
      '#required' => TRUE,
    );
    $form['date_argument']['#type'] = 'hidden';
    $form['date_argument']['#value'] = $this->options['date_argument'];
    $form['granularity']['#type'] = 'hidden';
    $form['granularity']['#value'] = $this->options['granularity'];
  }

  /**
   * 
   */
  function pre_execute(&$query) {
    // By fetching our data from the exposed input, it is possible to
    // feed pager data through some method other than $_GET.
    $input = $this->view->get_exposed_input();
    
    // Bring the argument information into the view so our theme can access it.
    $i = 0;    
    foreach ($this->view->argument as $id => $argument) {
      if (date_views_handler_is_date($argument, 'argument')) {
        
        // Storing this information in the pager so it's available for summary info.
        // The view argument information is not otherwise accessible to the pager.
        // Not working right yet, tho.
        $date_handler = $argument->date_handler;
        $this->options['date_argument'] = $id;
        $this->options['granularity'] = $argument->date_handler->granularity;
        
        $this->view->date_info = new stdClass();
        // See if the pager has reset the argument value.
        if (!empty($input[$this->options['date_id']])) {
          $argument->argument = $input[$this->options['date_id']];
          $argument->date_range = $date_handler->arg_range($argument->argument);
          $argument->min_date = $argument->date_range[0];
          $argument->max_date = $argument->date_range[1];
        }
        $this->view->date_info->granularity = $argument->date_handler->granularity;
        $this->view->date_info->date_arg = $argument->argument;
        $this->view->date_info->date_arg_pos = $i;
        $this->view->date_info->year = date_format($argument->min_date, 'Y');
        $this->view->date_info->month = date_format($argument->min_date, 'n');;
        $this->view->date_info->day = date_format($argument->min_date, 'j');
        $this->view->date_info->week = date_week(date_format($argument->min_date, DATE_FORMAT_DATE));
        $this->view->date_info->date_range = $argument->date_range;
        $this->view->date_info->min_date = $argument->min_date;
        $this->view->date_info->max_date = $argument->max_date;
        $this->view->date_info->limit = $argument->limit;
        $this->view->date_info->url = $this->view->get_url();
        $this->view->date_info->pager_id = $this->options['date_id'];
        $this->view->date_info->date_pager_position = $this->options['pager_position'];
        $this->view->date_info->date_pager_format = $this->options['link_format'];
      }
      $i++;
    }
  }

  function render($input) {
    // This adds all of our template suggestions based upon the view name and display id.
    $pager_theme = views_theme_functions('date_views_pager', $this->view, $this->display);
    return theme($pager_theme, array('plugin' => $this, 'input' => $input));
  }
}