#define _POSIX_C_SOURCE  200112L
#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<pthread.h>

#include<sys/un.h>
#include<sys/time.h>
#include<sys/socket.h>
#include<sys/select.h>

#include"hash.h"
#include"conn.h"
#include"server_lib.h"


/******* COSTANTI *******/
#define HASH_SIZE 100
#define TIMEOUT_SELECT 10000


/******* VARIABILI GLOBALI *******/
hash_t *hash_table;                                         //tabella hash per tenere traccia dei client connessi
char *data="./data";                                        //directory dove sono salvati tutti gli object
statistics_t statistiche;                                   //statistiche del server
static volatile sig_atomic_t stop=0;                        //se settata a true fa terminare il server
static volatile sig_atomic_t thread_conn=0;                 //tiene conto del numero di thread attivi
pthread_mutex_t mutex_stat=PTHREAD_MUTEX_INITIALIZER;       //mutex per le statistiche 
pthread_mutex_t mutex_hash_table=PTHREAD_MUTEX_INITIALIZER; //mutex per l'hash table


/******* FUNZIONI *******/
void *thread_handler();              //gestisce i segnali
void  create_thread_handler();       //crea un thread handler 
void *thread_worker(void *arg);      //gestisce la connessione col client
void  create_thread_worker(long fd); //crea un thread worker per connessione

void cleanup(){                     //funzione di cleanup quando il server termina
  unlink(SOCKNAME);
  delete_hash_table(hash_table);
}

void add_user(){                    //aggiorna il numero di client connessi e client totali 
  pthread_mutex_lock(&mutex_stat);
  statistiche.client_online++;
  statistiche.total_client++;
  pthread_mutex_unlock(&mutex_stat);
}

void remove_user(){                 //aggiorna il numero di client connessi
  pthread_mutex_lock(&mutex_stat);
  statistiche.client_online--;
  pthread_mutex_unlock(&mutex_stat);
}


/******* MAIN *******/
int main(int argc, char *argv[]){
  sigset_t mask;
  int sl,leave=0;
  int sockid, connfd;
  fd_set fdset,rfdset;
  struct timeval time;    //timer della select
  struct sockaddr_un sa;

  memset(&statistiche,0,sizeof(statistiche));
  statistiche.start_time=get_time();    //per maggiori info su get_time vedere server_lib.c

  mask=create_mask();
  pthread_sigmask(SIG_SETMASK,&mask,NULL);

  create_thread_handler(); 

  CHECKNULL(hash_table=create_hash_table(HASH_SIZE),"Fail hash table (server, main)");

  unlink(SOCKNAME);
  atexit(cleanup);

  if(!create_directory(data)){    //creo la directory data
    printf("Errore creazione %s (server, main)\n",data);
    return 0;
  }

  SYSCALL((sockid=socket(AF_UNIX,SOCK_STREAM,0)),"Fail socket (server, main)");

  sa.sun_family=AF_UNIX;
  strncpy(sa.sun_path,SOCKNAME,sizeof(SOCKNAME)+1);

  SYSCALL(bind(sockid,(struct sockaddr*)&sa,sizeof(sa)),"Fail bind (server, main)");
  SYSCALL(listen(sockid,MAXBACKLOG),"Fail listen (server, main)");

  FD_ZERO(&fdset);
  FD_SET(sockid,&fdset);

  do{
    rfdset=fdset;   //la select resetta rfdset ad ogni ciclo

    time.tv_sec=0;
    time.tv_usec=TIMEOUT_SELECT;  //imposto il timeout della select

    SYSCALL((sl=select(sockid+1,&rfdset,NULL,NULL,&time)),"Fail select");
    
    if(sl>0){   //se vi è un nuovo client per la connessione
      SYSCALL((connfd=accept(sockid,NULL,NULL)),"Fail accept");
      create_thread_worker(connfd);
    }
  
    if(stop){   //se il server deve arrestarsi attende la chiusura di tutti i client
      while(thread_conn) sleep(1);
      leave=1;
    }

  }while(!leave);
  
  return 0;
}



/******* IMPLEMENTAZIONE FUNZIONI DICHIARATE *******/

/*
Crea un thread worker per connessione.
*/
void create_thread_worker(long fd){
  pthread_t thid;
  pthread_attr_t thattr;

  if(pthread_attr_init(&thattr)!=0){
    fprintf(stderr,"Fail pthread_attr_init (server, create_thread_worker)\n");
    close(fd);
    return;
  }

  if(pthread_attr_setdetachstate(&thattr,PTHREAD_CREATE_DETACHED)!=0){
    fprintf(stderr,"Fail pthread_attr_setdetachstate  (server, create_thread_worker)\n");
    pthread_attr_destroy(&thattr);
    close(fd);
    return;
  }

  if(pthread_create(&thid,&thattr,&thread_worker,(void*)fd)!=0){
    fprintf(stderr,"Fail pthread_create (server, create_thread_worker)\n");
    pthread_attr_destroy(&thattr);
    close(fd);
    return;
  }
}


/*
Crea un thread handler.
*/
void create_thread_handler(){
  pthread_t thid;
  pthread_attr_t thattr;

  if(pthread_attr_init(&thattr)!=0){
    fprintf(stderr,"Fail pthread_attr_ini (server, create_thread_handler)\n");
    return;
  }

  if(pthread_attr_setdetachstate(&thattr,PTHREAD_CREATE_DETACHED)!=0){
    fprintf(stderr,"Fail pthread_attr_setdetachstate (server, create_thread_handler)\n");
    pthread_attr_destroy(&thattr);
    return;
  }

  if(pthread_create(&thid,&thattr,&thread_handler,NULL)!=0){
    fprintf(stderr,"Fail pthread_create (server, create_thread_handler)\n");
    pthread_attr_destroy(&thattr);
    return;
  }
}

/*
Thread che si occupa della gesitone dei segnali.
Tutti i segnali gestiti, ad eccezione di SIGUSR1, fanno chiudere il server e i relativi thread
 */
void *thread_handler(){
  int sig;
  sigset_t set=create_mask();
  
  while(!stop){
    sigwait(&set,&sig);

    if(sig==SIGUSR1){
      print_statistics();      
      thread_handler();   //mi rimetto in ascolto di nuovi segnali
    }
    else{
      stop=1;
    }
  }
  
  pthread_exit(NULL);
}


/*
Thread che si occupa di servire un client.
*/
void *thread_worker(void *arg){
  int write=1;
  int leave=0;
  char *path=NULL;        //path data/user
  long  fd=(long) arg;
  char *name_user=NULL;   //variabile usata per l'hash table 

  thread_conn++;

  do{
    messaggio_t mex;
    
    mex=raw_msg(fd);      //ricevo l'operazione richiesta dal client

    if(strcmp(mex.operation,"break")==0) break;
    if(strcmp(mex.operation,"malformed")==0){
      SYSCALL(writen(fd,mex.data,mex.len*sizeof(char)),"Fail writen (server, thread_worker - malformed)");
      write=0;
      break;
    }

    if(strcmp(mex.operation,"REGISTER")==0){
      pthread_mutex_lock(&mutex_hash_table);

        if(find_hash_table(hash_table,mex.name)){   //se l'utente è già connesso allora rifiuto la richiesta di register
          int len=27+strlen(mex.name);
          char messaggio[len];

          snprintf(messaggio,len,"KO Utente %s gia' connesso \n",mex.name);

          SYSCALL(writen(fd,messaggio,len*sizeof(char)),"Fail writen (server, thread_worker - register)");
          
          leave=1;    //faccio terminare il thread
        }
        else{
          if(register_usr(mex.name,&path,data,fd)){   //se la registrazione dell'utente è andata a buon fine lo inserisco nella tabella hash
            SYSCALL(insert_hash_table(hash_table,mex.name),"Fail insert_hash_table (server, thread_worker - register)");
            
            add_user(); 

            CHECKNULL((name_user=calloc(strlen(mex.name)+1,sizeof(char))),"Fail calloc name_user (server ,thread_worker - register");
            strncpy(name_user,mex.name,strlen(mex.name)+1);
          }
          else leave=1;   //se l'operazione di registrazione non è andata a buon fine faccio terminare il thread 
        }

      pthread_mutex_unlock(&mutex_hash_table);
    }
    else if(strcmp(mex.operation,"STORE")==0){ 
      store_data(mex,path,fd); 
    }
    else if(strcmp(mex.operation,"RETRIEVE")==0){
      retrieve_data(path,mex.name,fd);
    }
    else if(strcmp(mex.operation,"DELETE")==0){
      delete_data(path,mex.name,fd);
    }
    else if (strcmp(mex.operation,"LEAVE")==0){
      pthread_mutex_lock(&mutex_hash_table); 
        remove_hash_table(hash_table,name_user);
      pthread_mutex_unlock(&mutex_hash_table);

      char *messaggio="OK \n";
      int len=strlen(messaggio)+1;
      SYSCALL(writen(fd,messaggio,len*sizeof(char)),"Fail writen  (server, thread_worker - leave)");

      leave=1; 
    }

    if(mex.data) free(mex.data);
    if(mex.name) free(mex.name);
    if(mex.operation) free(mex.operation); 

  }while(!leave && !stop);

  if(!leave){   //Viene eseguito se il thread viene chiuso e non si è fatta l'operazione leave
    pthread_mutex_lock(&mutex_hash_table);
      remove_hash_table(hash_table,name_user);
    pthread_mutex_unlock(&mutex_hash_table);

    if(write) SYSCALL(writen(fd,"KO thread chiuso \n",strlen("KO thread chiuso \n")*sizeof(char)),"Fail writen (server, thread_worker)");   //se il client ha mandato un messaggio sulla socket ma non si è fatto in tempo a leggere
  }

  
  if(path!=NULL) free(path);
  if(name_user!=NULL){
    free(name_user);
    remove_user();    //se la condizione nell'if è vera vuol dire che ho incrementato in precedenza il contatore che tiene conto degli utenti online
  }

  close(fd);

  thread_conn--;
  
  pthread_exit(NULL);
}

