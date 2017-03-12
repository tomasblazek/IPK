
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>

#define BUFSIZE 1024

/*
bool sendfile(int sock, FILE *f)
{
    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
    rewind(f);
    if (filesize == EOF)
        return false;
    if (!sendlong(sock, filesize))
        return false;
    if (filesize > 0)
    {
        char buffer[1024];
        do
        {
            size_t num = min(filesize, sizeof(buffer));
            num = fread(buffer, 1, num, f);
            if (num < 1)
                return false;
            if (!senddata(sock, buffer, num, 0))
                return false;
            filesize -= num;
        }
        while (filesize > 0);
    }
    return true;
}
 */

int main (int argc, const char *argv[]) {
	int rc;
	int welcome_socket;
	struct sockaddr_in6 sa;
	struct sockaddr_in6 sa_client;
	char str[INET6_ADDRSTRLEN];
    int port_number = 6677;
    char root_folder[1024];
    bool root_folder_def = false;


    if(argc > 5 || argc == 4 || argc == 2){
        fprintf(stderr,"Error: Bad count of argument %d. (max 4)\n",argc-1);
        //fprintf(stderr,"usage: %s -r <ROOT-FOLDER> -p <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //fprintf(stderr,"usage: %s -r <ROOT-FOLDER> -p <port>\n", argv[0]);
    int argument;
	while ((argument = getopt(argc, (char* const*) argv, "p:r:")) != -1) {
		switch (argument) {
			case 'r':
				printf("Root directory: %s\n",optarg);
                strcpy(root_folder,optarg);
                root_folder_def = true;
				break;
			case 'p':
                port_number = atoi(optarg);
				printf("Port number: %s (%d)\n", optarg,port_number);
				break;
			default:
                fprintf(stderr,"Error: Unknown parameters.\n");
                exit(EXIT_FAILURE);
		}
	}

    if(!root_folder_def) {
        if (getcwd(root_folder, sizeof(root_folder)) == NULL) {
            perror("getcwd() error\n");
            exit(EXIT_FAILURE);
        }
    }

	fprintf(stderr,"-r <ROOT-FOLDER> %s -p <port> %d\n", root_folder , port_number);

	socklen_t sa_client_len=sizeof(sa_client);
	if ((welcome_socket = socket(PF_INET6, SOCK_STREAM, 0)) < 0)
	{
		perror("ERROR: socket");
		exit(EXIT_FAILURE);
	}
	
	memset(&sa,0,sizeof(sa));
	sa.sin6_family = AF_INET6;
	sa.sin6_addr = in6addr_any;
	sa.sin6_port = htons(port_number);	
        
    
    
	if ((rc = bind(welcome_socket, (struct sockaddr*)&sa, sizeof(sa))) < 0)
	{
		perror("ERROR: bind");
		exit(EXIT_FAILURE);		
	}
	if ((listen(welcome_socket, 1)) < 0)
	{
		perror("ERROR: listen");
		exit(EXIT_FAILURE);				
	}
	while(1)
	{

		int comm_socket = accept(welcome_socket, (struct sockaddr*)&sa_client, &sa_client_len);		
		if (comm_socket > 0)
		{
			if(inet_ntop(AF_INET6, &sa_client.sin6_addr, str, sizeof(str))) {
				printf("INFO: New connection:\n");
				printf("INFO: Client address is %s\n", str);
				printf("INFO: Client port is %d\n", ntohs(sa_client.sin6_port));
			}

			char buff[BUFSIZE];
			int res = 0;
			for (;;)		
			{
                bzero(buff, BUFSIZE);
				res = recv(comm_socket, buff, sizeof(buff),0);
                if (res <= 0)                
                    break;
                printf("Client message: %s", buff);

			    send(comm_socket, buff, strlen(buff), 0);
			}

            /*
            FILE *file = fopen("text.txt",rb);
            if(file == NULL)
                exit(EXIT_FAILURE);
            sendfile(comm_socket,file);
             */

		}
		else
		{
			printf(".");
		}
		printf("Connection to %s closed\n",str);
		close(comm_socket);
	}	
}


