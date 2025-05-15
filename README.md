一个基于C/S架构的高性能分布式云存储系统，支持多点并行下载、断点续传和文件完整性校验。

## 项目概述

Cloud Drive是一个分布式云存储解决方案，允许用户安全高效地上传、下载和管理文件。系统采用一个主服务器和多个文件服务器的架构，支持并发用户访问和高效的文件传输功能。

## 核心特性

- **多点并行下载**: 从多个服务器同时下载文件片段，大幅提升传输速度
- **断点续传**: 支持中断后继续传输，无需重新开始
- **文件完整性校验**: 使用MD5算法确保文件完整性
- **智能IO处理**: 针对大文件(>100MB)使用mmap内存映射，小文件使用普通IO
- **高并发处理**: 使用epoll事件驱动模型处理大量并发连接
- **实时传输监控**: 显示文件传输进度和实时网速
- **用户认证系统**: 安全的注册和登录功能
- **类Linux文件操作**: 支持ls、cd、mkdir、rm等命令
- **VIP功能**: 为特定用户提供更高级的服务

## 系统要求

- Linux系统(推荐Ubuntu 18.04+)
- GCC 7.0+
- MySQL 5.7+
- OpenSSL开发库

## 安装指南

### 1. 克隆仓库

```bash
git clone https://github.com/yourusername/cloud_drive.git
cd cloud_drive
```

### 2. 安装依赖项

```bash
sudo apt-get update
sudo apt-get install build-essential libssl-dev mysql-server mysql-client libmysqlclient-dev
```

### 3. 配置MySQL

```bash
# 登录MySQL
sudo mysql -u root -p

# 创建数据库和用户
CREATE DATABASE Netdisk;
CREATE USER 'netdisk'@'localhost' IDENTIFIED BY 'your_password';
GRANT ALL PRIVILEGES ON Netdisk.* TO 'netdisk'@'localhost';
FLUSH PRIVILEGES;
exit;
```

### 4. 编译项目

```bash
# 编译服务器
cd src
make
cd ../server
make
cd ../server1
make
cd ../server2
make

# 编译客户端
cd ../client
make
```

### 5. 配置项目

编辑配置文件 server.conf 和 client.conf 来设置服务器IP、端口和其他参数。

## 使用指南

### 启动服务器

```bash
# 启动主服务器
cd bin
./main_server

# 在不同终端启动文件服务器
./file_server1
./file_server2
./file_server3
```

### 使用客户端

```bash
cd bin
./client
```

### 客户端命令

```
登录: login <username> <password>
注册: register <username> <password>
列出文件: ls
切换目录: cd <directory>
创建目录: mkdir <directory>
删除文件/目录: rm <name>
下载文件: gets <filename>
VIP多点下载: mgets <filename>
上传文件: puts <filename>
查看传输进度: ps
退出: exit
帮助: help
```

## 项目结构

```
bin/              - 编译后的二进制文件
client/           - 客户端代码
  cfactory.c      - 客户端工厂模式实现
  child.c         - 子线程处理文件传输
  client.c        - 主客户端程序
  md5.c           - MD5实现
  more.c          - 多点下载实现
conf/             - 配置文件
include/          - 公共头文件
server/           - 主服务器实现
server1/          - 文件服务器1实现
server2/          - 文件服务器2实现
src/              - 核心代码
  factory.c       - 工厂模式实现
  main.c          - 服务器主程序
  sql.c           - 数据库操作
  tcpInit.c       - 网络初始化
```

## 架构设计

Cloud Drive采用三层架构:

1. **客户端层**: 处理用户交互和本地文件操作
2. **主服务器层**: 处理认证、文件管理和任务分发
3. **文件服务器层**: 处理实际的文件存储和传输

数据通过自定义协议在这三层之间流动，确保安全和高效的文件传输。

## 性能优化

- **文件分段处理**: 大文件被分为多个段进行传输
- **智能IO选择**: 根据文件大小选择最优IO方法
- **缓冲区优化**: 使用优化的缓冲区大小提高传输效率
- **并发连接管理**: 使用epoll高效管理大量连接