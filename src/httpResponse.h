#pragma once
#include <string>
#include <map>
namespace muduowebserv {
    //添加状态码
    enum HttpStatusCode {
        k200OK = 200,
        k400badRequest = 400,
        k404notFound = 404,
        k500internalServerError = 500
    };
    class HttpResponse {
    public:
        static std::string statusCodeToString(HttpStatusCode statusCode)  {
            switch(statusCode) {
                case k200OK: return "OK";
                case k400badRequest: return "Bad Request";
                case k404notFound: return "Not Found";
                case k500internalServerError: return "Internal Server Error";
                default: return "Unknown";
            }
        } 
        HttpResponse():statusCode_(k200OK) {
        }
        //设置状态码
        void setStatusCode(HttpStatusCode status) {
            statusCode_ = status;
            statusMessage_ = statusCodeToString(status);
        }
        HttpStatusCode getStatusCode() const {
            return statusCode_;  //这里选择值传递是因为数据小，引用反而要间接寻址，再取值
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
            response+= "HTTP/1.1 "+std::to_string(statusCode_)+" "+statusMessage_+"\r\n";
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
        HttpStatusCode statusCode_;    //状态码
        std::string statusMessage_;  //状态信息
        std::map<std::string,std::string>headers_;  //存储请求头
        std::string body_;  //内容
    };
}

    