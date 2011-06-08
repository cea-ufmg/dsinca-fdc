#ifndef IO_EFIS_H
#define IO_EFIS_H

//#include <windows.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>

#include <time.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include "udp_message_fg.h"

using namespace std;

class ioSocket{ 

private:
    int Sock;
    struct sockaddr_in SockAddrClient;
    FGNetFDM fdm; //Estrutura de dados do FlithGear a ser enviada para a EFIS  

public:

    ioSocket(const char*, uint16_t);   
	~ioSocket();

	int SendData(double, double, double);

};

#endif
