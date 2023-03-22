#ifndef __CONFIG_H__
#define __CONFIG_H__
#include <string>
#include <fstream>

#include <stdlib.h>
#include <string.h>


struct conf {
    unsigned short SERVER_PORT;
    short M_MYSQL_PORT;
    unsigned int TIME_OUT_MS;
    char SRC_DIR[128];
    bool DAEMON;
    char MYSQL_USER[32];
    char MYSQL_PWD[32];
    char MYSQL_DB[32];
};

static const char *config_file_name = "/home/axieyun_aly_kkb/cppWebServer/config.d/config";

// #define SET_CNF(cnf, key, val) cnf##(#key) = val

void initConf(struct conf &cnf) {
    std::string line;
    std::string key;
    std::string val;
    std::fstream config_file;

    config_file.open(config_file_name, std::ios::in);
    if (!config_file) {
        fprintf(stderr, "config open fail!\n");
        exit(-1);
    }

    int line_size = 512;
    char line_buf[line_size];
    //如果没有读到文件结束就一直读
    while (!config_file.eof()) {
        config_file.getline(line_buf, line_size);
        if (line_buf[0] == '#' || line_buf[0] == '\0'
            || line_buf[0] == '\r' || line_buf[0] == '\n') continue;

        line = line_buf;
        std::string::size_type idx = line.find_last_of('=');
        key = line.substr(0, idx);
        val = line.substr(idx + 1);

        // SET_CNF(cnf, key.c_str(), val.c_str());

        if (key == "SERVER_PORT") {
            cnf.SERVER_PORT = atoi(val.c_str());
        } else if (key == "TIME_OUT_MS") {
            cnf.TIME_OUT_MS = atoi(val.c_str());
        } else if (key == "MYSQL_PORT") {
            cnf.M_MYSQL_PORT = atoi(val.c_str());
        } else if (key == "DAEMON") {
            cnf.DAEMON = atoi(val.c_str());
        } else if (key == "MYSQL_DB") {
            strncpy(cnf.MYSQL_DB, val.c_str(), 16);
        } else if (key == "SRC_DIR") {
            strncpy(cnf.SRC_DIR, val.c_str(), 128);
        } else if (key == "MYSQL_USER") {
            strncpy(cnf.MYSQL_USER, val.c_str(), 32);
        } else if (key == "MYSQL_PWD") {
            strncpy(cnf.MYSQL_PWD, val.c_str(), 32);
        }

    }

    config_file.close();

}



#endif // !__CONFIG_H__
