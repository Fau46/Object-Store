#include<fcntl.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<signal.h>
#include<string.h>
#include<pthread.h>


#include<sys/stat.h>
#include<sys/types.h>
#include <sys/time.h>

#include"conn.h"
#include"server_lib.h"


/******* COSTANTI *******/
#define SIZE_BUFF 4096


/******* VARIABILI GLOBALI *******/
volatile sig_atomic_t stat_byte_share=0;    //variabile che tiene conto di quanti byte vengono scambiati tra client e server


/******* VARIABILI ESTERNE *******/
extern statistics_t statistiche;
extern pthread_mutex_t mutex_stat;


/******* FUNZIONI *******/

/*
Legge il messaggio passato dal client tramite l'fd e lo tokenizza, ritornando un messaggio di tipo messagio_t. 
Per sapere di più su come è strutturato messaggio_t controllare server_lib.h.
*/
messaggio_t raw_msg(long fd){ 
  int rd;             
  int aux_len;
  int malformed=0;
  messaggio_t mex;    //variabile usata per archiviare il messaggio del client
  msg_t mex_client;   //variabile usata per leggere il messaggio del client
  char *tmp,*token;   

  mex_client.buff=NULL;
  mex_client.len=SIZE_BUFF;
  
  memset(&mex,0,sizeof(messaggio_t)); 
  
  mex_client.buff=calloc(mex_client.len+1,sizeof(char));
  CHECKNULL(mex_client.buff,"Fail calloc buffer (server, raw_msg)");

  SYSCALL((rd=read(fd,mex_client.buff,mex_client.len*sizeof(char))),"Errore nella readn (server, raw_msg)");    //leggo una parte dell'header
  stat_byte_share+=strlen(mex_client.buff); 
  
  if(rd==0){    //se non leggo più nulla dalla socket termino la funzione
     mex.operation="break";
     return mex;
  }

  token=strtok_r(mex_client.buff," ",&tmp);   //leggo l'operazione da fare

  if(   //se l'operazione letta non corrisponde a quella del protocollo
    strcmp(token,"STORE")!=0 &&
    strcmp(token,"LEAVE")!=0 &&
    strcmp(token,"DELETE")!=0 &&
    strcmp(token,"REGISTER")!=0 &&
    strcmp(token,"RETRIEVE")!=0
  ){
    malformed=1;
    goto malf;
  }

  

  aux_len=strlen(token)+1;
  mex.operation=calloc(aux_len,sizeof(char));   //alloco lo spazio per inserire l'operazione letta
  CHECKNULL(mex.operation,"Fail calloc mex.operation (server, raw_msg)");
  snprintf(mex.operation,aux_len,"%s",token);   //copio l'operazione da fare 

  if(strcmp(mex.operation,"LEAVE")!=0){
    
    token=strtok_r(NULL," ",&tmp);                          //sposto il puntatore su name
    if(strcmp(token,"\n")==0){ malformed=1; goto malf; }    //se l'header non contiene name

    aux_len=strlen(token)+1;
    mex.name=calloc(aux_len,sizeof(char));
    CHECKNULL(mex.name,"Fail calloc mex.name (server, raw_msg)");
    snprintf(mex.name,aux_len,"%s",token);    //copio name

    if(strcmp(mex.operation,"STORE")==0){
      
      token=strtok_r(NULL," ",&tmp);                          //sposto il puntatore su len
      if(strcmp(token,"\n")==0){ malformed=1; goto malf; }    //se l'header non contiene len
      
      mex.len=atoi(token);                        //atoi se non si sono passate cifre restituisce 0
      if(mex.len<=0){ malformed=1; goto malf; }   //se len è minore o uguale a 0 o non si sono passate delle cifre
      
      token=strtok_r(NULL," ",&tmp);
      token=strtok_r(NULL,"\0",&tmp);             //leggo eventuali dati nel buffer

      mex.data=calloc(mex.len+1,sizeof(char));    //alloco lo spazio necessario per contenere i dati      
      CHECKNULL(mex.data,"Fail calloc mex.name (server,raw_msg)");
      aux_len=strlen(token)+1;
      
      snprintf(mex.data,aux_len,"%s",token);      //inserisco eventuali dati che ho letto con la prima read nel buffer

      if(mex.len!=strlen(mex.data)){              //controllo se ho ancora dati da leggere oppure se ho letto tutto il messaggio ad eccezione dell'ultimo \n
        
        if(mex_client.buff) free(mex_client.buff);
        
        mex_client.len=mex.len-strlen(mex.data);      //numero di byte rimanenti da leggere
        mex_client.buff=calloc(mex_client.len+1,sizeof(char));
        CHECKNULL(mex_client.buff,"Fail calloc (server, raw_msg)");

        SYSCALL((rd=readn(fd,mex_client.buff,(mex_client.len)*sizeof(char))),"Fail readn (server, raw_msg)");   //leggo i dati rimanenti dalla socket
        stat_byte_share+=strlen(mex_client.buff);

        strncat(mex.data,mex_client.buff,strlen(mex_client.buff));    //concateno i dati rimanenti con quelli letti in precedenza
      
      }
    }
  }

  malf: 
    if(malformed){    //Se l'header non corrisponde a quello del protocollo
      mex.operation="malformed";
      mex.data="KO Malformed header \n";
      mex.len=strlen(mex.data);
    }

  if(mex_client.buff!=NULL) free(mex_client.buff);
  return mex;
}


/*
Crea la directory specificata in str se non esiste.
Ritorna 1 se la directory già esisteva o se la sua creazione è andata a buon fine, 0 altrimenti
*/
int create_directory(char *str){
  struct stat st;
    
  if(stat(str,&st)==-1){
    if((mkdir(str, 0777))==-1) return 0; //se la creazione della cartella fallisce ritorna 0
  }

  return 1;
}


/*
Ritorna una stringa del tipo str1/str2
*/
char *create_path(char *str1,char *str2){
  msg_t path;

  path.len=strlen(str1)+strlen(str2)+2;
  path.buff=(char*)calloc(path.len,sizeof(char));
  CHECKNULL(path.buff,"Fail calloc path.buff (server, create_path)");
  
  
  snprintf(path.buff,path.len,"%s/%s",str1,str2); //concateno str1 e str2

  return path.buff;
}


/*
Registra l'utente usr_name creando l'apposita directory, nel caso in cui non fosse ancora presente, nella cartella data.
Risponde al client con un messaggio tramite l'fd della socket seguendo il protocollo stabilito.
Ritorna 1 se l'operazione è andata a buon fine, 0 altrimenti.
*/
int register_usr(char *usr_name, char **path_usr,char *data, int fd){
  msg_t mex;
  int dir_owner;

  *path_usr=create_path(data,usr_name);   //creo il path data/usr_name
  dir_owner=create_directory(*path_usr);  //inizializzo la cartella dell'utente
  
  mex.buff=((dir_owner)?"OK \n":"KO Errore registrazione \n");
  mex.len=strlen(mex.buff);

  SYSCALL(writen(fd,mex.buff,mex.len*sizeof(char)),"Fail writen (server, register)"); //rispondo al client
  stat_byte_share+=mex.len;
  
  if(mex.buff[0]=='O') return 1;
  else return 0;
}


/*
Scrive in file_name i dati in block che hanno una lunghezza len.
file_name viene creato se non esiste, altrimenti viene sovrascritto.
Ritorna un messaggio contenente l'esito dell'operazione.
*/
char* store(char *file_name,int len,char *block){
  int fd;
  char *messaggio;
  
  if((fd=open(file_name,O_WRONLY|O_CREAT|O_TRUNC,0666))==-1) return "KO Errore nell'apertura del file \n"; 

  pthread_mutex_lock(&mutex_stat);
    if((write(fd,block,len))!=-1){
      statistiche.size+=len;    //aggiorno la dimensione dell'object store
      
      messaggio="OK \n";
    }
    else{
      messaggio="KO Errore nella scrittura nel file \n";
    }
  pthread_mutex_unlock(&mutex_stat);
      

  close(fd);
  return messaggio;
}


/*
Archivia i dati presenti in mex nel path_usr.
Ritorna l'esito dell'operazione al client tramite l'fd seguendo il protocollo stabilito.
*/
void store_data(messaggio_t mex,char *path_usr,int fd){
  msg_t response;
  msg_t path_file;

  path_file.len=strlen(path_usr)+strlen(mex.name)+2;

  path_file.buff=(char*)calloc(path_file.len,sizeof(char));
  CHECKNULL(path_file.buff,"Fail calloc path_file.buff (server, store_data)");

  snprintf(path_file.buff,path_file.len,"%s/%s",path_usr,mex.name);   //creo il path per il file che conterrà i dati

  response.buff=store(path_file.buff,mex.len,mex.data);   //memorizzo i dati e ricevo il messaggio da mandare al client
  response.len=strlen(response.buff);

  SYSCALL(writen(fd,response.buff,response.len*sizeof(char)),"Fail writen (server, store_block)");
  stat_byte_share+=response.len; 

  free(path_file.buff);
}


/*
Legge il file path_file e memorizza il contenuto in una variabile di tipo msg_t, che viene ritornata.
*/
msg_t retrieve(char* path_file){
  int fd,rd;
  char *fail;
  msg_t block;      //variabile dove memorizzo i dati letti dal file
  struct stat st;   //variabile usata per acquisire la grandezza del file

  if((fd=open(path_file,O_RDONLY))!=-1){
    SYSCALL(fstat(fd,&st),"Fail stat (server, retrieve)");

    block.len=st.st_size;   //acquisisco la grandezza del file appena aperto
    block.buff=(char*)calloc(block.len+1,sizeof(char));   //alloco lo spazio per contenere i dati
    CHECKNULL(block.buff,"Fail calloc block (server, retrieve)");

    rd=read(fd,block.buff,block.len);   //leggo i dati dal file 

    if(rd!=-1){   //se la lettura è andata a buon fine
      close(fd);
      return block;
    }
    else fail="Errore nella lettura del file";
  }
  else fail="Errore nell'apertura del file";

  block.buff=calloc(strlen(fail)+1,sizeof(char));
  CHECKNULL(block.buff,"Fail calloc block.buff (server, retrieve)");

  strncpy(block.buff,fail,strlen(fail));
  block.len=-1;   //se l'operazione è fallita setto a -1 la lunghezza dei dati letti

  close(fd);
  return block;
}


/*
Legge i dati presenti in file_name salvato seguendo path_usr.
Ritorna al client, tramite l'fd, i dati letti o l'esito negativo dell'operazione, rispettando il protocollo stabilito.
*/
void retrieve_data(char* path_usr, char* file_name, int fd){
  msg_t response;
  char *path_file=create_path(path_usr,file_name);    //creo il path path_usr/file_name

  msg_t data=retrieve(path_file);   //ricevo i dati richiesti o eventuali errori

  if(data.len==-1){                 //se ci sono stati errori
    char *error="KO";
    response.len=strlen(error)+strlen(file_name)+strlen(data.buff)+5;   
    response.buff=calloc(response.len,sizeof(char));                    
    CHECKNULL(response.buff,"Fail calloc messaggio (server, retrieve_data");

    snprintf(response.buff, response.len, "%s %s %s \n",error,data.buff,file_name);
  }
  else{
    char *ok="DATA";
    char lung[100];

    snprintf(lung,100,"%d",data.len);

    response.len=data.len+strlen(ok)+strlen(lung)+5;    
    response.buff=calloc(response.len,sizeof(char));
    CHECKNULL(response.buff,"Fail calloc messaggio (server, retrieve_data");

    snprintf(response.buff,response.len,"%s %d \n %s",ok,data.len,data.buff);
  }

  SYSCALL(writen(fd,response.buff,(response.len-1)*sizeof(char)),"Fail writen (server, store_block)");
  stat_byte_share+=response.len;

  free(response.buff);
  free(path_file);
  free(data.buff);
}


/*
Se è presente nella cartella, elimina il file passato in path, altrimenti ritorna un messaggio di errore.
*/
char *delete(char *path){
  struct stat st;

  if(access(path,F_OK)!=-1){    //se il file è presente nella directory allora viene eliminato
    stat(path,&st);             //uso la stat per sapere la grandezza del file
    
    pthread_mutex_lock(&mutex_stat);
      statistiche.size-=st.st_size;   //aggiorno le statistiche sulla grandezza totale della directory data
      unlink(path);
    pthread_mutex_unlock(&mutex_stat);

    return "OK \n";
  }
  else{
    return "KO Il file passato non è presente \n";
  }
}


/*
Elimina file_name seguendo path_usr.
Ritorna l'esito dell'operazione al client tramite l'fd seguendo il protocollo stabilito. 
*/
void delete_data(char* path_usr, char* file_name, int fd){
  msg_t messaggio;
  char *path_file=create_path(path_usr,file_name);    //creo la stringa path_usr/file_name

  messaggio.buff=delete(path_file);   //vado a eliminare il file passato
  messaggio.len=strlen(messaggio.buff);

  SYSCALL(writen(fd,messaggio.buff,messaggio.len*sizeof(char)),"Fail writen (server, store_block)");
  stat_byte_share+=messaggio.len;                     

  if(path_file) free(path_file);
}


/*
Ritorna la maschera con i segnali da gestire
*/
sigset_t create_mask(){ 
  sigset_t mask;

  sigemptyset(&mask);
  sigaddset(&mask,SIGINT);
  sigaddset(&mask,SIGQUIT);
  sigaddset(&mask,SIGTSTP);
  sigaddset(&mask,SIGTERM);
  sigaddset(&mask,SIGALRM);
  sigaddset(&mask,SIGUSR1);

  return mask;
}


/*
Stampa le statistiche quando il server riceve il segnale SIGUSR1
*/
void print_statistics(){
  float time=(float)(get_time()-statistiche.start_time)/1000;

  printf("\n\n");
  printf("-----------------------------------------------------------------\n");
  printf("|                                                               |\n");
  printf("|                 Statistiche server (SIGUSR1)                  |\n");
  printf("|                                                               |\n");
  printf("-----------------------------------------------------------------\n");
  printf("-----------------------------------------------------------------\n");

  pthread_mutex_lock(&mutex_stat);
    printf("| Server attivo da \t\t %.3fs \t\t\t| \n\n",time);
    printf("| Client totali \t\t %d \t\t\t\t| \n\n",statistiche.total_client);
    printf("| Client online \t\t %d \t\t\t\t| \n\n",statistiche.client_online);
    printf("| Byte totali scambiati \t %.1f MB \t\t\t| \n\n",(float)stat_byte_share/1024/1024);
    printf("| Totale dimensione \t\t %.1f MB \t\t\t| \n\n",(float)statistiche.size/1024/1024);
  pthread_mutex_unlock(&mutex_stat);

  printf("-----------------------------------------------------------------\n");
}


/*
Ritorna il tempo in millisecondi. 
Viene usata per calcolare il tempo che il server è attivo.
*/
long get_time(){
  struct timeval tv;
  gettimeofday(&tv,NULL);

  return ((tv.tv_sec*1000)+(tv.tv_usec/1000));
}