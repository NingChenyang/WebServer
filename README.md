# WebServer

一个基于C++11的高性能网络服务器，支持HTTP/1.1协议和WebSocket协议，提供静态文件服务和API接口。

## 主要功能

- ✅ HTTP/1.1协议支持
- ✅ WebSocket全双工通信
- ✅ 静态文件服务（HTML/CSS/JS）
- ✅ 用户认证系统（登录/注册/注销）
- ✅ MySQL连接池管理
- ✅ 多线程并发处理
- ✅ 请求缓存控制
- ✅ 信号安全退出
- ✅ 集成性能测试工具（WebBench）

## 技术特点

1. **高性能架构**
   - Reactor事件驱动模型
   - 多线程 + 线程池设计
   - Epoll边缘触发优化
   - 非阻塞I/O操作

2. **安全可靠**
   - MySQL连接池自动回收
   - Cookie-based会话管理
   - 请求报文完整性校验
   - 优雅退出信号处理

3. **易扩展性**
   - 模块化设计（TCP/HTTP/WebSocket分离）
   - 支持自定义路由处理
   - 配置驱动部署（JSON配置支持）

## 快速部署

```bash
# 1. 编译项目
make

# 2. 创建必要目录
mkdir -p logs www/upload

# 3. 运行HTTP服务器（端口8888）
./server

# 4. 运行WebSocket服务器（端口9999）
./WS_Server
```
## MySQL数据库配置
在`config.json`文件中配置MySQL数据库连接信息：
```json
{
    "host": "localhost",
    "user": "webuser",
    "password": "securepass",
    "database": "webdb",
    "poolSize": 8,
    "timeout": 5
}
```
