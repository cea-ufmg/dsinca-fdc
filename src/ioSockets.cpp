#include "ioSockets.h"

ioSocket::ioSocket(const char* IP, uint16_t porta){
    
    Sock = 0;
    
    //WSAData wd;
    //WSAStartup(MAKEWORD(2,0),&wd);

    memset(&SockAddrClient,0,sizeof(SockAddrClient));
    SockAddrClient.sin_family = AF_INET;
    SockAddrClient.sin_addr.s_addr = inet_addr(IP);
    SockAddrClient.sin_port = htons(porta);
    
    // Cria socket para comunicação com a EFIS
    if ((Sock = socket(AF_INET,SOCK_DGRAM, IPPROTO_UDP)) < 0) 
	    cerr << "Nao consegui abrir o socket!!!\n\n";
    else
	    cerr << "Consegui abrir o socket!!\n";

    // Bind eh necessario na aplicacao servidor. Aqui, o cliente, nao eh necessario.
    
    //if ((bind(Sock, (const sockaddr*) &SockAddrServer, sizeof(SockAddrServer))) < 0) 
//	    cerr << "Vinculo (binding) do socket ao endereco do servidor nao foi possivel.\n";
    
    // Muda o modo de escrita e leitura no socket para nao bloqueante, isto 
    // eh, nao fica suspenso pelo escalonador do SO em uma operacao de 
    // escrita ou de leitura do scoket.
    fcntl(Sock,F_SETFL,O_NONBLOCK);
}

ioSocket::~ioSocket(){
     //closesocket(Sock);      
     //WSACleanup(); 
}

int ioSocket::SendData(double phi, double theta, double psi){
    int nbytes;
    
    fdm.phi   = phi/57.295;   // roll (radianos)
    fdm.theta = theta/57.295; // pitch (radianos)
    fdm.psi   = psi/57.295; 
    
    nbytes = sendto( Sock, (char *)&fdm, sizeof(fdm),0 ,(struct sockaddr *) &SockAddrClient, sizeof(SockAddrClient));

    if (nbytes != sizeof(fdm)) 
            cout<<"Erro ao enviar dado para o cliente EFIS!" << nbytes << " | " << sizeof(fdm) <<endl;
    
    return 1;
}
