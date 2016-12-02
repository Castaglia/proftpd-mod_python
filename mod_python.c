/*
 * ProFTPD - mod_python
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
 *
 * -----DO NOT EDIT BELOW THIS LINE-----
 * $Archive: mod_python.a $
 */

#include "mod_python.h"
#include "privs.h"

extern xaset_t *server_list;

/* From response.c */
extern pr_response_t *resp_list, *resp_err_list;

module python_module;

int python_logfd = -1;
unsigned long python_opts = 0UL;

static int python_engine = FALSE;
static const char *python_logfile = NULL;
static pool *python_pool = NULL;

static const char *trace_channel = "python";

static void open_logfile(void) {
  int res, xerrno;

  if (python_logfile == NULL) {
    return;
  }

  pr_signals_block();
  PRIVS_ROOT
  res = pr_log_openfile(python_logfile, &python_logfd, PR_LOG_SYSTEM_MODE);
  xerrno = errno;
  PRIVS_RELINQUISH
  pr_signals_unblock();

  if (res < 0) {
    if (res == -1) {
      pr_log_pri(PR_LOG_NOTICE, MOD_PYTHON_VERSION
        ": notice: unable to open PythonLog '%s': %s", python_logfile,
        strerror(xerrno));

    } else if (res == PR_LOG_WRITABLE_DIR) {
      pr_log_pri(PR_LOG_NOTICE, MOD_PYTHON_VERSION
        ": notice: unable to open PythonLog '%s': parent directory is "
        "world-writable", python_logfile);

    } else if (res == PR_LOG_SYMLINK) {
      pr_log_pri(PR_LOG_NOTICE, MOD_PYTHON_VERSION
        ": notice: unable to open PythonLog '%s': cannot log to a symlink",
        python_logfile);
    }
  }
}

/* Configuration handlers
 */

/* usage: PythonConnectHandler script */
MODRET set_pythonconnecthandler(cmd_rec *cmd) {
  return PR_HANDLED(cmd);
}

/* usage: PythonEngine on|off */
MODRET set_pythonengine(cmd_rec *cmd) {
  int engine = 1;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT);

  engine = get_boolean(cmd, 1);
  if (engine == -1) {
    CONF_ERROR(cmd, "expected Boolean parameter");
  }

  python_engine = engine;
  return PR_HANDLED(cmd);
}

/* usage: PythonLog path|"none" */
MODRET set_pythonlog(cmd_rec *cmd) {
  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT);

  if (python_logfile != NULL) {
    (void) close(python_logfd);
    python_logfd = -1;
  }

  python_logfile = pstrdup(python_pool, cmd->argv[1]);
  return PR_HANDLED(cmd);
}

/* usage: PythonLogHandler script */
MODRET set_pythonloghandler(cmd_rec *cmd) {
  return PR_HANDLED(cmd);
}

/* Event handlers
 */

#if defined(PR_SHARED_MODULE)
static void python_mod_unload_ev(const void *event_data, void *user_data) {
  if (strcmp((const char *) event_data, "mod_python.c") == 0) {
    /* Unregister ourselves from all events and timers. */
    pr_event_unregister(&python_module, NULL, NULL);
    pr_timer_remove(-1, &python_module);

    destroy_pool(python_pool);
    python_pool = NULL;

    (void) close(python_logfd);
    python_logfd = -1;
  }
}
#endif /* PR_SHARED_MODULE */

static void python_restart_ev(const void *event_data, void *user_data) {
  destroy_pool(python_pool);
  python_pool = make_sub_pool(permanent_pool);
  pr_pool_tag(python_pool, MOD_PYTHON_VERSION);

  (void) close(python_logfd);
  python_logfd = -1;
  open_logfile();
}

static void python_shutdown_ev(const void *event_data, void *user_data) {
  destroy_pool(python_pool);
  python_pool = NULL;

  (void) close(python_logfd);
  python_logfd = -1;
}

static void python_startup_ev(const void *event_data, void *user_data) {
  if (python_engine == FALSE) {
    return;
  }

  open_logfile();
}

/* XXX Do we want to support any Controls/ftpctl actions? */

/* Initialization routines
 */

static int python_init(void) {
  python_pool = make_sub_pool(permanent_pool);
  pr_pool_tag(python_pool, MOD_PYTHON_VERSION);

#if defined(PR_SHARED_MODULE)
  pr_event_register(&python_module, "core.module-unload", python_mod_unload_ev,
    NULL);
#endif
  pr_event_register(&python_module, "core.restart", python_restart_ev, NULL);
  pr_event_register(&python_module, "core.shutdown", python_shutdown_ev, NULL);
  pr_event_register(&python_module, "core.startup", python_startup_ev, NULL);

  return 0;
}

static int python_sess_init(void) {
  config_rec *c;

  if (python_engine == FALSE) {
    return 0;
  }

  /* Remove all timers registered during e.g. startup; we only want those
   * timers firing in the daemon process, not in session processes.
   */
  pr_timer_remove(-1, &python_module);

  return 0;
}

/* Module API tables
 */

static conftable python_conftab[] = {
  { "PythonConnectHandler",	set_pythonconnecthandler,	NULL },
  { "PythonEngine",		set_pythonengine,		NULL },
  { "PythonLog",		set_pythonlog,			NULL },
  { "PythonLogHandler",		set_pythonloghandler,		NULL },

  { NULL }
};

module python_module = {
  /* Always NULL */
  NULL, NULL,

  /* Module API version */
  0x20,

  /* Module name */
  "python",

  /* Module configuration handler table */
  python_conftab,

  /* Module command handler table */
  NULL,

  /* Module authentication handler table */
  NULL,

  /* Module initialization */
  python_init,

  /* Session initialization */
  python_sess_init,

  /* Module version */
  MOD_PYTHON_VERSION
};
