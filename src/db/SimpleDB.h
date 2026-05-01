#pragma once
#include <vector>
#include <string>
#include <sqlite3.h>
#include "Log/Logger.h"
namespace muduowebserv {

class SimpleDB {
public:
      //读取数据库
    static bool open(const std::string& dbname = "myserver.db");
      //关闭数据库
    static void close();
    //执行sql语句
    static bool execute(const std::string& sql);
    //查询数据
    static std::vector<std::vector<std::string>> query(const std::string& sql);
    //sql注入
    static std::vector<std::vector<std::string>> queryWithParams(
      const std::string& sql,
      const std::vector<std::string>&params
    );

private:
    static sqlite3* db_;  //数据库句柄，声明

};
}

