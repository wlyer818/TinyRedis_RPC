
#include"Global.h"
#include "Parse.h"


/*************构造函数******************/

/*
statics()函数返回一个静态的Statics实例的引用，它的null成员是一个指向RedisValueNull对象的shared_ptr指针
这个构造函数构建了一个RedisValue对象，并将其redisValue成员初始化为指向RedisValueNull对象的shared_ptr指针
表示RedisValue对象的值为null
*/
RedisValue::RedisValue() noexcept : redisValue(statics().null) {}

RedisValue::RedisValue(std::nullptr_t) noexcept : redisValue(statics().null) {}

//创建一个std::shared_ptr<RedisString>智能指针，指向一个RedisString对象，用该指针初始化redisValue成员
//std::make_shared<RedisString>(value)会调用RedisString的构造函数RedisString(const std::string& value)
//而RedisString继承自Value<RedisValue::STRING,std::string>，
//所以RedisString的构造函数会调用Value<RedisValue::STRING,std::string>的构造函数，将Value类的value成员变量初始化为传入的参数value
RedisValue::RedisValue(const std::string& value) : redisValue(std::make_shared<RedisString>(value)) {}

//同上，只不过这里传入的是右值引用，调用的是RedisString的构造函数RedisString(std::string&& value)
RedisValue::RedisValue(std::string&& value) : redisValue(std::make_shared<RedisString>(std::move(value))) {}

//首先，通过std::string构造函数将const char*类型的value转化为std::string类型的临时string对象，
//然后调用RedisString的构造函数RedisString(const std::string& value)
RedisValue::RedisValue(const char* value) : redisValue(std::make_shared<RedisString>(value)) {}

RedisValue::RedisValue(const RedisValue::array& value) : redisValue(std::make_shared<RedisList>(value)) {}

RedisValue::RedisValue(RedisValue::array&& value) : redisValue(std::make_shared<RedisList>(std::move(value))) {}

RedisValue::RedisValue(const RedisValue::object& value) : redisValue(std::make_shared<RedisObject>(value)) {}

RedisValue::RedisValue(RedisValue::object &&value) : redisValue(std::make_shared<RedisObject>(std::move(value))) {}

/************* Member Functions ******************/

RedisValue::Type RedisValue::type() const {
    //redisValue是一个std::shared_ptr<RedisValueType>类型的智能指针
    //调用RedisValueType类的type()函数，返回tag, tag是个RedisValue::Type类型
    /*
    RedisValue::Type类型包括以下六个枚举值：
    enum Type{
    NUL,NUMBER,BOOL,STRING,ARRAY,OBJECT
    };
    因此tag是这六个枚举值中的一个
    */
    return redisValue->type(); //返回NUL,NUMBER,BOOL,STRING,ARRAY,OBJECT中的一个
}

std::string & RedisValue::stringValue() {
    return redisValue->stringValue(); //调用73行的std::string& RedisValueType::stringValue()函数
}

std::vector<RedisValue> & RedisValue::arrayItems() {
    return redisValue->arrayItems(); //调用77行的std::vector<RedisValue> & RedisValueType::arrayItems()函数
}

std::map<std::string, RedisValue> & RedisValue::objectItems()  {
    return redisValue->objectItems(); //调用85行的std::map<std::string, RedisValue> & RedisValueType::objectItems()函数
}

//重载[]操作符，用于访问数组元素和对象成员
RedisValue & RedisValue::operator[] (size_t i)  {
    return (*redisValue)[i];
}

RedisValue & RedisValue::operator[] (const std::string& key) {
    return (*redisValue)[key];
}

//RedisValueType是抽象基类，其stringValue函数,arrayItems()函数,objectItems()函数，重载 [] 运算符都是虚函数，需要在派生类中重写
//这里提供了默认实现，返回一个静态的空字符串、空Json数组、空Json对象映射，一个静态的redisValueNull对象的引用（表示null值）
//当派生类没有重写这些函数时，就会调用这里的默认实现，防止调用失败
std::string& RedisValueType::stringValue() {
    return statics().emptyString; //返回一个静态的空字符串
}

std::vector<RedisValue> & RedisValueType::arrayItems() {
    return statics().emptyVector; //返回一个静态的空Json数组
}

std::map<std::string, RedisValue> & RedisValueType::objectItems() {
    return statics().emptyMap; //返回一个静态的空Json对象映射
}

//RedisValueType是抽象基类，不支持下标访问，所以总是返回一个静态的redisValueNull对象的引用，表示null值
RedisValue& RedisValueType::operator[] (size_t) {
    return staticNull();
}

RedisValue& RedisValueType::operator[] (const std::string&) {
    return staticNull();
}

//重载[]操作符，用于访问map对象成员
RedisValue& RedisObject::operator[] (const std::string&key) {
    auto it = value.find(key);
    return (it==value.end()) ? staticNull() : it->second;
}
//重载[]操作符，用于访问vector对象成员
RedisValue& RedisList::operator[](size_t i ) {
    if(i>=value.size()) return staticNull();
    return value[i];
}

/*比较*/

/*
redisValue 是一个基类智能指针，它指向一个RedisValueType 类型的对象。 
less 是 Value 类的成员函数， Value 类是 RedisValueType类的派生类。
基类指针 (或引用）可以指向派生类对象并且可以调用派生类中覆盖的虚函数。 
但是，基类指针 (或引用） 不能直接调用派生类中新增的成员函数，除非进行向下转型。
type(), less(), equal(), dump()这四个函数在抽象基类RedisValueType中都是纯虚函数，
并且在派生类Value类中都被覆盖重写，所以redisValue智能指针可以调用这四个函数 
*/


//重载==操作符，比较两个RedisValue对象是否相等
bool RedisValue::operator== (const RedisValue&other) const{
    //若两个RedisValue对象的redisValue成员（都是智能指针）指向同一个RedisValueType对象，则返回true
    if (redisValue == other.redisValue)
        return true;
    //若两个RedisValue对象的redisValue成员指向的RedisValueType对象的类型不同，则返回false
    if (redisValue->type() != other.redisValue->type())
        return false;
    //get()函数返回智能指针管理的原始指针
    return redisValue->equals(other.redisValue.get());
}
//重载<操作符，比较两个RedisValue对象的大小
bool RedisValue::operator< (const RedisValue& other) const{
    //若两个RedisValue对象的redisValue成员（都是智能指针）指向同一个RedisValueType对象，则返回false
    if (redisValue == other.redisValue)
        return false;
    //若两个RedisValue对象的redisValue成员指向的RedisValueType对象的类型不同，则返回类型小的那个
    //NUL < NUMBER < BOOL < STRING < ARRAY < OBJECT
    if (redisValue->type() != other.redisValue->type())
        return redisValue->type() < other.redisValue->type();
    //比较两个RedisValue对象的redisValue成员指向的RedisValueType对象的value成员变量
    //less()函数是Value类的成员函数，通过成员函数可以访问该类的所有成员变量，因此redisValue->less可以访问value成员变量
    return redisValue->less(other.redisValue.get());
}


// 定义Json类的成员函数dump，用于将Json对象转化为JSON字符串并追加到out中
void RedisValue::dump(std::string &out) const {
    redisValue->dump(out); // 调用JsonImpl类的dump函数将Json对象转化为JSON字符串并追加到out中
}



// 将字符串转化为Json对象

//解析 JSON 文本的静态函数 输入std::string类型的字符串
RedisValue RedisValue::parse(const std::string &in, std::string &err) {
    // 初始化一个Json解析器
    /*
    四个参数分别是：
    const std::string &str;  // 要解析的JSON字符串
    size_t i;                // 当前解析位置的索引
    std::string &err;        // 用于存储解析过程中的错误信息
    bool failed;             // 解析是否失败的标志
    */
    RedisValueParser parser { in, 0, err, false};
    // 解析输入字符串以得到Json结果
    RedisValue result = parser.parseRedisValue(0);

    // 消耗掉字符串in的垃圾字符（即空白字符：空格，\t, \n, \r）
    parser.consumeGarbage();
    if (parser.failed)
        return RedisValue(); // 如果解析失败，返回一个空的Json对象
    if (parser.i != in.size())
        return parser.fail("unexpected trailing " + esc(in[parser.i])); // 如果输入的字符串尚有未解析内容，报告错误

    return result; // 返回解析得到的Json对象
}
//解析 JSON 文本的静态函数  输入const char*类型的字符串
RedisValue RedisValue::parse(const char* in, std::string& err){
    if (in) {
            //将const char*类型的字符串转化为std::string类型的字符串，然后调用上面的parse函数
            return parse(std::string(in), err);
    } else {
        err = "null input";
        return nullptr;
    }
}
// 解析输入字符串中的多个Json对象
std::vector<RedisValue> RedisValue::parseMulti(const std::string &in,
                               std::string::size_type &parser_stop_pos,
                               std::string &err) {
    // 初始化一个Json解析器
    RedisValueParser parser { in, 0, err, false };
    parser_stop_pos = 0;
    std::vector<RedisValue> jsonList; // 存储解析得到的多个Json对象的容器

    // 当输入字符串还有内容并且解析未出错时继续
    while (parser.i != in.size() && !parser.failed) {
        jsonList.push_back(parser.parseRedisValue(0)); // 解析Json对象并添加到容器中
        if (parser.failed)
            break; // 如果解析失败，中断循环

        // 检查是否还有其他对象
        parser.consumeGarbage();
        if (parser.failed)
            break; // 如果发现垃圾字符或有错误，中断循环
        parser_stop_pos = parser.i; // 更新停止位置
    }
    return jsonList; // 返回解析得到的Json对象容器
}

std::vector<RedisValue> RedisValue::parseMulti(
        const std::string & in,
        std::string & err
    )
{
        std::string::size_type parser_stop_pos;
        return parseMulti(in, parser_stop_pos, err);
}

// 检查 JSON 对象是否具有指定的形状
bool RedisValue::hasShape(const shape & types, std::string & err)  {
    // 如果 JSON 不是对象类型，则返回错误
    if (!isObject()) {
        err = "expected JSON object, got " + dump();
        return false;
    }

    // 获取 JSON 对象的所有成员项
    auto obj_items = objectItems();
    
    // 遍历指定的形状
    for (auto & item : types) {
        // 查找 JSON 对象中是否存在形状中指定的项
        const auto it = obj_items.find(item.first);
        
        // 如果找不到项或者项的类型不符合指定的类型，则返回错误
        if (it == obj_items.cend() || it->second.type() != item.second) {
            err = "bad type for " + item.first + " in " + dump();
            return false;
        }
    }

    // 如果所有形状都匹配，则返回 true
    return true;
}