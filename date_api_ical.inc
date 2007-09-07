<?php
/* $Id$ */
/**
 * @file
 * Parse iCal imports and create iCal exports.
 * This file must be included when these functions are needed.
 */

/**
 * Turn an array of events into a valid iCalendar file
 *
 * @param $events
 *   An array of events where each event is an array with:
 *    'start'         => UTC start date in YYYYMMDDTHHMMSS format,
 *                         omit time for all day event.
 *    'end'           => UTC end date in YYYYMMDDTHHMMSS format,
 *                         optional, omit for all day event.
 *    'summary'       => Title of event (Text)
 *    'description'   => Description of event (Text)
 *    'location'      => Location of event (Text)
 *    'uid'           => ID of the event for use by calendaring program.
 *                         Recommend the url of the node
 *    'url'           => URL of event information
 *
 * @param $calname
 *   Name of the calendar.  Will use site name if none is specified.
 *
 * @return
 *   Text of a date_icalendar file.
 *
 * @todo
 *   add folding and more ical elements
 */
function date_ical_export($events, $calname = NULL) {
  $output .= "BEGIN:VCALENDAR\nVERSION:2.0\n";
  $output .= "METHOD:PUBLISH\n";
  $output .= 'X-WR-CALNAME:'. date_ical_escape_text($calname ? $calname : variable_get('site_name', '')) ."\n";
  $output .= "PRODID:-//Drupal iCal API//EN\n";
  foreach ($events as $uid => $event) {
    // Skip any items with empty dates.
    if (!empty($event['start'])  && ereg ('^(\d{4})(\d{2})(\d{2})$', $event['start'])) {
      $output .= "BEGIN:VEVENT\n";
      $output .= "DTSTAMP;VALUE=DATE-TIME:". gmdate("Ymd\THis\Z", time()) ."\n";
      if ($event['start'] && $event['end']) {
        $output .= "DTSTART;VALUE=DATE-TIME:". gmdate("Ymd\THis\Z", $event['start']) ."\n";
        $output .= "DTEND;VALUE=DATE-TIME:". gmdate("Ymd\THis\Z", $event['end']) ."\n";
      }
      else {
        $output .= "DTSTART;VALUE=DATE-TIME:". gmdate("Ymd\THis\Z", $event['start']) ."\n";
      }
      $output .= "UID:". ($event['uid'] ? $event['uid'] : $uid) ."\n";
      if ($event['url']) {
        $output .= "URL;VALUE=URI:". $event['url'] ."\n";
      }
      if ($event['location']) {
        $output .= "LOCATION:" . date_ical_escape_text($event['location']) . "\n";
      }
      $output .= "SUMMARY:". date_ical_escape_text($event['summary']) ."\n";
      if ($event['description']) {
        $output .= "DESCRIPTION:". date_ical_escape_text($event['description']) ."\n";
      }
      $output .= "END:VEVENT\n";
    }
  }
  $output .= "END:VCALENDAR\n";
  return $output;
}

/**
 * Escape #text elements for safe iCal use
 *
 * @param $text
 *   Text to escape
 *
 * @return
 *   Escaped text
 *
 */
function date_ical_escape_text($text) {
  //$text = strip_tags($text);
  $text = str_replace('"', '\"', $text);
  $text = str_replace("\\", "\\\\", $text);
  $text = str_replace(",", "\,", $text);
  $text = str_replace(":", "\:", $text);
  $text = str_replace(";", "\;", $text);
  $text = str_replace("\n", "\n ", $text);
  return $text;
}

/**
 * Return an array of iCalendar information from an iCalendar file.
 *
 *   No timezone adjustment is performed in the import since the timezone
 *   conversion needed will vary depending on whether the value is being
 *   imported into the database (when it needs to be converted to UTC), is being
 *   viewed on a site that has user-configurable timezones (when it needs to be
 *   converted to the user's timezone), if it needs to be converted to the
 *   site timezone, or if it is a date without a timezone which should not have
 *   any timezone conversion applied.
 *
 *   Properties that have dates and times are converted to sub-arrays like:
 *      'datetime'   => date in YYYY-MM-DD HH:MM format, not timezone adjusted
 *      'all_day'    => whether this is an all-day event
 *      'tz'         => the timezone of the date, could be blank for absolute
 *                      times that should get no timezone conversion.
 *
 *   Exception dates can have muliple values and are returned as arrays
 *   like the above for each exception date.
 *
 *   Most other properties are returned as PROPERTY => VALUE.
 *
 *   Each item in the VCALENDAR will return an array like:
 *   [0] => Array (
 *     [TYPE] => VEVENT
 *     [UID] => 104
 *     [SUMMARY] => An example event
 *     [URL] => http://example.com/node/1
 *     [DTSTART] => Array (
 *       [datetime] => 1997-09-07 09:00:00
 *       [all_day] => 0
 *       [tz] => US/Eastern
 *     )
 *     [DTEND] => Array (
 *       [datetime] => 1997-09-07 11:00:00
 *       [all_day] => 0
 *       [tz] => US/Eastern
 *     )
 *     [RRULE] => Array (
 *       [FREQ] => Array (
 *         [0] => MONTHLY
 *       )
 *       [BYDAY] => Array (
 *         [0] => 1SU
 *         [1] => -1SU
 *       )
 *     )
 *     [EXDATE] => Array (
 *       [0] = Array (
 *         [datetime] => 1997-09-21 09:00:00
 *         [all_day] => 0
 *         [tz] => US/Eastern
 *       )
 *       [1] = Array (
 *         [datetime] => 1997-10-05 09:00:00
 *         [all_day] => 0
 *         [tz] => US/Eastern
 *       )
 *     )
 *   )
 *
 * @param $filename
 *   Location (local or remote) of a valid iCalendar file
 * @return array
 *   An array with all the elements from the ical
 * @todo
 *   figure out how to handle this if subgroups are nested,
 *   like a VALARM nested inside a VEVENT.
 */
function date_ical_import($filename) {
  $items = array();

  // Fetch the iCal data. If file is a URL, use drupal_http_request. fopen
  // isn't always configured to allow network connections.
  if (substr($filename, 0, 4) == 'http') {
    // Fetch the ical data from the specified network location
    $icaldatafetch = drupal_http_request($filename);
    // Check the return result
    if ($icaldatafetch->error) {
      drupal_set_message('Request Error: ' . $icaldatafetch->error, 'error');
      return array();
    }
    // Break the return result into one array entry per lines
    $icaldatafolded = explode("\n", $icaldatafetch->data);
  }
  else {
    $icaldatafolded = @file($filename, FILE_IGNORE_NEW_LINES);
    if ($icaldatafolded === FALSE) {
      drupal_set_message('Failed to open file: ' . $filename, 'error');
      return array();
    }
  }

  // Verify this is iCal data
  if (trim($icaldatafolded[0]) != 'BEGIN:VCALENDAR') {
    drupal_set_message('Invalid calendar file:' . $filename, 'error');
    return array();
  }

  // "unfold" wrapped lines
  $icaldata = array();
  foreach ($icaldatafolded as $line) {
    if (substr($line, 0, 1) == ' ') {
      $line = array_pop($icaldata) . substr($line, 1);
    }
    $icaldata[] = $line;
  }
  $icaldatafolded = NULL;

  // Parse the iCal information
  $is_subgroup = FALSE;
  $skip = FALSE;
  foreach ($icaldata as $line) {
    $line = trim($line);
    $vcal .= $line ."\n";

    switch ($line) {

    	case 'BEGIN:VEVENT':
    	case 'BEGIN:VALARM':
    	case 'BEGIN:VTODO':
    	case 'BEGIN:VJOURNAL':
    	case 'BEGIN:VVENUE':
    	case 'BEGIN:VFREEBUSY':
    	case 'BEGIN:VTIMEZONE':
    		unset($subgroup);
    		$type = str_replace('BEGIN:', '', $line);
	      $subgroup['TYPE'] = $type;
        $is_subgroup = TRUE;
	      break;

	    case 'END:VEVENT':
	    case 'END:VALARM':
	    case 'END:VTODO':
    	case 'END:VJOURNAL':
    	case 'END:VVENUE':
    	case 'END:VFREEBUSY':
    	case 'END:VTIMEZONE':
    	  // Check for all-day events.
    	  if (array_key_exists('DTSTART', $subgroup) && !array_key_exists(DTEND, $subgroup)) {
    	    $subgroup['DTSTART']['all_day'] = 1;
    	  }
        $items[] = $subgroup;
        unset($subgroup);
    		$is_subgroup = FALSE;
        break;

      default:
        unset ($field, $data, $prop_pos, $property);
        ereg ("([^:]+):(.*)", $line, $line);
        $field = $line[1];
        $data = $line[2];
        $property = $field;
        $prop_pos = strpos($property,';');
        if ($prop_pos !== false) $property = substr($property,0,$prop_pos);
        $property = strtoupper(trim($property));

        switch ($property) {
          // Keep blank lines out of the results.
          case '':
            break;

          // Lots of properties have date values that must be parsed out.
          case 'CREATED':
          case 'LAST-MODIFIED':
          case 'DTSTART':
          case 'DTEND':
          case 'DTSTAMP':
          case 'RDATE':
          case 'TRIGGER':
          case 'FREEBUSY':
          case 'DUE':
          case 'COMPLETED':
            if (!$is_subgroup) {
              $items[$property] = date_ical_parse_date($field, $data);
            }
	          else {
	            $subgroup[$property] = date_ical_parse_date($field, $data);
	          }
            break;

          case 'EXDATE':
            $subgroup[$property] = date_ical_parse_exceptions($field, $data);
            break;

          case 'DURATION':
            $subgroup[$property] = date_ical_parse_duration($field, $data);
	          break;

          case 'RRULE':
          case 'EXRULE':
	          $subgroup[$property] = date_ical_parse_rule($field, $data);
	          break;

          case 'SUMMARY':
          case 'DESCRIPTION':
          case 'LOCATION':
            $subgroup[$property] = date_ical_parse_text($field, $data);
            break;

          // For all other properties, just store the property and the value.
          // This can be expanded on in the future if other properties should
          // be given special treatment.
          default:
	          if (!$is_subgroup) {
              $items[$property] = $data;
	          }
	          else {
	            $subgroup[$property] = $data;
	          }
	          break;
	      }
    }
  }
  // Store the original text in the array for reference.
  $items['VCALENDAR'] = $vcal;
  return $items;
}

/**
 * Parse a ical date element.
 *
 * Possible formats to parse include:
 *   PROPERTY:YYYYMMDD[T][HH][MM][SS][Z]
 *   PROPERTY;VALUE=DATE:YYYYMMDD[T][HH][MM][SS][Z]
 *   PROPERTY;VALUE=DATE-TIME:YYYYMMDD[T][HH][MM][SS][Z]
 *   PROPERTY;TZID=XXXXXXXX;VALUE=DATE:YYYYMMDD[T][HH][MM][SS]
 *   PROPERTY;TZID=XXXXXXXX:YYYYMMDD[T][HH][MM][SS]
 *
 *   The property and the colon before the date are removed in the import
 *   process above and we are left with $field and $data.
 *
 *  @param $field
 *    The text before the colon and the date, i.e.
 *    ';VALUE=DATE:', ';VALUE=DATE-TIME:', ';TZID='
 *  @param $data
 *    The date itself, after the colon, in the format YYYYMMDD[T][HH][MM][SS][Z]
 *    'Z', if supplied, means the date is in UTC.
 *
 *  @return array
 *   $items array, consisting of:
 *      'datetime'   => date in YYYY-MM-DD HH:MM format, not timezone adjusted
 *      'all_day'    => whether this is an all-day event with no time
 *      'tz'         => the timezone of the date, could be blank if the ical
 *                      has no timezone; the ical specs say no timezone
 *                      conversion should be done if no timezone info is
 *                      supplied
 *  @todo
 *   Another option for dates is the format PROPERTY;VALUE=PERIOD:XXXX. The period
 *   may include a duration, or a date and a duration, or two dates, so would
 *   have to be split into parts and run through date_ical_parse_date() and
 *   date_ical_parse_duration(). This is not commonly used, so ignored for now.
 *   It will take more work to figure how to support that.
 */
function date_ical_parse_date($field, $data) {
  $tz = '';
  $data = trim($data);
  $items = array();
  if (substr($data, -1) == 'Z') {
    $tz = 'UTC';
  }
  if (strstr($field, 'TZID=')) {
    $tmp = explode('=', $field);
    // Fix commonly used alternatives like US-Eastern which should be US/Eastern.
    $tz = str_replace('-', '/', $tmp[1]);
  }
  $data = str_replace(array('T', 'Z'), '', $data);
  preg_match (DATE_REGEX_LOOSE, $data, $regs);
  if (!empty($regs[5])) {
    $time = ' '.date_pad($regs[5]) .':'. date_pad($regs[6]) .':'. date_pad($regs[7]);
  }
  $items['datetime'] = $regs[1] .'-'. $regs[2] .'-'. $regs[3] . $time;
  $items['all_day'] = empty($time) ? 1 : 0;
  $items['tz'] = $tz;
  return $items;
}

/**
 * Parse an ical repeat rule.
 *
 * @return array
 *   Array in the form of PROPERTY => array(VALUES)
 *   PROPERTIES include FREQ, INTERVAL, COUNT, BYDAY, BYMONTH, BYYEAR, UNTIL
 */
function date_ical_parse_rule($field, $data) {
  $items = array();
  $rule = explode(';', $data);
  foreach ($rule as $key => $value) {
    $param = explode('=', $value);
    if ($param[0] == 'UNTIL') {
      $values = date_ical_parse_date('', $param[1]);
    }
    else {
      $values = explode(',', $param[1]);
    }
    $items[$param[0]] = $values;
  }
  return $items;
}

/**
 * Parse exception dates (can be multiple values).
 *
 * @return array
 *   an array of date value arrays.
 */
function date_ical_parse_exceptions($field, $data) {
  $items = array();
  $ex_dates = explode(',', $data);
  foreach ($ex_dates as $ex_date) {
    $items[] = date_ical_parse_date('', $ex_date);
  }
	return $items;
}

/**
 * Parse the duration of the event.
 *
 *  @return the duration in seconds.
 */
function date_ical_parse_duration($field, $data) {
  $items = array();
  ereg ('^P(\d{1,4}[Y])?(\d{1,2}[M])?(\d{1,2}[W])?(\d{1,2}[D])?([T]{0,1})?(\d{1,2}[H])?(\d{1,2}[M])?(\d{1,2}[S])?', $data, $duration);
	$items['year'] = str_replace('Y', '', $duration[1]);
	$items['month'] = str_replace('M', '', $duration[2]);
	$items['week'] = str_replace('W', '', $duration[3]);
	$items['day'] = str_replace('D', '', $duration[4]);
	$items['hour'] = str_replace('H', '', $duration[6]);
	$items['minute'] = str_replace('M', '', $duration[7]);
	$items['second'] = str_replace('S', '', $duration[8]);
	$date = date_create('now', timezone_open('UTC'));
	$date2 = drupal_clone($date);
	foreach ($items as $item => $count) {
	  if ($count > 0) {
	    date_modify($date2, '+'. $count .' '. $item);
	  }
	}
	$items['DURATION'] = date_format($date2, 'U') - date_format($date, 'U');
  return $items;
}

/**
 * Parse and clean up ical text elements.
 */
function date_ical_parse_text($field, $data) {
  if (strstr($field, 'QUOTED-PRINTABLE')) {
    $data = quoted_printable_decode($data);
  }
  $data = stripslashes($data);
  return $data;
}

/**
 * Adjust ical date to appropriate timezone and format it.
 *
 *  @param $ical_date
 *    an array of ical date information created in the ical import.
 *  @param $to_tz
 *    the timezone to convert the date's value to.
 *  @return object
 *    a timezone-adjusted date object
 */
function date_ical_date($ical_date, $to_tz = FALSE) {
  // If the ical date has no timezone, must assume it is stateless
  // so treat it as a local date.
  if (empty($ical_date['tz'])) {
    $from_tz = date_default_timezone_name();
  }
  else {
    $from_tz = $ical_date['tz'];
  }
  $date = date_create($ical_date['datetime'], timezone_open($from_tz));
  if ($ical_date['tz'] != '' && $to_tz != $ical_date['tz']) {
    $date = date_timezone_set($date, timezone_open($to_tz));
  }
  return $date;
}