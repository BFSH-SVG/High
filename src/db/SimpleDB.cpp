#include "SimpleDB.h"
#include <vector>
namespace muduowebserv {
sqlite3* SimpleDB::db_ = nullptr;   //静态成员初始化

bool SimpleDB::open(const std::string& dbname) {
    int ret = sqlite3_open(dbname.c_str(),&db_);
    if(ret != SQLITE_OK) {
        LOG_ERROR <<"open db error"<<sqlite3_errmsg(db_);
        return false;
    }else {
        LOG_INFO <<"Database opened: "<< dbname;
        return true;
    }
}
void SimpleDB::close() {
    if(db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}
bool SimpleDB::execute(const std::string& sql) {
    char* errMsg = nullptr;
    int ret = sqlite3_exec(db_,sql.c_str(),nullptr,nullptr,&errMsg);
    if(ret != SQLITE_OK) {
        LOG_ERROR <<"sqlite3_exec error" << errMsg;
        sqlite3_free(errMsg);   //错误信息
        return false;
    }else {
        LOG_INFO <<"sqlite3_exec success"<< sql;
        return true;
    }
}
//查询数据
std::vector<std::vector<std::string>> SimpleDB::query(const std::string& sql) {
    //1创建一个空的二维数组，用来存储结果
    std::vector<std::vector<std::string>> result;
    //2执行sql回调语句
    auto callback = [](void *data,int argc,char **argv,char** colNames)->int{
        //转化指针
        auto result = static_cast<std::vector<std::vector<std::string>>*>(data);
        //4创建一行数据
        std::vector<std::string> row;
        for(int i=0;i<argc;i++) {
            row.push_back(argv[i]?argv[i]:"NULL");
        }
        result->push_back(row);
        return 0;
    };
    //执行sql查询
    char* errMsg = nullptr;
    int ret = sqlite3_exec(db_,sql.c_str(),callback,&result,&errMsg);
    if(ret != SQLITE_OK) {
        LOG_ERROR<<"query error!"<<errMsg;
    } 
    LOG_INFO<<"query success!"<<sql;
    return result;
}
//sql注入实现
std::vector<std::vector<std::string>> SimpleDB::queryWithParams(
      const std::string& sql,
      const std::vector<std::string>&params
) {
    std::vector<std::vector<std::string>> results;
    //预编译
    sqlite3_stmt * stmt = nullptr;
    int ret = sqlite3_prepare16_v2(db_,sql.c_str(),-1,&stmt,nullptr);
    if(ret!=SQLITE_OK){
        LOG_ERROR<<"prepare error"<<sqlite3_errmsg(db_);
        return results;
    }
    //绑定参数
    for(size_t i=0;i<params.size();i++) {
        sqlite3_bind_text(stmt,i+1,params[i].c_str(),-1,SQLITE_TRANSIENT);
    }
    //3逐行读取
    while(sqlite3_step(stmt)==SQLITE_ROW) {
        std::vector<std::string>row;
        int cols = sqlite3_column_count(stmt);
        for(int i=0;i<cols;i++) {
            const char* value = (const char*)sqlite3_column_text(stmt,i);
            row.push_back(value?value:"nullptr");
        }
        results.push_back(row);
    }
    //释放资源
    sqlite3_finalize(stmt);
    LOG_INFO<<"queryWithParams success";
    return results;
}
}
