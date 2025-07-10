#ifndef SKIPLIST_H
#define SKIPLIST_H
#include<iostream>
#include<vector>
#include<memory>
#include<random>
#include<cstring>
#include<string>
#include<fstream>
#include<mutex>
#include"global.h"
#include"RedisValue/RedisValue.h"
#define MAX_SKIP_LIST_LEVEL 32  //跳表的最大层数
#define  PROBABILITY_FACTOR 0.25 //晋升概率 每4个节点中抽取一个节点当作高一层索引的节点
#define  DELIMITER ":"
#define SAVE_PATH "data_file"
//定义跳表节点，包含key，value和指向当前层下一个节点的指针数组
/*

*/

// 跳表节点类，每个节点包含一个key和value，以及一个指向下一个节点的指针数组
template<typename Key,typename Value>
class SkipListNode{
public:
    Key key;
    Value value;
    //因为一个节点可能指向同一层的节点，也可能指向低一层的节点，所以forward是一个数组，其每个元素都是一个指向节点的指针
    //forward[i]中的i表示第i层
    //node->forward[i]表示node节点在第i层的下一个节点
    std::vector<std::shared_ptr<SkipListNode<Key,Value>>>forward; // 指向下一个节点的指针数组
    //跳表节点的构造函数，forward数组的大小为maxLevel，每个元素都初始化为nullptr，表示每层的头节点的下一个节点都是nullptr
    SkipListNode(Key key,Value value,int maxLevel=MAX_SKIP_LIST_LEVEL):
    key(key),value(value),forward(maxLevel,nullptr){}
    
};

// 跳表类，包含头节点和当前跳表的最大层数
template<typename Key, typename Value>
class SkipList{
private:
    int currentLevel; //当前跳表的最大层数
    std::shared_ptr<SkipListNode<Key,Value>>head; //头节点
    std::mt19937 generator{ std::random_device{}()}; //随机数生成器
    std::uniform_real_distribution<double> distribution; //随机数均匀分布
    int elementNumber=0; //跳表元素个数
    std::ofstream writeFile; //写文件
    std::ifstream readFile; //读文件
    std::mutex mutex;
private:
    //随机生成新节点的层数
    int randomLevel();
    bool parseString(const std::string&line,std::string&key,std::string&value);
    bool isVaildString(const std::string&line);
public:
    SkipList();
    ~SkipList();
    bool addItem(const Key& key, const Value& value); //添加节点
    bool modifyItem(const Key& key, const Value& value); //修改节点
    std::shared_ptr<SkipListNode<Key,Value>> searchItem(const Key& key); //查找节点
    bool deleteItem(const Key& key); //删除节点
    void printList(); //打印跳表
    void dumpFile(std::string save_path); //保存跳表到文件
    void loadFile(std::string load_path); //从文件加载跳表
    int size(); //返回跳表元素个数
public:
    int getCurrentLevel(){return currentLevel;} //返回当前跳表的最大层数
    std::shared_ptr<SkipListNode<Key,Value>> getHead(){return head;} //返回头节点
};

/*--------------函数定义---------------------*/

template<typename Key,typename Value>
bool SkipList<Key,Value>::addItem(const Key& key,const Value& value){
    mutex.lock();
    auto currentNode=this->head; //从头节点开始查找
    std::vector<std::shared_ptr<SkipListNode<Key,Value>>>update(MAX_SKIP_LIST_LEVEL,head); //记录每层需要更新的节点
    //从高层向底层查找，找到小于目标键值的最大节点
    for(int i=currentLevel-1;i>=0;i--){
        while(currentNode->forward[i]&&currentNode->forward[i]->key<key){
            currentNode=currentNode->forward[i];
        }
        update[i]=currentNode;
    }
    
    int newLevel=this->randomLevel(); //生成新节点的层数
    currentLevel=std::max(newLevel,currentLevel); //更新当前跳表的最大层数
    std::shared_ptr<SkipListNode<Key,Value>> newNode=std::make_shared<SkipListNode<Key,Value>>(key,value,newLevel); //只分配节点的层高
    //在第 i 层索引中插入新节点
    for(int i=0;i<newLevel;i++){
        newNode->forward[i]=update[i]->forward[i]; //比如在4->6中插入5,先让5->6
        update[i]->forward[i]=newNode; //再让4->5
    }
    elementNumber++;
    mutex.unlock();
    return true;
}

template<typename Key,typename Value>
bool SkipList<Key,Value>::modifyItem(const Key& key, const Value& value){

    std::shared_ptr<SkipListNode<Key,Value>> targetNode=this->searchItem(key);
    mutex.lock();
    if(targetNode==nullptr){ //如果要修改的节点在原始链表中不存在
        mutex.unlock();
        return false;
    }
    targetNode->value=value; //修改节点的值
    mutex.unlock();
    return true;

}

//查找节点
template<typename Key,typename Value>
std::shared_ptr<SkipListNode<Key,Value>> SkipList<Key,Value>::searchItem(const Key& key){
    mutex.lock();
    std::shared_ptr<SkipListNode<Key,Value>> currentNode=this->head; //从头节点开始查找
    if(!currentNode){ //如果头节点为空
        mutex.unlock();
        return nullptr;
    }
    for(int i=currentLevel-1;i>=0;i--){ //从最高层开始往低层查找
        while(currentNode->forward[i]!=nullptr&&currentNode->forward[i]->key<key){ //查找每一层
            currentNode=currentNode->forward[i];
        }
    }
    /*
    循环结束后，到了原始链表层，此时currentNode是原始链表中 key<要查找的key的 最大节点
    比如：原始链表：1->2->3->4->6->7->8->9，要查找的key是6，那么循环结束后，此时currentNode是4
    */

    //若要查找的key存在，则currentNode->forward[0]就是要查找的节点
    //若要查找的key不存在，则currentNode->forward[0]是 第一个key>要查找的key 的节点
    currentNode=currentNode->forward[0]; 
    //判断currentNode->forward[0]是否是要查找的节点
    if(currentNode&&currentNode->key==key){
        mutex.unlock();
        return currentNode;
    }
    mutex.unlock();
    return nullptr;
}

//删除节点
template<typename Key,typename Value>
bool SkipList<Key,Value>::deleteItem(const Key& key){
    mutex.lock();
    std::shared_ptr<SkipListNode<Key,Value>> currentNode=this->head; //从头节点开始查找
    std::vector<std::shared_ptr<SkipListNode<Key,Value>>>update(MAX_SKIP_LIST_LEVEL,head);
    //在每一层中找到要删除的节点的前一个节点（即update[i]），记录在update数组中
    for(int i=currentLevel-1;i>=0;i--){
        while(currentNode->forward[i]&&currentNode->forward[i]->key<key){
            currentNode=currentNode->forward[i];
        }
        update[i]=currentNode;
    }

    //若要删除的key存在，则currentNode->forward[0]就是要删除的节点
    //若要删除的key不存在，则currentNode->forward[0]是 第一个key>要删除的key 的节点
    currentNode=currentNode->forward[0];
    //判断currentNode->forward[0]是否是要删除的节点
    if(!currentNode||currentNode->key!=key){
        mutex.unlock();
        return false;
    }
    for(int i=0;i<currentLevel;i++){
        if(update[i]->forward[i]!=currentNode){
            break; //如果update[i]->forward[i]!=currentNode，说明要删除的节点currentNode在第i层索引中不存在
        }
        //在第i层索引中删除节点，直接将 要删除的节点的前一节点 指向 要删除的节点的下一个节点
        update[i]->forward[i]=currentNode->forward[i];
    }
    currentNode.reset();
    //如果删除掉要删除的节点后，当前索引层就只剩下了头节点head，则将索引层数减1（因为索引层至少要有两个节点）
    while(currentLevel>1&&head->forward[currentLevel-1]==nullptr){
        currentLevel--;
    }
    elementNumber--; //完成删除操作后，元素个数减1
    mutex.unlock();
    return true;
}

//打印跳表
template<typename Key,typename Value>
void SkipList<Key,Value>::printList(){
    mutex.lock();
    for(int i=currentLevel;i>=0;i--){
        auto node=this->head->forward[i];
        std::cout<<"Level"<<i+1<<":";
        while(node!=nullptr){
            std::cout<<node->key<<DELIMITER<<node->value<<"; ";
            node=node->forward[i];
        }
        std::cout<<std::endl;
    }
    mutex.unlock();
}

//保存跳表到文件
template<typename Key,typename Value>
void SkipList<Key,Value>::dumpFile( std::string save_path){
    mutex.lock();
    writeFile.open(save_path); //打开文件
    auto node=this->head->forward[0];  //从第一层开始遍历
    //只遍历原始链表层，因此只保存原始链表层的节点
    while(node!=nullptr){
        writeFile<<node->key<<DELIMITER<<node->value.dump()<<"\n"; //写入文件 dump()函数将value转换为字符串 整数 1->"1" 字符串 "hello"->"hello" 二进制数据 0x01 0x02 0x03->"0x01 0x02 0x03"
        node=node->forward[0];
    }
    writeFile.flush(); //刷新缓冲区 写入文件 直接写入文件 不用等到文件关闭 
    // 众所周与,你所要输出的内容会先存入缓冲区,而flush()的作用正是强行将缓冲区的数据清空
    writeFile.close(); //关闭文件
    mutex.unlock();
}


//从文件加载跳表
template<typename Key,typename Value>
void SkipList<Key,Value>::loadFile(std::string load_path){

    readFile.open(load_path); //打开文件
    if(!readFile.is_open()){ 
        mutex.unlock();
        return;
    }
    std::string line;
    std::string key;
    std::string value;
    std::string err;
    while(std::getline(readFile,line)){ //读取文件
        if(parseString(line,key,value)){
            addItem(key,RedisValue::parse(value,err)); //将字符串转换为RedisValue对象
        }
    }
    readFile.close();

}


template<typename Key,typename Value>
bool SkipList<Key,Value>::isVaildString(const std::string&line){
    if(line.empty()){ //如果line为空
        return false;
    }
    if(line.find(DELIMITER)==std::string::npos){ //如果没有找到分隔符
        return false;
    }
    return true;
}

//解析字符串
template<typename Key,typename Value>
bool SkipList<Key,Value>::parseString(const std::string&line,std::string&key,std::string&value){
    if(!isVaildString(line)){
        return false;
    }
    int index=line.find(DELIMITER); //找到分隔符的位置 返回的是分隔符在line中的索引
    key=line.substr(0,index); //分隔符之前的是key
    value=line.substr(index+1); //分隔符之后的是value
    return true;

} 

//返回跳表元素个数
template<typename Key,typename Value>
int SkipList<Key,Value>::size(){
    mutex.lock();
    int ret=this->elementNumber;
    mutex.unlock();
    return ret;
}


//跳表构造函数
template<typename Key,typename Value>
SkipList<Key,Value>::SkipList()
    :currentLevel(0),distribution(0, 1)
{
    Key key;
    Value value;
    this->head=std::make_shared<SkipListNode<Key,Value>>(key,value); //初始化头节点,层数为最大层数
}

//随机生成 要插入的新节点 应建立的索引的层数
template<typename Key, typename Value>
int SkipList<Key,Value>::randomLevel()
{
    /*
    1/4的概率返回1，表示该要插入的新节点不需要建立索引
    1/16的概率返回2，表示需要建立一级索引
    1/64的概率返回3，表示需要建立二级索引
    1/256的概率返回4，表示需要建立三级索引
    .....
    最多返回level = MAX_SKIP_LIST_LEVEL = 32
    当建立高级索引时，低级索引也会被建立，例如为要插入的新节点建立三级索引时，一级索引和二级索引也会被建立
    */
    int level=1;
    while(distribution(generator)< PROBABILITY_FACTOR
        && level<MAX_SKIP_LIST_LEVEL){
        level++;
    }
    return level;
}

// 跳表析构函数 关闭文件
template<typename Key,typename Value>
SkipList<Key,Value>::~SkipList(){
    if(this->readFile){
        readFile.close();
    }
    if(this->writeFile){
        writeFile.close();
    }
}
#endif