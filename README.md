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
    
then, load the plugin into mysql

    mysql> INSTALL PLUGIN DISABLE_MYISAM SONAME 'dm.so';
    
MYISAM tables can not be created now. 

    mysql> CREATE TABLE `test4` (
      `id` int(11) AUTO_INCREMENT,
      `value` varchar(30),
      PRIMARY KEY (`id`)
    ) ENGINE=MyISAM;
    ERROR 1030 (HY000): Got error 1 from storage engine
    
    
