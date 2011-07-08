<?php
/**
 * @file
 * Date pager. 
 * Works with a Date argument, the argument filters the view and the pager provides back/next navigation.
 *
 * USER NOTES:
 *
 * To use this, add a pager to a view, and choose the option to 'Page by date'.
 * There are several settings:
 * - The pager id: Set an id to be used as the identifier in the url for pager values, defaults to 'date'.
 * - Pager position: Choose whether to display the date pager above, below, or both above and below the content.
 * - Link format: Choose whether the pager links will be in the simple 'calendar/2011-12' format or the 
 *   more complex 'calendar/2011-12?date=2011-12' pager format. The second one is more likely to work correctly
 *   if the pager is used in blocks and panels.
 *
 * The pager works in combination with a Date argument and it will use the date fields and granularity
 * set in that argument to create its back/next links. If the view has no Date argument, the pager can
 * do nothing. The argument can either be a 'Date' argument that lets you select one or more date fields
 * in the argument, or the simple 'Content' argument for an individual date field.
 *
 * DEVELOPER NOTES
 *
 * The pager could technically create a query of its own rather than depending on the date argument to set the query,
 * but it has only a limited set of tools to work with because it is a plugin, not a handler: 
 * it has no knowledge about relationships, it cannot use the ensure_my_table() function, 
 * plugins are not even invoked in pre_query() so it can do nothing there. 
 *
 * My conclusion was that the date pager simply is not powerful enough to create its own queries for
 * date fields, which require very complex queries. Instead, we can combine this with a date argument and
 * let the argument create the query and let the pager just provide the back/next links. If there is no
 * date argument, the pager will do nothing.
 *
 * There are still other problems. The pager is not even initialized until after all the handlers
 * have created their queries, so it has no chance to alter values ahead of that. That means
 * we can't have the pager swap in a new argument value based on the input from the pager.
 * For the moment, the solution is to alter the back/next links so they always include the
 * correct date argument as well as the pager value. This means you have pager links like
 * 'calendar/2011-12?date=2011-12', which looks a little silly. But this means both the pager
 * and the argument have the right date information.
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
   * Transfer date information from the argument to the view so the pager theme can use it.
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