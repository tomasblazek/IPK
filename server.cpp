
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

const char OK[] = "HTTP/1.1 200 OK\r\n";
const char NotFound[] = "HTTP/1.1 404 Not Found\r\n";
const char BadRequest[] = "HTTP/1.1 400 Bad Request\r\n";

const char ErrNotDir[] = "Not a directory.";
const char ErrDirNotFound[] = "Directory not found.";
const char ErrDirNotEmpty[] = "Directory not empty.";
const char ErrAlreadyExist[] = "Already exists.";
const char ErrNotFile[] = "Not a file.";
const char ErrFileNotFound[] = "File not found.";
const char ErrUserNotFound[] = "User Account Not Found";


/**
 * Get information about existence of file or directory.
 * @param path
 * @return False when obeject not exist.
 */
bool isExist(char* path){
    FILE *f = fopen(path,"r");
    if(f==NULL) {
        return false;
    }

    fclose(f);
    return true;
}
/**
 * Get information if object on path is folder or file by FLAG (FilOrFol).
 * @param path
 * @param FilOrFol  Set searching type of object.
 * @return  If obejct not exit return false.
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


string makeResponse(char* path, long lenght, const char *errCode){
    string response;
    char head[BUFSIZE];
    //DATUM
    char date[BUFSIZE];
    time_t now = time(0);
    tm tm = *gmtime(&now);
    strftime(date, sizeof(date), "Date: %a, %d %b %Y %H:%M:%S %Z\r\n", &tm);

    //CONTENT-TYPE
    char MIME[BUFSIZE];
    if(!strcmp(errCode,OK) && path != NULL) {
        string buf = "file --mime-type ";
        buf += path;
        FILE *mimeFile = popen(buf.c_str(), "r");
        if (mimeFile == NULL) {
            perror("Error: Cannot get MIME type of file.\n");
        }
        char *mime_type = NULL;
        if (fgets(MIME, BUFSIZE, mimeFile) != NULL) {
            mime_type = strstr(MIME, " ");
        }
        fclose(mimeFile);
        strcpy(MIME, "Content-Type: ");
        strcat(MIME, mime_type + 1);//+1 jump over space TODO \r\n
    }
    else
    {
        sprintf(MIME,"Content-Type: text/plain\r\n");
    }
    //OK + others
    sprintf(head,"%s%s%sContent-Lenght: %ld\r\n",errCode,date,MIME,lenght);

    //ACCEPT ECODING
    strcat(head,"Content-Encoding: identity\r\n\r\n");
    printf("%ld",strlen(head));

    response = head;

    return response;
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

bool send_file(int socket, FILE *f, char* path){
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    rewind(f);

    string data = makeResponse(path,file_size,OK);
    long headLen = strlen(data.c_str());

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

        writeDataToClient(socket, data.c_str(), headLen + file_size);
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
				//printf("Root directory: %s\n",optarg);
                strcpy(root_folder,optarg);
                strcat(root_folder,"/");
                root_folder_def = true;
				break;
			case 'p':
                port_number = atoi(optarg);
				//printf("Port number: %s (%d)\n", optarg,port_number);
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

	//fprintf(stderr,"-r <ROOT-FOLDER> %s -p <port> %d\n", root_folder , port_number);

	socklen_t sa_client_len=sizeof(sa_client);
	if ((welcome_socket = socket(PF_INET6, SOCK_STREAM, 0)) < 0)
	{
		perror("Error: socket.");
		exit(EXIT_FAILURE);
	}
	memset(&sa,0,sizeof(sa));
	sa.sin6_family = AF_INET6;
	sa.sin6_addr = in6addr_any;
	sa.sin6_port = htons(port_number);
    
	if ((rc = bind(welcome_socket, (struct sockaddr*)&sa, sizeof(sa))) < 0)
	{
		perror("Error: bind");
		exit(EXIT_FAILURE);		
	}
	if ((listen(welcome_socket, 1)) < 0)
	{
		perror("Error: listen");
		exit(EXIT_FAILURE);				
	}


	while(1)
	{

        int comm_socket = accept(welcome_socket, (struct sockaddr *) &sa_client, &sa_client_len);
        if (comm_socket < 0) {
            fprintf(stderr, "Unknown error.");
        }
        /* MULTI PROCESS VARIANT
        pid_t client_processPID = fork();
        if(client_processPID < 0) {
            perror("Error: Fork client_process failed.\n");

            //kill(0,SIGTERM);
            exit(2);
        }
        if(client_processPID == 0) {
        */
        if (inet_ntop(AF_INET6, &sa_client.sin6_addr, str, sizeof(str))) {
            //printf("INFO: New connection:\n");
            //printf("INFO: Client address is %s\n", str);
            //printf("INFO: Client port is %d\n", ntohs(sa_client.sin6_port));
        }



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

        //printf("Client message(%d):%s", res,buff);
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
                    string response = makeResponse(NULL,strlen(ErrFileNotFound),NotFound);
                    response += ErrFileNotFound;
                    writeDataToClient(comm_socket, response.c_str(), strlen(response.c_str()));
                } else {

                    if (!send_file(comm_socket, file, workingPath))
                        fprintf(stderr,"Error: Chyba pri posilani.");

                    fclose(file);
                }
            } else if (!strcmp(fileOrFolder_Flag, "?type=folder")) {
                if(!isExist(workingPath)){
                    string response = makeResponse(NULL,strlen(ErrDirNotFound),NotFound);
                    response += ErrDirNotFound;
                    writeDataToClient(comm_socket, response.c_str(), strlen(response.c_str()));
                } else {
                    if (!fileOrDirectory(workingPath, S_FOLDER)) {
                        string response = makeResponse(NULL, strlen(ErrNotDir), BadRequest);
                        response += ErrNotDir;
                        writeDataToClient(comm_socket, response.c_str(), strlen(response.c_str()));
                    } else {
                        DIR *dir;
                        struct dirent *dent;
                        dir = opendir(workingPath);
                        string lsOut;

                        if (dir != NULL) {
                            while ((dent = readdir(dir)) != NULL) {
                                if (!(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0 ||
                                      (*dent->d_name) == '.')) {
                                    lsOut += dent->d_name;
                                    lsOut += '\n';
                                }
                            }
                        }
                        string response = makeResponse(NULL, strlen(lsOut.c_str()), OK);
                        response += lsOut.c_str();
                        writeDataToClient(comm_socket, response.c_str(), strlen(response.c_str()));
                        closedir(dir);
                    }
                }
            } else {
                fprintf(stderr, "Error: In function PUT defined other type (file/folder).\n");
            }
        }else if(!strcmp(c,"PUT")){
            if (!strcmp(fileOrFolder_Flag, "?type=file")) {
                if(isExist(workingPath)){
                    string response = makeResponse(NULL,strlen(ErrAlreadyExist),BadRequest);
                    response += ErrAlreadyExist;
                    writeDataToClient(comm_socket, response.c_str(), strlen(response.c_str()));
                }
                else
                {
                    file = fopen(workingPath, "wb");
                    if (file == NULL) {
                        perror("Error: Cant open file for write.");
                    } else {
                        bool ok = read_file(comm_socket, file, buff, res);
                        if (!ok) {
                            fprintf(stderr, "Error: Read in operation PUT failed.\n");
                        }
                        string response = makeResponse(NULL,0,OK);
                        writeDataToClient(comm_socket, response.c_str(), strlen(response.c_str()));
                        fclose(file);
                    }
                }
            } else if(!strcmp(fileOrFolder_Flag, "?type=folder"))
            {
                int result = mkdir(workingPath, 0777);
                if(result == 0){
                    string response = makeResponse(NULL,0,OK);
                    writeDataToClient(comm_socket, response.c_str(), strlen(response.c_str()));
                } else{
                    string response = makeResponse(NULL,strlen(ErrAlreadyExist),BadRequest);
                    response += ErrAlreadyExist;
                    writeDataToClient(comm_socket, response.c_str(), strlen(response.c_str()));
                }
            } else {
                fprintf(stderr, "Error: In function PUT defined other type (file/folder).\n");
            }

        }else if(!strcmp(c,"DELETE")) {
            if (!strcmp(fileOrFolder_Flag, "?type=file")) {
                if(!isExist(workingPath)){
                    string response = makeResponse(NULL,strlen(ErrFileNotFound),NotFound);
                    response += ErrFileNotFound;
                    writeDataToClient(comm_socket, response.c_str(), strlen(response.c_str()));
                } else {
                    if (fileOrDirectory(workingPath, S_FILE)) {
                        remove(workingPath);
                        string response = makeResponse(NULL, 0, OK);
                        writeDataToClient(comm_socket, response.c_str(), strlen(response.c_str()));
                    } else {
                        string response = makeResponse(NULL, strlen(ErrNotFile), BadRequest);
                        response += ErrNotFile;
                        writeDataToClient(comm_socket, response.c_str(), strlen(response.c_str()));
                    }
                }
            } else if(!strcmp(fileOrFolder_Flag, "?type=folder")) {
                if(!isExist(workingPath)){
                    string response = makeResponse(NULL,strlen(ErrDirNotFound),NotFound);
                    response += ErrDirNotFound;
                    writeDataToClient(comm_socket, response.c_str(), strlen(response.c_str()));
                }
                else {
                    if (fileOrDirectory(workingPath, S_FOLDER)) {
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
                            string response = makeResponse(NULL, 0, OK);
                            writeDataToClient(comm_socket, response.c_str(), strlen(response.c_str()));
                        } else {
                            string response = makeResponse(NULL,strlen(ErrDirNotEmpty),BadRequest);
                            response += ErrDirNotEmpty;
                            writeDataToClient(comm_socket, response.c_str(), strlen(response.c_str()));
                        }
                    } else {
                        string response = makeResponse(NULL, strlen(ErrNotDir), BadRequest);
                        response += ErrNotDir;
                        writeDataToClient(comm_socket, response.c_str(), strlen(response.c_str()));
                    }
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

        //printf("Connection to %s closed\n", str);
        //exit(EXIT_SUCCESS);
        //}//fork
        close(comm_socket);
	}	
}


