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

 /* Log various Pythonic info. */
 pr_trace_msg(trace_channel, 7, "prefix: %s", Py_GetPrefix());
 pr_trace_msg(trace_channel, 7, "exec prefix: %s", Py_GetExecPrefix());
 pr_trace_msg(trace_channel, 7, "python path: %s", Py_GetProgramFullPath());
 pr_trace_msg(trace_channel, 7, "python home: %s", Py_GetPythonHome());
 pr_trace_msg(trace_channel, 7, "module path: %s", Py_GetPath());
 pr_trace_msg(trace_channel, 7, "version: %s", Py_GetVersion());
 pr_trace_msg(trace_channel, 7, "platform: %s", Py_GetPlatform());
 pr_trace_msg(trace_channel, 7, "compiler: %s", Py_GetCompiler());
 pr_trace_msg(trace_channel, 7, "build info: %s", Py_GetBuildInfo());

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
