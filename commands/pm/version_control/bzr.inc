<?php
// $Id$

/**
 * @file Drush pm BZR extension
 */

class drush_pm_version_control_bzr implements drush_pm_version_control {
  
  /**
   * Helper function: get the root of a repository path.
   *
   * @param $path
   *   The path to check.
   * @return
   *   An absolute path to the repository root.
   */
  public function bzr_get_repository_root($path) {
    if (drush_shell_exec('bzr root %s', $path)) {
      $output = drush_shell_exec_output();
      return reset($output);
    }
  }

  /**
   * Implementation of pre_update().
   */
  public function pre_update(&$release) {
    // TODO: Add similar checks to SVN?
  }

  /**
   * Implementation of post_update().
   */
  public function post_update($release) {
    if (is_versioned($release['full_project_path']) && sync($release)) {
      // Only attempt commit on a sucessful sync
      commit($release);
    }
  }
  
  /**
   * Implementation of post_install().
   */
  public function post_install($release) {
    if (is_versioned($release['full_project_path']) && sync($release)) {
      // Only attempt commit on a sucessful sync
      commit($release);
    }
  }
  
  /**
   * Automatically add any unversioned files to Bzr and remove any files
   * that have been deleted on the file system
   */
  private function sync($release) {
    if (drush_get_option('bzrsync')) {
      $errors = '';
  
      // All paths returned by bzr status are relative to the repository root.
      $root = bzr_get_repository_root($release['full_project_path']);
  
      if (drush_shell_exec('bzr status -S %s', $release['full_project_path'])) {
        $output = drush_shell_exec_output();
  
        // Parse the output of bzr status to get the list of added and removed
        // files.
        $new_files = array();
        $removed_files = array();
        foreach ($output as $line) {
          if (preg_match('/^\? *(.*)/', $line, $matches)) {
            $new_files[] = $root . '/' . $matches[1];
          }
          if (preg_match('/^\ D *(.*)/', $line, $matches)) {
            $removed_files[] = $root . '/' . $matches[1];
          }
        }
  
        // Proceed adding new files.
        foreach (array_chunk($new_files, 100) as $chunk) {
          $chunk = array_map('escapeshellarg', $chunk);
          if (!drush_shell_exec('bzr add ' . implode(' ', $chunk))) {
            $errors .= implode("\n", drush_shell_exec_output());
          }
        }
  
        // Proceed removing old files.
        foreach (array_chunk($removed_files, 100) as $chunk) {
          $chunk = array_map('escapeshellarg', $chunk);
          if (!drush_shell_exec('bzr remove ' . implode(' ', $chunk))) {
            $errors .= implode("\n", drush_shell_exec_output());
          }
        }
  
        if (!empty($errors)) {
          return drush_set_error('DRUSH_PM_BZR_SYNC_PROBLEMS', dt("Problems were encountered adding or removing files to/from Bazaar.\nThe specific errors are below:\n!errors", array('!errors' => $errors)));
        }
      }
      else {
        return drush_set_error('DRUSH_PM_BZR_NOT_FOUND', dt("Drush was unable to get the bzr status. Check that you have Bazaar \ninstalled and that the site is a Bazaar working copy.\nThe specific errors are below:\n!errors", array('!errors' => implode("\n", drush_shell_exec_output()))));
      }
      return TRUE;
    }
  }
  
  /**
   * Automatically commit changes to the repository
   */
  private function commit($release) {
    if (drush_get_option('bzrcommit')) {
      $message = drush_get_option('bzrmessage');
      if (empty($message)) {
        $message = dt("Drush automatic commit: \n") . implode(' ', $_SERVER['argv']);
      }
      if (drush_shell_exec('bzr commit -m %s %s', $message, $release['full_project_path'])) {
        drush_log(dt('Project committed to Bazaar successfully'), 'ok');
      }
      else {
        drush_set_error('DRUSH_PM_BZR_COMMIT_PROBLEMS', dt("Problems were encountered committing your changes to Bazaar.\nThe specific errors are below:\n!errors", array('!errors' => implode("\n", drush_shell_exec_output()))));
      }
    }
    else {
      drush_print(dt("You should consider committing the new code to your Bazaar repository.\nIf this version becomes undesireable, use Bazaar to roll back."));
    }
  }

}