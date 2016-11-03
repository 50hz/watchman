/* Copyright 2012-present Facebook, Inc.
 * Licensed under the Apache License, Version 2.0 */

#include "watchman.h"

bool root_start(w_root_t* root, char**) {
  root->inner.view->startThreads(root);
  return true;
}

void watchman_root::scheduleRecrawl(const char* why) {
  auto info = recrawlInfo.wlock();

  if (!info->shouldRecrawl) {
    info->lastRecrawlReason = w_string::build(root_path, ": ", why);

    watchman::log(
        watchman::ERR, root_path, ": ", why, ": scheduling a tree recrawl\n");
  }
  info->shouldRecrawl = true;
  signal_root_threads(this);
}

void signal_root_threads(w_root_t *root) {
  root->inner.view->signalThreads();
}

// Cancels a watch.
bool w_root_cancel(w_root_t *root /* don't care about locked state */) {
  bool cancelled = false;

  if (!root->inner.cancelled) {
    cancelled = true;

    w_log(W_LOG_DBG, "marked %s cancelled\n", root->root_path.c_str());
    root->inner.cancelled = true;
    w_cancel_subscriptions_for_root(root);

    signal_root_threads(root);
    remove_root_from_watched(root);
  }

  return cancelled;
}

bool w_root_stop_watch(struct unlocked_watchman_root *unlocked) {
  bool stopped = remove_root_from_watched(unlocked->root);

  if (stopped) {
    w_root_cancel(unlocked->root);
    w_state_save(); // this is what required that we are not locked
  }
  signal_root_threads(unlocked->root);

  return stopped;
}

/* vim:ts=2:sw=2:et:
 */
