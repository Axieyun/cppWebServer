### 情况

* mysql服务启动成功
~~~shell
root@Axieyun ~ # service mysqld start
root@Axieyun ~ # service mysql start       
~~~

* 登录报错
~~~shell
root@Axieyun ~ # mysql -uroot -p     
ERROR 2002 (HY000): Can't connect to local MySQL server through socket '/var/run/mysqld/mysqld.sock' (2)
~~~



### 处理

* 查询mysql.sock路径
~~~shell
find / -name "mysql.sock"
~~~
* 查询结果
~~~shell
/tmp/mysql.sock
~~~



* 然后执行
~~~shell
root@Axieyun ~ # ln -s /tmp/mysql.sock /var/run/mysqld/mysqld.sock   
~~~
* 结果
~~~shell
ln: failed to create symbolic link '/var/run/mysqld/mysqld.sock': No such file or directory
~~~


* 创建文件夹
~~~shell
mkdir /var/run/mysqld
~~~

* 创建软连接
~~~shell
ln -s /tmp/mysql.sock /var/run/mysqld/mysqld.sock    
~~~


### 登录
~~~shell
mysql -uroot -p   
~~~


* 结果
~~~shell
Welcome to the MySQL monitor.  Commands end with ; or \g.
Your MySQL connection id is 2
Server version: 5.7.35 MySQL Community Server (GPL)

Copyright (c) 2000, 2021, Oracle and/or its affiliates.

Oracle is a registered trademark of Oracle Corporation and/or its
affiliates. Other names may be trademarks of their respective
owners.

Type 'help;' or '\h' for help. Type '\c' to clear the current input statement.

(root@localhost)[(none)]>
~~~

*登录成功*