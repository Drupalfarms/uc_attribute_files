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

/**
 * Provides file download menu callback
*/
function _uc_attribute_files_download($type, $file, $uid = NULL, $order_id = NULL) {
  global $user;

  switch ($type) {
    case 'cart':
       $dir = variable_get('uc_attribute_files_cart_filepath', NULL) . "/" . uc_cart_get_id();
       break;
       
    case 'anonymous':
    case 'order':
       $dir = variable_get('uc_attribute_files_order_filepath', NULL) . "/" . $uid . "/" . $order_id;
       
       if ($type !== 'anonymous' && $uid == $user->uid && $uid == 0) { //Force anonymous orders to provide zip
          return drupal_get_form('uc_attribute_files_anonymous_zip_form', $file, $uid, $order_id);
       }
       break;

    default:
       $dir = NULL;
       break;
  }

  if (!is_null(variable_get('uc_attribute_files_cart_filepath', NULL)) &&  //Validate variable set
      !is_null(variable_get('uc_attribute_files_order_filepath', NULL)) && //Validate variable set
      ($uid == $user->uid || user_access('view all uploads'))  &&         //Check permission
      !is_null($dir) && is_readable($dir . '/' . check_plain($file))       //Check dir & file are valid
  ) {
    uc_attribute_files_download_transfer(check_plain($file), $dir);

  }
  elseif ($user->uid == 0) {
    drupal_set_title(t('Access denied'));
    return t('You must logon to access this file.'); 
  }
  else {
    //Handle & Log Errors
    if (is_null(variable_get('uc_attribute_files_cart_filepath', NULL))) {
      watchdog('uc_attribute_files', 'uc_attribute_files_cart_filepath is not set');
      $dir = NULL;
      if ($user->uid == 1) {
        drupal_set_message(t('uc_attribute_files_cart_filepath is not set. !link to edit.', array('!link' => l(t('Click here'), 'admin/store/settings/attributes'))));
      }
    }
    
    if (is_null(variable_get('uc_attribute_files_order_filepath', NULL))) {
      watchdog('uc_attribute_files', 'uc_attribute_files_order_filepath is not set');
      $dir = NULL;
      if ($user->uid == 1) {
        drupal_set_message(t('uc_attribute_files_order_filepath is not set. !link to edit.', array('!link' => l(t('Click here'), 'admin/store/settings/attributes'))));
      }
    }
    
    if (is_readable($dir . '/' . check_plain($file))) {
      watchdog('uc_attribute_files', 'File not readable: %user attempted to access %file from %type: %id. in dir: %dir', array('%user' => $user->name, '%file' => check_plain($file), '%type' => $type, '%id' => $id, '%dir' => $dir));
    }
    
    if (is_null($dir)) {
      watchdog('uc_attribute_files', 'Invalid type of %type was passed by user %user for file %file', array('%user' => $user->name, '%file' => check_plain($file), '%type' => $type));
    }
    
    drupal_set_header('HTTP/1.1 404 Not Found');
    drupal_set_title(t('Page not found'));
    return t('The requested page could not be found.');
  }
}


function uc_attribute_files_anonymous_zip_form($form_state, $file, $uid, $order_id) {
  $form['zip'] = array( '#type'      => 'textfield',
                        '#description' => t('Enter the billing zip code for the order'),
                        '#title'     => t('Billing Zip Code'),
                        '#size'      => 5,
                        '#maxlength' => 5,
                      );
  $form['a'] = array( '#type' => 'hidden', '#value' => $file );
  $form['b'] = array( '#type' => 'hidden', '#value' => $uid );
  $form['c'] = array( '#type' => 'hidden', '#value' => $order_id );
  $form['submit'] = array('#type' => 'submit', '#value' => t('Submit'));
  return $form;
}

function uc_attribute_files_anonymous_zip_form_submit($form, &$form_state) {
  $zip = $form_state['values']['zip'];
  $file = $form_state['values']['a'];
  $uid = $form_state['values']['b'];
  $order_id = $form_state['values']['c'];

  if (intval($order_id) > 0) {
    $order = uc_order_load($order_id);
    if ($order->billing_postal_code == $zip) {
      return _uc_attribute_files_download('anonymous', $file, $uid, $order_id);
    }
  }
  drupal_set_message(t('Zip code does not match order.'));   
}

function uc_attribute_files_download_transfer($file, $path) {
  //Function copied and modified slightly from uc_file.module

  $filepath = $path . "/" . $file;

  // Gather relevant info about the file.
  $size = filesize($filepath);
  $mimetype = file_get_mimetype($filepath);

  // Workaround for IE filename bug with multiple periods / multiple dots in filename
  // that adds square brackets to filename - eg. setup.abc.exe becomes setup[1].abc.exe
  if (strstr($_SERVER['HTTP_USER_AGENT'], 'MSIE')) {
    $file = preg_replace('/\./', '%2e', $file, substr_count($file, '.') - 1);
  }

  // Check if HTTP_RANGE is sent by browser (or download manager)
  if (isset($_SERVER['HTTP_RANGE'])) {
    list($size_unit, $range_orig) = explode('=', $_SERVER['HTTP_RANGE'], 2);

    if ($size_unit == 'bytes') {
      // Multiple ranges could be specified at the same time, but for simplicity only serve the first range
      // See http://tools.ietf.org/id/draft-ietf-http-range-retrieval-00.txt
      list($range, $extra_ranges) = explode(',', $range_orig, 2);
    }
    else {
      $range = '';
    }
  }
  else {
    $range = '';
  }

  // Figure out download piece from range (if set)
  list($seek_start, $seek_end) = explode('-', $range, 2);

  // Set start and end based on range (if set), else set defaults and check for invalid ranges.
  $seek_end = intval((empty($seek_end)) ? ($size - 1) : min(abs(intval($seek_end)), ($size - 1)));
  $seek_start = intval((empty($seek_start) || $seek_end < abs(intval($seek_start))) ? 0 : max(abs(intval($seek_start)), 0));

  ob_end_clean();

  //Only send partial content header if downloading a piece of the file (IE workaround)
  if ($seek_start > 0 || $seek_end < ($size - 1)) {
    drupal_set_header('HTTP/1.1 206 Partial Content');
  }

  // Standard headers, including content-range and length
  drupal_set_header('Pragma: public');
  drupal_set_header('Cache-Control: cache, must-revalidate');
  drupal_set_header('Accept-Ranges: bytes');
  drupal_set_header('Content-Range: bytes '. $seek_start .'-'. $seek_end .'/'. $size);
  drupal_set_header('Content-Type: '. $mimetype);
  drupal_set_header('Content-Disposition: attachment; filename="'. $file .'"');
  drupal_set_header('Content-Length: '. ($seek_end - $seek_start + 1));

  // Last-Modified is required for content served dynamically
  drupal_set_header('Last-Modified: '. gmdate("D, d M Y H:i:s", filemtime($filepath)) ." GMT");

  // Etag header is required for Firefox3 and other managers
  drupal_set_header('ETag: '. md5($filepath));

  // Open the file and seek to starting byte
  $fp = fopen($filepath, 'rb');
  fseek($fp, $seek_start);

  // Start buffered download
  while (!feof($fp)) {
    // Reset time limit for large files
    set_time_limit(30);

    // Push the data to the client.
    print(fread($fp, 8192));
    flush();
    ob_flush();
  }

  fclose($fp);
}

