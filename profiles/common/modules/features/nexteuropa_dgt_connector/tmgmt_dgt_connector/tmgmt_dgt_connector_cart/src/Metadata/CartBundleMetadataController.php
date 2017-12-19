<?php

namespace Drupal\tmgmt_dgt_connector_cart\Metadata;

use EntityDefaultMetadataController;

/**
 * DGT FTT Translator mapping entity.
 */
class CartBundleMetadataController extends EntityDefaultMetadataController {
  public function entityPropertyInfo() {
    $info = parent::entityPropertyInfo();
    $properties = &$info[$this->type]['properties'];
    $properties['uid'] = array(
      'label' => t('User ID'),
      'schema field' => 'uid',
      'getter callback' => 'entity_property_getter_method',
      'setter callback' => 'entity_property_verbatim_set',
      'required' => TRUE,
      'description' => t('ID of the author.'),
    );
    $properties['tjid'] = array(
      'label' => t('Translation job ID'),
      'schema field' => 'tjid',
      'getter callback' => 'entity_property_getter_method',
      'setter callback' => 'entity_property_verbatim_set',
      'required' => FALSE,
      'description' => t('Related translation job ID.'),
    );
    $properties['target_languages'] = array(
      'label' => t('Target languages'),
      'schema field' => 'target_languages',
      'getter callback' => 'entity_property_getter_method',
      'setter callback' => 'entity_property_verbatim_set',
      'required' => TRUE,
      'description' => t('Target languages of the bundle.'),
    );
    $properties['status'] = array(
      'label' => t('Status'),
      'schema field' => 'status',
      'getter callback' => 'entity_property_getter_method',
      'setter callback' => 'entity_property_verbatim_set',
      'required' => TRUE,
      'description' => t('Status of the bundle.'),
    );
    $properties['created'] = array(
      'label' => t('Creation date'),
      'schema field' => 'created',
      'getter callback' => 'entity_property_getter_method',
      'setter callback' => 'entity_property_verbatim_set',
      'required' => TRUE,
      'description' => t('Creation date of the bundle.'),
    );
    $properties['updated'] = array(
      'label' => t('training updated'),
      'schema field' => 'Updated date',
      'getter callback' => 'entity_property_getter_method',
      'setter callback' => 'entity_property_verbatim_set',
      'required' => TRUE,
      'description' => t('Updated date of the bundle.'),
    );

    return $info;
  }
}