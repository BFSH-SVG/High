#pragma once 
#include <string>
#include <map>
#include "buffer.h"
namespace muduowebserv {

    class HttpRequest {
    public:
        //枚举状态机
        enum class ParseState {
            REQUEST_LINE,    //正在解析请求行
            HEADERS,   //请求头
            BODY,  //请求体
            FINISHED  //结束
        };
        HttpRequest() {
            state_ = ParseState::REQUEST_LINE;  //默认解析请求行
        }
        void setMethod(const std::string& method) {
            method_ = method;
        }
        const std::string& getMethod() const {
            return method_;
        }
        void setPath(const std::string&path) {
            path_ = path;
        }
        const std::string& getPath() const {
            return path_;
        }
        void setVersion(const std::string&version) {
            version_ = version;
        }
        const std::string& getVersion() const {
            return version_;
        }
        //设置请求头
        void setHeader(const std::string&key,const std::string&value){
            headers_[key] = value;
        }
        //获取请求头
        const std::string getHeader(const std::string&key) const {
            auto it =headers_.find(key);
            if(it!=headers_.end()) {
                return it->second;
            }
            return "";  //没有找到，返回空字符串
        }
        //解析
        bool parse(Buffer*buf) {     
            while(state_ != ParseState::FINISHED) {
                if(state_ == ParseState::REQUEST_LINE) {
                    //     //1.找到请求行
                    const char* crlf=buf->findCRLE();
                    if(!crlf) {
                        return false; //没有找到\r\n
                    }
                    //2.取出请求行
                    std::string requestLine=buf->retrieveAllAsString(crlf-buf->peek());
                    buf->retrieve(2);  //跳过\r\n
                    //3.解析请求行
                    size_t pos1 = requestLine.find(" ");
                    size_t pos2 = requestLine.find(" ",pos1+1);
                    if(pos1==std::string ::npos||pos2==std::string ::npos) {
                        return false;  //请求行格式错误          
                    }
                    method_ = requestLine.substr(0,pos1);
                    path_ = requestLine.substr(pos1+1,pos2-pos1-1);
                    version_ = requestLine.substr(pos2+1);
                    //状态转换
                    state_ = ParseState::HEADERS;  //转化成请求头
                }else if(state_ == ParseState::HEADERS) {
                    const char* crlf=buf->findCRLE();
                    if(!crlf) {
                        return false;  //没有找到\r\n
                    }
                    std::string headLine=buf->retrieveAllAsString(crlf-buf->peek());
                    //跳过\r\n
                    buf->retrieve(2);
                    if(headLine.empty()) {
                        state_ = ParseState::BODY;
                    }else {
                        //         //解析头部内容
                        size_t colon=headLine.find(":");
                        if(colon!=std::string::npos) {
                            std::string key = headLine.substr(0,colon);
                            std::string value = headLine.substr(colon+2);
                            headers_[key] = value;  //存储请求头部
                        }
                    }
                }else if(state_ == ParseState::BODY) {
                    break;
                }
                return true;
            }
            return state_ == ParseState::FINISHED;
        }
    
    private:
        std::string method_; 
        std::string path_;
        std::string version_;
        std::map<std::string,std::string> headers_;  //瀛樺偍璇锋眰澶撮儴
        ParseState state_;   //状态机状态  
    };

}