(function ($) {

Drupal.behaviors.dateAdmin = {};

Drupal.behaviors.dateAdmin.attach = function (context, settings) {
  // Turn the years back and forward fieldsets into dropdowns.
  var $fieldset, $textfield, $textfields, i;
  $fieldset = $('#edit-instance-years-back-and-forward').once('date-admin');
  $textfields = $fieldset.find('input.select-list-with-custom-option');
  for (i = 0; i < $textfields.length; i++) {
    $textfield = $($textfields[i]);
    new Drupal.dateAdmin.selectListWithCustomOption($textfield);
  }
};


Drupal.dateAdmin = {};

/**
 * Constructor for the selectListWithCustomOption object.
 *
 * This object is responsible for turning the years back and forward textfields
 * into dropdowns with an 'other' option that lets the user enter a custom
 * value.
 */
Drupal.dateAdmin.selectListWithCustomOption = function ($textfield) {
  this.$textfield = $textfield;
  this.$description = $textfield.next('div.description');
  this.defaultValue = $textfield.val();
  this.$dropdown = this.createDropdown();
  this.$dropdown.insertBefore($textfield);
};

/**
 * Get the value of the textfield as it existed on page load.
 *
 * @param {String} type
 *   The type of the variable to be returned. Defaults to string.
 * @return
 *   The original value of the textfield. Returned as an integer, if the type
 *   parameter was 'int'.
 */
Drupal.dateAdmin.selectListWithCustomOption.prototype.getOriginal = function (type) {
  var original;
  if (type === 'int') {
    original = parseInt(this.defaultValue, 10);
    if (window.isNaN(original)) {
      original = 0;
    }
  }
  else {
    original = this.defaultValue;
  }
  return original;
};

/**
 * Get the correct first value for the dropdown.
 */
Drupal.dateAdmin.selectListWithCustomOption.prototype.getStartValue = function () {
  var direction = this.getDirection();
  var start;
  switch (direction) {
    case 'back':
      // For the 'years back' dropdown, the first option should be -10, unless
      // the default value of the textfield is even smaller than that.
      start = Math.min(this.getOriginal('int'), -10);
      break;
    case 'forward':
      start = 0;
      break;
  }
  return start;
};

/**
 * Get the correct last value for the dropdown.
 */
Drupal.dateAdmin.selectListWithCustomOption.prototype.getEndValue = function () {
  var direction = this.getDirection();
  var end;
  var originalString = this.getOriginal();
  switch (direction) {
    case 'back':
      end = 0;
      break;
    case 'forward':
      // If the original value of the textfield is an absolute year such as
      // 2020, don't try to include it in the dropdown.
      if (originalString.indexOf('+') === -1) {
        end = 10;
      }
      // If the original value is a relative value (+x), we want it to be
      // included in the possible dropdown values.
      else {
        end = Math.max(this.getOriginal('int'), 10);
      }
      break;
  }
  return end;
};

/**
 * Create a dropdown select list with the correct options for this textfield.
 */
Drupal.dateAdmin.selectListWithCustomOption.prototype.createDropdown = function () {
  var $dropdown = $('<select>').addClass('form-select');
  var $option, i, value;
  var start = this.getStartValue();
  var end = this.getEndValue();
  var direction = this.getDirection();
  for (i = start; i <= end; i++) {
    // Make sure we include the +/- sign in the option value.
    value = i;
    if (i > 0) {
      value = '+' + i;
    }
    // Zero values must have a + or - in front.
    if (i === 0) {
      if (direction === 'back') {
        value = '-' + i;
      }
      else {
        value = '+' + i;
      }
    }
    $option = $('<option>' + value + '</option>').val(value);
    $dropdown.append($option);
    // When the user clicks on one of the preset options, hide the 'other'
    // textfield.
    $option.bind('click', $.proxy(this.hideTextfield, this));
  }
  // Create an 'Other' option.
  $option = $('<option>' + Drupal.t('Other') + '</option>').val('');
  $dropdown.append($option);
  // When the user clicks on it, show the textfield so they can type into it.
  $option.bind('click', $.proxy(this.revealTextfield, this));

  // Set the initial value of the dropdown.
  this._setInitialDropdownValue($dropdown);
  return $dropdown;
};

Drupal.dateAdmin.selectListWithCustomOption.prototype._setInitialDropdownValue = function ($dropdown) {
  var textfieldValue = this.getOriginal();
  // Determine whether the original textfield value exists in the dropdown.
  var possible = $dropdown.find('option[value=' + textfieldValue + ']');
  // If the original textfield value is one of the dropdown options, preselect
  // it and hide the 'other' textfield.
  if (possible.length) {
    $dropdown.val(textfieldValue);
    this.$textfield.hide();
    this.$description.hide();
  }
  // If the original textfield value isn't one of the dropdown options, choose
  // the 'Other' option in the dropdown.
  else {
    $dropdown.val('');
  }
};

/**
 * Determine whether this is the "years back" or "years forward" textfield.
 */
Drupal.dateAdmin.selectListWithCustomOption.prototype.getDirection = function () {
  if (this.direction) {
    return this.direction;
  }
  var direction;
  if (this.$textfield.hasClass('back')) {
    direction = 'back';
  }
  else if (this.$textfield.hasClass('forward')) {
    direction = 'forward';
  }
  this.direction = direction;
  return direction;
};

/**
 * Click handler for the 'Other' option in the dropdown.
 */
Drupal.dateAdmin.selectListWithCustomOption.prototype.revealTextfield = function () {
  this.syncTextfield();
  this.$textfield.show();
  this.$description.show();
};

/**
 * Click handler for the preset options in the dropdown.
 */
Drupal.dateAdmin.selectListWithCustomOption.prototype.hideTextfield = function () {
  this.syncTextfield();
  this.$textfield.hide();
  this.$description.hide();
};

/**
 * Copy the selected value of the dropdown to the textfield.
 *
 * FAPI doesn't know about the JS-only dropdown, so the textfield needs to
 * reflect the value of the dropdown.
 */
Drupal.dateAdmin.selectListWithCustomOption.prototype.syncTextfield = function () {
  var value = this.$dropdown.val();
  this.$textfield.val(value);
};

})(jQuery);
