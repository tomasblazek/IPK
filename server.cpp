
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>

using namespace std;

#define BUFSIZE 1024

#define S_FILE 100
#define S_FOLDER 101

/**
 * Určuje zdali na zadane ceste se nachazi adresar nebo soubor. Pokud neni nalezen vraci false.
 * @param path
 * @param FilOrFol
 * @return
 */
bool fileOrDirectory(char* path, int FilOrFol) {
    struct stat statbuf;

    FILE *f = fopen(path,"r");
    if(f==NULL) {
        return false;
    }
    stat(path, &statbuf);
    if(S_ISDIR(statbuf.st_mode)) {
        //printf("directory\n");
        if (FilOrFol == S_FOLDER){
            return true;
        }
        else {
            return false;
        }
    }
    else {
        //printf("file\n");
        if (FilOrFol == S_FILE) {
            return true;
        } else {
            return false;
        }
    }

}


bool read_file(int socket, FILE *f, char* Headbuffer, int bytesres) {
    long int file_size = 0;
    char buffer[BUFSIZE];
    long int size = 0;

    //Ukazatel na radek s delkou obsahu
    char *ptr2Lenght = strstr(Headbuffer,"Content-Lenght: ");
    while (*ptr2Lenght) {
        if (isdigit(*ptr2Lenght)) {
            file_size = strtol(ptr2Lenght, &ptr2Lenght, 10);
            break;
        } else { // Otherwise, move on to the next character.
            ptr2Lenght++;
        }
    }

    char *shift2Data = strstr(Headbuffer,"\r\n\r\n"); //dostanu ukazatel na \r takze jeste +4 k datům
    shift2Data += 4;

    //+4 protoze chci ukazovat jeste o znak dal na data
    //printf("strlen(shift2Data): %ld a %ld...pocet znaku: %ld",strlen(shift2Data),shift2Data-buffer,bytesres - (shift2Data-buffer));
    long int dataLong = bytesres - (shift2Data-Headbuffer);
    if(dataLong > 0) {
        if (fwrite(shift2Data, dataLong, 1, f) != 1) {
            fprintf(stderr, "Error: fwrite1 chyba.\n");
            return false;
        }
    }
    //else return true

    size += dataLong;
    //printf("size: %ld (%d)!= file_size: %ld\n",size,bytesres,file_size);

    while (size < file_size) {
        bzero(buffer, BUFSIZE);
        bytesres = (int) recv(socket, buffer, BUFSIZE, 0);
        if (bytesres < 0) {
            perror("Error: in recvfrom\n");
            return false;
        }

        if (fwrite(buffer, bytesres, 1, f) != 1) {
            fprintf(stderr, "Error: fwrite chyba.\n");
            return false;
        }
        size += bytesres;
        //printf("size: %ld (%d)!= file_size: %ld\n",size,bytesres,file_size);
    }

    if (size != file_size) {
        fprintf(stderr, "Error: size != file_size.\n");
        return false;
    }

    return true;
}

bool writeDataToClient(int sckt, const void *data,long int datalen)
{
    const char *pdata = (const char*) data;

    if (datalen > 0){
        int numSent =(int) send(sckt, pdata, datalen, 0);

        if (numSent <= 0){
            if (numSent == 0){
                fprintf(stderr,"The client was not written to: disconnected\n");
            } else {
                perror("The client was not written to");
            }
            return false;
        }
        //pdata += numSent;
        //datalen -= numSent;
    }


    return true;
}

bool send_file(int socket, FILE *f){
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    rewind(f);
    char head[BUFSIZE];

    sprintf(head,"HTTP/1.1 200 OK\r\nContent-Lenght: %ld\r\n\r\n",file_size);

    string data = head;

    if(file_size > 0) {
        char *file_content = (char *) malloc(file_size);
        if (file_content == NULL) {
            fprintf(stderr, "Error: Malloc chyba filecontent.\n");
            return false;
        }
        if (fread(file_content, file_size, 1, f) != 1) {
            fprintf(stderr, "Error: Chyba fread.\n");
            return false;
        }


        data += string(file_content,file_size);
        free(file_content);
        //printf("%s",data.c_str());

        writeDataToClient(socket, data.c_str(), file_size+strlen(head));
    }

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
        char fileOrFolder_Flag[BUFSIZE];
        strcpy(workingPath,root_folder);
        char c[BUFSIZE];
        FILE *file;
        int res = 0;

        bzero(buff, BUFSIZE);
        res = recv(comm_socket, buff, sizeof(buff), 0);
        if (res <= 0){
            //TODO chyba
        }

        printf("Client message(%d): %s", res,buff);
        //////////////PARSE HEAD///////////////////////
        //GET RESTful OPERATION
        int i = 0;
        for (i = 0; i < res; i++) {
            if (buff[i] == ' ') {
                break;
            }
            c[i] = buff[i];
        }
        c[i] = '\0';


        int shift;
        i++; //jump over space
        //i++; //jump over /
        shift = i;
        for (; i < res; i++) {
            if (buff[i] == '?') {
                break;
            }
            path[i - shift] = buff[i];
        }
        path[i - shift] = '\0';
        strcat(workingPath,path);
        printf("CESTA:%s\n",workingPath);


        //GET type file/folder
        shift = i;
        for (; i < res; i++) {
            if (buff[i] == ' ') {
                break;
            }
            fileOrFolder_Flag[i - shift] = buff[i];
        }
        fileOrFolder_Flag[i - shift] = '\0';

        ///////////////////////////////////////////////
        if (!strcmp(c, "GET")) { //get,lst
            if (!strcmp(fileOrFolder_Flag, "?type=file")) {
                file = fopen(workingPath, "rb");
                if (file == NULL) {
                    fprintf(stderr, "Error: Cant open file |%s| for write in operation read.", path);
                    send(comm_socket, NotFound, strlen(NotFound), 0);
                } else {

                    if (!send_file(comm_socket, file))
                        printf("Chyba pri posilani");

                    fclose(file);
                }
            } else if (!strcmp(fileOrFolder_Flag, "?type=folder")) {
                if(!fileOrDirectory(workingPath,S_FOLDER)){
                    //TODO send fail
                }else {
                    DIR *dir;
                    struct dirent *dent;
                    dir = opendir(workingPath);

                    if (dir != NULL) {
                        while ((dent = readdir(dir)) != NULL) {
                            if (!(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0 ||
                                  (*dent->d_name) == '.')) {
                                printf("%s\n", dent->d_name);
                            }
                        }
                    }
                    closedir(dir);
                }
            } else {
                fprintf(stderr, "Error: In function PUT defined other type (file/folder).\n");
            }
        }else if(!strcmp(c,"PUT")){
            if (!strcmp(fileOrFolder_Flag, "?type=file")) {
                file = fopen(workingPath, "wb");
                if (file == NULL) {
                    perror("Error: Cant open file for write.");
                } else {
                    bool ok = read_file(comm_socket, file, buff,res);
                    (void) ok;
                    fclose(file);
                }

            } else if(!strcmp(fileOrFolder_Flag, "?type=folder"))
            {
                int result = mkdir(workingPath, 0777);
                if(result == 0){
                    // send succes
                } else{
                    //Already exist dir or not permision
                    fprintf(stderr,"Already exists.");
                }

            } else {
                fprintf(stderr, "Error: In function PUT defined other type (file/folder).\n");
            }

        }else if(!strcmp(c,"DELETE")) {
            //TODO DELETE
            if (!strcmp(fileOrFolder_Flag, "?type=file")) {
                //del
                if(fileOrDirectory(workingPath,S_FILE)){
                    printf("Deleteeee");
                    remove(workingPath);
                } else{
                    printf("NEDeleteeee");
                }
            } else if(!strcmp(fileOrFolder_Flag, "?type=folder")) {
                //rmd
                if(fileOrDirectory(workingPath,S_FOLDER)) {
                    int n = 0;
                    struct dirent *d;
                    DIR *dir = opendir(workingPath);
                    while ((d = readdir(dir)) != NULL) {
                        if (++n > 2)
                            break;
                    }
                    closedir(dir);
                    if (n <= 2) { //Directory Empty
                        remove(workingPath);
                    } else {
                        //return neuspech
                    }
                }
                else
                {
                    //return neuspech
                }

            } else {
                fprintf(stderr, "Error: In function PUT defined other type (file/folder).\n");
            }
        }
        else
        {
            printf("Nedefinovana operace! (PUT,DELETE,GET)\n");
            send(comm_socket, buff, strlen(buff), 0);
        }

        printf("Connection to %s closed\n", str);
        //exit(EXIT_SUCCESS);
        //}//fork
        close(comm_socket);
	}	
}


