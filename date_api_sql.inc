<?php
// $Id$

/**
 *  Cross-database date SQL wrapper function
 *  allows use of normalized native date functions in both mysql and postgres.
 *  Designed to be extensible to other databases.
 *
 *  @param $result_type - NOW, DATE, YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, DOW, DOY, WEEK
 *  @param $field - the name of the date field to be analyzed
 *  @param $date_type - the type of date field being analyzed, int or iso
 *  @param $offset - timezone offset in seconds, can be either a value or fieldname
 *  @param $offset_op - the operation to perform on the offset, + or -
 *  @return a SQL statement appropriate for the $db_type
 *
 *  example:          date_sql('WEEK', 'MYFIELD', 'int', 'MYOFFSETFIELD', '+')
 *  mysql returns:    WEEK(FROM_UNIXTIME(MYFIELD) + INTERVAL MYOFFSETFIELD SECOND, 3)
 *  postgres returns: EXTRACT(WEEK FROM(TIMESTAMP(MYFIELD::ABSTIME::INT4) + INTERVAL MYOFFSETFIELD SECONDS))
 */
function date_sql($result_type, $field, $date_type = 'int', $offset = '', $offset_op = '+') {
  global $db_type;

  // NOW() is timezone-adjusted by OS, adjust only for the server adj,
  // correct offset will get added back in later step.
  if ($date_type == 'NOW' || $field == 'NOW()') {
    switch ($db_type) {
      case('mysql'):
      case('mysqli'):
        if (date_server_zone_adj()) {
          $field = "(NOW() - INTERVAL ". date_server_zone_adj() ." SECOND)";
        }
        break;
      case('pgsql'):
        if (date_server_zone_adj()) {
          $field = "(NOW() - INTERVAL '". date_server_zone_adj() ." SECONDS')";
        }
        break;
     }
  }
  // Convert integer field value to native date format.
  // Since FROM_UNIXTIME() and TIMESTAMP() are timezone adjusted,
  // remove server adj, correct offset will get added back in later step.
  elseif ($date_type == 'int' && $field) {
    switch ($db_type) {
      case('mysql'):
      case('mysqli'):
        $field = "FROM_UNIXTIME($field)";
        if (date_server_zone_adj()) {
          $field = "($field - INTERVAL ". date_server_zone_adj() ." SECOND)";
        }
                break;
      case('pgsql'):
        $field = "($field::ABSTIME)";
        if (date_server_zone_adj()) {
          $field = "($field - INTERVAL '". date_server_zone_adj() ." SECONDS')";
        }
        break;
    }
  }
  // Get rid of the 'T' in ISO dates to match native date field.
  // This makes it possible to use SQL date functions on the value.
  elseif ($date_type == DATE_ISO && $field) {
    $field = " REPLACE($field,'T',' ')";
  }
  // Now apply requested offset to the adjusted query field.
  if ($offset) {
    switch ($db_type) {
    case('mysql'):
    case('mysqli'):
      $field = "($field $offset_op INTERVAL ($offset) SECOND)";
      break;
    case('pgsql'):
      $field = "($field $offset_op INTERVAL '$offset SECONDS')";
      break;
    }
  }
  // Return requested sql.
  // Note there is no space after FROM to avoid db_rewrite problems
  // see http://drupal.org/node/79904.
  switch ($result_type) {
  case('NOW'):
  case('DATE'):
    return $field;
  case('YEAR'):
    return "EXTRACT(YEAR FROM($field))";
  case('MONTH'):
    return "EXTRACT(MONTH FROM($field))";
  case('DAY'):
    return "EXTRACT(DAY FROM($field))";
  case('HOUR'):
    return "EXTRACT(HOUR FROM($field))";
  case('MINUTE'):
    return "EXTRACT(MINUTE FROM($field))";
  case('SECOND'):
    return "EXTRACT(SECOND FROM($field))";
  case('WEEK'):  // ISO week number for date
    switch ($db_type) {
      case('mysql'):
      case('mysqli'):
        // WEEK using arg 3 in mysql should return the same value as postgres EXTRACT
        return "WEEK($field, 3)";
      case('pgsql'):
        return "EXTRACT(WEEK FROM($field))";
    }
  case('DOW'):
    switch ($db_type) {
      case('mysql'):
      case('mysqli'):
        // mysql returns 1 for Sunday through 7 for Saturday
        // php date functions and postgres use 0 for Sunday and 6 for Saturday
        return "INTEGER(DAYOFWEEK($field) - 1)";
      case('pgsql'):
        return "EXTRACT (DOW FROM($field))";
    }
  case('DOY'):
    switch ($db_type) {
      case('mysql'):
      case('mysqli'):
        return "DAYOFYEAR($field)";
      case('pgsql'):
        return "EXTRACT (DOY FROM ($field))";
    }
  }
}

/**
 *  A helper function to do cross-database concatation of date parts
 *
 *  @param $array - an array of values to be concatonated in sql
 *  @return - correct sql string for database type
 */
function date_sql_concat($array) {
  global $db_type;

  switch ($db_type) {
    case('mysql'):
    case('mysqli'):
      return "CONCAT(". implode(",", $array) .")";
    case('pgsql'):
      return implode(" || ", $array);
  }
}

/**
 *  A helper function to do cross-database padding of date parts
 *
 *  @param $str - a string to apply padding to
 *  @param $size - the size the final string should be
 *  @param $pad - the value to pad the string with
 *  @param $side - the side of the string to pad
 */
function date_sql_pad($str, $size = 2, $pad = '0', $side = 'l') {
  switch ($side) {
  case('r'):
    return "RPAD($str, $size, '$pad')";
  default:
    return "LPAD($str, $size, '$pad')";
  }
}

/**
 *  Server timezone adjustment.
 *
 *  Used to compute default timezone adjustment made by SQL server
 *  so server adjustment can be removed and replaced with correct timezone
 *  adjustment values.
 *
 *  @return amount in seconds that DB server adjusts for timezone.
 */
function date_server_zone_adj() {
  global $db_type;
  static $server_zone_adj;
  if (!isset($server_zone_adj)) {
    switch ($db_type) {
      case('mysql'):
      case('mysqli'):
        // In 4.1.1+ we can use difference between NOW() and UTC_TIMESTAMP().
        if (version_compare(mysql_get_server_info(), '4.1.1', '>=')) {
          $server_zone_adj = db_result(db_query("SELECT UNIX_TIMESTAMP(NOW()) - UNIX_TIMESTAMP(UTC_TIMESTAMP())"));
        }
        break;
      case('pgsql'):
        // EXTRACT TIMEZONE returns the timezone adjustment in seconds.
        $server_zone_adj = db_result(db_query("SELECT EXTRACT('TIMEZONE' FROM NOW())"));
        break;
    }
    // If no value got set by this point,
    // fall back to using php timezone adjustment as an estimate.
    if (!isset($server_zone_adj)) {
      $server_zone_adj = date('Z');
    }
  }
  return $server_zone_adj;
}