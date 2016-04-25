<?php

/**
 * @file
 * Nexteuropa Newsroom Forms.
 */

/**
 * Submit handler for config restore form.
 *
 * @param array $form
 *   Form array.
 * @param array $form_state
 *   Form state.
 *
 * @return string
 *   Form submit result.
 */
function nexteuropa_newsroom_restore_conf($form, &$form_state) {
  $voc = $form['vocabulary_machine_name']['#value'];
  // Get the id coming from the newsroom.
  $id = _nexteuropa_newsroom_get_ids($voc, $form['#term']);
  // The name of the variable contains the id.
  $name = $voc . '_backup';
  $backup = variable_get($name, array());

  if (empty($backup)) {
    drupal_set_message(t('No backup has been found for the vocabulary: @voc and the term: @name.', array('@voc' => $voc, '@name' => $form['#term']['name'])), 'warning');
    return '';
  }

  $term = $backup[$id];

  if (empty($term)) {
    drupal_set_message(t('No backup has been found for the term: @name.', array('@name' => $form['#term']['name'])), 'warning');
    return '';
  }

  // Override values basing on the backup ones.
  foreach ($term as $propname => $prop) {
    $form_state['values'][$propname][LANGUAGE_NONE][0]['value'] = $prop;
  }

  // Set a message for the user.
  drupal_set_message(t('The configuration for the term "@name" has been restored from the backup version.', array('@name' => $form['#term']['name'])), 'status');
  // Submit the form so the term get saved.
  return taxonomy_form_term_submit($form, $form_state);
}

/**
 * Checks backup info.
 *
 * @param array $form
 *   Form array.
 * @param array $form_state
 *   Form state array.
 *
 * @return bool
 *   Check result.
 */
function _nexteuropa_newsroom_check_backup($form, &$form_state) {
  $voc = $form['#vocabulary']->machine_name;
  // Get the current variable for the specific vocubulary.
  $backup = variable_get($voc . '_backup', array());

  if (empty($backup)) {
    return FALSE;
  }
  else {
    return TRUE;
  }
}

/**
 * Submit handler for config restore conf all form.
 *
 * @param array $form
 *   Form array.
 * @param array $form_state
 *   Form state.
 */
function nexteuropa_newsroom_restore_conf_all($form, &$form_state) {
  // Divide terms into chunks, grouping five of them.
  $chunks = array_chunk($backup, 5, TRUE);
  $num_operations = $sandbox['limit'] = count($chunks);
  $operations = array();

  for ($i = 0; $i < $num_operations; $i++) {
    $operations[] = array(
      'nexteuropa_newsroom_restore_batch',
      array(
        $chunks[$i],
        $voc,
        t('(Operation @operation)', array('@operation' => $i)),
      ),
    );
  }

  $batch = array(
    'operations' => $operations,
    'finished' => 'nexteuropa_newsroom_restore_batch_finished',
    'title' => t('Restore configuration for terms'),
    'init_message' => t('The restore of your configuration has been started'),
    'progress_message' => t('Current: @current | Remaining:
      @remaining | Total: @total | Percentage: @percentage | Estimate:
      @estimate | Elapsed: @elapsed'),
    'error_message' => t('The restore did not work.'),
  );

  batch_set($batch);
}

/**
 * Implements hook_form_FORM_ID_alter().
 */
function nexteuropa_newsroom_form_nexteuropa_newsroom_admin_settings_alter(&$form, &$form_state, $form_id) {
  // If we are in a confimation step, change the form visualization.
  if (isset($form_state['storage']['confirm'])) {
    drupal_set_message(t('You are in a confirmation page, to submit all the form settings check the box below'), 'warning');
    $form['newsroom_universe']['#collapsible'] = TRUE;
    $form['newsroom_universe']['#collapsed'] = TRUE;
    $form['newsroom_fields']['#collapsed'] = FALSE;
  }
}

/**
 * Submit handler for config form.
 *
 * @param array $form
 *   Form array.
 * @param array $form_state
 *   Form state.
 *
 * @return string
 *   Form submit result.
 */
function nexteuropa_newsroom_backup_conf($form, &$form_state) {
  $voc = $form['vocabulary_machine_name']['#value'];
  // Get the current backup.
  $current_values = variable_get($voc . '_backup', array());
  $id = _nexteuropa_newsroom_get_ids($voc, $form_state['values']);
  $term = (object) $form_state['values'];
  $new_values = _nexteuropa_newsroom_export_prepare_values($term);
  // If no backup have been saved, save it for for the first time.
  if (empty($current_values)) {
    variable_set($voc . '_backup', $new_values);
  }
  else {
    // Override the values for the specific term.
    $new_values = reset($new_values);
    $current_values[$id] = $new_values;
  }

  // Set values for the newsroom backup.
  variable_set($voc . '_backup', $current_values);

  // Set a message to the user.
  drupal_set_message(t('We saved a backup of your configuration for the term "@name". if you need to restore the values in the backup click on the "Restore & Save" button', array('@name' => $form['#term']['name'])), 'status');
  // Submit the form so the term get saved.
  return taxonomy_form_term_submit($form, $form_state);
}

/**
 * Implements hook_form_FORM_ID_alter().
 */
function nexteuropa_newsroom_form_taxonomy_form_term_alter(&$form, &$form_state) {
  $voc_name = $form['vocabulary_machine_name']['#value'];

  switch ($voc_name) {
    case NEXTEUROPA_NEWSROOM_TYPE_VOCABULARY:
    case NEXTEUROPA_NEWSROOM_SERVICE_VOCABULARY:
    case NEXTEUROPA_NEWSROOM_TOPIC_VOCABULARY:
      $form['actions']['restore_config'] = array(
        '#type' => 'submit',
        '#value' => t('Restore conf & Save'),
        '#submit' => array('nexteuropa_newsroom_restore_conf'),
      );
      $form['actions']['backup_config'] = array(
        '#type' => 'submit',
        '#value' => t('Backup conf & Save'),
        '#submit' => array('nexteuropa_newsroom_backup_conf'),
      );
      break;
  }
}

/**
 * Implements hook_form_FORM_ID_alter().
 */
function nexteuropa_newsroom_form_taxonomy_overview_terms_alter(&$form, &$form_state) {
  $vocs = array(
    NEXTEUROPA_NEWSROOM_TOPIC_VOCABULARY,
    NEXTEUROPA_NEWSROOM_SERVICE_VOCABULARY,
    NEXTEUROPA_NEWSROOM_TYPE_VOCABULARY,
  );

  $value = _nexteuropa_newsroom_check_backup($form, $form_state) ? t('[Newsroom] Override existing backup') : t('[Newsroom] Backup configuration');

  if (in_array($form['#vocabulary']->machine_name, $vocs)) {
    $form['actions']['backup_config'] = array(
      '#type' => 'submit',
      '#value' => $value,
      '#submit' => array('nexteuropa_newsroom_backup_conf_all'),
    );

    if ($check = _nexteuropa_newsroom_check_backup($form, $form_state)) {
      $form['actions']['restore_config'] = array(
        '#type' => 'submit',
        '#value' => t('[Newsroom] Restore configuration'),
        '#submit' => array('nexteuropa_newsroom_restore_conf_all'),
      );
    }
  }
}

/**
 * Implements hook_date_popup_process_alter().
 */
function nexteuropa_newsroom_date_popup_process_alter(&$element, &$form_state, $context) {
  if ($form_state['build_info']['form_id'] == 'nexteuropa_newsroom_agenda_datepicker_form') {
    unset($element['date']['#title']);
    unset($element['date']['#description']);
  }
}

/**
 * Implements hook_form_alter().
 */
function nexteuropa_newsroom_form_alter(&$form, &$form_state, $form_id) {

  if ($form_id == 'newsroom_item_node_form') {
    // Build language aware options list for taxonomy reference fields.
    $terms = array();
    global $language;
    $def_lang = language_default()->language;
    $curr_lang = $language->language;

    $taxonomy_fields = array(
      'field_newsroom_item_type',
      'field_newsroom_primary_topic',
      'field_newsroom_topics',
      'field_newsroom_highlighted_topic',
    );

    $type_vid = NewsroomHelper::getNewsroomTypeVocabulary()->vid;
    $topic_vid = NewsroomHelper::getNewsroomTopicVocabulary()->vid;

    $terms[NEXTEUROPA_NEWSROOM_TYPE_VOCABULARY] = i18n_taxonomy_get_tree($type_vid, $def_lang);
    $terms[NEXTEUROPA_NEWSROOM_TOPIC_VOCABULARY] = i18n_taxonomy_get_tree($topic_vid, $def_lang);

    // Fields with wrong translation in node edit form.
    foreach ($taxonomy_fields as $taxonomy_field) {
      $tree = $taxonomy_field == 'field_newsroom_item_type' ? $terms[NEXTEUROPA_NEWSROOM_TYPE_VOCABULARY] : $terms[NEXTEUROPA_NEWSROOM_TOPIC_VOCABULARY];

      foreach ($tree as $key => $value) {
        $i18n_object = i18n_get_object('taxonomy_term', $value->tid);
        $t_term = $i18n_object->localize($curr_lang);
        $pref = $value->parents[0] != 0 ? '- ' : '';
        $form[$taxonomy_field][LANGUAGE_NONE]['#options'][$value->tid] = $pref . $t_term->name;
      }

      // The default value is lost in the translations,
      // so we need to set it this way.
      if (!empty($form['#node']->$taxonomy_field)) {
        $values = array();
        $value_array = $form['#node']->$taxonomy_field;
        foreach ($value_array[LANGUAGE_NONE] as $value) {
          $values[] = $value['tid'];
          $form[$taxonomy_field][LANGUAGE_NONE]['#default_value'] = $values;
        }
      }
    }

    // The widget will loose this settings, otherwise.
    $form['field_newsroom_topics'][LANGUAGE_NONE]['#multiple'] = TRUE;
  }
}

/**
 * Gets form rendable array for newsroom proposal page.
 *
 * @param array $form
 *   Form array.
 * @param array $form_state
 *   Form state array.
 *
 * @return array
 *   Rendable form array.
 */
function nexteuropa_newsroom_newsroom_proposal_form($form, $form_state) {
  global $user;

  $account = user_load($user->uid);
  $form['#action'] = $newsroom_url = variable_get('newsroom_proposal_url', NULL);
  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#description' => 'Title of your Newsroom item (mandatory)',
    '#default_value' => '',
    '#required' => TRUE,
  );
  $form['teaser'] = array(
    '#title' => t('Teaser'),
    '#type' => 'textarea',
    '#description' => t('Teaser of your Newsroom item (a teaser is a brief description of the subject of your item)'),
    '#size' => 18,
  );
  $form['body'] = array(
    '#title' => t('Full content'),
    '#type' => 'textarea',
    '#description' => t('Full content of the Newsroom item (mandatory - The full content is the body of your item)'),
    '#required' => TRUE,
  );
  $form['url'] = array(
    '#title' => t('URL'),
    '#type' => 'textfield',
    '#description' => t('Add a link to another web page'),
  );
  $form['start_date'] = array(
    '#title' => t('Date'),
    '#type' => 'date',
    '#description' => t('Publication or event date'),
  );
  $form['end_date'] = array(
    '#title' => t('End date'),
    '#type' => 'date',
    '#description' => t('End date'),
  );
  $form['venue'] = array(
    '#title' => t('Venue'),
    '#type' => 'textfield',
    '#description' => t('Name and address of the venue'),
  );
  $form['topic_id'] = array(
    '#title' => t('Topic ID'),
    '#type' => 'hidden',
    '#value' => isset($_REQUEST['topic_id']) ? filter_xss($_REQUEST['topic_id']) : NULL,
    '#required' => TRUE,
  );
  $form['email'] = array(
    '#title' => t('E-mail'),
    '#type' => 'hidden',
    '#value' => $account->mail,
    '#required' => TRUE,
  );
  $form['lastname'] = array(
    '#title' => t('Last name'),
    '#type' => 'hidden',
    '#value' => isset($account->field_lastname[LANGUAGE_NONE][0]['value']) ? $account->field_lastname[LANGUAGE_NONE][0]['value'] : '',
    '#required' => TRUE,
  );
  $form['firstname'] = array(
    '#title' => t('First name'),
    '#type' => 'hidden',
    '#value' => isset($account->field_firstname[LANGUAGE_NONE][0]['value']) ? $account->field_firstname[LANGUAGE_NONE][0]['value'] : '',
    '#required' => TRUE,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Send'),
  );

  return $form;
}

/**
 * Gets form rendable array for newsletter subscription block.
 *
 * @param array $form
 *   Form array.
 * @param array $form_state
 *   Form state array.
 *
 * @return array
 *   Rendable form array.
 */
function nexteuropa_newsroom_newsletter_subscription_form($form, &$form_state) {
  $args = $form_state['build_info']['args'][0];
  $form = array();
  $form['#prefix'] = '<div class="CTAnewsletter">';
  $form['#suffix'] = '</div>';
  $form['#token'] = FALSE;
  $form['#action'] = NewsroomHelper::getNewsroomUrl('subscription-quick-generic.cfm');
  $form['intro'] = array(
    '#markup' => '<div class="title">' . $args['name'] . '</div>',
  );

  $form['feedback'] = array(
    '#markup' => '<div class="feedback"></div>',
  );

  $form['email'] = array(
    '#prefix' => '<div class="inputMail">',
    '#suffix' => '</div>',
    '#type' => 'textfield',
    '#attributes' => array(
      'placeholder' => t('Your e-mail'),
      'class' => array(),
    ),
  );

  $form['submit_email_prefix'] = array(
    '#markup' => '<div class="inputSubscribe">',
  );

  $form['submit_email'] = array(
    '#type' => 'button',
    '#value' => t('Subscribe'),
    '#ajax' => array(
      'callback' => '_nexteuropa_nexteuropa_newsroom_newsletter_subscription_form_ajax',
    ),
  );

  $form['submit_email_submit'] = array(
    '#markup' => '</div>',
  );

  $form['service_id'] = array(
    '#type' => 'hidden',
    '#value' => $args['tid'],
  );

  $form['previous'] = array(
    '#markup' => '<div class="linkPreviousIssues"><a href="' . NewsroomHelper::getNewsroomUrl('newsletter-specific-archive.cfm?serviceId=' . $args['tid']) . '">' . t('Previous issues') . '</a></div>',
  );

  return $form;
}

/**
 * Sends request to the newsroom subscription script.
 *
 * @param string $url
 *   Service URL.
 * @param int $service_id
 *   Service ID.
 * @param string $email
 *   User email.
 *
 * @return string
 *   Object decoded from JSON.
 */
function nexteuropa_newsroom_send_subscription_request($url, $service_id, $email) {
  $input = array(
    'service_id' => $service_id,
    'email' => $email,
    'response_type' => 'json',
  );
  $query = http_build_query($input);

  $request = drupal_http_request($url . '?' . $query);
  $json_data = $request->data;
  $data = drupal_json_decode($json_data);

  return $data['subscription'];
}

/**
 * Newsletter subscription form ajax callback.
 *
 * @param array $form
 *   Form array.
 * @param array $form_state
 *   Form state array.
 *
 * @return array
 *   Ajax callback array.
 */
function _nexteuropa_nexteuropa_newsroom_newsletter_subscription_form_ajax($form, &$form_state) {
  $form_id = $form['#form_id'];
  $selector = '#' . str_replace('_', '-', $form_id);

  $subscription = nexteuropa_newsroom_send_subscription_request($form['#action'], $form_state['input']['service_id'], $form_state['input']['email']);

  if ($subscription['status'] == 'Failed') {
    form_set_error('email', $subscription['feedbackMessage']);
    $commands[] = ajax_command_invoke($selector . ' #edit-email', 'addClass', array('error'));
  }
  else {
    $type = ($subscription['isNewUser'] === 'False' && $subscription['isNewSubscription'] === 'False') ? 'warning' : 'status';
    drupal_set_message($subscription['feedbackMessage'], $type);
    $commands[] = ajax_command_invoke($selector . ' #edit-email', 'removeClass', array('error'));
  }

  $replacement = theme_status_messages(array('display' => NULL));
  $commands[] = ajax_command_replace($selector . ' .feedback', '<div class="feedback">' . $replacement . '</div>');

  return array('#type' => 'ajax', '#commands' => $commands);
}

/**
 * Gets form rendable array for filter on the newsroom page.
 *
 * @param array $form
 *   Form array.
 * @param array $form_state
 *   Form state array.
 *
 * @return array
 *   Rendable form array.
 */
function nexteuropa_newsroom_list_filter_form($form, &$form_state) {
  $arguments = $form_state['build_info']['args'];
  $form = array();
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Reset filter'),
    '#weight' => 10,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Reset filter'),
    '#weight' => 10,
  );

  if (!empty($arguments[0])) {
    // Selected country.
    $selected_country = !empty($arguments[2]) && !empty($arguments[2][1]) ? $arguments[2][1] : 0;
    $countries = $arguments[0];

    if ($selected_country !== 0) {
      $form['countries'] = array(
        '#title' => t('Filter by country'),
        '#type' => 'hidden',
        '#default_value' => '0',
      );

      $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Reset filter'),
        '#weight' => 10,
      );
    }
    else {
      $form['countries'] = array(
        '#title' => t('Filter by country'),
        '#type' => 'select',
        '#options' => $countries,
        '#default_value' => $selected_country,
      );

      $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Go'),
        '#weight' => 10,
      );
    }
  }

  if (!empty($form)) {
    $form['#submit'] = array('_nexteuropa_newsroom_list_filter_submit');
  }

  return $form;
}

/**
 * Submit handler for filter form.
 *
 * @param array $form
 *   Form array.
 * @param array $form_state
 *   Form state array.
 */
function _nexteuropa_newsroom_list_filter_submit(&$form, &$form_state) {
  $arguments = $form_state['build_info']['args'];
  $target_country = $form_state['values']['countries'] == '0' ? '' : $form_state['values']['countries'];

  if (!empty($arguments[2]) && !empty($arguments[2][0])) {
    $path = NewsroomHelper::getNewsroomPageUrl($arguments[2][0], $target_country);
  }
  else {
    $path = NewsroomHelper::getNewsroomPageUrl('all', $target_country);
  }

  drupal_goto($path, array(), 301);
}

/**
 * Gets form rendable array for filter on the agenda page.
 *
 * @param array $form
 *   Form array.
 * @param array $form_state
 *   Form state array.
 * @param string $view
 *   View type: schedule, month, week, day.
 * @param int $type_id
 *   Newsroom type ID.
 * @param int $topic_id
 *   Newsroom Topic ID.
 * @param string $current_date
 *   Current date.
 * @param array $result_topics
 *   Topics to filter out the option list.
 *
 * @return array
 *   Rendable form array.
 */
function nexteuropa_newsroom_agenda_filter_form($form, &$form_state, $view, $type_id, $topic_id, $current_date, $result_topics) {
  $topic_options = NewsroomHelper::getTaxonomyTermOptions(NewsroomHelper::getNewsroomTopics());
  foreach ($topic_options as $key => $topic) {
    if (!in_array($key, $result_topics)) {
      unset($topic_options[$key]);
    }
  }

  $empty = array(t('- None -'));
  $topic_options = $empty + $topic_options;

  $form = array();
  $form['type_id'] = array(
    '#type' => 'select',
    '#title' => t('Type'),
    '#options' => NewsroomHelper::getNewsroomTypeEventsOptions(),
    '#default_value' => $type_id,
  );
  $form['topic_id'] = array(
    '#type' => 'select',
    '#title' => t('Topic'),
    '#options' => $topic_options,
    '#default_value' => $topic_id,
  );
  $form['current_date'] = array(
    '#type' => 'hidden',
    '#value' => $current_date,
  );
  $form['view'] = array(
    '#type' => 'hidden',
    '#value' => $view,
  );
  $form['button'] = array(
    '#type' => 'button',
    '#button_type' => 'button',
    '#ajax' => array(
      'callback' => '_nexteuropa_newsroom_agenda_ajax_form_submit',
    ),
    '#value' => t('Filter'),
  );

  return $form;
}

/**
 * Gets form rendable array for date fitler block on the newsroom page.
 *
 * @param array $form
 *   Form array.
 * @param array $form_state
 *   Form state array.
 *
 * @return array
 *   Rendable form array.
 */
function nexteuropa_newsroom_agenda_datepicker_form($form, &$form_state, $view, $type_id, $topic_id, $current_date) {
  $form = array();
  $form['current_date'] = array(
    '#type' => 'date_popup',
    '#title' => t('Current date'),
    '#default_value' => $current_date,
    '#date_type' => DATE_DATETIME,
    '#date_timezone' => date_default_timezone(),
    '#date_format' => 'd-m-Y',
    '#date_increment' => 1,
    '#date_year_range' => '-2:+2',
    '#ajax' => array(
      'callback' => '_nexteuropa_newsroom_agenda_ajax_form_submit',
    ),
  );

  $form['view'] = array(
    '#type' => 'hidden',
    '#value' => $view,
  );

  $form['type_id'] = array(
    '#type' => 'hidden',
    '#value' => $type_id,
  );

  $form['topic_id'] = array(
    '#type' => 'hidden',
    '#value' => $topic_id,
  );

  return $form;
}

/**
 * Ajax submit handler.
 *
 * @param array $form
 *   Form array.
 * @param array $form_state
 *   Form state array.
 *
 * @return array
 *   Ajax callback array.
 */
function _nexteuropa_newsroom_agenda_ajax_form_submit(&$form, &$form_state) {
  $view = isset($form_state['input']['view']) ? $form_state['input']['view'] : NULL;

  if (isset($form_state['input']['current_date']['date'])) {
    $date = $form_state['input']['current_date']['date'];
  }
  elseif (isset($form_state['input']['current_date'])) {
    $date = $form_state['input']['current_date'];
  }
  else {
    $date = NewsroomHelper::getFormatedDate();
  }

  $topic_id = isset($form_state['input']['topic_id']) ? $form_state['input']['topic_id'] : NULL;
  $type_id = isset($form_state['input']['type_id']) ? $form_state['input']['type_id'] : NULL;

  $topic = NewsroomHelper::getNewsroomTopic($topic_id);
  $type = NewsroomHelper::getNewsroomType($type_id);
  $factory = new NewsroomAgendaFactory();
  $agenda = $factory->build($view, $type, $topic, new DateTime($date));

  ctools_include('ajax');
  $commands[] = ctools_ajax_command_redirect($agenda->getNavigationUrl($agenda->getCurrentDate()));

  return array('#type' => 'ajax', '#commands' => $commands);
}

/**
 * Gets form rendable array for usbscription button on newsletter page.
 *
 * @param array $form
 *   Form array.
 * @param array $form_state
 *   Form state array.
 * @param int $service_id
 *   Newsletter subscription service ID.
 *
 * @return array
 *   Rendable form array.
 */
function nexteuropa_newsroom_newsletter_form($form, $form_state, $service_id) {
  global $user;

  $account = user_load($user->uid);
  $form['#action'] = NewsroomHelper::getNewsroomUrl('subscription-quick-generic.cfm');
  $form['email'] = array(
    '#title' => t('E-mail'),
    '#type' => 'hidden',
    '#value' => $account ? $account->mail : NULL,
    '#required' => TRUE,
  );

  $form['service_id'] = array(
    '#type' => 'hidden',
    '#value' => $service_id,
    '#required' => TRUE,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#ajax' => array(
      'callback' => '_nexteuropa_nexteuropa_newsletter_subscription_form_ajax',
    ),
    '#value' => t('Subscribe'),
  );

  $form['submit-response'] = array(
    '#markup' => '<div class="submit-response"></div>',
  );

  return $form;
}

/**
 * Newsletter subscription form ajax callback.
 *
 * @param array $form
 *   Form array.
 * @param array $form_state
 *   Form state array.
 *
 * @return array
 *   Ajax callback array.
 */
function _nexteuropa_nexteuropa_newsletter_subscription_form_ajax($form, &$form_state) {
  $subscription = nexteuropa_newsroom_send_subscription_request($form['#action'], $form_state['input']['service_id'], $form_state['input']['email']);

  if ($subscription['status'] == 'Failed') {
    $type = 'error';
  }
  else {
    $type = ($subscription['isNewUser'] === 'False' && $subscription['isNewSubscription'] === 'False') ? 'warning' : 'status';
  }

  drupal_set_message($subscription['feedbackMessage'], $type);
  $replacement = theme_status_messages(array('display' => NULL));
  $commands[] = ajax_command_replace('#' . $form['#id'] . ' .submit-response', '<div class="submit-response">' . $replacement . '</div>');
  $commands[] = ajax_command_replace('#' . $form['#id'] . ' input', '');

  return array('#type' => 'ajax', '#commands' => $commands);
}

/**
 * The batch operation.
 *
 * @param array $chunk
 *   The terms to be updated.
 * @param string $voc
 *   Vocabulary's machine name.
 * @param array $operation_details
 *   Array of values.
 */
function nexteuropa_newsroom_restore_batch(array $chunk, $voc, array $operation_details) {

  foreach ($chunk as $id => $fields) {
    // Try to find the corresponding term basing on its ID.
    $term = _nexteuropa_newsroom_get_term_from_id($id, $voc);
    if ($term) {
      // Let's work with an array.
      $term = get_object_vars($term);

      // This is special, make them handable!.
      $special = array(
        'field_newsroom_featured_item',
      );

      if (!empty($fields)) {
        // Override values.
        foreach ($fields as $field_name => $value) {
          if (!in_array($propname, $special)) {
            $term[$field_name][LANGUAGE_NONE][0]['value'] = $value;
          }
          else {
            if (is_numeric($value)) {
              $term[$field_name][LANGUAGE_NONE][0]['target_id'] = $value;
            }
            else {
              unset($term[$field_name][LANGUAGE_NONE][0]);
            }
          }
        }

        if (isset($context)) {
          // Build the worked term name list.
          $context['results'][] .= $conf['name'] . ', ';
          $context['message'] = t('Restoring "@title" @operations', array('@title' => $term['name'], '@operations' => $operation_details));
        }
      }
      // Save the term with the new values.
      taxonomy_term_save((object) $term);
    }
  }
}

/**
 * Backup config form submit handler.
 *
 * @param array $form
 *   Form array.
 * @param array $form_state
 *   Form state array.
 */
function nexteuropa_newsroom_backup_conf_all($form, &$form_state) {
  $voc = $form['#vocabulary']->machine_name;
  // We need the whole tree here.
  $tree = taxonomy_get_tree($form['#vocabulary']->vid, 0, NULL, TRUE);

  foreach ($tree as $term) {
    $id = _nexteuropa_newsroom_get_ids($voc, (array) $term);

    // Working with arrays.
    $term = _nexteuropa_newsroom_export_prepare_values($term);
    $term = reset($term);
    $values[$id] = $term;
  }

  // Save the variable.
  variable_set($voc . '_backup', $values);

  // Set a message to the user.
  drupal_set_message(t('We saved a backup of your configuration for the vocabulary: "@name". if you need to restore the values in the backup click on the "Restore" button', array('@name' => $voc)), 'status');
}

/**
 * Gets newsroom id values.
 *
 * @param string $voc
 *   Vocabulary name.
 * @param array $data
 *   Taxonomy term.
 *
 * @return int
 *   Newsroom ID.
 */
function _nexteuropa_newsroom_get_ids($voc, $data) {
  switch ($voc) {
    case NEXTEUROPA_NEWSROOM_TYPE_VOCABULARY:
      $id = $data['field_newsroom_type_id'][LANGUAGE_NONE][0]['value'];
      break;

    case NEXTEUROPA_NEWSROOM_SERVICE_VOCABULARY:
      $id = $data['field_newsroom_service_id'][LANGUAGE_NONE][0]['value'];
      break;

    case NEXTEUROPA_NEWSROOM_TOPIC_VOCABULARY:
      $id = $data['field_newsroom_topic_id'][LANGUAGE_NONE][0]['value'];
      break;
  }
  return $id;
}

/**
 * Grabs the newsroom fields from a term.
 *
 * @param object $term
 *   The term to get the values from.
 *
 * @return array
 *   Formatted values for the newsroom related fields.
 */
function _nexteuropa_newsroom_export_prepare_values($term) {
  $term = get_object_vars($term);
  $voc = $term['vocabulary_machine_name'];
  $id = _nexteuropa_newsroom_get_ids($voc, $term);
  $values = array();
  $values[$id] = array();
  $voc = str_replace('_item', '', $voc);
  foreach ($term as $propname => $prop) {
    // Should work only with custom fields.
    if (strpos($propname, 'field') === 0) {
      if ($propname == 'field_' . $voc . '_id') {
        continue;
      }

      if (isset($prop[LANGUAGE_NONE][0]['value'])) {
        $values[$id][$propname] = $prop[LANGUAGE_NONE][0]['value'];
      }
    }
  }

  return $values;
}

/**
 * Finds a term basing on its newsroom id.
 *
 * @param string $id
 *   The id coming from the newsroom.
 * @param string $voc
 *   Vocabulay's machine name.
 *
 * @return object
 *   The term object.
 */
function _nexteuropa_newsroom_get_term_from_id($id, $voc) {
  $field = str_replace('_item', '', $voc);
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'taxonomy_term')
    ->entityCondition('bundle', $voc)
    ->fieldCondition('field_' . $field . '_id', 'value', $id, '=');

  $results = $query->execute();

  if (!empty($results)) {
    $result = reset($results['taxonomy_term']);
    $tid = $result->tid;
    $term = taxonomy_term_load($tid);
  }
  else {
    $term = FALSE;
  }
  return $term;
}