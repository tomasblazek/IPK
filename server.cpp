
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>

using namespace std;

#define BUFSIZE 1024

bool read_file(int socket, FILE *f, char* buf) {
    long int file_size = atol(buf);

    if (file_size > 0) {
        char buffer[BUFSIZE];
        int size = 0;
        while (size < file_size) {
            bzero(buffer, BUFSIZE);
            int bytesres = (int) recv(socket, buffer, BUFSIZE, 0);
            if (bytesres < 0) {
                perror("Error: in recvfrom\n");
                return false;
            }
            if (fwrite(buffer, bytesres, 1, f) != 1) {
                fprintf(stderr, "Error: fwrite chyba.\n");
                return false;
            }
            size += bytesres;
        }

        if (size != file_size) {
            fprintf(stderr, "Error: size != file_size.\n");
            return false;
        }

    }
    return true;
}

bool writeDataToClient(int sckt, const void *data, int datalen)
{
    const char *pdata = (const char*) data;

    while (datalen > 0){
        int numSent =(int) send(sckt, pdata, datalen, 0);
        if (numSent <= 0){
            if (numSent == 0){
                fprintf(stderr,"The client was not written to: disconnected\n");
            } else {
                perror("The client was not written to");
            }
            return false;
        }
        pdata += numSent;
        datalen -= numSent;
    }

    return true;
}

bool send_file(int socket, FILE *f){
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    rewind(f);
    printf("%ld\n",file_size);
    int bytesc = send(socket,to_string(file_size).c_str(),BUFSIZE, 0);
    if(bytesc < 0){
        fprintf(stderr,"The client was not written to: disconnected\n");
        return false;
    }

    char *file_content = (char*) malloc(file_size);
    if(file_content == NULL){
        fprintf(stderr,"Error: Malloc chyba filecontent.\n");
        return false;
    }
    if(fread(file_content,file_size,1,f) != 1){
        fprintf(stderr,"Error: Chyba fread.\n");
        return false;
    }

    writeDataToClient(socket,file_content,file_size);

    return true;
}


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
                strcat(root_folder,"/");
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

        int comm_socket = accept(welcome_socket, (struct sockaddr *) &sa_client, &sa_client_len);
        if (comm_socket < 0) {
            fprintf(stderr, "Unknown error.");
        }
        /*
        pid_t client_processPID = fork();
        if(client_processPID < 0) {
            perror("Error: Fork client_process failed.\n");

            //kill(0,SIGTERM);
            exit(2);
        }
        if(client_processPID == 0) {
        */
            if (inet_ntop(AF_INET6, &sa_client.sin6_addr, str, sizeof(str))) {
                printf("INFO: New connection:\n");
                printf("INFO: Client address is %s\n", str);
                printf("INFO: Client port is %d\n", ntohs(sa_client.sin6_port));
            }

            //char OK[] = "HTTP/1.1 404 200 OK\n";
            char NotFound[] = "HTTP/1.1 404 Not Found\n";
            //char BadRequest[] = "HTTP/1.1 400 Bad Request\n";

            char buff[BUFSIZE];
            char path[BUFSIZE];
            char workingPath[BUFSIZE];
            strcpy(workingPath,root_folder);
            char c[BUFSIZE];
            FILE *file;
            int res = 0;

            bzero(buff, BUFSIZE);
            res = recv(comm_socket, buff, sizeof(buff), 0);
            if (res <= 0)
                break;
            printf("Client message: %s", buff);

            //GET RESTful OPERATION
            unsigned int i = 0;
            for (i = 0; i < strlen(buff); i++) {
                if (i == 3) {
                    break;
                }
                c[i] = buff[i];
            }
            c[i] = '\0';

            int shift;
            if (!strcmp(c, "GET")) { //get,lst
                //GET path
                i++; //jump over space
                //i++; //jump over /
                shift = i;
                for (; i < strlen(buff); i++) {
                    if (buff[i] == '?') {
                        break;
                    }
                    path[i - shift] = buff[i];
                }
                path[i - shift] = '\0';
                strcat(workingPath,path);

                //GET type file/folder
                shift = i;
                for (; i < strlen(buff); i++) {
                    if (buff[i] == ' ') {
                        break;
                    }
                    c[i - shift] = buff[i];
                }
                c[i - shift] = '\0';
                if (!strcmp(c, "?type=file")) {
                    printf("CESTA:%s\n",workingPath);
                    file = fopen(workingPath, "rb");
                    if (file == NULL) {
                        fprintf(stderr, "Error: Cant open file |%s| for write in operation read.", path);
                        send(comm_socket, NotFound, strlen(NotFound), 0);
                    } else {

                        if (!send_file(comm_socket, file))
                            printf("Chyba pri posilani");

                        fclose(file);
                    }
                } else if (!strcmp(c, "?type=folder")) {
                        //TODO lst

                } else {
                    fprintf(stderr, "Error: Bad specification in header file/folder");
                    exit(EXIT_FAILURE);
                }

            }
            else if(!strcmp(c,"PUT")){
                i++; //jump over space
                i++; //jump over /
                shift = i;
                for (; i < strlen(buff); i++) {
                    if (buff[i] == '?') {
                        break;
                    }
                    path[i - shift] = buff[i];
                }
                path[i - shift] = '\0';
                strcat(workingPath,path);
                printf("%s\n",workingPath);

                //GET type file/folder
                shift = i;
                for (; i < strlen(buff); i++) {
                    if (buff[i] == ' ') {
                        break;
                    }
                    c[i - shift] = buff[i];
                }
                c[i - shift] = '\0';
                if (!strcmp(c, "?type=file")) {

                    FILE *file;
                    file = fopen(workingPath, "wb");
                    if (file == NULL) {
                        perror("Error: Cant open file for write.");
                    } else {
                        bzero(buff, BUFSIZE);
                        res = recv(comm_socket, buff, sizeof(buff), 0);
                        if (res <= 0)
                            break;
                        printf("Client SIZE: %s", buff);

                        bool ok = read_file(comm_socket, file, buff);
                        (void) ok;
                        fclose(file);
                    }

                }
                else
                {

                }

            }
            else {
                printf("Zatim nedefinovana operace!\n");
                send(comm_socket, buff, strlen(buff), 0);
            }

            printf("Connection to %s closed\n", str);
            //exit(EXIT_SUCCESS);
        //}//fork
        close(comm_socket);
	}	
}


