/*
 * IPK.2015L
 *
 * Demonstration of trivial TCP communication.
 *
 * Ondrej Rysavy (rysavy@fit.vutbr.cz)
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>

using namespace std;

#define BUFSIZE 1024

enum Operations{
    oper_del = 1,
    oper_get,
    oper_put,
    oper_mkd,
    oper_rmd,
    oper_lst
};

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
    printf("Echo to server: %ld\n", file_size);

    int bytesc = (int) send(socket,to_string(file_size).c_str(),BUFSIZE, 0);
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
    free(file_content);
    return true;
}

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

char* makeHeader(int operation, char *remotePath) {
    char* head = (char*) malloc(BUFSIZE);
    if(head == NULL) {
        fprintf(stderr,"Error: Malloc head.");
        exit(EXIT_FAILURE);
    }
    if(operation == oper_del || operation == oper_rmd){
        strcpy(head,"DELETE ");
    }
    else if(operation == oper_put || operation == oper_mkd){
        strcpy(head,"PUT ");
    }
    else if(operation == oper_get || operation == oper_lst){
        strcpy(head,"GET ");
    }
    else{
        fprintf(stderr,"Error: Unknown operation.");
        free(head);
        exit(EXIT_FAILURE);
    }

    unsigned int i = 0;
    char c[BUFSIZE];
    for (i = 0; i < strlen(remotePath); i++){
        c[i] = remotePath[i];
    }
    c[i] = '\0';
    strcat(head,c);

    strcat(head,"?type=");
    if(operation == oper_get ||operation == oper_put || operation == oper_del) {
        strcat(head, "file");
    }
    else
    {
        strcat(head,"folder");
    }
    strcat(head," HTTP/1.1\n");


    return head;
}


void parse_remotePath(const char *argv2, char *hostname, int *port_number, char *remotePath){
    char c[BUFSIZE];


    //http prefix control
    if(strlen(argv2) < 7) {
        fprintf(stderr, "Error: Missing http:// at parameter REMOTE-PATH.\n");
        exit(EXIT_FAILURE);
    }
    unsigned int i = 0;
    for (i = 0; i < 7; i++) {
        c[i] = argv2[i];
    }
    c[i] = '\0';
    if(strcmp(c,"http://")) {
        fprintf(stderr, "Error: Missing http:// at parameter REMOTE-PATH.\n");
        exit(EXIT_FAILURE);
    }

    //get hostname processing
    for (i = 7; i < strlen(argv2); i++){
        if(argv2[i] == ':' || argv2[i] == '/') {
            break;
        }
        c[i-7] = argv2[i];
    }
    c[i-7] = '\0';
    strcpy(hostname,c);

    //get port number
    int shift;
    if (argv2 [i] == ':') {
        i++;//cut colon
        shift = i;
        for (; i < strlen(argv2); i++) {
            if (argv2[i] == '/') {
                break;
            }
            c[i - shift] = argv2[i];
        }
        c[i - shift] = '\0';
        *port_number = atoi(c);
    } else{
        // default port
        *port_number = 6677;
    }

    //get remote path to working directory
    shift = i;

    for (; i < strlen(argv2); i++){
        c[i-shift] = argv2[i];
    }
    c[i-shift] = '\0';
    strcpy(remotePath,c);

}



int parse_arguments(int argc,const char *argv[]){
    if (argc == 3 || argc == 4) {
        if(!strcmp(argv[1],"mkd")){
            return oper_mkd;
        }
        else if(!strcmp(argv[1],"put")){
            if(argc != 4){
                fprintf(stderr,"Error: Bad counaaat of arguments.\n");
                exit(EXIT_FAILURE);
            }
            else
                return oper_put;
        }
        else if(!strcmp(argv[1],"get")){
            return oper_get;
        }
        else if(!strcmp(argv[1],"del")){
            return oper_del;
        }
        else if(!strcmp(argv[1],"rmd")){
            return oper_rmd;
        }
        else if(!strcmp(argv[1],"lst")){
            return oper_lst;
        }
        else {
            fprintf(stderr,"Error: Unknown operation.\n");
            exit(EXIT_FAILURE);
        }
    }
    else{
        fprintf(stderr,"Error: Bad count of arguments.\n");
        fprintf(stderr,"usage: %s <operation> <http://hostname:port/remotePath> <localPath>\n", argv[0]); //TODO delete
        exit(EXIT_FAILURE);
    }
}

int main (int argc, const char * argv[]) {
	int client_socket, port_number = 6677, bytestx, bytesrx;
    //socklen_t serverlen;
    char server_hostname[1024];
    char remotePath[1024];
    char localPath[1024];
    struct hostent *server;
    struct sockaddr_in server_address;

    /* 1. test vstupnich parametru: */
    //Zpracovani argumentu
    int operation = parse_arguments(argc,argv);
    if(argc == 4){
        if(operation == oper_get || operation == oper_put) {
            strcpy(localPath, argv[3]);
        }
        else {
            fprintf(stderr, "Error: Bad count (3) of arguments for operation.\n");
            exit(EXIT_FAILURE);
        }
    }

    parse_remotePath(argv[2], server_hostname, &port_number, remotePath);

    printf("HOSTNAME: %s ,PORT: %d, PATH: %s\n", server_hostname, port_number, remotePath);//TODO smazat


    /* 2. ziskani adresy serveru pomoci DNS */
    if ((server = gethostbyname(server_hostname)) == NULL) {
        fprintf(stderr,"Error: no such host as %s\n", server_hostname);
        exit(EXIT_FAILURE);
    }

    /* 3. nalezeni IP adresy serveru a inicializace struktury server_address */
    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
    server_address.sin_port = htons(port_number);

    /* tiskne informace o vzdalenem soketu */
    printf("INFO: Server socket: %s : %d \n", inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));

    /* Vytvoreni soketu */
	if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
	{
		perror("Error: socket");
		exit(EXIT_FAILURE);
	}


    if (connect(client_socket, (const struct sockaddr *) &server_address, sizeof(server_address)) != 0)
    {
		perror("Error: connect");
		exit(EXIT_FAILURE);
    }


    char* buf = makeHeader(operation, remotePath);
    printf("%s\n",buf); //TODO smazat

    /* odeslani zpravy header na server */
    bytestx = (int)send(client_socket, buf, BUFSIZE, 0);
    if (bytestx < 0) {
        free(buf);
        perror("Error in sendto");
        exit(EXIT_FAILURE);
    }


//    /* prijeti odpovedi a jeji vypsani */
//    bzero(buf, BUFSIZE);
//    bytesrx = (int)recv(client_socket, buf, BUFSIZE, 0);
//    if (bytesrx < 0) {
//        free(buf);
//        perror("Error: in recvfrom");
//        exit(EXIT_FAILURE);
//    }
//    printf("Echo from server: %s\n", buf);

    FILE *file;
    if(operation == oper_get) {
        /* prijeti odpovedi a jeji vypsani */
        bzero(buf, BUFSIZE);
        bytesrx = (int)recv(client_socket, buf, BUFSIZE, 0);
        if (bytesrx < 0) {
            free(buf);
            perror("Error: in recvfrom");
            exit(EXIT_FAILURE);
        }
        printf("Echo from server: %s\n", buf);

        if(argc == 4) {
            file = fopen(localPath,"wb");
        }
        else
        {
            file = fopen(basename(remotePath), "wb");
        }

        if(file == NULL){
            perror("Error: Cant open file for write in operation get.");
        }
        else
        {
            bool ok = read_file(client_socket, file, buf);
            (void) ok;
            fclose(file);
        }
    }
    else if(operation == oper_put){
        file = fopen(localPath, "rb");
        if (file == NULL) {
            fprintf(stderr, "Error: Cant open file for write in operation put.\n");
            free(buf);
            close(client_socket);
            exit(EXIT_FAILURE);
        }

        if (!send_file(client_socket, file))
            printf("Chyba pri posilani");

        fclose(file);

    }
    /*
    DIR *dir;
    struct dirent *dent;
    dir = opendir(".");

    if(dir!=NULL)
    {
        while((dent=readdir(dir))!=NULL)
        {
            if(!(strcmp(dent->d_name,".")==0 || strcmp(dent->d_name,"..")==0 || (*dent->d_name) == '.' )) {
                printf("%s\n",dent->d_name);
            }
        }
    }
    closedir(dir);
    */
    free(buf);
    close(client_socket);
    return 0;
}
