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

#include <mysql_version.h>
#include <mysql/plugin.h>

#include "mysqld.h"
#include "handler.h"
#include "sql_plugin.h"                         // st_plugin_int
#include "table.h"

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

/*
  Name of default MySQL database (containing users, permissions, etc.)
  Note that compare in create function is done case-sensitive
*/
#define DEFAULT_MYSQL_DATABASE_NAME "mysql" 


static handler* (*old_myisam_create)(handlerton*, TABLE_SHARE*, MEM_ROOT*);


class ha_myisam_wrap :public ha_myisam {
public:
  ha_myisam_wrap(handlerton *hton, TABLE_SHARE *table_arg)
    :ha_myisam(hton, table_arg) {
  }

  int create(const char *name, TABLE *form, HA_CREATE_INFO *create_info) {
    DBUG_ENTER("ha_myisam_wrap::create");

    // avoid breaking manipulations in database containing users, permissions, etc.
    char *buf = (char *)malloc((strlen(DEFAULT_MYSQL_DATABASE_NAME)+3)*sizeof(char));

    // we will be looking for /database/ as name looks like ./database/table
    strcpy(buf, "/");
    strcat(buf, DEFAULT_MYSQL_DATABASE_NAME);
    strcat(buf, "/");

    // disallow creation of table if not in DEFAULT_MYSQL_DATABASE_NAME
    if (!strstr(name, buf)) { return 1; }

    // free memory
    free(buf);

    // allow table creation
    int ret = ha_myisam::create(name, form, create_info);

    // not sure about this one
    DBUG_RETURN(ret);
  }
};


static handler* new_myisam_create(handlerton *hton, TABLE_SHARE *table, MEM_ROOT *mem_root) {
  return new (mem_root) ha_myisam_wrap(hton, table);
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
  old_myisam_create = myisam_hton->create;
  myisam_hton->create = new_myisam_create;
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
  myisam_hton->create = old_myisam_create;
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
  NULL,                       /* system variables                */
  NULL,                       /* config options                  */
  0,                          /* flags                           */
}
mysql_declare_plugin_end;

