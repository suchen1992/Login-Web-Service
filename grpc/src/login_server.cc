#include <memory>
#include <iostream>
#include <string>
#include <sstream>
#include <stdio.h>
#include <vector>
#include <map>
#include <fstream>

#include <grpcpp/grpcpp.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include "mysqlpool.h"

#include "login.grpc.pb.h"

#define  MAX_RECEIVE_SIZE 5000
#define  MAX_SEND_SIZE 5000

#define  HASH_ITERATION 10000
#define  KEY_LEN      32
#define  KEK_KEY_LEN  20
#define  ALGORITHM "PBKDF2SHA1"

#define  MYSQL_ADDRESS "127.0.0.1"
#define  MYSQL_USRNAME "root"
#define  MYSQL_PORT 3306
#define  MYSQL_USRPASSWORD "sc92115"
#define  MYSQL_USEDB "login_service"
#define  MYSQL_MAX_CONNECTCOUNT 10

using grpc::Server;
using grpc::ServerWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using login::LoginService;
using login::CommonRequest;
using login::CheckRequest;
using login::SignUpReply;
using login::LoginReply;
using login::CheckReply;

using namespace std;

class LoginServiceImpl final : public LoginService::Service {
  Status SignUp(ServerContext* context, const CommonRequest* request, SignUpReply* reply) {
    string username = request -> user_name();
    string pwd = request -> password();
    cout<<"name " << username <<" pwd " << pwd << endl;
    string dbPwd = searchPwdByUserName(username);
    // db一致
    cout << "db pwd:" << dbPwd << endl;
    if(!dbPwd.empty()) {
      reply -> set_resp_code(1001);
      reply -> set_message("already exist");
      return Status::OK;
    }
    unsigned char salt[16] = {0};
    RAND_bytes(salt, sizeof(salt));
    string saltStr="";
    stringstream ss;
    for(int i=0;i<sizeof(salt);i++) { 
      string tmp;
      ss<<hex<<int(salt[i])<<endl;
      ss>>tmp;
      saltStr+=tmp;
    }

    cout << "pass: " << pwd << endl;
    cout << "ITERATION: : " << HASH_ITERATION << endl;
    cout << "salt: " << saltStr << endl;
    string cypher = getCyphertext(saltStr, pwd);
    cout<<cypher<<endl;
    // insert db
    bool result = insertUserInfo(username, cypher);
    if (result) {
      // 注册成功
      reply -> set_resp_code(200);
      reply -> set_message("succ");
    } else {
      // 注册失败
      reply -> set_resp_code(1002);
      reply -> set_message("sign up failed");
    }
    return Status::OK;
  }

  Status Login(ServerContext* context, const CommonRequest* request, LoginReply* reply) {
    string username = request -> user_name();
    string pwd = request -> password();
    string dbPwd = searchPwdByUserName(username);
    string token;
    if (dbPwd.empty()) {
      reply -> set_resp_code(1003);
      reply -> set_message("user not exists");
      return Status::OK;
    }
    vector<string> dbCypherVec = strSplit(dbPwd, "$");
    string cypherTxt = getCyphertext(dbCypherVec[2], pwd);
    vector<string> cypherVec = strSplit(cypherTxt, "$");
    if (dbCypherVec[3].compare(cypherVec[3]) == 0) {
      // give token
      unsigned char token[6] = {0};
      RAND_bytes(token, sizeof(token));
      string tokenStr="";
      stringstream ss;
      for(int i=0;i<sizeof(token);i++) { 
        string tmp;
        ss<<hex<<int(token[i])<<endl;
        ss>>tmp;
        tokenStr+=tmp;
      }
      cout << "token:" << tokenStr << endl;
      tokenMap[username] = tokenStr;
      reply -> set_resp_code(200);
      reply -> set_token(tokenStr);
      reply -> set_message("succ");
    } else {
      reply -> set_resp_code(1004);
      reply -> set_message("pwd is wrong");
      return Status::OK;
    }
    return Status::OK;
  }

  Status CheckStatus(ServerContext* context, const CheckRequest* request, ServerWriter<CheckReply>* writer) {
    cout << "check" << endl;
    string username = request->user_name();
    string expectToken = request->token();
    bool loopFlag = true;
    while(loopFlag) {
      CheckReply reply;
      map<string, string>::iterator iter = tokenMap.find(username);
      string cacheToken;
      if (iter != tokenMap.end()) {
        cacheToken = iter -> second;
        reply.set_token(cacheToken);
      } else {
        loopFlag = false;
      }
      loopFlag = writer -> Write(reply) && (expectToken.compare(cacheToken) == 0);
    }
    return Status::OK;
  }

  string getCyphertext(string saltStr, string pwd) {
    unsigned char *out;
    out = (unsigned char *) malloc(sizeof(unsigned char) * KEK_KEY_LEN);

    string result="";
    stringstream ss;
    if( PKCS5_PBKDF2_HMAC_SHA1(pwd.c_str(), strlen(pwd.c_str()), (unsigned char*)saltStr.c_str(), strlen(saltStr.c_str()), HASH_ITERATION, KEK_KEY_LEN, out) != 0 ) {
        string tmp;
        for(int i=0;i<KEK_KEY_LEN;i++)
        {
            ss<<hex<<int(out[i])<<endl;
            ss>>tmp;
            result+=tmp;
        }
    } else {
        fprintf(stderr, "PKCS5_PBKDF2_HMAC_SHA1 failed\n");
    }
    free(out);
    ostringstream oss;
    oss << ALGORITHM << "$" << HASH_ITERATION << "$" << saltStr << "$" << result;
    return oss.str();
  }

  string searchPwdByUserName(string username) {
    MYSQL *connection = mysql -> getOneConnect();
    string pwd;
    char searchSql[255] = {'\0'};
    sprintf(searchSql, "SELECT pwd FROM user_info WHERE user_name ='%s'", username.c_str());
    cout << searchSql << endl;
    if(mysql_query(connection, searchSql)) {
        cout << "Query Usr_Info Error:" << mysql_error(connection);
    } else {
        MYSQL_RES *searchResult = mysql_use_result(connection);
        MYSQL_ROW searchrow;
        while((searchrow = mysql_fetch_row(searchResult)) != NULL) {
          if (mysql_num_fields(searchResult) > 0) {
            pwd =  searchrow[0];
          }
        }
        if (searchResult != NULL)
        {
          mysql_free_result(searchResult);
        }
    } 
    return pwd;
  }

  bool insertUserInfo(string username, string cypherTxt) {
    MYSQL *connection = mysql -> getOneConnect();
    bool isSucc = true;
    char insertSql[200] = {'\0'};
    sprintf(insertSql, "INSERT INTO user_info(user_name,pwd) VALUES ('%s','%s')", username.c_str(), cypherTxt.c_str());
    if(mysql_query(connection,insertSql)) {
      cout << "insertSql was error" << endl;
      isSucc = false;
    }

    if(!isSucc) {
      mysql_rollback(connection);
      return false;
    } else {
      mysql_commit(connection); 
      return true; 
    } 
  }
  
  vector<string> strSplit(const string& str, const string& delim) {
    vector<string> res;
    if("" == str) return res;
    char * strs = new char[str.length() + 1];
    strcpy(strs, str.c_str()); 
  
    char * d = new char[delim.length() + 1];
    strcpy(d, delim.c_str());
  
    char *p = strtok(strs, d);
    while(p) {
      string s = p; //分割得到的字符串转换为string类型
      res.push_back(s); //存入结果数组
      p = strtok(NULL, d);
    }
  
    return res;
  }   

  public:
    void initMysql() {
      mysql = MysqlPool::getMysqlPoolObject();
      mysql->setParameter(MYSQL_ADDRESS,MYSQL_USRNAME,MYSQL_USRPASSWORD,MYSQL_USEDB,MYSQL_PORT,NULL,0,MYSQL_MAX_CONNECTCOUNT);
    }

  private:
    MysqlPool *mysql;
    map<string, string> tokenMap;
};

void read ( const string& filename, string& data )
{
  ifstream file ( filename.c_str (), ios::in );

	if (file.is_open ()) {
		stringstream ss;
		ss << file.rdbuf ();

		file.close ();

		data = ss.str ();
	}

	return;
}

void RunServer() {
  LoginServiceImpl service;
  service.initMysql();
	std::string server_address ( "localhost:50051" );

	std::string key;
	std::string cert;
	std::string root;
	read ( "../../../../../server.crt", cert );
	read ( "../../../../../server.key", key );
	read ( "../../../../../ca.crt", root );
	grpc::SslServerCredentialsOptions::PemKeyCertPair keycert = { key, cert };
	grpc::SslServerCredentialsOptions sslOps;
	sslOps.pem_root_certs = root;
	sslOps.pem_key_cert_pairs.push_back ( keycert );

  ServerBuilder builder;
	builder.AddListeningPort(server_address, grpc::SslServerCredentials( sslOps ));
  builder.RegisterService(&service);
  unique_ptr<Server> server(builder.BuildAndStart());
  cout << "Server listening on " << server_address << endl;

  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();

  return 0;
}