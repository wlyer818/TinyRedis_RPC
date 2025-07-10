#ifndef REDISVALUETYPE_H
#define REDISVALUETYPE_H
#include<map>
#include<vector>
#include<iostream>
#include<cassert>
#include"Parse.h"
#include"RedisValue.h"
#include"Dump.h"

/*
     
*/
//抽象基类
class RedisValueType{ 
protected:
    friend class RedisValue;
    /*纯虚常量函数，=0说明是纯虚函数；const说明是常量函数*/
    virtual RedisValue::Type type() const = 0; //函数type()的返回值为RedisValue::Type类型（枚举值），且不会修改它所属的类对象的状态。
    virtual bool equals(const RedisValueType*other) const = 0; //比较两个对象是否相等，不会修改它所属的类对象的状态
    virtual bool less(const RedisValueType*other) const = 0; //比较两个对象的大小，不会修改它所属的类对象的状态
    virtual void dump(std::string& out) const = 0; //将派生类的value对象转化为string字符串,并追加到out字符串中,不会修改它所属的类对象的状态
    /*虚函数*/
    virtual std::string &stringValue() ;
    virtual RedisValue::array &arrayItems() ;
    virtual RedisValue::object &objectItems() ;
    //重载 [] 运算符
    virtual RedisValue& operator[] (size_t i) ; //类似于数组的下标运算符，返回第i个元素
    virtual RedisValue& operator[](const std::string &key) ; //类似于字典的下标运算符，返回key对应的value
    
    virtual ~RedisValueType(){} //虚析构函数

};

template<RedisValue::Type tag,typename T>
class Value : public RedisValueType{
protected:
    T value;
protected:
    //拷贝构造函数，explicit防止隐式转换
    explicit Value(const T& value) : value(value){} 
    //移动构造函数，接受类型T的右值引用
    explicit Value(T&&value) : value(std::move(value)){}
    //获取value
    T getValue(){
        return value;
    }
    /*
    tag是六个枚举值中的一个：
    enum Type{
    NUL,NUMBER,BOOL,STRING,ARRAY,OBJECT
    };
    */
    RedisValue::Type type() const override{
        return tag;
    }
    //比较当前对象和other对象的value成员变量是否相等
    bool equals(const RedisValueType* other) const override {
        //将RedisValueType*类型的other指针强转为Value<tag,T>*类型的指针，然后获取其value值用来比较
        return value == static_cast<const Value<tag,T>*>(other)->value;
    }
    //比较当前对象的value成员变量是否 小于 other对象的value成员变量
    bool less(const RedisValueType* other) const override{
        return value < static_cast<const Value<tag, T> *>(other)->value;
    }
    //将T类型的value转化为string类型，并追加到out字符串中
    void dump(std::string&out) const override{ ::dump(value,out);}
};



//RedisString类是Value类的派生类，而Value类是RedisValueType类的派生类
//RedisString类是Value类的特化版本，RedisValue::STRING,std::string是Value类的模板参数
//final关键字表示RedisString类不能被继承
//value是RedisValue::STRING类型（std::string字符串）
class RedisString final:public Value<RedisValue::STRING,std::string>{
    //重写RedisValueType类的stringValue虚函数，返回value（string类型）
    std::string & stringValue()override { return value;}
public:
    explicit RedisString(const std::string& value): Value(value){} //拷贝构造函数
    explicit RedisString(std::string&& value) : Value(std::move(value)) {} //移动构造函数
};

//RedisList类是Value类的派生类，而Value类是RedisValueType类的派生类
//RedisList类是Value类的特化版本，RedisValue::ARRAY,RedisValue::array是Value类的模板参数
//final关键字表示RedisList类不能被继承
//value是RedisValue::array类型（std::vector<RedisValue>数组）
class RedisList final : public Value<RedisValue::ARRAY, RedisValue::array>{
    //value是RedisValue::array类型（std::vector<RedisValue>数组）
    RedisValue::array & arrayItems()  override{ return value;}
    RedisValue & operator[](size_t i)  override;
public:
    explicit RedisList(const RedisValue::array &value): Value(value){} //拷贝构造函数
    explicit RedisList(RedisValue::array &&value) : Value(std::move(value)){} //移动构造函数
};


//RedisObject类是Value类的派生类，而Value类是RedisValueType类的派生类
//RedisObject类是Value类的特化版本，RedisValue::OBJECT,RedisValue::object是Value类的模板参数
//final关键字表示RedisObject类不能被继承
//value是RedisValue::object类型（std::map<std::string,RedisValue>对象）
class RedisObject final : public Value<RedisValue::OBJECT,RedisValue::object>{
    //value是RedisValue::object类型（std::map<std::string,RedisValue>字典）
    RedisValue::object & objectItems()  override{ return value;}
    RedisValue& operator[] (const std::string& key)  override;
public:
    explicit RedisObject(const RedisValue::object &value) : Value(value) {} //拷贝构造函数
    explicit RedisObject(RedisValue::object &&value)      : Value(std::move(value)) {}  //移动构造函数
};

class RedisValueNull final: public Value<RedisValue::NUL,NullStruct>{
public:
    //构造函数
    //调用Value类的移动构造函数，传入NullStruct类型的空值
    //使成员变量value的值为NullStruct类型的实例
    RedisValueNull() : Value({}) {}
};
#endif