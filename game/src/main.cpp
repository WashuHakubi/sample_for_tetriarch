/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#include <wut/component.h>
#include <wut/entity.h>

#include <ng-log/logging.h>

int main(int argc, char** argv) {
  FLAGS_logtostdout = true;
  nglog::InitializeLogging(argv[0]);
}
