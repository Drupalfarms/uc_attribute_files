<?php
/*
 * @file uc_attribute_files.admin.inc
 * Provides admin functions/callbacks for the module
 * @copyright Copyright(c) 2011 Doug Logan
 * @license GPL v2 http://www.fsf.org/licensing/licenses/gpl.html
 * @author Doug Logan <my first name at dlogan dot com>
 * @author Lee Rowlands contact at rowlandsgroup dot com
 * 
 */

/**
 * Our overriden callback for the store attributes table
*/
function uc_attribute_files_admin() {
  $header = array(
    array('data' => t('Name'), 'field' => 'a.name', 'sort' => 'asc'),
    array('data' => t('Label'), 'field' => 'a.label'),
    t('Required'),
    array('data' => t('List position'), 'field' => 'a.ordering'),
    t('Number of options'),
    t('Display type'),
    t('Operations'),
  );

  $display_types = array(
    0                  => t('Text field'),
    1                  => t('Select box'),
    2                  => t('Radio buttons'),
    3                  => t('Checkboxes'),
    UC_ATTRIBUTE_FILES_TYPE => t('File Upload'),
  );

  $result = pager_query("SELECT a.aid, a.name, a.label, a.required, a.ordering,
                          a.display, COUNT(ao.oid) AS options
                        FROM {uc_attributes} AS a
                        LEFT JOIN {uc_attribute_options} AS ao
                          ON a.aid = ao.aid
                        GROUP BY a.aid, a.name, a.label, a.ordering, a.required,
                          a.display". tablesort_sql($header), 30, 0,
                          "SELECT COUNT(aid) FROM {uc_attributes}");
  while ($attr = db_fetch_object($result)) {
    if (empty($attr->label)) {
      $attr->label = $attr->name;
    }
    $ops = array(
      l(t('edit'), 'admin/store/attributes/'. $attr->aid .'/edit'),
      l(t('options'), 'admin/store/attributes/'. $attr->aid .'/options'),
      l(t('delete'), 'admin/store/attributes/'. $attr->aid .'/delete'),
    );
    $rows[] = array(
      check_plain($attr->name),
      check_plain($attr->label),
      $attr->required == 1 ? t('Yes') : t('No'),
      array('data' => $attr->ordering, 'align' => 'center'),
      array('data' => $attr->options, 'align' => 'center'),
      $display_types[$attr->display],
      implode(' ', $ops),
    );
  }

  if (count($rows) == 0) {
    $rows[] = array(
      array('data' => t('No product attributes have been added yet.'),
            'colspan' => '6')
    );
  }

  $output = theme('table', $header, $rows) . theme('pager', NULL, 30) . l(t('Add an attribute'), 'admin/store/attributes/add');

  return $output;
}