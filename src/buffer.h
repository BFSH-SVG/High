#pragma once 
#include <vector>
#include <string>
#include <unistd.h>   //read write
#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
namespace muduowebserv {
    static const int kTempBufferSize = 65536;
    class Buffer{
    public:
    Buffer():readIndex_(0),writeIndex_(0) {
        
    }
    //可读字节数
    size_t readableBytes() const {
        return writeIndex_ - readIndex_;
    }
    //可写空间
    size_t writeableBytes() const {
        return buffer_.size() - writeIndex_;
    }
    //可读数据的起始位置
    const char* peek() const{
        return &buffer_[readIndex_];
     } 
     //可写数据的起始位置
     const char*beginWrite() const {
         return &buffer_[writeIndex_];
     }
     //移动读指针，检索
     void retrieve(size_t len) {
        readIndex_ += len;
     }
     //取出所有数据并转化成string
     std::string retrieveAllAsString(size_t len) {
        const char* startRead = peek();
        std::string str(startRead,len);
        retrieve(len);  //移动读指针
        return str;
     }
     //向缓冲区增加数据,第一步先判断缓冲区是否大于数据大小
     void ensureWriteableBytes(size_t len) {
        if(writeableBytes() < len) {
            //空间不够，需要扩容
            buffer_.resize(writeIndex_+len);
        }
     }
     void append(const char* data,size_t len) {
        //先确保写空间够
        ensureWriteableBytes(len);
        for(size_t i=0;i<len;i++) {
            buffer_[writeIndex_] = data[i];
            writeIndex_++;
        }

     }
     //从socketfd里面读取数据到缓冲区
     ssize_t readFromFd(int fd) {
        char buff[kTempBufferSize];
         ssize_t n=::read(fd,buff,kTempBufferSize);  //fd可能为负
         if(n>0) {
            append(buff,n);
            return n;  //读取了也需要返回让调用者知道
         } else if(n==0) {
            return n; //EOF
         }else {
            return -1;
         }

     }
     //查找\r\n,换行符,方便按行处理数据
     const char* findCRLE() const {
        const char* target = "\r\n";
        const size_t len = strlen(target);
        const char* found=std::search(peek(),beginWrite(),target,target+len); 
        //检查是否找到
        if(found!=beginWrite()) {
			return found;
        }else {
			return nullptr;
		}
     }
	 //将数据写入缓冲区发送给fd
	int writeToFd(int fd,int* saveErrno) {
		int n=::write(fd,peek(),readableBytes());
		if(n<0) {
            if(saveErrno){
                *saveErrno = errno;
            }
			
		} else {
			retrieve(n);  //移动读指针，到读完数据的下一个字符
		}
		return n;
	}
	//查看32位长度,不移动指针  [4位长度][数据]
	int32_t catInt32() {
		int32_t len = 0;
		memcpy(&len,peek(),sizeof(len));
		return ntohl(len);
	}
	//读取32位整数(移动指针) 
	int32_t readInt32() {
		int32_t result = catInt32();
		//移动指针
		retrieve(sizeof(int32_t));
		return result;
	}
	//交换
	void swap(Buffer&buf) {
		buffer_.swap(buf.buffer_);  //交换数据
		std::swap(buf.readIndex_,readIndex_); //交换读指针
		std::swap(buf.writeIndex_,writeIndex_); //交换写指针
	}
	//收缩buffer
	void shrink() {
		Buffer newBuffer;
		newBuffer.ensureWriteableBytes(readableBytes());
		//添加数据
		newBuffer.append(peek(),readableBytes());
		//交换
		swap(newBuffer);  //交换缓冲区，新缓冲区的数据就是旧缓冲区的数据
	}





    private:
        std::vector<char>buffer_; //存储数据
        size_t readIndex_;  //读位置
        size_t writeIndex_;   //写位置
    };

}