<?php
// $Id$

/**
 * Helper function to test if the database is set up to do timezone
 * conversions natively.
 */
function date_sql_db_timezone_support() {
  global $db_type;

  switch ($db_type) {
    case 'mysql':
    case 'mysqli':
      // If the timezone tables are empty, there is no support.
      $tables = db_result(db_query("SELECT COUNT(*) FROM mysql.time_zone_name;"));
      // Make sure the function returns a valid value and the data is current.
      $test = db_result(db_query("SELECT CONVERT_TZ('2007-03-11 2:00:00','US/Eastern','US/Central')"));
      if (empty($tables)) {
        return FALSE;
      }
      // This will test that the tables are current.
      elseif ($test == '2007-03-11 01:00:00') {
        return TRUE;
      }
      return FALSE;

    case 'postgres':
      // TODO, need to find a way to test this for sure in postgres, but it
      // seems to be the default situation.
      return TRUE;
  }
}

/**
 * Cross-database date SQL wrapper function for databases that support
 * native timezone handling, allows use of normalized native date
 * functions in both mysql and postgres.
 *
 * Designed to be extensible to other databases.
 *
 * @param $result_type - NOW, DATE, YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, DOW, DOY, WEEK
 * @param $field - the name of the date field to be analyzed
 * @param $timezone_from - the timezone that the date in the db is stored in.
 * @param $timezone_to - the name of the timezone to convert the date to
 * @param $date_type - the type of date field being analyzed, DATE_DATETIME, DATE_UNIX or DATE_ISO
 * @return a SQL statement appropriate for the $db_type
 *
 * $timezone_from and $timezone_to can be either a string name of a timezone
 * or the field name of a field that contains the timezone name. To make the
 * code work either way, string names must be prefixed and suffixed with single
 * quotes, i.e. "'UTD'".
 *
 * example:
 * date_db_sql('WEEK', 'MYFIELD', 'UTC', 'US/Central', DATE_ISO)
 *
 * mysql returns:
 * WEEK(CONVERT_TZ(STR_TO_DATE(MYFIELD, '%Y-%m-%dT%T), 'UTC', 'US/Central'), 3)
 *
 * postgres returns:
 * EXTRACT(WEEK FROM((SELECT TIMESTAMP WITH TIME ZONE 'TO_DATE(MYFIELD, 'FMYYYY-FMMM-FMDDTFMHH:FMMI:FMSS') UTC' AT TIME ZONE 'US/Central')))
 */
function date_db_sql($result_type, $field, $timezone_from = "'UTC'", $timezone_to = NULL, $date_type = DATE_DATETIME) {
  global $db_type;
  if (empty($timezone_to)) {
    $timezone_to = "'". date_default_timezone_name() ."'";
  }

  // NOW() is timezone-adjusted by db to the server timezone,
  // re-adjust to the desired timezone.
  if ($date_type == 'NOW' || $field == 'NOW()') {
    switch ($db_type) {
      case('mysql'):
      case('mysqli'):
        $field = " (CONVERT_TZ(NOW(), 'SERVER', $timezone_to))";
        break;
      case('pgsql'):
        $field = " (NOW() AT TIMEZONE $timezone_to)";
        break;
    }
  }
  // FROM_UNIXTIME() and TIMESTAMP() are timezone adjusted by db
  // to the server timezone, adjust back to the desired timezone.
  elseif ($date_type == DATE_UNIX && $field) {
    switch ($db_type) {
      case('mysql'):
      case('mysqli'):
        // Mysql FROM_UNIXTIME always converts from UTC to the server zone,
        // so if we have a db value stored in some zone other than UTC we
        // need to do some extra manipulation.
        if ($timezone_from != 'UTC') {
          $field = " (CONVERT_TZ(FROM_UNIXTIME($field), 'SERVER', 'UTC'))";
          $field = " (CONVERT_TZ(FROM_UNIXTIME($field), 'UTC', $timezone_from))";
          $field = " (CONVERT_TZ(FROM_UNIXTIME($field), $timezone_from, $timezone_to))";
        }
        else {
          $field = " (CONVERT_TZ(FROM_UNIXTIME($field), 'SERVER', $timezone_to))";
        }
        break;
      case('pgsql'):
        $field = " ($field::ABSTIME AT TIMEZONE '$timezone_from')";
        $field = " (SELECT TIMESTAMP WITH TIME ZONE $field $timezone_from AT TIME ZONE $timezone_to)";
        break;
    }
  }
  // ISO dates are not timezone_adjusted by db when converted to dates.
  elseif ($date_type == DATE_ISO && $field) {
    switch ($db_type) {
      case('mysql'):
      case('mysqli'):
        // Double %% so Drupal won't strip them out.
        $field = " STR_TO_DATE($field, '%%Y-%%m-%%%%dT%%T')";
        $field = " (CONVERT_TZ($field, $timezone_from, $timezone_to))";
        break;
      case('pgsl'):
        $field = " TO_DATE($field, 'FMYYYY-FMMM-FMDDTFMHH:FMMI:FMSS')";
        $field = " (SELECT TIMESTAMP WITH TIME ZONE '$field $timezone_from' AT TIME ZONE $timezone_to)";
        break;
    }
  }
  elseif ($date_type == DATE_DATETIME && $field) {
    switch ($db_type) {
      case('mysql'):
      case('mysqli'):
        $field = "(CONVERT_TZ('$field', $timezone_from, $timezone_to))";
        break;
      case('pgsql'):
        $field = "(SELECT TIMESTAMP WITH TIME ZONE $field $timezone_from AT TIME ZONE $timezone_to)";
        break;
    }
  }
  // Now that the date is timezone-adjusted, get the date parts we need.
  return date_sql_extract($result_type, $field);
}


/**
 *  Cross-database method of extracting date information from datetime fields.
 *  Allows use of normalized native date functions in both mysql and postgres.
 *  Designed to be extensible to other databases.
 *
 *  @param $result_type - NOW, DATE, YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, DOW, DOY, WEEK
 *  @param $field - the name of the date field to be analyzed
 *  @return a SQL statement appropriate for the $db_type
 */
function date_sql_extract($result_type, $field) {
  global $db_type;

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
        return "EXTRACT(DOW FROM($field))";
    }
  case('DOY'):
    switch ($db_type) {
      case('mysql'):
      case('mysqli'):
        return "DAYOFYEAR($field)";
      case('pgsql'):
        return "EXTRACT(DOY FROM($field))";
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