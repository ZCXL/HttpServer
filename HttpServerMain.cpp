/* HTTPServerMain.cpp */

#include<iostream>
#include<string>

#include<stdlib.h>

#include"HttpServer.h"

using namespace std;

int main(int argc, char* argv[])
{
	int port;
	HttpServer* httpServer;

	if(argc == 2){
		port = atoi(argv[1]);
		httpServer = new HttpServer(port);
	}else{
		httpServer = new HttpServer();
	}

	if(httpServer->run()){
		cerr<<"Error starting HTTPServer"<<endl;
	}

	free(httpServer);

	return 0;
}