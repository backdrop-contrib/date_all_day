Drupal date_popup.module README.txt
==============================================================================

Javascript popup calendar and timeentry using the 
jquery UI calendar and jquery-timeentry libraries,

Read the instructions in the /lib/ folder for 
where to find and download the jquery libraries.

==================================================================================
Usage
==================================================================================

To include a popup calendar in a form, use the type 'date_popup':

  $form['date'] = array(
    '#type' => 'date_popup':
    '#title => t('My Date'),
    ....
  );

==================================================================================
Customization
==================================================================================

To change the default display and functionality of the calendar, set startup
parameters by adding selectors to your element. The configurable options 
are:

#date_type
  The type of date to convert the input value to, DATE_DATETIME, DATE_ISO, or DATE_UNIX

#date_format
  a standard PHP date format string that represents the way the month, day, 
  and year will be displayed in the textfield, like m/d/Y. Months and days 
  must be in the 'm' and 'd' formats that include the zero prefix, the year 
  must be in the 'Y' (four digit) format.

  Any standard separator can be used, '/', '-', '.', or a space.

  The m, d, and Y elements can be in any order and the order will be preserved.

  The time selector will add AM/PM if 'a' is in the format string.

  The default format uses the short site default format.


#date_year_range
  the number of years to go backwards and forwards from current year 
  in year selector, in the format -{years back}:+{years forward},
  like -3:+3

#date_timezone_db
   The timezone to convert the input value to, default is 'UTC'.

#date_timezone_handling
  'site', 'user', 'date', 'UTC', or 'none'

#date_timezone_local
   Only applies if #date_timezone_handling is set to 'date', the timezone name to use

#date_granularity
   an array of date parts to display, leave out time elements to omit a time selector,
   include or exclude 'S' to control whether seconds are included
   Allowed values are:
   Y (year), M (month), D (day), H (hours), N (minutes), S (seconds)

#date_increment
   increment minutes and seconds by this amount, default is 1

#date_empty
   handling for empty date values:
     'blank' to show blank value
     'now' to show current time
     'strftime' to use strftime to adjust the current time
     'date' to insert a specific date

#date_empty_code
   if #date_empty is 'strftime' use the value in #date_empty_code, like:
     [+-][##] [years|months|days|hours|minutes|seconds], i.e. '+90 days'
   if #date_empty is 'date' use the value in #date_empty_code as the date
     should be formatted as #date_type, using timezone from #date_timezone_db
  
==================================================================================
Example:
==================================================================================

$form['date'] = array(
  '#type' => 'date_popup',
  '#default_value' => '2007-01-01 10:30:00,
  '#date_type' => DATE_DATETIME,
  '#date_format' => 'm.d.Y h:ia',
  '#date_year_range' => -3:+3,
  '#date_timezone_handling => 'date',
  '#date_timezone_local' => 'US/Central',
  '#date_increment' => 15,
  '#date_granularity' => array('Y', 'M', 'D', 'H', 'N'),
);

==================================================================================
Localization
==================================================================================

The module will use the t() function for abbreviated month names, abbreviated 
day names, and the words 'Today', 'Clear', 'Close', 'Prev', and 'Next'. 

The module will use the site default for the first day of the week.