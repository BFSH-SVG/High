#pragma once
#include <string>
#include <map>
namespace muduowebserv {
    class HttpResponse {
    public:
        HttpResponse():statusCode_("200") {
        }
        //设置状态码
        void setStatusCode(const std::string& statuscode) {
            statusCode_ = statuscode;
        }
        const std::string& getStatusCode() const {
            return statusCode_;
        }
        void statusMessage(const std::string&statusmessage) {
            statusMessage_ = statusmessage;
        }
        const std::string& getStatusMessage() const {
            return statusMessage_;
        }
        //添加头部
        void addHeader(const std::string& key,const std::string&value) {
            headers_[key] = value;
        }
        //设置内容
        void setBody(const std::string& body){
            body_ = body;
        }
        //转成字符串
        std::string toString() const {
            //提取状态行
            std::string response;
            response+= "HTTP/1.1 "+statusCode_+" "+statusMessage_+"\r\n";
            //提取头部
            for(auto header:headers_) {
                response+=header.first+": "+header.second+"\r\n";
            }
            response+="Content-Length: "+std::to_string(body_.size())+"\r\n";
            response+="\r\n";
            //提取内容
            response+=body_;
            return response;
        }
    private:
        std::string statusCode_;    //状态码
        std::string statusMessage_;  //状态信息
        std::map<std::string,std::string>headers_;  //存储请求头
        std::string body_;  //内容
    };
}

    