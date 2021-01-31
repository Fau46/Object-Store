#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>

#include<sys/un.h>
#include<sys/socket.h>

#include"conn.h"
#include"client_lib.h"


/******* MACRO *******/
#define SYSCALL(x,str)\
  if((x)==-1){\
    perror(str);\
    fprintf(stderr,"Error at line %d of file %s\n", __LINE__, __FILE__);\
    return 0;\
  }

#define CHECKNULL(x,str)\
  if((x)==NULL){\
    perror(str);\
    fprintf(stderr,"Error at line %d of file %s\n", __LINE__, __FILE__);\
    return 0;\
  }


/******* COSTANTI *******/
#define SIZE_BUFF 1024
#define SIZE_ERR_MSG 512


/******* VARIABILI GLOBALI *******/
static int sockfd=-1;    //fd del socket


/******* FUNZIONI *******/

/*
Funzione che si occupa di inviare il messaggio al server e di leggere la risposta.
Quest'ultima viene poi ritornata al chiamante.
*/
char *read_write_socket(char* message,int lenMsg,char* error_str){
  char *token, *tmp;
  msg_t server_message;
  char *str2="Fail read";
  char *str1="Fail writen";
  int   len_error_message=SIZE_ERR_MSG;
  char  error_message[len_error_message];

  server_message.buff=NULL;
  server_message.len=SIZE_BUFF;
  
  snprintf(error_message,len_error_message,"%s %s",str1,error_str);     //genero il primo messaggio di errore 

  SYSCALL(writen(sockfd,message,lenMsg*sizeof(char)),error_message);    //invio il messaggio al server

  server_message.buff=calloc(server_message.len+1,sizeof(char));
  CHECKNULL(server_message.buff,"Fail calloc buffer (client, read_write_socket)");
  
  snprintf(error_message,len_error_message,"%s %s",str2,error_str);     //genero il secondo messaggio di errore 
  
  SYSCALL(read(sockfd,server_message.buff,server_message.len*sizeof(char)),error_message);    //leggo la risposta dal server

  if(server_message.buff[0]=='D'){    //controllo se è stata fatta retrieve
    char *aux=NULL;
    CHECKNULL((aux=calloc(server_message.len+1,sizeof(char))),"Fail calloc aux 1 (client,read_write_socket)")
    
    strncpy(aux,server_message.buff,strlen(server_message.buff));   //copio il messaggio appena letto in una variabile di appoggio
    token=strtok_r(aux," ",&tmp);     //leggo DATA
    token=strtok_r(NULL," ",&tmp);    //leggo len

    int len=atoi(token);

    token=strtok_r(NULL," ",&tmp);    //leggo \n
    token=strtok_r(NULL,"\0",&tmp);   //leggo data
    
    if(len!=strlen(token)){           //controllo se ho ancora dati da leggere
      int dif=len-strlen(token);
       
      free(aux);
      CHECKNULL((aux=calloc(dif+1,sizeof(char))),"Fail calloc aux 2 (client,read_write_socket)");

      SYSCALL((readn(sockfd,aux,dif)),"Fail readn (client,read_write_socket)");   //leggo nella socket i dati rimanenti

      server_message.buff=realloc(server_message.buff,(server_message.len+strlen(aux)+1)*sizeof(char));   //alloco lo spazio necessario per contenere tutti i dati
      CHECKNULL(server_message.buff,"Fail realloc server_message (client,read_write_socket)"); 

      strncat(server_message.buff,aux,strlen(aux));
    }
    
  if(aux) free(aux);
  }
  
  return server_message.buff;
}

/*
Richiede al server di effettuare la registrazione col nome name.
Restituisce true se la connessione ha avuto successo, false altrimenti. 
*/
int os_connect(char *name){
  int   ret=1;
  msg_t message;
  char *tmp, *token;
  char *op="REGISTER";
  struct sockaddr_un sa;

  memset(&sa,0,sizeof(sa));
  sa.sun_family=AF_UNIX;
  strncpy(sa.sun_path,SOCKNAME,strlen(SOCKNAME)+1);

  SYSCALL((sockfd=socket(AF_UNIX,SOCK_STREAM,0)),"Fail socket (client, os_connect)");
  SYSCALL(connect(sockfd,(struct sockaddr*)&sa, sizeof(sa)),"Fail connect (client, os_connect)");

  message.len=strlen(name)+strlen(op)+4;    //+4 per gli spazi, lo \n e lo \0
  CHECKNULL((message.buff=calloc(message.len,sizeof(char))),"Fail calloc message.buff (client, os_connect)");
  
  snprintf(message.buff,message.len,"%s %s \n",op,name);    //creo l'header

  char *response_server=read_write_socket(message.buff,message.len-1,"(client, os_connect)");    //-1 perchè non tengo conto di \0
  
  token=strtok_r(response_server," ",&tmp);
  
  if(strcmp(token,"OK")!=0){    //se non è andato a buon fine controllo il messaggio di errore
      ret=0;
      printf("%s",response_server+strlen(token)+1);
  }
  
  if(message.buff) free(message.buff);
  if(response_server) free(response_server);

  return ret;
}


/*
Richiede all'object store la memorizzazione dell'oggetto block di lunghezza len, con il nome name. 
Restituisce true se la memorizzazione ha avuto successo, false altrimenti. 
*/
int os_store(char *name, void* block, size_t len){
  int   ret=1;
  int   m=100;
  char  lung[m];
  msg_t message;
  char *op="STORE";
  char *token,*tmp;

  if(sockfd==-1){
    printf("Stabilire prima una connessioneo col server\n");
    return 0;
  }

  snprintf(lung,m,"%ld",len);   //trasformo len in un char in modo da poterlo inserire nella stringa da inviare al server 
  
  message.len=strlen(op)+strlen(name)+strlen(lung)+len+6;
  CHECKNULL((message.buff=calloc(message.len,sizeof(char))),"Fail calloc message.buff (client,os_store)");

  snprintf(message.buff,message.len,"%s %s %s \n %s",op,name,lung,(char*)block);    //creo l'header

  char *response_server=read_write_socket(message.buff,message.len-1,"(client, os_store)");  //-1 perchè non tengo conto di \0
  
  token=strtok_r(response_server," ",&tmp);   //leggo la prima parte dell'header

  if(strcmp(token,"OK")!=0){    //se non è andato a buon fine 
      ret=0;
      printf("%s",response_server+strlen(token)+1);
  }

  if(message.buff) free(message.buff);
  if(response_server) free(response_server);

  return ret;

}


/*
Richiede all'object store i dati memorizzati con name. 
Se la richiesta ha avuto successo restituisce i dati richiesti, altrimenti NULL. 
*/
void *os_retrieve(char *name){
  char *token, *tmp;
  msg_t str,message;
  char *op="RETRIEVE";

  if(sockfd==-1){
    printf("Stabilire prima una connessioneo col server\n");
    return 0;
  }

  message.len=strlen(op)+strlen(name)+4;    //+4 per gli spazi, lo \n e lo \0 
  CHECKNULL((message.buff=calloc(message.len,sizeof(char))),"Fail calloc message.buff (client,os_retrieve)");

  snprintf(message.buff,message.len,"%s %s \n",op,name);    //creo il messaggio da inviare
  
  char *response_server=read_write_socket(message.buff,message.len-1,"(client, os_retrieve)");    //-1 perchè non tengo conto di \0

  token=strtok_r(response_server," ",&tmp);

  if(strcmp(token,"DATA")!=0){    //se non è andato a buon fine controllo il messaggio di errore
    str.buff=NULL;
    printf("%s",response_server+strlen(token)+1);
  }
  else{
    token=strtok_r(NULL," ",&tmp);
    str.len=atoi(token);    //leggo la grandezza dei dati
    CHECKNULL((str.buff=calloc(str.len+1,sizeof(char))),"Fail calloc str.buff (client, retrieve)"); 
    
    token=strtok_r(NULL," ",&tmp);    //leggo \n
    token=strtok_r(NULL,"\0",&tmp);   //leggo i dati

    strncpy(str.buff,token,str.len);
  }


  if(message.buff) free(message.buff);
  if(response_server) free(response_server);

  return (void*)str.buff;
}


/*
Richiede al server di eliminare l'oggetto memorizzato con name.
Restituisce true se la cancellazione ha avuto successo, false altrimenti
*/
int os_delete(char *name){
  int ret=1;
  msg_t message;
  char *op="DELETE",*token,*tmp;

  if(sockfd==-1){
    printf("Stabilire prima una connessioneo col server\n");
    return 0;
  }

  message.len=strlen(op)+strlen(name)+4;    //+4 per gli spazi, lo \n e lo \0 
  CHECKNULL((message.buff=calloc(message.len,sizeof(char))),"Fail calloc message.buff (client,os_delete)");

  snprintf(message.buff,message.len,"%s %s \n",op,name);    //creo il messaggio da inviare

  char *response_server=read_write_socket(message.buff,message.len-1,"(client, os_delete)");    //-1 perchè non tengo conto di \0
  
  token=strtok_r(response_server," ",&tmp);
  
  if(strcmp(token,"OK")!=0){ //se non è andato a buon fine
      ret=0;
      printf("%s",response_server+strlen(token)+1);
  }

  if(message.buff) free(message.buff);
  if (response_server) free(response_server);
  
  return ret;
}


/*
Richiede all'object store di chiudere la connessione.
Restituisce true se l'operazione è andata a buon fine, false altrimenti
*/
int os_disconnect(){
  int cl; 
  msg_t message;

  if(sockfd==-1){
    printf("Stabilire prima una connessioneo col server\n");
    return 0;
  }

  message.buff="LEAVE \n";
  message.len=strlen(message.buff)+1;
  
  char *response_server=read_write_socket(message.buff,message.len-1,"(client, os_disconnect)");    //-1 perchè non tengo conto di \0

  cl=close(sockfd);
  
  if(cl==-1) printf("Errore nella chiusura dell'fd\n");

  if(response_server) free(response_server);

  return (cl==0);
}
