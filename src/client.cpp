#include <iostream>
#include <string>
#include "buttonrpc.hpp"
using namespace std;
int main() {
    string hostName = "127.0.0.1";
    int port = 5555;

    buttonrpc client;
    client.as_client(hostName, port); //将client设置为rpc客户端，连接到指定的主机和端口
    client.set_timeout(2000);  //设置超时时间为2000ms

    string message;
    while(true){
        //发送数据
        std::cout << hostName << ":" << port << "> ";
        std::getline(std::cin, message);
        /*
        call<string>函数会调用string类型的net_call函数，net_call函数返回value_t<string>类型的value_t类对象
        value_t<string>的val()成员函数返回type_xx<string>::type类型（即string类型）的数据
        */

       //通过client.call<string>("redis_command", message)向server请求
       //调用.val()获取返回值，存到 res 中
        string res = client.call<string>("redis_command", message).val();
        //添加结束字符
        if(res.find("stop") != std::string::npos){
            break;
        }
        std::cout << res << std::endl;
    }
    return 0;
}
