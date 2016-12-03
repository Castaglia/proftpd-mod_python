/*
 * ProFTPD - mod_python: Interpreter
 * Copyright (c) 2016 TJ Saunders
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA.
 *
 * As a special exemption, TJ Saunders and other respective copyright holders
 * give permission to link this program with OpenSSL, and distribute the
 * resulting executable, without including the source code for OpenSSL in the
 * source distribution.
 */

#include "mod_python.h"
#include "interp.h"

/* Undefine this macro to avoid warnings due to unconditional (re)defining
 * of the same macro by the Python headers.
 */
#ifdef _GNU_SOURCE
# undef _GNU_SOURCE
#endif
#include <Python.h>

static const char *trace_channel = "python.interp";

/* Log various Pythonic info. */
static void log_python_info(void) {
  const char *info;

  if (pr_trace_get_level(trace_channel) < 7) {
    return;
  }

  info = Py_GetPrefix();
  if (info != NULL) {
    pr_trace_msg(trace_channel, 7, "Prefix: %s", info);
  }

  info = Py_GetExecPrefix();
  if (info != NULL) {
    pr_trace_msg(trace_channel, 7, "Exec prefix: %s", info);
  }

  info = Py_GetProgramFullPath();
  if (info != NULL) {
    pr_trace_msg(trace_channel, 7, "Python path: %s", info);
  }

  info = Py_GetPythonHome();
  if (info != NULL) {
    pr_trace_msg(trace_channel, 7, "Python home: %s", info);
  }

  info = Py_GetPath();
  if (info != NULL) {
    pr_trace_msg(trace_channel, 7, "Module path: %s", info);
  }

  info = Py_GetVersion();
  if (info != NULL) {
    pr_trace_msg(trace_channel, 7, "Version: %s", info);
  }

  info = Py_GetPlatform();
  if (info != NULL) {
    pr_trace_msg(trace_channel, 7, "Platform: %s", info);
  }

  info = Py_GetCompiler();
  if (info != NULL) {
    pr_trace_msg(trace_channel, 7, "Compiler: %s", info);
  }

  info = Py_GetBuildInfo();
  if (info != NULL) {
    pr_trace_msg(trace_channel, 7, "Build info: %s", info);
  }
}

int python_interp_init(void) {
  int init_sigs = 0;

  if (Py_IsInitialized()) {
    pr_trace_msg(trace_channel, 19, "Python interpreter already initialized");
    return 0;
  }

  /* We do NOT want Python handling signals; we will handle signals
   * ourselves.
   */
  Py_InitializeEx(init_sigs);

  if (!Py_IsInitialized()) {
    errno = EPERM;
    return -1;
  }

  log_python_info();
  return 0;
}

int python_interp_free(void) {
  if (Py_IsInitialized()) {
    pr_trace_msg(trace_channel, 19, "freeing Python interpreter");
    Py_Finalize();

  } else {
    pr_trace_msg(trace_channel, 19, "Python interpreter not initialized");
  }

  return 0;
}
