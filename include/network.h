#ifndef NETWORK_H
#define NETWORK_H

#define VF_PORT 9090
#define VF_DEFAULT_SERVER "127.0.0.1"

// Protocol Constants
#define CMD_HELLO "HELLO"
#define CMD_PUSH  "PUSH"
#define CMD_PULL  "PULL" 
#define CMD_FORK  "FORK" 

#define RESP_OK   "OK"
#define RESP_ERR  "ERR"

#endif // NETWORK_H
