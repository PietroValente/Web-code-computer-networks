/*
   1.Funzionalità puntuali da aggiungere:
   -accettare la richiesta dal client nel proxy
   -modificare la richiesta del client aggiungendo l'header range e mandarla al server
   -ripetere l'operazione precedente modificando il range di volta in volta fino alla fine dei caratteri del file
   -scaricamento della response temporanea nel buffer total per ogni richiesta
   -alla fine dello scaricamento completo del file nel buffer, il buffer viene mandato al client
   2.Punti di intervento neĺ programma:
   -da quando viene effettuata la request al server a quando viene effettuata la response al client
   3.Eventuali scelte implementative:
   -tutte le richieste effettuate sono di tipo conncetion close perchè l'header range viene utilizzato di solito per quando si ha una connessione poco stabile e si devono scaricare grandi quantità di un file
   4.Descrizione dell'esperimento:
   5.Descrizione dell'esito e verifica correttezza:
   -E' stato verificato che con http://88.80.187.84:80/image.jpg il proxy funziona correttamente
 */
#include <sys/types.h>          /* See NOTES */
#include <signal.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>

struct hostent * he;
struct sockaddr_in local,remote,server;
char request[2000],response[2000],request2[2000],response2[2000],r[2000];
char * method, *path, *version, *host, *scheme, *resource,*port, *total, *statusline;
struct headers {
        char *n;
        char *v;
}h[30];
struct request_headers {
        char *n;
        char *v;
}rh[30];

int main()
{
        FILE *f;
        char command[100];
        int i,s,t,t2,s2,s3,n,len,c,yes=1,j,k,pid,entity_length;
        s = socket(AF_INET, SOCK_STREAM, 0);
        if ( s == -1) { perror("Socket Fallita\n"); return 1;}
        local.sin_family=AF_INET;
        local.sin_port = htons(38356);
        local.sin_addr.s_addr = 0;
        setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
        t = bind(s,(struct sockaddr *) &local, sizeof(struct sockaddr_in));
        if ( t == -1) { perror("Bind Fallita \n"); return 1;}
        t = listen(s,10);
        if ( t == -1) { perror("Listen Fallita \n"); return 1;}
        while( 1 ){
                f = NULL;
                remote.sin_family=AF_INET;
                len = sizeof(struct sockaddr_in);
                s2 = accept(s,(struct sockaddr *) &remote, &len);
                if(fork()) continue;
                if (s2 == -1) {perror("Accept Fallita\n"); return 1;}

                j=0;k=0;
                h[k].n = request;
                while(read(s2,request+j,1)){
                        if((request[j]=='\n') && (request[j-1]=='\r')){
                                request[j-1]=0;
                                if(h[k].n[0]==0) break;
                                h[++k].n=request+j+1;
                        }
                        if(request[j]==':' && (h[k].v==0) &&k ){
                                request[j]=0;
                                h[k].v=request+j+1;
                        }
                        j++;
                }
                printf("%s",request);
                method = request;
                for(i=0;(i<2000) && (request[i]!=' ');i++); request[i]=0;
                path = request+i+1;
                for(   ;(i<2000) && (request[i]!=' ');i++); request[i]=0;
                version = request+i+1;

                printf("Method = %s, path = %s , version = %s\n",method,path,version);
                if(!strcmp("GET",method)){
                        //  http://www.google.com/path
                        scheme=path;
                        for(i=0;path[i]!=':';i++); path[i]=0;
                        host=path+i+3;
                        for(i=i+3;path[i]!='/';i++); path[i]=0;
                        for(j=0;host[j] != ':';j++);
                        host[j] = 0;
                        resource=path+i+1;
                        printf("Scheme=%s, host=%s, resource = %s\n", scheme,host,resource);
                        he = gethostbyname(host);
                        if (he == NULL) { printf("Gethostbyname Fallita\n"); return 1;}
                        printf("Server address = %u.%u.%u.%u\n", (unsigned char ) he->h_addr[0],(unsigned char ) he->h_addr[1],(unsigned char ) he->h_addr[2],(unsigned char ) he->h_addr[3]);
                        int start = 0;
                        int end = 1000-1;
                        int total_length = 0;
                        int index = 0;
                        total = (char * ) malloc(2000000);
                        do{
                                s3=socket(AF_INET,SOCK_STREAM,0);
                                if(s3==-1){perror("Socket to server fallita"); return 1;}
                                server.sin_family=AF_INET;
                                server.sin_port=htons(80);
                                server.sin_addr.s_addr=*(unsigned int*) he->h_addr;
                                t=connect(s3,(struct sockaddr *)&server,sizeof(struct sockaddr_in));
                                if(t==-1){perror("Connect to server fallita"); return 1;}
                                sprintf(request2,"GET /%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\nRange: bytes=%d-%d\r\n\r\n",resource,host,start,end);
                                write(s3,request2,strlen(request2));
                                bzero(rh,30*sizeof(struct request_headers *));
                                statusline = rh[0].n=response2;
                                for( j=0,k=0; read(s3,response2+j,1);j++){
                                        r[j] = response2[j];
                                        if(response2[j]==':' && (rh[k].v==0) ){
                                                response2[j]=0;
                                                rh[k].v=response2+j+1;
                                        }

                                        else if((response2[j]=='\n') && (response2[j-1]=='\r') ){
                                                response2[j-1]=0;
                                                if(rh[k].n[0]==0) break;
                                                rh[++k].n=response2+j+1;
                                        }
                                }
                                //range non supportato
                                if(strcmp(statusline,"HTTP/1.1 200 OK") == 0){
                                        //...
                                }
                                entity_length = -1;
                                for(i=1;i<k;i++){
                                        if(strcmp(rh[i].n,"Content-Length")==0){
                                                entity_length=atoi(rh[i].v);
                                        }
                                        if(strcmp(rh[i].n,"Content-Range")==0){
                                                int y = 0;
                                                for(;rh[i].v[y] != '/';y++);
                                                y++;
                                                total_length = atoi(rh[i].v+y);
                                        }
                                }
                                if(entity_length == -1) entity_length=1000000;
                                for(j=0; (t2=read(s3,total+index,entity_length-j))>0;j+=t2,index+=t2);
                                start = end;
                                end += 1000;
                                if(end>total_length){
                                        end = total_length;
                                }
                                index--;
                                shutdown(s3,SHUT_RDWR);
                                close(s3);
                        }while(start<total_length);
                        total[index++] = 0;
                        sprintf(response2,"HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n",index);
                        write(s2,response2,strlen(response2));
                        for(j=0; (t2=write(s2,total+j,index-j-1))>0;j+=t2);
                }
                else if(!strcmp("CONNECT",method)) { // it is a connect  host:port
                        host=path;
                        for(i=0;path[i]!=':';i++); path[i]=0;
                        port=path+i+1;
                        printf("host:%s, port:%s\n",host,port);
                        printf("Connect skipped ...\n");
                        he = gethostbyname(host);
                        if (he == NULL) { printf("Gethostbyname Fallita\n"); return 1;}
                        printf("Connecting to address = %u.%u.%u.%u\n", (unsigned char ) he->h_addr[0],(unsigned char ) he->h_addr[1],(unsigned char ) he->h_addr[2],(unsigned char ) he->h_addr[3]);
                        s3=socket(AF_INET,SOCK_STREAM,0);

                        if(s3==-1){perror("Socket to server fallita"); return 1;}
                        server.sin_family=AF_INET;
                        server.sin_port=htons((unsigned short)atoi(port));
                        server.sin_addr.s_addr=*(unsigned int*) he->h_addr;
                        t=connect(s3,(struct sockaddr *)&server,sizeof(struct sockaddr_in));
                        if(t==-1){perror("Connect to server fallita"); exit(0);}
                        sprintf(response,"HTTP/1.1 200 Established\r\n\r\n");
                        write(s2,response,strlen(response));
                        // <==============
                        if(!(pid=fork())){ //Child
                                while(t=read(s2,request2,2000)){
                                        write(s3,request2,t);
                                        //printf("CL >>>(%d)%s \n",t,host); //SOLO PER CHECK
                                }
                                exit(0);
                        }
                        else { //Parent
                                while(t=read(s3,response2,2000)){
                                        write(s2,response2,t);
                                        //printf("CL <<<(%d)%s \n",t,host);
                                }
                                kill(pid,SIGTERM);
                                shutdown(s3,SHUT_RDWR);
                                close(s3);
                        }
                }
                shutdown(s2,SHUT_RDWR);
                close(s2);
                exit(0);
        }
}