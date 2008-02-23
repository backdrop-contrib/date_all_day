<?php
//$Id$
/**
 * @file
 * Token module integration.
 */

function _date_token_list($type = 'all') {
  if ($type == 'field' || $type == 'all') {
    $tokens = array();

    $tokens['date']['value']          = t("The raw date value.");
    $tokens['date']['view']           = t("The formatted date.");
    $tokens['date']['timestamp']      = t("The raw date timestamp.");
    $tokens['date']['yyyy']           = t("Date year (four digit)");
    $tokens['date']['yy']             = t("Date year (two digit)");
    $tokens['date']['month']          = t("Date month (full word)");
    $tokens['date']['mon']            = t("Date month (abbreviated)");
    $tokens['date']['mm']             = t("Date month (two digit, zero padded)");
    $tokens['date']['m']              = t("Date month (one or two digit)");
    $tokens['date']['ww']             = t("Date week (two digit)");
    $tokens['date']['date']           = t("Date date (day of month)");
    $tokens['date']['day']            = t("Date day (full word)");
    $tokens['date']['ddd']            = t("Date day (abbreviation)");
    $tokens['date']['dd']             = t("Date day (two digit, zero-padded)");
    $tokens['date']['d']              = t("Date day (one or two digit)");

    $tokens['date']['to-????']        = t("If the field has a to-date defined, the same tokens exist in the form: [to-????], where ???? is the normal token.");

    return $tokens;
  }
}

function _date_token_values($type, $object = NULL, $options = array()) {
  if ($type == 'field') {
    $item = $object[0];

    $item['value'] = trim($item['value']);
    $item_type = (is_numeric($item['value']) ? DATE_UNIX : DATE_ISO);
    $timezone = !empty($item['timezone']) ? $item['timezone'] : date_default_timezone_name();
    $date = date_make_date($item['value'], $timezone, $item_type);

    $tokens['value']          = $item['value'];
    $tokens['view']           = $item['view'];
    $tokens['timestamp']      = date_format_date($date, 'U');
    $tokens['yyyy']           = date_format_date($date, 'Y');
    $tokens['yy']             = date_format_date($date, 'y');
    $tokens['month']          = date_format_date($date, 'F');
    $tokens['mon']            = date_format_date($date, 'M');
    $tokens['mm']             = date_format_date($date, 'm');
    $tokens['m']              = date_format_date($date, 'n');
    $tokens['ww']             = date_format_date($date, 'W');
    $tokens['date']           = date_format_date($date, 'N');
    $tokens['day']            = date_format_date($date, 'l');
    $tokens['ddd']            = date_format_date($date, 'D');
    $tokens['dd']             = date_format_date($date, 'd');
    $tokens['d']              = date_format_date($date, 'j');

    if (!empty($item['value2'])) {

      $item['value2'] = trim($item['value2']);
      $item_type = (is_numeric($item['value2']) ? DATE_UNIX : DATE_ISO);
      $date = date_make_date($item['value2'], $timezone, $item_type);

      $tokens['to-value']          = $item['value2'];
      $tokens['to-view']           = $item['view2'];
      $tokens['to-timestamp']      = date_format_date($date, 'U');
      $tokens['to-yyyy']           = date_format_date($date, 'Y');
      $tokens['to-yy']             = date_format_date($date, 'y');
      $tokens['to-month']          = date_format_date($date, 'F');
      $tokens['to-mon']            = date_format_date($date, 'M');
      $tokens['to-mm']             = date_format_date($date, 'm');
      $tokens['to-m']              = date_format_date($date, 'n');
      $tokens['to-ww']             = date_format_date($date, 'W');
      $tokens['to-date']           = date_format_date($date, 'N');
      $tokens['to-day']            = date_format_date($date, 'l');
      $tokens['to-ddd']            = date_format_date($date, 'D');
      $tokens['to-dd']             = date_format_date($date, 'd');
      $tokens['to-d']              = date_format_date($date, 'j');
    }
    return $tokens;
  }
}
