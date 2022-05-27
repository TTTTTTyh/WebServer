#include "server/web_server.h"
#include "log/log.h"
int main() {
    web_server server(3366, web_server::triger_mode::con_listen_ET, 60000, false, 3306, "onvacation", "1234"
        , "updb", 12, 6, true, 1, 1024);
    server.start();    
}