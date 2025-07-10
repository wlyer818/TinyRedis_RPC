#ifndef REDIS_SERVER_H
#define REDIS_SERVER_H
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <future>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <unistd.h>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <signal.h>
#include<fcntl.h>
#include <cstring> 
#include "ParserFlyweightFactory.h"
#include <queue>
#include <string>
using namespace std;

//单例模式，局部静态变量懒汉模式，体现在getInstance()静态成员函数中
class RedisServer {
private:
    std::unique_ptr<ParserFlyweightFactory> flyweightFactory; // 解析器工厂
    int port;
    std::atomic<bool> stop{false};
    pid_t pid; 
    std::string logoFilePath;
    bool startMulti = false;  //是否已开启事务
    bool fallback = false;  //是否需要回滚
    std::queue<std::string>commandsQueue;//事物指令队列，存储一条条的命令：{"set a 1", "get a", "del a"}

private:
    //构造函数声明为私有，防止外部创建对象
    RedisServer(int port = 5555, const std::string& logoFilePath = "logo");
    static void signalHandler(int sig);  //信号处理函数，当Ctrl+C终止程序时，执行signalHandler函数，将未保存的数据写入到文件中
    void printLogo(); //打印logo
    void printStartMessage(); //打印Redisserver的启动信息
    void replaceText(std::string &text, const std::string &toReplaceText, const std::string &replaceText); //替换字符串，用于printLogo()和printStartMessage()函数
    std::string getDate(); //获取当前时间，例如：2021-07-01 12:00:00
    string executeTransaction(std::queue<std::string>&commandsQueue);
public:
    string handleClient(string receivedData);
    static RedisServer* getInstance();  //静态成员函数，获取单例
    void start();
};

#endif 
