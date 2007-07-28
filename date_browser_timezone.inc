<?php
//$Id$

/**
 * @file
 * Get a timezone name from browser datetime string.
 *
 * The timezone offset is pretty reliably available using javascript's
 * Date function. The timezone name is buried in a string created by the
 * browser. There are many different ways that string could be configured,
 * so try to tease it out using a couple regex strings.
 *
 * This has a limitation built into the timezone_name_from_abbr() function
 * which is that it will return only the first timezone name that matches,
 * so if there are multiple timezones in the database with the same abbr and
 * offset or offset and dst value, later timezone names won't be found.
 * However, this will produce a timezone that has the same rules as the
 * browser timezone, so timezone conversions should work correctly even if
 * it's not the same as the browser zone.
 *
 * This should work pretty well for any browser that uses the timezone
 * abbreviation somewhere in its date string, less well for other
 * browsers.
 *
 */
if (!isset($_GET['tzinfo'])) {
?>
<html>
<script type="text/javascript">
var d = new Date()
var tzs = d.toString()
var tzo = d.getTimezoneOffset() * -60
window.location = window.location + '?tzinfo=' + tzs + '|' + tzo
</script>
</html>
<?php
} else {

  // Must use PHP 5 function timezone_name_from_abbr() function for this to work.
  if (!function_exists('timezone_name_from_abbr')) {
    exit;
  }
  list( $string, $offset ) = explode( '|', $_GET['tzinfo'] );

  // Try to parse a timezone abbreviation out of the browser date string
  // by looking for any 3-5 character value that exists in the string
  // then checking for that value in the timezone_abbreviations_list.
  // Make sure not to find a match for things like GMT-0500 since that
  // should not be interpreted as being in GMT. Also skip day and
  // month name abbreviations.
  preg_match_all('([a-z]{3,5}[^\-\+])', strtolower($string), $abbrs);
  $omit = array('mon', 'tue', 'wed', 'thu', 'fri', 'sat', 'sun', 'jan', 'feb',
    'mar', 'apr', 'may', 'jun', 'jul', 'aug', 'sep', 'oct', 'nov', 'dec');
  foreach ((array) $abbrs[0] as $abbr) {
    $abbr = trim($abbr);
    if (!in_array($abbr, $omit)) {
      if (in_array($abbr, array_keys(timezone_abbreviations_list()))) {
        if ($tz = timezone_name_from_abbr($abbr, $offset)) {
          // We found a match, print it and exit
          print $tz;
          exit;
        }
      }
    }
  }
  // If that didn't work, try to find whether we are in dst. Most long
  // versions of timezone names that are in dst include the words
  // 'Daylight Time' or 'Summer Time', so see if either phrase is
  // in the string anywhere. If the string values are in languages other
  // than English, this won't work.
  if (preg_match('([ daylight time| summer time])', strtolower($string)) ) {
    if ($tz = timezone_name_from_abbr(NULL, $offset, 1)) {
      // We found a match, print it and exit
      print $tz;
      exit;
    }
  }

  // If the above things did not work, we're left with searching for
  // an abbreviation by the offset. That won't work unless we supply
  // something for the dst argument. Since we didn't find anything that
  // indicates we're in dst, make the dst argument zero and hope for the
  // best.
  print timezone_name_from_abbr(NULL, $offset, 0);
  exit;
}