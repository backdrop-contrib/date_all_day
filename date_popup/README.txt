Drupal jquery-datetime.module README.txt
==============================================================================

Javascript popup calendars using the jquery-calendar library,

Pop-Up Calendar Built from Scratch by Marc Grabanski */
Enhanced by Keith Wood (kbwood@iprimus.com.au). */
Under the Creative Commons License http://creativecommons.org/licenses/by/3.0/

Javascript time entry widget using the jquery-timeentry library,

Written by Keith Wood (kbwood@iprimus.com.au) June 2007.
Under the Creative Commons Licence http://creativecommons.org/licenses/by/3.0/


Installation
------------------------------------------------------------------------------
Create a directory modules/jquery_calendar and copy all the module's files 
into it, as well as the folder 'lib' and its contents. Enable the module via the 
administer > build > modules page.

Contents
------------------------------------------------------------------------------
The 'lib' folder contains the original jquery-calendar.js and jquery-calendar.css
files from the jquery-calendar home page (http://marcgrabanski.com/code/jquery-calendar/).


Developer usage
------------------------------------------------------------------------------

Adding to a textfield
----------------------

To include a jscalendar popup with a textfield, add the class 'jquery-calendar':

  $form['date'] = array(
    '#type' => 'textfield',
    '#attributes' => array('class' => 'jquery-calendar'),
  );

The pop up calendar does not include a time selector, so if time is used, it 
should be in a separate form item from the date.


Customization
----------------------

To change the default display and functionality of the calendar, set startup
parameters by adding selectors to your element. The configurable options 
are:

#date_format
  a standard PHP date format string that represents the way the month, day, 
  and year will be displayed in the textfield, like m/d/Y. Months and days 
  must be in the 'm' and 'd' formats that include the zero prefix, the year 
  must be in the 'Y' (four digit) format.

  Any standard separator can be used, '/', '-', '.', or a space.

  The m, d, and Y elements can be in any order and the order will be preserved.

  All other formatting elements will be ignored.

  The default format is the month, day, and year elements of the short 
  site default format, after converting 'n' to 'm', and 'j' to 'd',
  i.e. a short site format of 'n/j/Y - H:i' will become 'm/d/Y'.


#date_year_range
  the number of years to go backwards and forwards from current year 
  in year selector, in the format -{years back}:+{years forward}
  
Example:
  $form['date'] = array(
    '#type' => 'textfield',
    '#attributes' => array('class' => 'jquery-calendar'),
    '#date_format' => 'm.d.Y',
    '#date_year_range' => -3:+3,
  );

Localization
----------------------

The module will use the t() function for abbreviated month names, abbreviated 
day names, and the words 'Today', 'Clear', 'Close', 'Prev', and 'Next'. 

The module will use the site default for the first day of the week.