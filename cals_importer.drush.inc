<?php

/**
 * Drush commands for cals_importer.
 */
function cals_importer_drush_command() {
  return array(
    'cals-fci-host-repair' => array(
      'description' => 'Report or repair Field Collection host revision mismatches.',
      'options' => array(
        'write' => 'Persist repairs. Default is dry-run.',
        'nid' => array(
          'description' => 'Restrict to a parent node ID.',
          'example-value' => '320595',
        ),
        'item-id' => array(
          'description' => 'Restrict to a Field Collection item ID.',
          'example-value' => '3677889',
        ),
        'limit' => array(
          'description' => 'Limit reported rows.',
          'example-value' => '25',
        ),
      ),
      'examples' => array(
        'drush cals-fci-host-repair --nid=320595' => 'Show mismatches for one node without writing.',
        'drush cals-fci-host-repair --item-id=3677889 --write' => 'Repair one Field Collection item.',
      ),
    ),
  );
}

/**
 * Report or repair mismatched Field Collection host revisions.
 */
function drush_cals_importer_cals_fci_host_repair() {
  $write = (bool) drush_get_option('write', FALSE);
  $nid = drush_get_option('nid');
  $item_id = drush_get_option('item-id');
  $limit = drush_get_option('limit', 0);

  if ($nid !== NULL && (filter_var($nid, FILTER_VALIDATE_INT) === FALSE || (int) $nid < 1)) {
    return drush_set_error('CALS_FCI_REPAIR', 'The --nid value must be a positive integer.');
  }
  if ($item_id !== NULL && (filter_var($item_id, FILTER_VALIDATE_INT) === FALSE || (int) $item_id < 1)) {
    return drush_set_error('CALS_FCI_REPAIR', 'The --item-id value must be a positive integer.');
  }
  if ($limit !== 0 && (filter_var($limit, FILTER_VALIDATE_INT) === FALSE || (int) $limit < 1)) {
    return drush_set_error('CALS_FCI_REPAIR', 'The --limit value must be a positive integer.');
  }
  if ($write && $nid === NULL && $item_id === NULL) {
    return drush_set_error('CALS_FCI_REPAIR', 'Write mode requires --nid or --item-id.');
  }

  $rows = _cals_importer_fci_host_repair_rows($nid, $item_id, $limit);
  drush_print($write ? 'mode=write' : 'mode=dry-run');
  drush_print(count($rows) . ' mismatched row(s) found.');

  $success = 0;
  foreach ($rows as $row) {
    if (_cals_importer_fci_host_repair_one($row, $write)) {
      $success++;
    }
  }

  drush_print($success . ' row(s) processed.');
  return $success === count($rows) ? TRUE : FALSE;
}

/**
 * Find active Field Collection rows whose parent revision is stale.
 */
function _cals_importer_fci_host_repair_rows($nid = NULL, $item_id = NULL, $limit = 0) {
  $sql = "SELECT f.entity_id AS nid,
      f.delta,
      f.field_file_resource_value AS item_id,
      f.field_file_resource_revision_id AS parent_revision,
      i.revision_id AS current_revision
    FROM {field_data_field_file_resource} f
    INNER JOIN {field_collection_item} i ON i.item_id = f.field_file_resource_value
    WHERE f.deleted = 0
      AND i.archived = 0
      AND f.field_file_resource_revision_id IS NOT NULL
      AND f.field_file_resource_revision_id <> i.revision_id";
  $args = array();

  if ($nid !== NULL) {
    $sql .= ' AND f.entity_id = :nid';
    $args[':nid'] = (int) $nid;
  }
  if ($item_id !== NULL) {
    $sql .= ' AND f.field_file_resource_value = :item_id';
    $args[':item_id'] = (int) $item_id;
  }

  $sql .= ' ORDER BY f.entity_id, f.delta';
  if ($limit) {
    $sql .= ' LIMIT ' . (int) $limit;
  }

  return db_query($sql, $args)->fetchAll();
}

/**
 * Process one mismatch, optionally saving it through the parent node.
 */
function _cals_importer_fci_host_repair_one($row, $write) {
  $nid = (int) $row->nid;
  $item_id = (int) $row->item_id;
  $delta = (int) $row->delta;
  $node = node_load($nid, NULL, TRUE);
  $fci = field_collection_item_load($item_id, TRUE);

  if (!$node || !$fci) {
    drush_set_error('CALS_FCI_REPAIR', sprintf('Could not load node %d or Field Collection item %d.', $nid, $item_id));
    return FALSE;
  }
  if ($fci->field_name !== 'field_file_resource' || $fci->archived) {
    drush_set_error('CALS_FCI_REPAIR', sprintf('Skipping item %d: unexpected field or archived item.', $item_id));
    return FALSE;
  }
  if ((int) $fci->revision_id !== (int) $row->current_revision) {
    drush_set_error('CALS_FCI_REPAIR', sprintf('Skipping item %d: revision changed while preparing repair.', $item_id));
    return FALSE;
  }

  $langcode = field_language('node', $node, 'field_file_resource');
  $items = field_get_items('node', $node, 'field_file_resource', $langcode);
  if (empty($items[$delta]) || (int) $items[$delta]['value'] !== $item_id) {
    drush_set_error('CALS_FCI_REPAIR', sprintf('Skipping item %d: node %d delta %d no longer points to it.', $item_id, $nid, $delta));
    return FALSE;
  }

  if (!$write) {
    drush_print(sprintf('WOULD REPAIR nid=%d item=%d langcode=%s delta=%d old_revision=%d new_revision=%d', $nid, $item_id, $langcode, $delta, $row->parent_revision, $row->current_revision));
    return TRUE;
  }

  $node->field_file_resource[$langcode][$delta]['entity'] = $fci;
  try {
    $fci->updateHostEntity($node, 'node');
    if (!node_save($node)) {
      drush_set_error('CALS_FCI_REPAIR', sprintf('Repair failed for item %d: node save returned no ID.', $item_id));
      return FALSE;
    }
  }
  catch (Exception $e) {
    drush_set_error('CALS_FCI_REPAIR', sprintf('Repair failed for item %d: %s', $item_id, $e->getMessage()));
    return FALSE;
  }

  $saved_fci = field_collection_item_load($item_id, TRUE);
  $saved_node = node_load($nid, NULL, TRUE);
  if (!$saved_fci || !$saved_node) {
    drush_set_error('CALS_FCI_REPAIR', sprintf('Post-save verification failed for node %d, item %d.', $nid, $item_id));
    return FALSE;
  }
  $saved_items = field_get_items('node', $saved_node, 'field_file_resource', $langcode);
  if ((int) $saved_fci->hostEntityId() !== $nid || $saved_fci->hostEntityType() !== 'node' || empty($saved_items[$delta]) || (int) $saved_items[$delta]['revision_id'] !== (int) $saved_fci->revision_id) {
    drush_set_error('CALS_FCI_REPAIR', sprintf('Post-save verification failed for node %d, item %d.', $nid, $item_id));
    return FALSE;
  }

  drush_print(sprintf('REPAIRED nid=%d item=%d delta=%d revision=%d', $nid, $item_id, $delta, $saved_fci->revision_id));
  return TRUE;
}
