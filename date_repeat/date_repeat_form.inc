<?php
// $Id$
/**
 * @file
 * Code to add a date repeat selection form to a date field and create
 * an iCal RRULE from the chosen selections.
 *
 * Moved to a separate file since it is not used on most pages
 * so the code is not parsed unless needed.
 *
 * Currently implemented:
 * INTERVAL, UNTIL, EXDATE, BYDAY, BYMONTHDAY, BYMONTH,
 * YEARLY, MONTHLY, WEEKLY, DAILY
 *
 * Currently not implemented:
 *
 * BYYEARDAY, MINUTELY, HOURLY, SECONDLY, BYMINUTE, BYHOUR, BYSECOND
 *   These could be implemented in the future.
 *
 * COUNT
 *   The goal of this module is to create a way we can parse an iCal
 *   RRULE and pull out just dates for a specified date range, for
 *   instance with a date that repeats daily for several years, we might
 *   want to only be able to pull out the dates for the current year.
 *
 *   Adding COUNT to the rules we create makes it impossible to do that
 *   without parsing and computing the whole range of dates that the rule
 *   will create. COUNT is left off of the user form completely for this
 *   reason.
 *
 * BYSETPOS
 *   Seldom used anywhere, so no reason to complicated the code.
 */
/**
 * Generate the repeat setting form.
 */
function _date_repeat_rrule_process($element, $edit = NULL) {
  include_once('./'. drupal_get_path('module', 'date_api') .'/date_api_ical.inc');

  if (!empty($edit)) {
    $rrule = date_api_ical_build_rrule($edit);
  }
  else {
    $rrule = $element['#value'];
  }
  // Unset the original string value of the RRULE so we can create
  // an array of values for the form from the RRULE's contents.
  unset($element['#value']);

  $parts = explode("\n", $rrule);
  foreach ($parts as $part) {
    if (strstr($part, 'RRULE')) {
      $RRULE = str_replace('RRULE:', '', $part);
      $rrule = (array) date_ical_parse_rrule('RRULE:', $RRULE);
    }
    elseif (strstr($part, 'EXDATE')) {
      $EXDATE = str_replace('EXDATE:', '', $part);
      $exceptions = (array) date_ical_parse_exceptions('EXDATE:', $EXDATE);
      unset($exceptions['DATA']);
    }
  }
  $form_values = $rrule;

  $collapsed = TRUE;
  if(!empty($form_values['BYDAY']) || !empty($form_values['BYMONTH'])
  || !empty($exceptions) || !empty($form_values['EXCEPT'])){
    $collapsed = FALSE;
  }

  $element['#type'] = 'fieldset';
  $element['#title'] = t('Repeat');
  $element['#description'] = t('Choose a frequency and period to repeat this date. If nothing is selected, the date will not repeat.');
  $element['#collapsible'] = TRUE;
  $element['#collapsed'] = FALSE;

  $element['INTERVAL'] = array(
    '#type' => 'select',
    //'#title' => t('Interval'),
    '#default_value' => (!empty($form_values['INTERVAL']) ? $form_values['INTERVAL'] : 0),
    '#options' => INTERVAL_options(),
    '#prefix' => '<div class="date-repeat-input">',
    '#suffix' => '</div>',
  );

  $element['FREQ'] = array(
    '#type' => 'select',
    //'#title' => t('Frequency'),
    '#default_value' => !empty($form_values['FREQ']) ? $form_values['FREQ'] : 'NONE',
    '#options' => FREQ_options(),
    '#prefix' => '<div class="date-repeat-input">',
    '#suffix' => '</div>',
  );

  if (!empty($form_values['UNTIL']['datetime'])) {
    if (!is_array(!empty($form_values['UNTIL']['datetime']))) {
      $UNTIL = $form_values['UNTIL']['datetime'];
    }
    else {
      $UNTIL = $form_values['UNTIL']['datetime']['date'];
    }
  }
  $element['UNTIL'] = array(
    '#tree' => TRUE,
    'datetime' => array(
      '#type' => module_exists('date_popup') ? 'date_popup' : 'date_select',
      '#title' => t('Until'),
      '#description' => t('Date to stop repeating this item.'),
      '#default_value' => $UNTIL,
      '#date_timezone' => $element['#date_timezone'],
      '#date_format' => 'Y-m-d',
      '#date_type' => DATE_DATETIME,
      '#date_label_position' => 'within',
      ),
    'tz' => array('#type' => 'hidden', '#value' => $element['#date_timezone']),
    'all_day' => array('#type' => 'hidden', '#value' => 1),
    'granularity' => array('#type' => 'hidden', '#value' => serialize(array('year', 'month', 'day'))),
    );

	// start the advanced fieldset
  $element['advanced'] = array(
    '#type' => 'fieldset',
    '#title' => t('Advanced'),
    '#collapsible' => TRUE,
    '#collapsed' => $collapsed,
    '#description' => t('If no advanced options are selected, the date will repeat on the month and day of the start date. Use the options below to override that behavior to select specific months and days to repeat on. Use the \'Except\' box to input dates that should be omitted from the results. '),
    );

  $element['advanced']['BYMONTH'] = array(
    '#type' => 'select',
    '#title' => t('Month'),
    '#default_value' => !empty($form_values['BYMONTH']) ? $form_values['BYMONTH'] : '',
    '#options' => array('' => t('-- Any')) + date_month_names(TRUE),
    '#multiple' => TRUE,
    '#size' => 10,
    '#prefix' => '<div class="date-repeat-input">',
    '#suffix' => '</div>',
  );

  $element['advanced']['BYMONTHDAY'] = array(
    '#type' => 'select',
    '#title' => t('Day of Month'),
    '#default_value' => !empty($form_values['BYMONTHDAY']) ? $form_values['BYMONTHDAY'] : '',
    '#options' => array('' => t('-- Any')) + drupal_map_assoc(range(1, 31)) + drupal_map_assoc(range(-1, -31)),
    '#multiple' => TRUE,
    '#size' => 10,
    '#prefix' => '<div class="date-repeat-input">',
    '#suffix' => '</div>',
  );

  $element['advanced']['BYDAY'] = array(
    '#type' => 'select',
    '#title' => t('Day of Week'),
    '#default_value' => !empty($form_values['BYDAY']) ? $form_values['BYDAY'] : '',
    '#options' => array('' => t('-- Any')) + date_repeat_dow_options(),
    //'#attributes' => array('size' => '5'),
    '#multiple' => TRUE,
    '#size' => 10,
    '#prefix' => '<div class="date-repeat-input">',
    '#suffix' => '</div>',
  );

  //exception editor date box
  if (!empty($form_values['EXCEPT']['datetime'])) {
    if (!is_array(!empty($form_values['EXCEPT']['datetime']))) {
      $EXCEPT = $form_values['EXCEPT']['datetime'];
    }
    else {
      $EXCEPT = $form_values['EXCEPT']['datetime']['date'];
    }
  }
  $element['advanced']['EXCEPT'] = array(
    '#tree' => TRUE,
    'datetime' => array(
       '#type' => module_exists('date_popup') ? 'date_popup' : 'date_select',
        '#title' => t('Except'),
        '#default_value' => $EXCEPT,
        '#date_timezone' => $element['#date_timezone'],
        '#date_format' => 'Y-m-d',
        '#date_type' => DATE_DATETIME,
        '#date_label_position' => 'within',
        ),
     'tz' => array('#type' => 'hidden', '#value' => $element['#date_timezone']),
     'all_day' => array('#type' => 'hidden', '#value' => 1),
     'granularity' => array('#type' => 'hidden', '#value' => serialize(array('year', 'month', 'day'))),
     );

  //If there's any exception date data, store the value and format
  //display of the current values.

  $element['advanced']['EXDATE'] = array(
    '#type' => 'hidden',
    '#value' => $EXDATE,
    );

  if (!empty($exceptions)) {
    //parse out the individual exception dates, and put them in human-readable format
    $element['advanced']['exceptions']['current']['#theme'] = 'date_repeat_current_exceptions';

    foreach ($exceptions as $key => $value) {
      $date = date_ical_date($value, date_default_timezone_name());
      $date_value = date_format($date, 'Y-m-d');
      $element['advanced']['exceptions']['current'][$date_value]['action'] = array(
        '#return_value' => 'remove_exception_'. str_replace(' ', '_', $date_value),
        '#type' => 'checkbox',
        );
      $element['advanced']['exceptions']['current'][$date_value]['display'] = array(
        '#type' => 'markup',
        '#value' => '<div>'. $date_value .'</div>',
        );
    }
  }

  $element['advanced']['help'] = array(
    '#type' => 'fieldset',
    '#title' => t('Help'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#description' => t('<p>Dates will match any selection within the same box [January OR June]. When more than one box has criteria selected, all of them are combined to create repeats [[January OR June] AND [Day 1 OR Day 15]]. Positive numbers count from the beginning of the period. Negative numbers count from the end of the period, i.e. -1 is the last, -2 is the next to last, etc.</p><ul><li>If you select \'Every Year\' above, and \'March\' from \'Month\' and \'1\' and \'15\' from \'Day of Month\' in the Advanced options you will be selecting the 1st and 15th days of March of every year.</li><li>If you select \'Every other Month\' above, and \'Second Tuesday\' in the Advanced options you will be selecting the second Tuesday of every other month.</li><li>If you select \'Every Year\' above, and \'Second Tuesday\' in the Advanced options you will be selecting the second Tuesday of every year.</li><li>If you select \'Every Month\' above, and \'January\' and \'June\' and \'First Saturday\' in the Advanced options, you will be selecting the first Saturday in January or June.</li><li>If you select \'Every Month\' above, and \'-1\' from \'Day of Month\' in the Advanced options you will be selecting the last day of every month.</li></ul>' ),
    );

  return $element;
}

/**
 * Regroup values back into a consistant array, no matter what state it is in.
 */
function date_repeat_merge($form_values) {
  if (empty($form_values) || !is_array($form_values)) {
    return $form_values;
  }
  if (array_key_exists('advanced', $form_values)) {
    $form_values = array_merge($form_values, $form_values['advanced']);
    unset($form_values['advanced']);
  }
  unset($form_values['BYDAY']['']);
  unset($form_values['BYMONTH']['']);
  unset($form_values['BYMONTHDAY']['']);

  if (is_array($form_values['UNTIL']['datetime']) && array_key_exists('date', $form_values['UNTIL']['datetime'])) {
    $form_values['UNTIL']['datetime'] = $form_values['UNTIL']['datetime']['date'];
  }
  if (is_array($form_values['EXCEPT']['datetime']) && array_key_exists('date', $form_values['EXCEPT']['datetime'])) {
    $form_values['EXCEPT']['datetime'] = $form_values['EXCEPT']['datetime']['date'];
  }
  return $form_values;
}

/**
 * Adds and removes exceptions from the list of current exceptions.
 */
function date_repeat_rrule_validate($element){
  include_once('./'. drupal_get_path('module', 'date_api') .'/date_api_ical.inc');

  global $form_values;

  $field_name = $element['#parents'][0];
  $delta = $element['#parents'][1];
  $item = $form_values[$field_name]['rrule'];
  $item = date_repeat_merge($item);

  // Remove exception dates that need to be deleted.
  $exceptions = array();
  foreach((array) $item['EXDATE'] as $e){
  	$removecheck = 'remove_exception_'. str_replace(' ', '_', $e);
  	if (!$_POST[$removecheck]) {
  	  $exceptions[] = $e;
  	}
  }

  // See if a new date needs to be added.
  if(!empty($item['EXCEPT']['datetime'])){
    $value = date_convert($item['EXCEPT']['datetime'], DATE_DATETIME, DATE_ICAL);
    if (!in_array($value, $exceptions)) {
      $exceptions[] = $value;
    }
  }

  // Make sure our exceptions are sorted correctly,
  // then implode them back to a string and remove the added date
  // from the date edit box.
  asort($exceptions);
  $item['EXDATE'] = implode(',', $exceptions);
  $item = date_repeat_merge($item);
  $rrule = date_api_ical_build_rrule($item);
  form_set_value($element['advanced']['EXCEPT'], NULL);
  form_set_value($element, $rrule);
}

/**
 * Theme the exception list as a table so the buttons line up
 */
function theme_date_repeat_current_exceptions($rows = array()){
  $rows_info = array();
  foreach($rows as $key => $value){
    if(substr($key, 0, 1) != '#'){
      $rows_info[] = array(drupal_render($value['action']), drupal_render($value['display']));
    }
  }
  return theme('table', array(t('Delete'), t('Current Exceptions')), $rows_info);
}

/**
 * Themes the date repeat element.
 */
function theme_date_repeat($element) {
  //return drupal_render($element);
  //return theme('form_element', $element, '<div class="container-inline">'. drupal_render($element). '</div>');
}