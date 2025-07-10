
#ifndef GLOBAL
#define GLOBAL
#include<iostream>
#include<unordered_map>
#include<sstream>
enum SET_MODEL{ //set命令的模式
    NONE,NX,XX
};

enum Command{ //命令枚举
    SET,
    SETNX,
    SETEX,
    GET,
    SELECT,
    DBSIZE,
    EXISTS,
    DEL,
    RENAME,
    INCR,
    INCRBY,
    INCRBYFLOAT,
    DECR,
    DECRBY,
    MSET,
    MGET,
    STRLEN,
    APPEND,
    KEYS,
    LPUSH,
    RPUSH,
    LPOP,
    RPOP,
    LRANGE,
    HSET,
    HGET,
    HDEL,
    HKEYS,
    HVALS,
    INVALID_COMMAND
};

static std::unordered_map<std::string,enum Command>commandMaps={ //命令映射
    {"set",SET},
    {"setnx",SETNX},
    {"setex",SETEX},
    {"get",GET},
    {"select",SELECT},
    {"dbsize",DBSIZE},
    {"exists",EXISTS},
    {"del",DEL},
    {"rename",RENAME},
    {"incr",INCR},
    {"incrby",INCRBY},
    {"incrbyfloat",INCRBYFLOAT},
    {"decr",DECR},
    {"decrby",DECRBY},
    {"mset",MSET},
    {"mget",MGET},
    {"strlen",STRLEN},
    {"append",APPEND},
    {"keys",KEYS},
    {"lpush",LPUSH},
    {"rpush",RPUSH},
    {"lpop",LPOP},
    {"rpop",RPOP},
    {"lrange",LRANGE},
    {"hset",HSET},
    {"hget",HGET},
    {"hdel",HDEL},
    {"hkeys",HKEYS},
    {"hvals",HVALS}
};


//命令解析，将字符串s转换为命令，s是待拆分的命令字符串，delimiter是分隔符
static std::vector<std::string> split(const std::string &s, char delimiter=' ') {
    std::vector<std::string> tokens; //存放拆分后的字符串
    std::string token; //临时存放拆分后的子字符串
    std::istringstream tokenStream(s); //将s转换为输入流
    /*
    从输入流tokenStream中读取字符，遇到分隔符delimiter就拆分字符串，将拆分得到的子字符串存放在token中
    然后将token存放在tokens数组中
    */
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}







#endif