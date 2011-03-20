<?php
// $Id$
/**
 * @file
 * Token module integration.
 */

/**
 * Implements hook_tokens_info().
 */
function date_token_info() {
  // Date module provides three different date field types.
  $info['types']['date-field'] = array(
    'name' => t('Date fields'),
    'description' => t('Tokens related to date field instances.'),
    'needs-data' => 'date-field',
    'field' => TRUE,
  );
  $info['types']['datestamp-field'] = array(
    'name' => t('Datestamp fields'),
    'description' => t('Tokens related to datestamp field instances.'),
    'needs-data' => 'datestamp-field',
    'field-value-type' => 'date-field-value',
    'field' => TRUE,
  );
  $info['types']['datetime-field'] = array(
    'name' => t('Datetime fields'),
    'description' => t('Tokens related to datetime field instances.'),
    'needs-data' => 'datetime-field',
    'field-value-type' => 'date-field-value',
    'field' => TRUE,
  );
  // All date types can share the same date value type.
  $info['types']['date-field-value'] = array(
    'name' => t('Date field values'),
    'description' => t('Tokens related to date field values.'),
    'needs-data' => 'date-field-value',
    'field-value' => TRUE,
  );
  // Provide two tokens: 'date' (the date or from-date), and 'to-date'.
  $info['tokens']['date-field-value']['date'] = array(
    'name' => t('Date'),
    'description' => t('The date value.'),
    'type' => 'date',
  );
  $info['tokens']['date-field-value']['to-date'] = array(
    'name' => t('To Date'),
    'description' => t('The to date value.'),
    'type' => 'date',
  );

  return $info;
}

/**
 * Implements hook_tokens().
 */
function date_tokens($type, $tokens, array $data = array(), array $options = array()) {
  $replacements = array();
  $language_code = isset($options['language']) ? $options['language']->language : NULL;

  if (($type == 'date-field-value') && !empty($data['item'])) {
    $item = $data['item'];

    // Create tokens for the field "Date" or "From date".
    if (($date_tokens = token_find_with_prefix($tokens, 'date')) && !empty($item['value'])) {
      // Load the from date and convert to a unix timestamp.
      $date = new DateObject($item['value'], $item['timezone_db'], date_type_format($item['date_type']));
      if (!empty($date) && $item['timezone_db'] != $item['timezone']) {
        date_timezone_set($date, timezone_open($item['timezone']));
      }
      $timestamp = !empty($date) ? date_format_date($date, 'custom', 'U') : '';
      // Generate the token replacements, using the date token type provided
      // by system.module.
      $replacements += token_generate('date', $date_tokens, array('date' => $timestamp), $options);
    }

    // Create tokens for the field "To date".
    if (($date_tokens = token_find_with_prefix($tokens, 'to-date')) && !empty($item['value2'])) {
      // Load the to date and convert to a unix timestamp.
      $date = new DateObject($item['value2'], $item['timezone_db'], date_type_format($item['date_type']));
      if (!empty($date) && $item['timezone_db'] != $item['timezone']) {
        date_timezone_set($date, timezone_open($item['timezone']));
      }
      $timestamp = !empty($date) ? date_format_date($date, 'custom', 'U') : '';
      // Generate the token replacements, using the date token type provided
      // by system.module.
      $replacements += token_generate('date', $date_tokens, array('date' => $timestamp), $options);
    }
  }

  return $replacements;
}