mysql-plugin-disable-myisam
===========================

MySQL plugin to disable creating MYISAM tables.

usage
-----

first, compile the plugin and install in to plugin dir

    cp -r src /path/to/mysql-src/plugin/disable_myisam
    cd /path/to/mysql-src
    cmake . -DBUILD_CONFIG=mysql_release
    cd plugin/disable_myisam
    make
    make install
    
CAUTION: mysql plugins MUST be built using the same version of the source code and the same build arguments. If mysqld is built as a debug version without cmake parameter -DBUILD_CONFIG, the parameter must not be added when compiling plugins.
    
then, load the plugin into mysql

    mysql> INSTALL PLUGIN DISABLE_MYISAM SONAME 'disable_myisam.so';
    
MYISAM tables can not be created now. 

    mysql> CREATE TABLE `test4` (
      `id` int(11) AUTO_INCREMENT,
      `value` varchar(30),
      PRIMARY KEY (`id`)
    ) ENGINE=MyISAM;
    ERROR 1030 (HY000): Got error 1 from storage engine
    
There is also a global variable disable_myisam_allow_sys to control whether creating MYISAM table in system databases like mysql or tmp is allowed.

    mysql> show variables like '%allow_sys%';
    +--------------------------+-------+
    | Variable_name            | Value |
    +--------------------------+-------+
    | disable_myisam_allow_sys | OFF   |
    +--------------------------+-------+
    1 row in set (0.00 sec)
    
    mysql> use mysql;
    Database changed
    mysql> create table x(id int primary key)engine=myisam;
    ERROR 1030 (HY000): Got error 1 from storage engine
    mysql> set global disable_myisam_allow_sys=1;
    Query OK, 0 rows affected (0.00 sec)
    
    mysql> show variables like '%allow_sys%';
    +--------------------------+-------+
    | Variable_name            | Value |
    +--------------------------+-------+
    | disable_myisam_allow_sys | ON    |
    +--------------------------+-------+
    1 row in set (0.00 sec)

    mysql> create table x(id int primary key)engine=myisam;
    Query OK, 0 rows affected (0.00 sec)
    mysql> use test
    Database changed
    mysql> create table x(id int primary key)engine=myisam;
    ERROR 1030 (HY000): Got error 1 from storage engine
    

