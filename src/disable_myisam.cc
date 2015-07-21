/* Copyright (c) 2006, 2010, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */
#define MYSQL_SERVER

#include <mysql_version.h>
#include <mysql/plugin.h>
#include "mysqld.h"
#include "handler.h"
#include "sql_plugin.h"                         // st_plugin_int
#include "log.h"
#include <set>
//#include "sp.h"
//#include "table_cache.h"
// Fix when building for MySQL 5.5
#ifndef SQL_STRING_INCLUDED
#include "sql_string.h"
#endif

#include "../storage/myisam/ha_myisam.h"
/*
  Disable __attribute__() on non-gcc compilers.
*/
#if !defined(__attribute__) && !defined(__GNUC__)
#define __attribute__(A)
#endif

class ha_myisam_disable_myisam_wrapper;
static int disable_myisam_plugin_init(void *p);
static int disable_myisam_plugin_deinit(void *p);
static handler* new_myisam_create(handlerton *hton, TABLE_SHARE *table, MEM_ROOT *mem_root);

static my_bool allow_sys_value;
static handler* (*old_myisam_create)(handlerton*, TABLE_SHARE*, MEM_ROOT*) = NULL;
static mysql_mutex_t LOCK_set;
static std::set<ha_myisam_disable_myisam_wrapper*> cur_handlers;


static MYSQL_SYSVAR_BOOL(allow_sys, allow_sys_value,
  PLUGIN_VAR_RQCMDARG,
  "Allow creating MYISAM tables in mysql and tmp databases",
  NULL, NULL, 0);

static struct st_mysql_sys_var* system_variables[]= {
  MYSQL_SYSVAR(allow_sys),
  NULL
};

class ha_myisam_disable_myisam_wrapper : public ha_myisam {
public:
  ha_myisam_disable_myisam_wrapper(handlerton *hton, TABLE_SHARE *table_arg)
    :ha_myisam(hton, table_arg) {
    mysql_mutex_lock(&LOCK_set);
    cur_handlers.insert(this);
    mysql_mutex_unlock(&LOCK_set);
  }

  virtual ~ha_myisam_disable_myisam_wrapper() {
    mysql_mutex_lock(&LOCK_set);
    cur_handlers.erase(this);
    mysql_mutex_unlock(&LOCK_set);
  }

  int create(const char *name, TABLE *form, HA_CREATE_INFO *create_info) {
    const char *MYSQL_DATABASE_PREFIX = "/mysql/";
    const char *TMP_DATABASE_PREFIX   ="/tmp/";
    const int FAIL = 1;
    DBUG_ENTER("ha_myisam_disable_myisam_wrapper::create");
    if (allow_sys_value) {
      if (!(strstr(name, MYSQL_DATABASE_PREFIX) || strstr(name, TMP_DATABASE_PREFIX))) {
        DBUG_RETURN(FAIL);
      }
      // delegate to the origin function
      int ret = ha_myisam::create(name, form, create_info);
      DBUG_RETURN(ret);
    }
    DBUG_RETURN(FAIL);
  }
  
  TABLE *get_table() {
    return table;
  }
};

static handler* new_myisam_create(handlerton *hton, TABLE_SHARE *table, MEM_ROOT *mem_root) {
  handler *ret = new (mem_root) ha_myisam_disable_myisam_wrapper(hton, table);
  return ret;
}

/*
  SYNOPSIS
    disable_myisam_plugin_init()

  RETURN VALUE
    0                    success
    1                    failure (cannot happen)
*/
static int disable_myisam_plugin_init(void *p)
{
  DBUG_ENTER("disable_myisam_plugin_init");
  if (myisam_hton->create != new_myisam_create) {
    old_myisam_create = myisam_hton->create;
    myisam_hton->create = new_myisam_create;
  }
  DBUG_RETURN(0);
}

/*

  SYNOPSIS
    disable_myisam_plugin_deinit()

  RETURN VALUE
    0                    success
    1                    failure (cannot happen)

*/
static int disable_myisam_plugin_deinit(void *p)
{
  DBUG_ENTER("disable_myisam_plugin_deinit");
  /*
   Hack handler's vtable to the origin ha_myisam.
   Openned myisam tables after plugin loaded carries a handler point 
   to an ha_myisam_disable_myisam_wrapper which will be unaccessable
   after the plugin uninstalled and cause crash. 
   While no myisam table is openned before the uninstall operation, 
   crash also occurred because of uninstall operation will access the
   mysql.plugin table.
  */
  handler *old = new ha_myisam(NULL, NULL); // just used to fetch vtable address
  mysql_mutex_lock(&LOCK_set);
  for (std::set<ha_myisam_disable_myisam_wrapper*>::iterator it = cur_handlers.begin(); it != cur_handlers.end(); ++it) {
    if ((*it)->get_table()) {
      void **vtn = (void**)(*it);
      void **vto = (void**)old;
      *vtn = *vto;
    }
  }
  mysql_mutex_unlock(&LOCK_set);
  delete old;

  if (old_myisam_create != NULL) {
    myisam_hton->create = old_myisam_create;
  }
  DBUG_RETURN(0);
}

struct st_mysql_daemon disable_myisam_plugin=
{ MYSQL_DAEMON_INTERFACE_VERSION  };

/*
  Plugin library descriptor
*/

mysql_declare_plugin(disable_myisam)
{
  MYSQL_DAEMON_PLUGIN,
  &disable_myisam_plugin,
  "disable_myisam",
  "Xie Zhenye",
  "Disable creating new MYISAM tables",
  PLUGIN_LICENSE_GPL,
  disable_myisam_plugin_init, /* Plugin Init */
  disable_myisam_plugin_deinit, /* Plugin Deinit */
  0x0100 /* 1.0 */,
  NULL,                       /* status variables                */
  system_variables,                       /* system variables                */
  NULL,                       /* config options                  */
  0,                          /* flags                           */
}
mysql_declare_plugin_end;

