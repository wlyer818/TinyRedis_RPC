#ifndef REDISVALUE_H 
#define REDISVALUE_H
#include<iostream>
#include<vector>
#include<map>
#include<initializer_list>
#include<memory>
#include<cmath>
#include<limits>

class RedisValueType;

// RedisValue 类定义
class RedisValue{
public:
    // 定义 RedisValue 支持的数据类型
    enum Type{
        NUL,NUMBER,BOOL,STRING,ARRAY,OBJECT
    };
    typedef std::vector<RedisValue> array; // 定义数组类型
    typedef std::map<std::string,RedisValue> object; // 定义对象类型
    // 构造函数
    RedisValue() noexcept; //noexcept表示该构造函数保证不抛出异常
    RedisValue(std::nullptr_t) noexcept; 
    RedisValue(const std::string& value);
    RedisValue(std::string&& value);
    RedisValue(const char* value);
    RedisValue(const array&value);
    RedisValue(array&& values);
    RedisValue(const object& values);
    RedisValue(object && values);

    // 从具有 toJson 成员函数的类实例构造 RedisValue
    template<class T,class = decltype(&T::toJson)> //模板的第二个参数类型是T的toJson成员函数的类型,进行 SFINAE（Substitution Failure Is Not An Error，替换失败不是错误）检查
    //委托构造函数，把T &t传给RedisValue(const T &t)构造函数,然后RedisValue(const T &t)构造函数又把t.toJson()的返回值传给另一个RedisValue构造函数
    RedisValue(const T & t) : RedisValue(t.toJson()){}

    // 从支持 begin/end 迭代器的容器（map和vector）构造 RedisValue 对象

    /*
    这个构造函数的作用是接受一个M 类型的对象（map），
    如果这个对象的  begin 方法返回的迭代器所指向的元素的 first 成员可以用来构造 std::string， 
    并且 second 成员可以用来构造RedisValue ，
    那么就调用 object(m. begin()  m.end()) 构造一个object（std::map<std::string,RedisValue>类型）对象，
    并用构造出的object对象来构造 RedisValue 对象
    */
    template <class M, typename std::enable_if<
        std::is_constructible<std::string, decltype(std::declval<M>().begin()->first)>::value
        && std::is_constructible<RedisValue, decltype(std::declval<M>().begin()->second)>::value,
            int>::type = 0>
            //委托构造函数
    RedisValue(const M & m) : RedisValue(object(m.begin(), m.end())) {}

    /*
    这个构造函数的作用是接受一个V 类型的对象（vector）， 
    如果这个对象的  begin 方法返回的迭代器所指向的元素可以用来构造RedisValue ，
    那么就通过 array(v. begin(), V.end()) 构造一个array（std::vector<RedisValue>类型）对象， 
    并用该array对象来构造 RedisValue 对象。 
    */
    template <class V, typename std::enable_if<
        std::is_constructible<RedisValue, decltype(*std::declval<V>().begin())>::value,
            int>::type = 0>
    RedisValue(const V & v) : RedisValue(array(v.begin(), v.end())) {}
    
    RedisValue(void*) = delete; // 禁止从 void* 构造

    // 类型判断函数
    Type type() const;
    bool isNull() const{ return type()==NUL;}
    bool isNumber() const { return type()==NUMBER;}
    bool isBoolean() const { return type()==BOOL;}
    bool isString() const { return type()==STRING; }
    bool isArray() const { return type() == ARRAY; }
    bool isObject() const { return type() == OBJECT; }

    // 获取值的函数
    std::string& stringValue() ;
    array& arrayItems() ;
    object &objectItems() ;

    // 重载 [] 操作符，用于访问数组元素和对象成员
    RedisValue & operator[] (size_t i) ;
    RedisValue & operator[] (const std::string &key) ;

    // 序列化函数 dump函数将 RedisValue 对象转为 string 类型
    void dump(std::string &out) const;
    std::string dump() const{
        std::string out;
        dump(out);
        return out;
    }

    // 解析 JSON 文本的静态函数
    static RedisValue parse(const std::string&in, std::string& err);
    static RedisValue parse(const char* in, std::string& err);

    // 解析多个 JSON 值的静态函数
    static std::vector<RedisValue> parseMulti(
        const std::string&in,
        std::string::size_type & parserStopPos,
        std::string& err
    );

    static std::vector<RedisValue>parseMulti(
        const std::string & in,
        std::string & err
    );

    // 重载比较运算符
    bool operator== (const RedisValue &rhs) const;
    bool operator< (const RedisValue &rhs) const;
    bool operator!= (const RedisValue &rhs) const {return !(*this==rhs);}
    bool operator<= (const RedisValue &rhs) const {return !(rhs<*this);}
    bool operator> (const RedisValue &rhs) const { return (rhs<*this);}
    bool operator>= (const RedisValue &rhs) const {return !(*this<rhs);}
    
    // 检查 RedisValue 对象是否符合指定形状
    typedef std::initializer_list<std::pair<std::string,Type>> shape;
    bool hasShape(const shape &types,std::string &err) ;
    
private:
    std::shared_ptr<RedisValueType> redisValue; // 指向实际存储的智能指针
    
};

#endif