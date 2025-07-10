#include "Parse.h"
#include "Global.h"


// 处理失败情况，设置错误信息并返回错误的Json对象
//接受一个右值引用，返回一个RedisValue对象
RedisValue RedisValueParser::fail(std::string &&msg) {
        //调用下面的T RedisValueParser::fail(std::string &&msg, const T err_ret)函数
        //通过std::move将msg转换为右值  RedisValue()构造函数返回一个空的RedisValue对象
        return fail(move(msg), RedisValue());
    }

// 通用的失败处理函数，设置错误信息并返回指定的错误返回值
template <typename T>
T RedisValueParser::fail(std::string &&msg, const T err_ret) {
    if (!failed) //如果failed标志为false
        err = std::move(msg); //std::move将msg从右值引用转换为右值，并赋值给err
    failed = true; //设置failed标志为true，表示解析失败
    return err_ret;
}

//消耗掉字符串中的空白字符，使 i 指向下一个非空白字符
void RedisValueParser::consumeWhitespace() {
    //i 是当前解析位置的索引
    while (str[i] == ' ' || str[i] == '\r' || str[i] == '\n' || str[i] == '\t')
        i++;
}

//消耗掉注释内容，支持解析策略中包含注释的情况
bool RedisValueParser::consumeComment() {
    bool comment_found = false;
    if (str[i] == '/') { //第一个字符是/，说明接下来的内容可能是行内注释或多行注释
    i++;
    if (i == str.size()) //若'/'后面没有字符，说明刚开始注释就结束输入了，调用fail函数处理失败情况
        return fail("在注释开始后意外结束输入", false);
    //若'/'后面的字符是/，说明是行内注释
    if (str[i] == '/') { // 行内注释
        i++;
        // 一直前进，直到下一行或输入结束
        while (i < str.size() && str[i] != '\n') {
        i++;
        }
        comment_found = true; //找到注释
    }
    //若'/'后面的字符是*，说明是多行注释
    else if (str[i] == '*') { // 多行注释
        i++;
        if (i > str.size()-2) //因为寻找多行注释结束标记需要访问str[i+1]，所以i不能超过str.size()-2
        return fail("在多行注释内部意外结束输入", false);
        // 前进直到找到关闭标记
        while (!(str[i] == '*' && str[i+1] == '/')) {
        i++;
        if (i > str.size()-2)
            return fail(
            "在多行注释内部意外结束输入", false);
        }
        i += 2; // 当前解析位置的索引+2, 跳过多行注释的结束标记
        comment_found = true; //找到注释
    }
    else
        return fail("注释格式错误", false);
    }
    return comment_found;
}

//消耗掉空白字符
void RedisValueParser::consumeGarbage() {
    consumeWhitespace();
}

//获取下一个有效的JSON标记（token），并移动解析位置
char RedisValueParser::getNextToken() {
    consumeGarbage(); // 跳过空白字符
    if (failed) return static_cast<char>(0); // 如果解析失败，返回0
    if (i == str.size())
        return fail("意外到达输入的末尾", static_cast<char>(0)); // 到达输入末尾，标记错误并返回0

    return str[i++]; // 返回下一个字符并将位置前进
}

// 将Unicode码点pt转换为UTF-8编码，并追加到输出字符串out中
void RedisValueParser::encodeUTF8(long pt, std::string & out) {
    // 如果pt小于0，表示无效的Unicode码点，不执行编码
    if (pt < 0)
        return;
    if (pt < 0x80) {   // 对于单字节UTF-8编码，Unicode码点在0x00-0x7F范围内
        out += static_cast<char>(pt); // 直接将pt添加到输出中
    } else if (pt < 0x800) {  // 对于双字节UTF-8编码，Unicode码点在0x80-0x7FF范围内
        out += static_cast<char>((pt >> 6) | 0xC0); // 2字节UTF-8编码的第一个字节
        out += static_cast<char>((pt & 0x3F) | 0x80); // 2字节UTF-8编码的第二个字节
    } else if (pt < 0x10000) {   // 对于三字节UTF-8编码，Unicode码点在0x800-0xFFFF范围内
        out += static_cast<char>((pt >> 12) | 0xE0); // 3字节UTF-8编码的第一个字节
        out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80); // 3字节UTF-8编码的第二个字节
        out += static_cast<char>((pt & 0x3F) | 0x80); // 3字节UTF-8编码的第三个字节
    } else {   // 对于四字节UTF-8编码，Unicode码点在0x10000-0x10FFFF范围内
        out += static_cast<char>((pt >> 18) | 0xF0); // 4字节UTF-8编码的第一个字节
        out += static_cast<char>(((pt >> 12) & 0x3F) | 0x80); // 4字节UTF-8编码的第二个字节
        out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80); // 4字节UTF-8编码的第三个字节
        out += static_cast<char>((pt & 0x3F) | 0x80); // 4字节UTF-8编码的第四个字节
    }
}

std::string RedisValueParser::parseString() {
    std::string out;  // 用于存储解析后的字符串
    long last_escaped_codepoint = -1;  // 用于存储上一个转义的Unicode码点，初始化为-1
    //str是RedisValueParser类的成员变量，是要解析的JSON字符串
    while (true) {
        if (i == str.size())
            return fail("在字符串中意外遇到输入结束", "");

        char ch = str[i++];  // 获取str的当前字符

        if (ch == '"') {
            encodeUTF8(last_escaped_codepoint, out);  // 将上一个转义的Unicode码点编码为UTF-8并添加到输出字符串out
            return out;  // 返回解析后的字符串
        }

        if (in_range(ch, 0, 0x1f)) //判断字符ch是否是控制字符
            //esc函数对字符ch进行转义，返回字符ch本身和其ASCII码
            return fail("在字符串中出现未转义的控制字符 " + esc(ch) + "", "");

        // 常见情况：非转义字符
        if (ch != '\\') {
            encodeUTF8(last_escaped_codepoint, out);  // 将上一个转义的Unicode码点编码为UTF-8并添加到输出字符串
            last_escaped_codepoint = -1;  // 重置上一个转义的Unicode码点
            out += ch;  // 将当前字符添加到输出字符串
            continue;
        }

        // 处理转义字符
        if (i == str.size())
            return fail("在字符串中意外遇到输入结束", "");

        ch = str[i++];  // 获取下一个字符
        /*
        unicode转义序列以 \u 开始，后面跟着4个十六进制数字，表示一个Unicode码点，例如 \u4e2d
        */
        if (ch == 'u') {
            // 提取4字节的转义序列，4个字节对应4个char字符
            std::string esc = str.substr(i, 4);
            // 明确检查子字符串的长度，以下循环依赖于std::string在访问str[length]时返回终止的NUL。在此处进行检查可以减少脆弱性。
            if (esc.length() < 4) {
                return fail("不合法的 \\u 转义序列: " + esc, "");
            }
            //遍历这4个字符，检查每个字符是否是合法的十六进制数字
            for (size_t j = 0; j < 4; j++) {
                //每个字符十六进制数字都应该在0-9或a-f或A-F范围内
                if (!in_range(esc[j], 'a', 'f') && !in_range(esc[j], 'A', 'F')
                        && !in_range(esc[j], '0', '9'))
                    return fail("不合法的 \\u 转义序列: " + esc, "");
            }
            //将string转为十六进制的long int类型，得到一个Unicode码点
            long codepoint = strtol(esc.data(), nullptr, 16);

            // JSON规定超出BMP的字符应编码为一对4位十六进制数字的\u转义，分别编码其代理对组件。检查我们是否处于这样的情况：
            // 上一个码点是一个已转义的前导（高位）代理，而这是一个尾随（低位）代理。
            if (in_range(last_escaped_codepoint, 0xD800, 0xDBFF)
                    && in_range(codepoint, 0xDC00, 0xDFFF)) {
                // 将两个代理对重新组合成一个astral-plane字符，按照UTF-16算法。
                encodeUTF8((((last_escaped_codepoint - 0xD800) << 10)
                                | (codepoint - 0xDC00)) + 0x10000, out);
                last_escaped_codepoint = -1;  // 重置上一个转义的Unicode码点
            } else {
                encodeUTF8(last_escaped_codepoint, out);  // 将上一个转义的Unicode码点编码为UTF-8并添加到输出字符串
                last_escaped_codepoint = codepoint;  // 更新上一个转义的Unicode码点
            }

            i += 4;
            continue;
        }

        encodeUTF8(last_escaped_codepoint, out);  // 将上一个转义的Unicode码点编码为UTF-8并添加到输出字符串
        last_escaped_codepoint = -1;

        if (ch == 'b') {
            out += '\b';
        } else if (ch == 'f') {
            out += '\f';
        } else if (ch == 'n') {
            out += '\n';
        } else if (ch == 'r') {
            out += '\r';
        } else if (ch == 't') {
            out += '\t';
        } else if (ch == '"' || ch == '\\' || ch == '/') {
            out += ch;
        } else {
            return fail("无效的转义字符 " + esc(ch), "");
        }
    }
}


RedisValue RedisValueParser::expect(const std::string &expected, RedisValue res) {
    assert(i != 0);  // 断言确保输入位置不为0，即确保有字符可读取
    i--;  // 回退一个字符位置，以便从当前字符重新开始比较
    //比较str中从i开始的expected.length()个字符 和 expected字符串 是否相等
    if (str.compare(i, expected.length(), expected) == 0) { // 如果相等
        //将i前进expected.length()个字符，跳过匹配的字符串   
        i += expected.length();  // 前进输入的位置，以匹配预期的字符串
        return res;  // 返回指定的结果
    } else {
        return fail("解析错误：期望 " + expected + "，但实际得到 " + str.substr(i, expected.length()));
        // 如果未找到预期的字符串，返回解析错误信息，包括预期的字符串和实际找到的部分
    }
}


RedisValue RedisValueParser::parseRedisValue(int depth) {
    if (depth > max_depth) { // 如果深度超过了最大嵌套深度
        return fail("超过了最大嵌套深度");
    }

    char ch = getNextToken(); // 获取下一个字符符
    if (failed) // 如果解析失败，返回空的RedisValue对象
        return RedisValue();


    if (ch == '"') // 如果是字符串
        return parseString(); // 解析字符串


    if (ch == '{') { // 如果是对象开始 {"key" : value}
        std::map<std::string, RedisValue> data; // 创建一个键值对映射
        ch = getNextToken();
        if (ch == '}') //如果键值对为空，返回空map
            return data;

        while (1) {
            if (ch != '"')
                return fail("在对象中期望 '\"'，得到 " + esc(ch));

            std::string key = parseString(); // 解析键
            if (failed)
                return RedisValue();

            ch = getNextToken();
            if (ch != ':')
                return fail("在对象中期望 ':'，得到 " + esc(ch));

            data[std::move(key)] = parseRedisValue(depth + 1); // 解析值并存入映射
            if (failed)
                return RedisValue();

            ch = getNextToken();
            if (ch == '}')
                break;
            if (ch != ',')
                return fail("在对象中期望 ','，得到 " + esc(ch));

            ch = getNextToken();
        }
        return data; // 返回解析后的对象
    }

    if (ch == '[') { // 如果是数组开始  [value, value, ...]
        std::vector<RedisValue> data; // 创建一个 JSON 数组
        ch = getNextToken();
        if (ch == ']')  // 如果数组为空，返回空数组
            return data;

        while (1) {
            i--;
            data.push_back(parseRedisValue(depth + 1)); // 解析值并添加到数组
            if (failed)
                return RedisValue();

            ch = getNextToken();
            if (ch == ']')
                break;
            if (ch != ',') //解析下一个值之前应该有逗号
                return fail("在数组中期望 ','，得到 " + esc(ch));

            ch = getNextToken();
            (void)ch;
        }
        return data; // 返回解析后的数组
    }

    return fail("期望值，得到 " + esc(ch)); // 如果都不匹配，返回解析失败
}