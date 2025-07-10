#pragma once
#include <string>
#include <map>
#include <string>
#include <sstream>
#include <functional>
#include <zmq.hpp> //这个是zeroMQ的头文件
#include "Serializer.hpp"//这个是序列化和反序列化的头文件


//模板的别名，需要一个外敷类
// type_xx<int>::type a = 10;
//给 T 起别名为 type
template <typename T>
struct type_xx {
    typedef T type;   //type就是T
};

//上面的外敷类等价于：
/*
template <typename T>
using type = T;
*/

//特例化的模板，当T为void时，type为int8_t
template<>
struct type_xx<void> {
    typedef int8_t type;
};


class buttonrpc
{

public:
	//RPC角色：客户端或服务器
    enum rpc_role{  
		RPC_CLIENT,
		RPC_SERVER
	};
	//RPC错误码
    enum rpc_err_code {  
		RPC_ERR_SUCCESS = 0, //成功
		RPC_ERR_FUNCTIION_NOT_BIND,//函数未绑定
		RPC_ERR_RECV_TIMEOUT //接收超时
	};

    //返回值
    template<typename T>
    class value_t{
       public:
	   	//通过typename关键字告诉编译器type_xx<T>::type 是一个类型，而不是成员函数或者成员变量
         typedef typename type_xx<T>::type type; 
         typedef std::string msg_type;
         typedef uint16_t code_type;

        value_t() { code_ = 0; msg_.clear(); }  //value_t类的构造函数
        bool valid() { return (code_ == 0 ? true : false); } //判断是否有效,错误码code_=0则有效
        int error_code() { return code_; } //返回错误码
        std::string error_msg() { return msg_; }  //返回错误信息
        type val() { return val_; }	//返回值


        void set_val(const type& val) { val_ = val; }      //设置值
        void set_code(code_type code) { code_ = code; }    //设置错误码
        void set_msg(msg_type msg) { msg_ = msg; }  	   //设置错误信息

		//重载输入运算符 >>
		//反序列化，从字节流中读取错误码、错误信息和值
        friend Serializer& operator >> (Serializer& in, value_t<T>& d) { //定义友元函数
            in >> d.code_ >> d.msg_; //从字节流中读取错误码和错误信息
			if (d.code_ == 0) {
				in >> d.val_; //如果错误码为0，则从序列化字节流中读取返回值，存储到val_成员变量中
			}
			return in;
        }
		//重载输出运算符 <<
		//序列化，将错误码、错误信息和值写入字节流
        friend Serializer& operator << (Serializer& out, value_t<T> d) {
			out << d.code_ << d.msg_ << d.val_; //将错误码、错误信息和值写入字节流
			return out;
		}

       private:
        code_type code_;  //uint16_t code_; //错误码
		msg_type msg_;    //string msg_;    //错误信息
		type val_;        //T val_;         //返回值

    };

    buttonrpc(); //buttonrpc类的构造函数
	~buttonrpc(); //buttonrpc类的析构函数

    // network
    void as_client(std::string ip, int port); //将类对象设为客户端
	void as_server(int port); //将类对象设为服务器
	void send(zmq::message_t& data); //发送数据，将data发送出去
	void recv(zmq::message_t& data);//接收数据，存储到data中
	void set_timeout(uint32_t ms);//只有客户端可以设置超时时间
	void run();   //只有服务器可以调用run()函数,循环接收客户端命令，调用相应的函数，将序列化的调用结果发送给客户端


public:
    // server
	//server的作用：注册被调用函数；将收到的序列化数据进行反序列化，并在函数映射表找到对应的调用函数和参数,执行调用再将结果序列化后返回

	//bind函数作用：将函数名和函数指针绑定，映射到map m_handlers 中
	//bind 函数两种模板的实现是为了兼容多参数函数的设计,其中 name 为函数名,同时也是对应的函数的索引,func 为函数指针, s为函数参数
    //bind可以在绑定时候就绑定一部分参数，未提供的参数则使用占位符表示，然后在运行时传入实际的参数值
	//bind全局/静态函数：std::bind(函数指针, 参数1, 参数2, ...);
	//bind非静态成员函数：std::bind(函数指针, 类对象指针(可以是this), 参数1, 参数2, ...);
	template<typename F>
	void bind(std::string name, F func);

    template<typename F, typename S>
	void bind(std::string name, F func, S* s);

   
    // client
	//调用函数call()为了兼容多参数函数的设计,重载了很多的类型
	//目的都是为了将函数名和函数参数序列化后通过网络调用模块发送给 server
    template<typename R>
	value_t<R> call(std::string name); //无参

    template<typename R, typename P1>
	value_t<R> call(std::string name, P1); //一个参数

    template<typename R, typename P1, typename P2>
	value_t<R> call(std::string name, P1, P2);//两个参数

    template<typename R, typename P1, typename P2, typename P3>
	value_t<R> call(std::string name, P1, P2, P3);//三个参数

    template<typename R, typename P1, typename P2, typename P3, typename P4>
	value_t<R> call(std::string name, P1, P2, P3, P4); //四个参数

    template<typename R, typename P1, typename P2, typename P3, typename P4, typename P5>
	value_t<R> call(std::string name, P1, P2, P3, P4, P5); //五个参数

private:
    Serializer* call_(std::string name, const char* data, int len);

    template<typename R>
	value_t<R> net_call(Serializer& ds);

    template<typename F>
	void callproxy(F fun, Serializer* pr, const char* data, int len);

    template<typename F, typename S>
	void callproxy(F fun, S* s, Serializer* pr, const char* data, int len); //类成员函数

    // PROXY FUNCTION POINT
    template<typename R>
	void callproxy_(R(*func)(), Serializer* pr, const char* data, int len) {
		callproxy_(std::function<R()>(func), pr, data, len); //无参函数
	}
 
    template<typename R, typename P1>
	//参数func1是一个R(*func)(P1)类型的函数指针，表示它指向一个返回值类型为R，接受参数类型为P1的函数
	void callproxy_(R(*func)(P1), Serializer* pr, const char* data, int len) {
		//std::function将函数指针func包装成一个std::function对象
		callproxy_(std::function<R(P1)>(func), pr, data, len);//一个参数
	}

    template<typename R, typename P1, typename P2>
	void callproxy_(R(*func)(P1, P2), Serializer* pr, const char* data, int len) {
		callproxy_(std::function<R(P1, P2)>(func), pr, data, len);
	}

	template<typename R, typename P1, typename P2, typename P3>
	void callproxy_(R(*func)(P1, P2, P3), Serializer* pr, const char* data, int len) {
		callproxy_(std::function<R(P1, P2, P3)>(func), pr, data, len);
	}

    template<typename R, typename P1, typename P2, typename P3, typename P4>
	void callproxy_(R(*func)(P1, P2, P3, P4), Serializer* pr, const char* data, int len) {
		callproxy_(std::function<R(P1, P2, P3, P4)>(func), pr, data, len);
	}

	template<typename R, typename P1, typename P2, typename P3, typename P4, typename P5>
	void callproxy_(R(*func)(P1, P2, P3, P4, P5), Serializer* pr, const char* data, int len) {
		callproxy_(std::function<R(P1, P2, P3, P4, P5)>(func), pr, data, len);
	}

    // PROXY CLASS MEMBER，function不能包装类成员变量或函数，需要配合Bind,传入函数地址和类对象地址
    template<typename R, typename C, typename S>
	void callproxy_(R(C::* func)(), S* s, Serializer* pr, const char* data, int len) {
		//通过std::function将func和s绑定，作为一个新的函数对象传入callproxy_函数
		callproxy_(std::function<R()>(std::bind(func, s)), pr, data, len);
	}

    template<typename R, typename C, typename S, typename P1>
	void callproxy_(R(C::* func)(P1), S* s, Serializer* pr, const char* data, int len) {
		callproxy_(std::function<R(P1)>(std::bind(func, s, std::placeholders::_1)), pr, data, len);
	}

    template<typename R, typename C, typename S, typename P1, typename P2>
	void callproxy_(R(C::* func)(P1, P2), S* s, Serializer* pr, const char* data, int len) {
		callproxy_(std::function<R(P1, P2)>(std::bind(func, s, std::placeholders::_1, std::placeholders::_2)), pr, data, len);
	}

	template<typename R, typename C, typename S, typename P1, typename P2, typename P3>
	void callproxy_(R(C::* func)(P1, P2, P3), S* s, Serializer* pr, const char* data, int len) {
		callproxy_(std::function<R(P1, P2, P3)>(std::bind(func, s, 
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)), pr, data, len);
	}

	template<typename R, typename C, typename S, typename P1, typename P2, typename P3, typename P4>
	void callproxy_(R(C::* func)(P1, P2, P3, P4), S* s, Serializer* pr, const char* data, int len) {
		callproxy_(std::function<R(P1, P2, P3, P4)>(std::bind(func, s,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)), pr, data, len);
	}

	template<typename R, typename C, typename S, typename P1, typename P2, typename P3, typename P4, typename P5>
	void callproxy_(R(C::* func)(P1, P2, P3, P4, P5), S* s, Serializer* pr, const char* data, int len) {
		callproxy_(std::function<R(P1, P2, P3, P4, P5)>(std::bind(func, s,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)), pr, data, len);
	}

    // PORXY FUNCTIONAL
    template<typename R>
	void callproxy_(std::function<R()>, Serializer* pr, const char* data, int len);

    template<typename R, typename P1>
	void callproxy_(std::function<R(P1)>, Serializer* pr, const char* data, int len);

    template<typename R, typename P1, typename P2>
	void callproxy_(std::function<R(P1, P2)>, Serializer* pr, const char* data, int len);

	template<typename R, typename P1, typename P2, typename P3>
	void callproxy_(std::function<R(P1, P2, P3)>, Serializer* pr, const char* data, int len);

	template<typename R, typename P1, typename P2, typename P3, typename P4>
	void callproxy_(std::function<R(P1, P2, P3, P4)>, Serializer* pr, const char* data, int len);

	template<typename R, typename P1, typename P2, typename P3, typename P4, typename P5>
	void callproxy_(std::function<R(P1, P2, P3, P4, P5)>, Serializer* pr, const char* data, int len);

private:
	/*
	m_handlers是一个函数映射表map，用于存储函数名和函数实现的映射关系，它的键是函数名，值是一个函数对象。
	这个函数对象接受 个Serializer 对象的指针，字符指针和整数作为参数，没有返回值。
	当客户端调用某个函数时, 服务器就可以通过这个唤射表找到对应的函数实现。
	*/
    std::map<std::string, std::function<void(Serializer*, const char*, int)>> m_handlers; ////函数名——函数指针 的映射
	/*
	ZeroMQ是个高性能的异步消息库，用于构建分布式或并行应用程序。它提供了一种消息队列的模型，但不同于传统的消息队列中间件
	ZeroMQ更像是一个网络编程库，它提供了套接字 (socket) 的抽象，可以用来实现各种复杂的网络模式。
	*/
    zmq::context_t m_context; //上下文，可以设置 IO 线程的数量
    zmq::socket_t* m_socket; //套接字，用于发送和接收数据

    rpc_err_code m_error_code; //错误码
    int m_role; //角色，客户端或服务器
};  

//buttonrpc类的构造函数
//m_context(1)表示使用一个 IO 线程，这个线程负责处理所有的 I/O 操作，包括网络和文件 I/O
buttonrpc::buttonrpc() : m_context(1){ 
	m_error_code = RPC_ERR_SUCCESS; 
}

//buttonrpc类的析构函数
buttonrpc::~buttonrpc(){ 
	m_socket->close(); //关闭套接字
	delete m_socket;   //删除套接字
	m_context.close(); //关闭上下文
}

// network
//将buttonrpc类对象设为客户端，连接到指定的服务器地址， ip + port就是服务器的地址
void buttonrpc::as_client( std::string ip, int port )
{
    m_role = RPC_CLIENT;
    m_socket = new zmq::socket_t(m_context, ZMQ_REQ); //创建一个套接字 参数为上下文和套接字类型	 //ZMQ_REQ 用于请求-应答模式
    ostringstream os;//创建一个字符串流
    os << "tcp://" << ip << ":" << port; //拼接成一个字符串"tcp://ip:port"，即服务器的地址
    m_socket->connect (os.str()); //连接到指定的服务器地址; os.str()获取字符串流中的字符串
}
//将buttonrpc类对象设为服务器
void buttonrpc::as_server( int port )
{
	m_role = RPC_SERVER; //设置角色为服务器
	m_socket = new zmq::socket_t(m_context, ZMQ_REP); //创建一个套接字 参数为上下文和套接字类型	 //ZMQ_REP 用于请求-应答模式
	ostringstream os;
	os << "tcp://*:" << port;  //拼接成一个字符串: "tcp://port"，即服务器要监听的端口
	m_socket->bind (os.str()); //服务器开始监听这个端口
}


//将 data 发送给socket
void buttonrpc::send( zmq::message_t& data )
{
	m_socket->send(data);  //发送数据
}

//从socket接收数据，存储在 data 中
void buttonrpc::recv( zmq::message_t& data )
{
	m_socket->recv(&data); //接收数据
}

//设置超时时间 只有客户端可以设置
inline void buttonrpc::set_timeout(uint32_t ms)
{
	// only client can set
	// if (m_role == RPC_CLIENT) {
	// 	m_socket->setsockopt(ZMQ_RCVTIMEO, ms); //设置接收超时时间
	// }
}

//只有服务器可以调用 run() 函数
void buttonrpc::run()
{
    if (m_role != RPC_SERVER) { //如果不是服务器
		return;
	}
	while (1){
		zmq::message_t data;  //创建一个消息
		recv(data); //从客户端接收序列化的消息，存储到data中
		//将data.data()强转为char*类型，data.size()为size_t类型
		StreamBuffer iodev((char*)data.data(), data.size());//创建一个字节流缓冲区
		Serializer ds(iodev); //用iodev初始化一个序列化器

		std::string funname;
		ds >> funname; //从序列化数据中读取要调用的函数名，存到funname中
		
		//执行被调函数，将序列化的调用结果存储到 r 中
		//从ds.current()开始的ds.size()-funname.size()个字节的数据是函数参数
		Serializer* r = call_(funname, ds.current(), ds.size()- funname.size());

		//下面两行是将序列化的调用结果 r 转为 zmq::message_t 类型
		zmq::message_t retmsg (r->size()); //创建一个 zmq::message_t 类型的消息，大小为 r->size()一样
		//将 r 中的序列化调用结果转为 zmq::message_t 类型
		memcpy (retmsg.data (), r->data(), r->size()); //拷贝数据，将r中的数据拷贝到retmsg中

		//将序列化的调用结果发送给客户端
		send(retmsg);
		delete r;
	}

}

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

template<typename F>
void buttonrpc::bind( std::string name, F func ) //普通函数
{
	/*调用m_handlers[name]函数时，会调用buttonrpc::callproxy<F>函数，
	传入的func函数指针作为buttonrpc::callproxy<F>函数的fun参数，
	并用placeholders::_1, _2, _3占位buttonrpc::callproxy<F>函数的pr, data, len这三个参数。
	因为&buttonrpc::callproxy<F> 是一个成员函数指针，它需要由一个buttonrpc对象来调用，this指针就是这个buttonrpc对象，
	否则std::bind就不知道调用哪个buttonrpc对象来调用callproxy成员函数，导致报错。
	*/
	//将函数名和函数指针绑定，映射到map m_handlers 中
	m_handlers[name] = std::bind(&buttonrpc::callproxy<F>, this, func, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

template<typename F, typename S>
inline void buttonrpc::bind(std::string name, F func, S* s) //类函数 server.bind("redis_command", &RedisServer::handleClient, RedisServer::getInstance());
{
	/*
	将成员函数callproxy<F, S>绑定到this指针，func和s是callproxy<F, S>函数的参数，
	m_handlers的值是一个std::function<void(Serializer*, const char*, int)>类型的函数对象，
	因此后面三个占位符是std::function<void(Serializer*, const char*, int)的参数
	也就是说，当调用m_handlers[name]函数时，会调用callproxy函数
	*/
	m_handlers[name] = std::bind(&buttonrpc::callproxy<F, S>, this, func, s, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

template<typename F>
void buttonrpc::callproxy( F fun, Serializer* pr, const char* data, int len )//代理普通函数
{
	callproxy_(fun, pr, data, len);
}

template<typename F, typename S>
inline void buttonrpc::callproxy(F fun, S * s, Serializer * pr, const char * data, int len)//代理类函数
{
	callproxy_(fun, s, pr, data, len);
}

#pragma region 区分返回值
// help call return value type is void function ,c++11的模板参数类型约束
template<typename R, typename F>
/*
std::is_same<R, void>::value 判断R是否为void类型，返回值为bool类型，如果R为void类型，则返回值为true，否则返回值为false
std::enable_if：当第一个参数为true时（即 std::is_same<R, void>::value == true 时，value是比较结果（True或false）），enable_if的type成员变量会被设为第二个参数的类型。
最外边尖括号的::type就是 enable_if 的 type 成员变量
如果R为void类型，那么call_helper函数的返回值为type_xx<R>::type类型
*/
//call_helper 用来辅助判断调用函数是否需要返回值,如果调用函数是void 类型,返回值 默认为 0
typename std::enable_if<std::is_same<R, void>::value, typename type_xx<R>::type >::type call_helper(F f) {
	f();
	return 0; //如果R为void类型，返回0
}
template<typename R, typename F>
typename std::enable_if<!std::is_same<R, void>::value, typename type_xx<R>::type >::type call_helper(F f) {
	return f();  //如果R不为void类型，返回f()的返回值
}
#pragma endregion



template<typename R>
void buttonrpc::callproxy_(std::function<R()> func, Serializer* pr, const char* data, int len)
{
    /*
    typename关键字用于指定一个依赖类型,依赖类型是指在模板参数中定义的类型，其具体类型直到模板实例化时才能确定。
    type_xx<R>::type是一个依赖类型，因为它依赖于模板参数R。在这种情况下，你需要使用typename关键字来告诉编译器type_xx<R>::type是一个类型。
    如果不使用typename，编译器可能会将type_xx<R>::type解析为一个静态成员
    */
	typename type_xx<R>::type r = call_helper<R>(std::bind(func)); //call_helper函数实际调用func函数
	//如果R为void类型，call_helper函数的返回0，否则返回func函数的返回值func()

	value_t<R> val;
	val.set_code(RPC_ERR_SUCCESS);
	val.set_val(r);
	(*pr) << val;
}


template<typename R, typename P1>
void buttonrpc::callproxy_(std::function<R(P1)> func, Serializer* pr, const char* data, int len)
{
	Serializer ds(StreamBuffer(data, len));
	P1 p1;
	ds >> p1;
	typename type_xx<R>::type r = call_helper<R>(std::bind(func, p1));

	value_t<R> val;
	val.set_code(RPC_ERR_SUCCESS);
	val.set_val(r);
	(*pr) << val;
}


template<typename R, typename P1, typename P2>
void buttonrpc::callproxy_(std::function<R(P1, P2)> func, Serializer* pr, const char* data, int len )
{
	Serializer ds(StreamBuffer(data, len));
	P1 p1; P2 p2;
	ds >> p1 >> p2;
	typename type_xx<R>::type r = call_helper<R>(std::bind(func, p1, p2));
	
	value_t<R> val;
	val.set_code(RPC_ERR_SUCCESS);
	val.set_val(r);
	(*pr) << val;
}

template<typename R, typename P1, typename P2, typename P3>
void buttonrpc::callproxy_(std::function<R(P1, P2, P3)> func, Serializer* pr, const char* data, int len)
{
	Serializer ds(StreamBuffer(data, len));
	P1 p1; P2 p2; P3 p3;
	ds >> p1 >> p2 >> p3;
	typename type_xx<R>::type r = call_helper<R>(std::bind(func, p1, p2, p3));
	value_t<R> val;
	val.set_code(RPC_ERR_SUCCESS);
	val.set_val(r);
	(*pr) << val;
}

template<typename R, typename P1, typename P2, typename P3, typename P4>
void buttonrpc::callproxy_(std::function<R(P1, P2, P3, P4)> func, Serializer* pr, const char* data, int len)
{
	Serializer ds(StreamBuffer(data, len));
	P1 p1; P2 p2; P3 p3; P4 p4;
	ds >> p1 >> p2 >> p3 >> p4;
	typename type_xx<R>::type r = call_helper<R>(std::bind(func, p1, p2, p3, p4));
	value_t<R> val;
	val.set_code(RPC_ERR_SUCCESS);
	val.set_val(r);
	(*pr) << val;
}

template<typename R, typename P1, typename P2, typename P3, typename P4, typename P5>
void buttonrpc::callproxy_(std::function<R(P1, P2, P3, P4, P5)> func, Serializer* pr, const char* data, int len)
{
	Serializer ds(StreamBuffer(data, len));
	P1 p1; P2 p2; P3 p3; P4 p4; P5 p5;
	ds >> p1 >> p2 >> p3 >> p4 >> p5;
	typename type_xx<R>::type r = call_helper<R>(std::bind(func, p1, p2, p3, p4, p5));
	value_t<R> val;
	val.set_code(RPC_ERR_SUCCESS);
	val.set_val(r);
	(*pr) << val;
}



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

//client
template<typename R>
inline buttonrpc::value_t<R> buttonrpc::call(std::string name)
{
	Serializer ds;
	ds << name;
	return net_call<R>(ds);
}

//调用函数call函数为了兼容有多参数函数的设计,重载了很多的类型,如下:
//目的都是一样:将函数名(string)和参数进行序列化,然后调用网络发送模块
template<typename R, typename P1>
inline buttonrpc::value_t<R> buttonrpc::call(std::string name, P1 p1)
{
	Serializer ds;
	ds << name << p1;
	return net_call<R>(ds);
}

template<typename R, typename P1, typename P2>
inline buttonrpc::value_t<R> buttonrpc::call( std::string name, P1 p1, P2 p2 )
{
	Serializer ds;
	ds << name << p1 << p2;
	return net_call<R>(ds);
}

template<typename R, typename P1, typename P2, typename P3>
inline buttonrpc::value_t<R> buttonrpc::call(std::string name, P1 p1, P2 p2, P3 p3)
{
	Serializer ds;
	ds << name << p1 << p2 << p3;
	return net_call<R>(ds);
}

template<typename R, typename P1, typename P2, typename P3, typename P4>
inline buttonrpc::value_t<R> buttonrpc::call(std::string name, P1 p1, P2 p2, P3 p3, P4 p4)
{
	Serializer ds;
	ds << name << p1 << p2 << p3 << p4;
	return net_call<R>(ds);
}

template<typename R, typename P1, typename P2, typename P3, typename P4, typename P5>
inline buttonrpc::value_t<R> buttonrpc::call(std::string name, P1 p1, P2 p2, P3 p3, P4 p4, P5 p5)
{
	Serializer ds;
	ds << name << p1 << p2 << p3 << p4 << p5;
	return net_call<R>(ds);
}






