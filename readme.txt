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
* date_views.inc - integration for the date field with the views module

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


VIEWS SUPPORT

Date range argument handler
 
Argument is based on ISO 8601 date duration and time interval standards

See http://en.wikipedia.org/wiki/ISO_8601#Week_dates for definitions of ISO weeks
See http://en.wikipedia.org/wiki/ISO_8601#Duration for definitions of ISO duration and time interval

Argument expects a value like 2006-01-01--2006-01-15, or 2006-W24, or @P1W
Separate from and to dates or date and period with a double hyphen (--)
  
From and to dates in argument are ISO dates, but can be shortened and missing parts will be added
Omitted parts of ISO dates will be assumed to be the first possible (for the from date)
or the last possible (for the to date) value in that time period
  
The 'to' portion of the argument can be eliminated if it is the same as the 'from' portion
Use @ instead of a date to substitute in the current date and time.
  
Shortcuts are available:
  
Use periods (P1H, P1D, P1W, P1M, P1Y) to get next hour/day/week/month/year from now
Use date before P sign to get next hour/day/week/month/year from that date
The ISO standard calls for a separator (--) between a date and the P, 
but the separator is optional between a start date and a period in this argument 
to make the result easier to read.
  
This module does not currently handle the option of using a period with an end date,
only a start date followed by a period.
This module does not currently handle the option of using a period with an end date,
only a start date followed by a period.
  
Use format like 2006-W24 to find ISO week number 24 in year 2006
  
Examples:
  
--------- ARGUMENT --------    >> -------- RESULTING QUERY RANGE --------
2006-W24                       >> 24th ISO week in 2006
2006                           >> the whole year of 2006
2006-03                        >> the whole month of Mar 2006
2006-02--2007-03               >> Feb 1 2006 to Mar 31 2006
2006-08-31T14--2006-08-31T16   >> the 14th to 16th hours of Aug 8 2006
@--2006-12-31                  >> NOW to 2006-12-31T23:59:59
@P3H                           >> NOW to three hours from now
@P1Y90D                        >> NOW to 1 year and 90 days from now
2006-03-05P1W                  >> the week starting Mar 5 2006
2006-01P3M                     >> 3 months starting Jan 2006
  
The groupby selector values are used only if a summary view of the argument is requested
possible values are by year, by month, by week, by day, and by hour
  
if summaries are used, navigating to the view with no argument will display subtotals for the query, 
grouped by the selected range, with a link to the complete query for each range


Date Browser (Works only with views that use the date range argument)

* Adds this/next period navigation links to a date argument range view
* Adds 'week of XXX', 'month of XXX' headings to views and blocks
* Defaults blocks and views w/out arguments to current period to start paging
* Choose period increments by selecting the option value of date range argument
   (year, month, week, day, hour)

To use it:

* Enable the module in admin/modules.

* Create a view, and enable the Date: date range argument for the date field you want
  to page on. Set the argument option to year, month, day, week, or hour, depending 
  on the increment you want the browser to use as a period.
  
* Don't add any other arguments to the view, but you can add as many filters as needed to
  get the desired results.
  
* In the page section of the view, select the option to display the view as Date: Date Browser. 
  (instead of teaser or table). Make sure display as a page is checked, and provide a url.
  
* Be sure to put something in 'empty text' for the page. This is what will be displayed
  if you navigate to a period that has no results.
  
* Go to the view url. When no date is selected, the view will automatically display the
  current period (year, month, day, week, or hour), with back/next links to page through
  data by the selected period.
  
* The page, navigation, label, and block displays are themed, and can be customized by overriding
  the provided themes.

