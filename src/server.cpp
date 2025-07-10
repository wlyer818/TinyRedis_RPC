#include "RedisServer.h"
#include "buttonrpc.hpp"

int main() {
    //创建一个rpc server
    buttonrpc server;  
    server.as_server(5555); //监听5555端口
    //server.bind("redis_command", redis_command);
    RedisServer::getInstance()->start(); //启动RedisServer，打印logo和启动信息

//”redis_command“ 是m_handlers中的key, handleClient是命令处理函数，RedisServer::getInstance()是RedisServer实例
    server.bind("redis_command", &RedisServer::handleClient, RedisServer::getInstance());
   // std::cout << "run rpc server on: " << 5555 << std::endl;
    server.run();  //启动 server，循环处理客户端的远程命令，并返回序列化的执行结果


    RedisServer::getInstance()->start();
}
