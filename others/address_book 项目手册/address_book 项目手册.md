# 通讯录项目

node-v22.11.0    C 语言    MySQL8.0    Vue3

# 一.  C 语言链接数据库

## 1.  安装 MySQL

#### [1]  安装 MySQL 服务器

```shell
# 更新软件包列表
sudo apt update
# 安装 mysql
sudo apt install mysql-server -y
# 安全配置 mysql (遇到 Yes 或 No 都输入 y，遇到配置密码策略选 0 （LOW）)
sudo mysql_secure_installation
```



#### [2]  验证 MySQL 服务状态

```shell
# 验证 mysql 服务状态
sudo systemctl status mysql
# 如果服务未运行，使用以下命令启动它：
sudo systemctl start mysql
# 设置开机自启
sudo systemctl enable mysql
# 检查状态
sudo systemctl status mysql
```



#### [3]  登录 MySQL 数据库

```shell
# 登录 mysql 数据库 (第一次登录没有密码直接敲回车)
sudo mysql -u root -p
# 登录 mysql 进入后可以修改密码
ALTER USER 'root'@'localhost' IDENTIFIED WITH mysql_native_password BY 'new_password';
```



#### [4]  修改密码策略 

默认密码策略不让设置简单的(如123456),  所以要修改密码策略

```shell
# 查看密码策略
show variables like 'validate_password%';
```

粘贴命令时用这个

```shell
set global validate_password.policy=0;set global validate_password.mixed_case_count=0;set global validate_password.number_count=0;set global validate_password.special_char_count=0;set global validate_password.length=6;
```

命令介绍

```shell
-- 密码验证策略低要求(0或LOW代表低级)
set global validate_password.policy=0;

-- 密码至少要包含的小写字母个数和大写字母个数
set global validate_password.mixed_case_count=0;

-- 密码至少要包含的数字个数。
set global validate_password.number_count=0; 

-- 密码至少要包含的特殊字符数
set global validate_password.special_char_count=0; 

-- 密码长度
set global validate_password.length=6;  
```

然后再修改密码

```shell
ALTER USER 'root'@'localhost' IDENTIFIED WITH mysql_native_password BY 'new_password';
```



#### [5]  安装 MySQL 开发库

```shell
# 安装开发库（包含头文件和库文件）
sudo apt install libmysqlclient-dev -y
```

```shell
# 查看头文件位置
ls /usr/include/mysql/mysql.h
# 查看库文件位置
ls /usr/lib/x86_64-linux-gnu/libmysqlclient*
```

![image-20260125160908001](address_book%20%E9%A1%B9%E7%9B%AE%E6%89%8B%E5%86%8C.assets/image-20260125160908001.png)



## 2.  Win10 上的 Navicat 链接 Ubuntu 中的 MySQL

#### [1]  虚拟机网络模式改为桥接模式

#### [2]  获取 Ubuntu 的 IP 地址

```shell
ifconfig
```



#### [3]  修改 mysql 的配置文件允许远程链接

```shell
# 编辑 mysql 配置文件
sudo vim /etc/mysql/mysql.conf.d/mysqld.cnf
```

```shell
# 找到 bind-address
# 原来是：bind-address = 127.0.0.1
# 修改为：bind-address = 0.0.0.0  # 允许所有IP连接
# 或：bind-address = 192.168.1.105  # 只允许指定IP

# 找到 mysqlx-bind-address
# 原来是：mysqlx-bind-address = 127.0.0.1
# 改为：mysqlx-bind-address = 0.0.0.0
```

![image-20260125162522653](address_book%20%E9%A1%B9%E7%9B%AE%E6%89%8B%E5%86%8C.assets/image-20260125162522653.png)

```shell
# 重启 mysql 服务
sudo systemctl restart mysql
# 确认服务正常运行
sudo systemctl status mysql
```



#### [4]  配置防火墙（如果启用）

```shell
# 查看防火墙状态
sudo ufw status
# 如果防火墙启用，开放MySQL端口
sudo ufw allow 3306/tcp
sudo ufw reload
```



#### [5]  在 MySQL 中创建远程连接用户

```shell
# 登录 MySQL（使用root）
mysql -u root -p

############### 创建专门用于远程连接的用户 ###############
# 注意：'%' 表示允许从任何主机连接，也可以指定具体IP
CREATE USER 'jiahao'@'%' IDENTIFIED BY 'jiahao123';  # 用户名: jiahao  密码: jiahao123
# 如果有报密码不安全, 那就先修改一下密码策略, 再创建用户

############### 授予权限（根据需求调整） ################
# 授予所有数据库的所有权限 (给我自己用)
GRANT ALL PRIVILEGES ON *.* TO 'jiahao'@'%' WITH GRANT OPTION;
# 或者只授予特定数据库的权限 (给组员用)
# 授予 address_book 数据库下的所有表
GRANT ALL PRIVILEGES ON address_book.* TO 'jiahao'@'%';

############### 刷新权限 ###########################
FLUSH PRIVILEGES;

############### 查看用户权限 ########################
SELECT user, host FROM mysql.user;
SHOW GRANTS FOR 'jiahao'@'%';
```

![image-20260125164344665](address_book%20%E9%A1%B9%E7%9B%AE%E6%89%8B%E5%86%8C.assets/image-20260125164344665.png)



#### [6]  去 Navicat 上链接数据库

![image-20260125164441194](address_book%20%E9%A1%B9%E7%9B%AE%E6%89%8B%E5%86%8C.assets/image-20260125164441194.png)



```c
#include <stdio.h>
#include <mysql/mysql.h>

int main(void)
{
	MYSQL *conn;
	const char *server = "localhost";
	const char *user = "jiahao";
	const char *password = "jiahao123";
	const char *database = "address_book";

	printf("=== Ubuntu虚拟机C程序连接MySQL测试 ===\n");

	// 1. 初始化连接
	conn = mysql_init(NULL);
	if (conn == NULL)
	{
		fprintf(stderr, "mysql_init() 失败\n");
		return 1;
	}

	// 2. 连接到数据库
	printf("正在连接到MySQL服务器...\n");
	if (mysql_real_connect(conn, server, user, password, database, 0, NULL, 0) == NULL)
	{
		fprintf(stderr, "连接失败: %s\n", mysql_error(conn));
		mysql_close(conn);
		return 1;
	}

	printf("成功连接到数据库: %s\n", database);

	// 3. 设置字符集为 UTF-8
	if (mysql_set_character_set(conn, "utf8"))
		fprintf(stderr, "设置字符集失败: %s\n", mysql_error(conn));
	else
		printf("字符集已设置为: %s\n", mysql_character_set_name(conn));

	// 4. 执行简单查询
	printf("\n执行查询...\n");
	if (mysql_query(conn, "select * from contacts"))
		fprintf(stderr, "查询失败: %s\n", mysql_error(conn));
	else
	{
		MYSQL_RES *result = mysql_store_result(conn);
		if (result == NULL)
			fprintf(stderr, "获取结果失败: %s\n", mysql_error(conn));
		else
		{
			int num_fields = mysql_num_fields(result);  // 获取字段个数
			MYSQL_ROW row = NULL;

			printf("查询结果 (%d 列):\n", num_fields);
			printf("ID\t姓名\t年龄\t邮箱\n");
			printf("---------------------------\n");
			while ((row = mysql_fetch_row(result)))
			{
				for (int i = 0; i < num_fields; i++)
				{
					printf("%s\t", row[i] ? row[i] : "NULL");
				}
				printf("\n");
			}
			mysql_free_result(result);
		}
	}

	// 5. 执行一个插入操作测试（可选）
	/*if(mysql_query(conn,"insert into students (name,age,email) values ('测试用户', 25, 'test@example.com')"))
		fprintf(stderr, "插入失败: %s\n", mysql_error(conn));
	else 
		printf("插入成功，影响行数: %lld\n", (long long)mysql_affected_rows(conn));

	printf("\n程序执行完成，连接已关闭\n");*/

	// 5. 关闭连接
	mysql_close(conn);

	return 0;
}
```



更换 minio 地址 sql

```sql
UPDATE contacts 
SET image = REPLACE(image, '10.11.17.141', '192.168.2.7')
```



makefile

```makefile
address_book : main.o contact.o contact_llist.o db_mysql.o utils.o minio_server.o http_server.o
	gcc -o $@ $^ -lmysqlclient -lmicrohttpd -lcurl -lpthread
clean :
	rm -rf *.o address_book
```



# 二.  C 和 Vue 前后端交互

```shell
sudo apt install libmysqlclient-dev -y # mysql 库文件
sudo apt-get install libjansson-dev -y  # json 库文件
sudo apt-get install libmicrohttpd-dev -y  # http 库文件
sudo apt-get install libcurl4-openssl-dev -y  # curl 库文件

sudo apt install curl  # curl命令, 调试用
```



```sql
# 创建联系人表
create table contacts(
	id int primary key auto_increment,
	name varchar(20) not NULL,
	telephone varchar(12) not NULL,
	email varchar(50),
	initial char(1) not NULL,
	imgae varchar(255),
	del int not NULL,
	created_at timestamp default CURRENT_TIMESTAMP,
  updated_at timestamp default CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
	index idx_name (name),
	index idx_telephone (telephone)
);
```





# 三.  Nginx 反向代理

让项目实现跨局域网

#### 安装 Nginx

```shell
# 安装 nginx
sudo apt update
sudo apt install nginx -y
# 查看安装版本
nginx -v
```

#### 启动 nginx

```shell
# 启动 nginx
sudo systemctl start nginx
# 开机自启
sudo systemctl enable nginx
# 重启 nginx
sudo systemctl restart nginx
# 查看状态
sudo systemctl status nginx
```



配置 Nginx 反向代理

```shell
# cd 到 nginx 的配置目录下
cd /etc/nginx/sites-available/
# 备份 default 文件, 备份完剪切到家目录去
sudo cp default default.bak 
# 编辑 default 配置文件
sudo vim default
# 测试 nginx 配置 (有 successful 就成功了)
sudo nginx -t
# 重新加载配置文件，但不停止服务
sudo systemctl reload nginx
# 或者使用 nginx 自带的命令
sudo nginx -s reload
# 检查 nginx 状态
sudo systemctl status nginx
```



```shell



# 3. 测试配置
sudo nginx -t
```





