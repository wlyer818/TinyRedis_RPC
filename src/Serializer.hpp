#ifndef SERIALIZER_HPP
#define SERIALIZER_HPP

#include <vector>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
using namespace std;

/*
序列化和反序列化：
序列化是将数据结构或对象转换为可以存储或传输的格式的过程，以便在需要的时候可以恢复到原始的数据结构或对象。
反序列化是将序列化的数据转换为原始数据结构或对象的过程。
例如一个结构体：
序列化就是将这个结构体的数据解释为char*
反序列化就是将这个char*的字符串重新解释为目标结构体
*/


/*
    采用vector<char> 因为是一个动态数组，可以存储任意数量的 char 元素，、
    并且可以动态地增加或减少元素。这使得 std::vector<char> 非常适合用作字节流的缓冲区

    然而，std::vector<char> 并没有提供一些字节流操作所需要的功能，
    例如移动当前位置、查找特定的字节、检查是否已经到达末尾等。
    因此，StreamBuffer 类继承自 std::vector<char>，并添加了这些功能。
*/

class StreamBuffer : public vector<char>
{
public:
    //默认构造函数：初始化当前字节流的位置m_curpos为0
    StreamBuffer():m_curpos(0){}
    //构造函数：初始化当前字节流的位置m_curpos为0，并将输入的字符数组in的前len个字节插入到StreamBuffer缓冲区的起始位置
    StreamBuffer(const char* in, size_t len)
    {
        m_curpos = 0;
        insert(begin(), in, in+len);
    }
    //析构函数
    ~StreamBuffer(){};
    
    //将当前字节流的位置m_curpos重置为0
    void reset(){ m_curpos = 0; }
    
    //获取缓冲区的数据，不能直接返回this，因为this是指向当前StreamBuffer对象的指针，而不是指向其中数据的指针
    const char* data(){ return &(*this)[0]; } //(*this)[0]是StreamBuffer缓冲区中的第一个元素，&(*this)[0]就是第一个元素的地址
    
    // 获取当前位置的数据
    const char* current(){ 
        return&(*this)[m_curpos]; 
    } 

    void offset(int offset){ m_curpos += offset; } //移动当前位置，将当前位置向后移动offset个字节

    bool is_eof(){ return m_curpos >= size(); } //检查是否已经到达缓冲区末尾

    //将字符数组in的前len个字节插入到m_iodevice的末尾（序列化）
    void input(const char* in, size_t len) //输入字符数组（追加到缓冲区末尾）
    {
        insert(end(), in, in+len);
    }

    int findc(char c) //在缓冲区 [当前位置, 缓冲区末尾] 区间查找特定的字节c，返回c的位置
    {
       iterator itr = find(begin()+m_curpos, end(), c);		
        if (itr != end())
        {
            return itr - (begin()+m_curpos);
        }
        return -1;
    }

private:
    unsigned int m_curpos; //当前字节流的位置

};

/*
   序列号和反序列号的类
   用于将数据序列化为字节流，或者将字节流反序列化为数据

    存储数据：写入数据长度和数据本身
    读取数据：读取数据长度，然后读取数据本身
*/

class Serializer
{
    
public:
    //字节序 左边是高位，右边是低位；栈底在高地址，栈顶在低地址
    /*
    例如 0x12345678，从高位到低位的字节依次是0x12、0x34、0x56和0x78
    数据表示：左边是低地址，右边是高地址
    大端：高位字节存储在低地址，低位字节存储在高地址，即：0x12345678
    小端：低位字节存储在低地址，高位字节存储在高地址，即：0x78563412
    */
    enum ByteOrder
    {
        BigEndian = 0, //大端：高位字节存储在低地址
        LittleEndian = 1 //小端：低位字节存储在低地址
    };

    //默认构造函数：初始化字节序为小端存储
    Serializer(){ m_byteorder = LittleEndian; }

    //构造函数
    Serializer(StreamBuffer dev, int byteorder = LittleEndian)
    {
        m_byteorder = byteorder; //初始化字节序，默认为小端存储
        m_iodevice = dev; //将输入的字节流缓冲区赋值给m_iodevice
    }

    //重置字节流缓冲区
    void reset(){ 
        m_iodevice.reset();
    }

    //获取字节流缓冲区的大小
    int size(){
		return m_iodevice.size();
	}

    //跳过k个字节的数据
    void skip_raw_date(int k){
        m_iodevice.offset(k);
    }

    //获取字节流缓冲区的数据（缓冲区中第一个元素的地址）
    const char* data(){
        return m_iodevice.data();
    }

    //大端序转小端序
    void byte_orser(char* in, int len){
		if (m_byteorder == BigEndian){
			reverse(in, in+len); //大端的话直接反转 
		}
	}
    /**
	 * @brief 将指定数据写入序列化器。
	 * @param in 输入的字符数组。
	 * @param len 输入字符数组的长度。
	 */

    //将输入的字符数组in的前len个字节写入到字节流缓冲区的末尾
    void write_raw_data(char* in, int len){
		m_iodevice.input(in, len); //写入字符数组（追加到缓冲区末尾）
		m_iodevice.offset(len); //并将当前位置向后移动len个字节
	}

    // 获取m_iodevice当前位置的数据
    const char* current(){
		return m_iodevice.current();
	}

    /**
	 * @brief 清空序列化器中的数据。
	 */
	void clear(){
		m_iodevice.clear();  //清空字节流缓冲区
		reset(); //m_curpos重置为0
	}
        /**
	 * @brief 输出指定类型的数据。
	 * @tparam T 要输出的数据类型。
	 * @param t 要输出的数据。
	 */
    template<typename T>
	void output_type(T& t);  //反序列化，将字节流缓冲区m_iodevice中的数据写入到t中

    /**
	 * @brief 输入指定类型的数据。
	 * @tparam T 要输入的数据类型。
	 * @param t 要输入的数据。
	 */
    template<typename T>
	void input_type(T t);  //序列化，将t中的数据写入到字节流缓冲区m_iodevice的末尾
    /**
	 * @brief 重载运算符>>，用于从序列化器中读取数据。
	 * @tparam T 要读取的数据类型。
	 * @param i 用于存储读取结果的变量。
	 * @return 当前序列化器对象的引用。
	 */

    //重载输入运算符>>，用于从序列化器中读取数据
    //反序列化，将字节流缓冲区m_iodevice中的数据写入到i中
    template<typename T>
    Serializer &operator >>(T& i){
        output_type(i); 
        return *this;
    }

    /**
	 * @brief 重载运算符<<，用于向序列化器中写入数据。
	 * @tparam T 要写入的数据类型。
	 * @param i 要写入的数据。
	 * @return 当前序列化器对象的引用。
	 */

    //重载输出运算符<<，用于向序列化器中写入数据
    //序列化，将i中的数据写入到字节流缓冲区m_iodevice的末尾
    template<typename T>
	Serializer &operator << (T i){
		input_type(i);
		return *this;
	}    

private:
    int m_byteorder; //字节序,大端存储还是小端存储
    StreamBuffer m_iodevice; //字节流缓冲区
    

};  

//input_type，output_type 提供了多种的特化和偏特化的实现，同样是为了满足 多参数函数 的序列化的需求

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
template<>
inline void Serializer::output_type(std::string& in){
    int marklen = sizeof(uint16_t); //读取长度

    char* d = new char[marklen];
    memcpy(d, m_iodevice.current(), marklen); //将字节流的数据的两个字节拷贝到d中
    int len = *reinterpret_cast<uint16_t*>(&d[0]);  //取出长度
    byte_orser(d, marklen); //将d转为小端序
    m_iodevice.offset(marklen);  //将当前字节流位置向后移动 2 个字节
    delete [] d;
    if(len == 0) return;
    in.insert(in.begin(), m_iodevice.current(), m_iodevice.current() + len); //输入到in中
	m_iodevice.offset(len);
}

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

/**
 * @brief 输入字符串到序列化器中。
 * @param in 要输入的字符串。
 */
template<>
inline void Serializer::input_type(std::string in)
{
    //先将字符串的长度输入到字节流中
	uint16_t len = in.size(); 
	char* p = reinterpret_cast<char*>(&len);
	byte_orser(p, sizeof(uint16_t));
	m_iodevice.input(p, sizeof(uint16_t)); 
	if (len == 0) return;

    //再将字符串的数据输入到字节流中
	char* d = new char[len];
	memcpy(d, in.c_str(), len); //将字符串的数据拷贝到d中
	m_iodevice.input(d, len);
	delete [] d;
}

/**
 * @brief Inputs a null-terminated string into the serializer.
 * @param in The null-terminated string to input.
 */
template<>
inline void Serializer::input_type(const char* in)
{
	input_type<std::string>(std::string(in)); //调用input_type<std::string>函数
}

#endif