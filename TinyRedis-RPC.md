# TinyRedis-RPC





## Serializer.hpp

`Serializer` 类的作用是实现序列化和反序列化



字节流缓冲区 `StreamBuffer` 类，继承自 `vector<char>`：

```
class StreamBuffer : public vector<char>
```

`vector<char> `是一个动态数组，可以存储任意数量的 char 元素，并且可以动态地增加或减少元素。非常适合用作字节流的缓冲区。

`StreamBuffer` 类提供了下面几个方法：

1. **构造函数 `StreamBuffer()`**：
   - 初始化当前字节流的位置 `m_curpos` 为 0。
2. **构造函数 `StreamBuffer(const char* in, size_t len)`**：
   - 初始化当前字节流的位置 `m_curpos` 为 0，并将输入的字符数组 `in` 插入到 `StreamBuffer` 缓冲区的起始位置。
3. **析构函数 `~StreamBuffer()`**：
   - 析构函数，用于清理资源。
4. **`void reset()`**：
   - 将当前字节流的位置 `m_curpos` 重置为 0。
5. **`const char* data()`**：
   - 获取缓冲区的数据，返回指向缓冲区第一个元素的指针。
6. **`const char* current()`**：
   - 获取当前位置的数据，返回指向当前字节流位置 `m_curpos` 的指针。
7. **`void offset(int offset)`**：
   - 移动当前位置，将当前位置向后移动 `offset` 个字节。
8. **`bool is_eof()`**：
   - 检查是否已经到达缓冲区末尾，返回 `true` 表示已到达末尾。
9. **`void input(const char* in, size_t len)`**：
   - 将字符数组 `in` 插入到缓冲区的末尾（追加到缓冲区末尾）。
10. **`int findc(char c)`**：
    - 在缓冲区 `[当前位置, 缓冲区末尾]` 区间查找特定的字节 `c`，返回 `c` 的位置。如果未找到，返回 -1。



`Serializer`类是序列化/反序列化类，它的 `m_iodevice`成员变量是个  `StreamBuffer` 类对象，用于存储序列化的字节流数据。

序列化数据默认是小端序，如果是大端序，会调用 `byte_orser(char *in, int len)` 转为小端序：

```c++
    //大端序转小端序
    void byte_orser(char* in, int len){
		if (m_byteorder == BigEndian){
			reverse(in, in+len); //大端的话直接反转 
		}
	}
```



通过  `void Serializer::input_type()` 函数 和 重载 `operator <<` 运算符，实现序列化： 

```c++
//input_type函数用于将t中的数据写入到字节流缓冲区m_iodevice的末尾（序列化）
template<typename T>
inline void Serializer::input_type(T t)
{
    //求出放入缓存区数据的长度
	int len = sizeof(T); 
	char* d = new char[len];   
	const char* p = reinterpret_cast<const char*>(&t);
	memcpy(d, p, len); //将p的数据拷贝到d中
	byte_orser(d, len); 
	m_iodevice.input(d, len); //将d中的数据追加到字节流缓冲区（m_iodevice是个vector<char>）的末尾
	delete [] d;
}

    //重载输出运算符<<，用于向序列化器中写入数据
    //序列化，将i中的数据写入到字节流缓冲区m_iodevice的末尾
    template<typename T>
	Serializer &operator << (T i){
		input_type(i);
		return *this;
	}    
```





通过  `void Serializer::output_type()` 函数 和 重载 `operator >>` 运算符，实现反序列化： 

```c++
    //output_type函数用于将字节流缓冲区m_iodevice的数据写入到t中（反序列化）
template<typename T>
inline void Serializer::output_type(T& t)
{
	int len = sizeof(T);
	char* d = new char[len];
	if (!m_iodevice.is_eof()){ //如果没有到达字节流的末尾
		memcpy(d, m_iodevice.current(), len); //将字节流 [当前位置, 当前位置+len] 区间的数据拷贝到字符数组d中
		m_iodevice.offset(len); //并将当前位置向后移动len个字节
		byte_orser(d, len); //将字符数组d转为小端序
		t = *reinterpret_cast<T*>(&d[0]); //reinterpret_cast<T*>(&d[0])将char*类型的&d[0]转换为T*类型,再通过*解引用取出T类型的数据
	}
	delete [] d;
}
    
    //重载输入运算符>>，用于从序列化器中读取数据
    //反序列化，将字节流缓冲区m_iodevice中的数据写入到i中
    template<typename T>
    Serializer &operator >>(T& i){
        output_type(i); 
        return *this;
    }
```



input_type，output_type 函数提供了多种的特化和偏特化的实现，同样是为了满足 多参数函数 的序列化的需求





## buttonrpc.hpp

buttonrpc类对象既可以作为服务器，也可以作为客户端。通过调用 `void as_client(std::string ip, int port)`和 `void as_server(int port)` 设置 **m_role** 成员变量确定角色 

关键业务函数：

```c++
    void as_client(std::string ip, int port); //将类对象设为客户端
	void as_server(int port); //将类对象设为服务器
	void send(zmq::message_t& data); //发送数据，将data发送出去
	void recv(zmq::message_t& data);//接收数据，存储到data中
	void set_timeout(uint32_t ms);//只有客户端可以设置超时时间
	void run();   //只有服务器可以调用run()函数,循环接收客户端命令，调用相应的函数，将序列化的调用结果发送给客户端
```

server的作用：注册被调用函数；将收到的序列化数据进行反序列化，并在函数映射表找到对应的调用函数和参数,执行调用再将结果序列化后返回

通过两个友元函数实现 序列化和反序列化：

重载 `operator >>` 将序列化字节流中的数据写到 `buttonrpc` 类的成员变量：

```c++
		//重载输入运算符 >>
		//反序列化，从字节流中读取错误码、错误信息和值
        friend Serializer& operator >> (Serializer& in, value_t<T>& d) { //定义友元函数
            in >> d.code_ >> d.msg_; //从字节流中读取错误码和错误信息
			if (d.code_ == 0) {
				in >> d.val_; //如果错误码为0，则从序列化字节流中读取返回值，存储到val_成员变量中
			}
			return in;
        }
```



重载 `operator <<` 将 `buttonrpc` 类的成员变量的值序列化为字节流：

```c++
		//重载输出运算符 <<
		//序列化，将错误码、错误信息和值写入字节流
        friend Serializer& operator << (Serializer& out, value_t<T> d) {
			out << d.code_ << d.msg_ << d.val_; //将错误码、错误信息和值写入字节流
			return out;
		}
```



通过 bind 将被调函数注册到 函数映射表 `m_handlers`中

```c++
	//bind函数作用：将函数名和函数指针绑定，映射到map m_handlers 中过程
	//bind 函数两种模板的实现是为了兼容多参数函数的设计,其中 name 为函数名,同时也是对应的函数的索引,func 为函数指针, s为函数参数
    //bind可以在绑定时候就绑定一部分参数，未提供的参数则使用占位符表示，然后在运行时传入实际的参数值
	//bind全局/静态函数：std::bind(函数指针, 参数1, 参数2, ...);
	//bind非静态成员函数：std::bind(函数指针, 类对象指针(可以是this), 参数1, 参数2, ...);
	template<typename F>
	void bind(std::string name, F func); //将 函数名和普通函数的实现 绑定

    template<typename F, typename S>
	void bind(std::string name, F func, S* s); //将 函数名和类成员函数的实现 绑定
```





将函数名——函数对象保存到 `m_handles` 映射表的过程：

```c++
std::map<std::string, std::function<void(Serializer*, const char*, int)>> m_handlers; //函数名——函数指针映射表

template<typename F, typename S>
inline void buttonrpc::bind(std::string name, F func, S* s) 
{
	m_handlers[name] = std::bind(&buttonrpc::callproxy<F, S>, this, func, s, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

template<typename F, typename S>
inline void buttonrpc::callproxy(F fun, S * s, Serializer * pr, const char * data, int len)//代理类函数
{
	callproxy_(fun, s, pr, data, len);
}

```

​    将成员函数callproxy<F, S>绑定到this指针，func和s是callproxy<F, S>函数的参数，

​    m_handlers的值是一个std::function<void(Serializer*, const char*, int)>类型的函数对象，

​    因此后面三个占位符是std::function<void(Serializer*, const char*, int)的参数

​    也就是说， `m_handlers` 中保存的是 `std::bind(&buttonrpc::callproxy<F, S>, this, func, s, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)`形成的新函数对象， 当调用m_handlers[name]函数时，会调用callproxy函数



**call, callproxy, callproxy_, bind等函数有很多版本的实现，是为了兼容多参数函数的设计**



==**服务器执行客户端远程调用的函数的过程：**==

当调用 **server.run()**时，run函数里会调用 **call_()**函数，call_ 函数会在 **m_handlers** 映射表中找到对应的函数名， 然后调用 **m_handlers[name]** 代表的函数，m_handlers[name]函数会先调用 **callproxy** 代理函数，然后callproxy 函数里面又会调用 **callproxy_** 函数，而 callproxy_ 函数里又会调用 **call_helper** 函数，call_helper 函数里才真正调用 **func** 函数。

call_helper 函数通过 **std::enable_if** 根据模板参数R的类型返回u不同的返回值：如果 R 为 void类型，那么 call_helper 函数返回0；如果R不为void类型，返回func()的返回值

函数远程调用的讲解可以看：『远程过程调用-buttonrpc源码解析6-函数调用-CSDN博客』https://blog.csdn.net/qq_40945965/article/details/137182004



**ZeroMQ** 网络库

在 buttonrpc 中，通过 ZMQ 进行网络调用

ZMQ (Zero Message Queue)是一种**消息队列**，核心引擎由C++编写，是轻量级消息通信库，是在对传统的标准Socket接口扩展的基础上形成的特色消息通信中间件。ZMQ有多种模式可以使用，常用的模式包括**Request-Reply(请求-应答)**，Publish/Subscribe（发布/订阅）和Push/Pull（推/拉）三种模式。

在 `buttonrpc.hpp` 中，采用的是 `ZMQ_REP` 请求-应答模式，代码如下：

```c++
void buttonrpc::as_server( int port )
{
	m_role = RPC_SERVER; //设置角色为服务器
	m_socket = new zmq::socket_t(m_context, ZMQ_REP); //创建一个套接字 参数为上下文和套接字类型	 //ZMQ_REP 用于请求-应答模式
	ostringstream os;
	os << "tcp://*:" << port;  //拼接成一个字符串: "tcp://port"，即服务器要监听的端口
	m_socket->bind (os.str()); //服务器开始监听这个端口
}
```



## RedisServer类

**getInstance()成员函数**

本项目是单线程项目，不存在多个线程同时调用 `getInstance()` 成员函数的情况，因此不需要额外处理，但还是实现了 **多线程单例模式的局部静态变量懒汉模式**，实现了线程安全：

单例模式的特点：

- 构造函数和析构函数为**private**类型，目的**禁止**外部构造和析构
- 拷贝构造和赋值构造函数为**private**类型，目的是**禁止**外部拷贝和赋值，确保实例的唯一性
- 类里有个获取实例的**静态函数**，可以全局访问

RedisServer类的单例模式的实现如下：

将构造函数生命为 `private`，禁止外部创建对象；静态成员函数 `getInstance()` 声明为public, 可以全局访问



```c++
class RedisServer {
private:
    std::unique_ptr<ParserFlyweightFactory> flyweightFactory; // 解析器工厂
    int port;
    std::atomic<bool> stop{false};
    pid_t pid;
    std::string logoFilePath;
    bool startMulti = false;
    bool fallback = false;
    std::queue<std::string>commandsQueue;//事物指令队列

private:
    //构造函数声明为私有，防止外部创建对象
    RedisServer(int port = 5555, const std::string& logoFilePath = "logo");
    static void signalHandler(int sig);
    void printLogo();
    void printStartMessage();
    void replaceText(std::string &text, const std::string &toReplaceText, const std::string &replaceText);
    std::string getDate();
    string executeTransaction(std::queue<std::string>&commandsQueue);
public:
    string handleClient(string receivedData);
    static RedisServer* getInstance();  //静态成员函数，获取单例
    void start();
};

//getInstance()是静态成员函数，要在类外定义
RedisServer* RedisServer::getInstance()
{
    //C++11特性确保局部静态变量是线程安全的，static确保只有一个 RedisServer 类实例
	static RedisServer redis;  //只会被创建一次
	return &redis;
}
```



**start()函数**

`signal(SIGINT, signalHandler); `  注册sigint信号处理函数，sighandler的作用是在使用sigint时将缓冲区刷新一遍。

`SIGINT` 信号由用户按下 `ctrl + c` 触发， `signalHandler` 函数的作用是在程序中断时将所有数据写入硬盘，防止数据丢失，保证数据持久化。



```c++
void RedisServer::start() {
    signal(SIGINT, signalHandler);  
    printLogo();
    printStartMessage();
    // string s ;
    // while (!stop) {
    //     getline(cin,s);
    //     cout << s <<endl;
    //     cout << RedisServer::handleClient(s) << endl;;                            
    // }
}
```





















## client.cpp





```c++
        /*
        call<string>函数会调用string类型的net_call函数，net_call函数返回value_t<string>类型的value_t类对象
        value_t<string>的val()成员函数返回type_xx<string>::type类型（即string类型）的数据
        */

       //通过client.call<string>("redis_command", message)向server请求
       //调用.val()获取返回值，存到 res 中
        string res = client.call<string>("redis_command", message).val();
```

通过 `client.call<string>("redis_command", message)`向server请求，调用.`val()`获取返回值，存到 res 中。



`client.call()` 会调用 `net_call()`：

Serializer类重载了 operator << 运算符，这里就是先把name和p1的内容写入到 ds.m_iodevice，然后再调用 `net_call()`

```c++
//调用函数call函数为了兼容有多参数函数的设计,重载了很多的类型,如下:
//目的都是一样:将函数名(string)和参数进行序列化,然后调用网络发送模块
template<typename R, typename P1>
inline buttonrpc::value_t<R> buttonrpc::call(std::string name, P1 p1)
{
	Serializer ds;
	ds << name << p1; //Serializer类重载了 operator << 运算符，这里就是把name和p1的内容写入到 ds.m_iodevice
	return net_call<R>(ds);
}
```



net_call()实现如下：

```c++
//客户端通过net_call()发起远程调用，将序列化的数据发送给服务器，接收服务器返回的数据，并将其反序列化后返回
template<typename R>
inline buttonrpc::value_t<R> buttonrpc::net_call(Serializer& ds)
{
	zmq::message_t request(ds.size() + 1);
	//将字节流ds中的序列化的数据拷贝到request中
	memcpy(request.data(), ds.data(), ds.size());
	if (m_error_code != RPC_ERR_RECV_TIMEOUT) {
		//客户端发送序列化的数据给服务器
		send(request); //由m_socket->send(request)发送数据
	}
	//接收服务器返回的序列化的调用结果，存储到reply中
	zmq::message_t reply;
	recv(reply);  //m_socket->recv(&reply)接收数据，存放到reply中
	value_t<R> val;
	if (reply.size() == 0) { //如果没有收到数据，说明超时了
		// timeout
		//枚举值（RPC_ERR_RECV_TIMEOUT）可以自动转换为整数
		m_error_code = RPC_ERR_RECV_TIMEOUT; //设置buttonrpc类对象的错误码
		val.set_code(RPC_ERR_RECV_TIMEOUT);  //设置value_t类对象的错误码
		val.set_msg("recv timeout"); //设置value_t类对象的错误信息(string类型)
		return val;
	}
	m_error_code = RPC_ERR_SUCCESS;
	ds.clear(); //清空ds中m_iodevice的数据，并将m_curpos置为0
	//将reply中的数据转为char*类型，写到ds中的StreamBuffer类对象m_iodevice中，并将m_curpos后移reply.size()个字节
	//将 server 返回的序列化的调用结果写入到字节流ds中
	ds.write_raw_data((char*)reply.data(), reply.size());
	ds.reset(); //将读取位置m_curpos重置为0，因为下一步要从头读取ds中的数据，存到val中
	/*
	>>运算符被重载，调用>>运算符会调用Serializer类的output_type函数，调用output_type函数会将ds的读取位置m_curpos后移
	*/
	ds >> val; //反序列化，将字节流ds中的数据读取到val中
	return val;
}
```

`send(request)` 将请求发送给服务端，服务端处理后返回给客户端，客户端通过 `recv(reply)` 接收处理结果，并保存在 reply 中。





## server.cpp



```c++
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
```

`server.run()` 中获取客户端的命令名，调用 `Serializer* r = call_(funname, ds.current(), ds.size()- funname.size());`，call_函数的实现如下：

```c++
// 处理函数相关
//这是 server 真正调用函数的地方，在函数名——函数指针映射表m_handlers中找到被调用函数，执行后将调用结果序列化后返回
//name是函数名；从data指针位置开始的len个字节的数据是函数参数
Serializer* buttonrpc::call_(std::string name, const char* data, int len)
{
    Serializer* ds = new Serializer(); //创建一个序列化器
    if (m_handlers.find(name) == m_handlers.end()) { //如果在函数映射表中没有找到被调函数名
		(*ds) << value_t<int>::code_type(RPC_ERR_FUNCTIION_NOT_BIND); //设置错误码，将错误码写入ds
		(*ds) << value_t<int>::msg_type("function not bind: " + name); //设置错误信息,将错误信息写入ds
		return ds;
	}

    auto fun = m_handlers[name]; //获取被调函数指针
    fun(ds, data, len);  //执行被调函数，将结果序列化后存储到ds中
    ds->reset(); //重置序列号容器 Serializer有个StreamBuffer类型的成员变量，将StreamBuffer对象的当前位置m_curpos重置为0
    return ds;  //返回序列化的调用结果
}

```



从 m_handlers 函数映射表中取出命令名（函数名） name 对应的函数对象，然后调用执行，但在 `server.run()` 之前，通过 `server.bind("redis_command", &RedisServer::handleClient, RedisServer::getInstance());`  将真正要执行的函数`RedisServer::handleClient`层层绑定：

先是：`m_handlers[name] = std::bind(&buttonrpc::callproxy<F, S>, this, func, s, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);`

再是：`callproxy_(fun, pr, data, len);`

最后：`call_helper<R>(std::bind(func, p1, p2, p3, p4, p5));`

最终在 call_helper()中调用了  `RedisServer::handleClient` 函数

```c++
 auto fun = m_handlers[name]; //获取函数
 fun(ds, data, len); 
```

