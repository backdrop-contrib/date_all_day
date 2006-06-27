A flexible date/time field type for the cck content module and a date API. 
The date.module requires the content.module to be installed.

Features:

* date.module can create two types of dates, an iso date (YYYY-MM-DDTHH:MM:SS) or a unix timestamp

* multiple widgets are available for date input - a drop-down selector, jscalendar pop-up (if jscalendar is installed), 
and plain text that accepts ISO format or anything strtotime allows

* the date.inc api creates a date object that has both db and local values for a date and timezone info. 
That object is then passed to all other functions that need it.

* optional incusion of adodb date library extends the valid date range from year 100 to year 3000, 
without the library it is the usual 1970 to 2038. To keep overhead to a minimum, the library is only 
included when used. If you don't want/need adodb time handling, just remove the file and native 
php date handling will be used instead.

* timezone handling options include using gmt, the site timezone, a date-specific timezone 
(selected when the date is edited), or no timezone handling. The last option will store and 
display dates exactly as entered, useful in situations where timezone conversions don't work 
accurately, or when they are just not desired.

* actual timezones are used, not offsets, because offsets don't provide enough info to do dst 
adjustments. The event_timezones.inc file is used to calculate offsets that take into account dst 
for many common zones. If event module is enabled, the api uses the event file, if it is not, 
it uses the included copy of that file. To keep overhead as low as possible, it is only included when used.

Files:

* date.module - the cck module
* date.css - mostly for the date selector drop-downs
* date.install - the date.module install file
* date_timezones.inc - a copy of event_timezones.inc, used only if event is not enabled
* adodb-time-inc.php - this library will be used if the file is in the same folder as the date.inc file, 
remove it if you do not want to use the adodb date library and the api will use native php date functions.
* date.inc - the date api, used for date calcs by date.module, but could be also used by any other module

Usage example for the api:

* create a new date object:
$date = date_make_date();

* set local value of 2006-05-04T10:24:00 US/Eastern
date_set_date($date, '2006-05-04T10:24:00', 'US/Eastern', 'local', DATE_ISO);

* display the local value using the format string 'm/d/Y H:i'
print date_show_date($date, 'm/d/Y H:i', 'local');

* display the db values of the date object:
print date_show_value($date, 'db', DATE_UNIX); // the unix db value
print date_show_value($date, 'db', DATE_ISO); // the iso db value

* display the date object and all its parts:
print_r($date);




