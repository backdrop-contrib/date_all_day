<?php
// $Id$

/**
 * @file
 * Handles sorts and deals properly with multiple
 * values by allowing the views administrator to select deltas.
 */
class date_handler_sort extends content_handler_sort {

  function option_definition() {
    $options = parent::option_definition();
    $options['date'] = array('default' => '');

    return $options;
  }

  /**
   * Add a delta selector for multiple fields.
   */
  function options_form(&$form, &$form_state) {
    $field = $this->content_field;
    parent::options_form($form, $form_state);

    // Only add the form gadget if the field is multiple.
    if ($field['cardinality']) {
      $max_delta = $field['cardinality'];
      // 1 means unlimited.
      if ($max_delta == 1) {
        $max_delta = 10;
      }

      $options = array('-1' => t('All'));
      for ($i = 0; $i < $max_delta; $i++) {
        $options[$i] = $i + 1;
      }
      $form['date'] = array(
        '#type' => 'select',
        '#options' => $options,
        '#default_value' => $this->options['date'],
        '#title' => t('Date'),
        '#description' => t('The date allows you to select which item in a multiple value field will be used for sorting. Select "1" to use the first item, "2" for the second item, and so on. If you select "All", each item in the field will create a new row, which may appear to cause duplicates.'),
      );
    }
  }

  function ensure_my_table() {
    if (!isset($this->table_alias)) {
      $join = $this->get_join();
      if (!isset($join->extra)) {
        $join->extra = array();
      }
      $delta = isset($this->options['delta']) ? $this->options['delta'] : -1;
      if ($delta != -1) {
        $join->extra[] = array(
          'field' => 'value',
          'value' => $delta,
          'numeric' => FALSE,
        );
      }

      $this->table_alias = $this->query->ensure_table($this->table, $this->relationship, $join);
    }
    return $this->table_alias;
  }
}