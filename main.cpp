#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <string>
#include <string.h>

using namespace std;

#include "./pool/threadPool.h"
#include "./http/httpconn.h"
#include "./server/webServer.h"
#include "./config.d/config.h"


int main(/*int argc, char *argv[]*/) {

    // 服务器配置初始化
    struct conf cnf;
    memset(&cnf, 0, sizeof(cnf));
    initConf(cnf);

    printf(GREEN "server port : %d" NONE "\n", cnf.SERVER_PORT);
    printf(GREEN "server scr dir : %s" NONE "\n", cnf.SRC_DIR);
    printf(GREEN "mysql port : %d" NONE "\n", cnf.M_MYSQL_PORT);
    printf(GREEN "mysql user : %s" NONE "\n", cnf.MYSQL_USER);
    printf(GREEN "mysql pwd : %s" NONE "\n", cnf.MYSQL_PWD);
    printf(GREEN "mysql db : %s" NONE "\n", cnf.MYSQL_DB);
    printf(GREEN "time out ms : %d" NONE "\n", cnf.TIME_OUT_MS);
    printf(GREEN "DAEMON : %d" NONE "\n", cnf.DAEMON);

    // 是否启动守护进程
	if(cnf.DAEMON && -1 == daemon(1, 0)) {
	  	printf("daemon error\n");
 	    exit(1);
    }

    axy::WebServer server{cnf.SERVER_PORT, cnf.TIME_OUT_MS
        , cnf.M_MYSQL_PORT, cnf.MYSQL_USER, cnf.MYSQL_PWD, cnf.MYSQL_DB
        , cnf.SRC_DIR
    };

    server.start();

}