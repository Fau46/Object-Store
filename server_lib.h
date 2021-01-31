#ifndef SERVER_LIB_H
#define SERVER_LIB_H

#include<stdio.h>
#include<stdlib.h>
#include<dirent.h>
#include<sys/types.h>

#define MAXBACKLOG 32

#define SYSCALL(x,str)\
  if((x)==-1){\
    perror(str);\
    fprintf(stderr,"Error at line %d of file %s\n", __LINE__, __FILE__);\
    exit(EXIT_FAILURE);\
  }
  
#define CHECKNULL(x,str)\
  if((x)==NULL){\
    perror(str);\
    fprintf(stderr,"Error at line %d of file %s\n", __LINE__, __FILE__);\
    exit(errno);\
  }


typedef struct messaggio{
  int len;
  char *name;
  char *data;
  char *operation;
}messaggio_t;

typedef struct statistics{
  int size;
  int object;
  long start_time;
  int total_client;
  int client_online;
}statistics_t;


/*
Legge il messaggio passato dal client tramite l'fd e lo tokenizza, ritornando un messaggio di tipo messagio_t. 
Per sapere di più su come è strutturato messaggio_t controllare server_lib.h.
*/
messaggio_t raw_msg(long fd);


/*
Crea la directory specificata in str se non esiste.
Ritorna 1 se la directory già esisteva o se la sua creazione è andata a buon fine, 0 altrimenti
*/
int create_directory(char *str);


/*
Registra l'utente usr_name creando l'apposita directory, nel caso in cui non fosse ancora presente, nella cartella data.
Risponde al client con un messaggio tramite fd della socket seguendo il protocollo stabilito.
Ritorna 1 se l'operazione è andata a buon fine, 0 altrimenti
*/
int register_usr(char *usr_name, char **path_usr, char *data, int fd);


/*
Archivia i dati presenti in mex nel path_usr.
Ritorna l'esito dell'operazione al client tramite l'fd seguendo il protocollo stabilito.
*/
void store_data(messaggio_t mex,char *path_usr,int fd);


/*
Legge i dati presenti in file_name salvato seguendo path_usr.
Ritorna al client, tramite l'fd, i dati letti o l'esito negativo dell'operazione, rispettando il protocollo stabilito.
*/
void retrieve_data(char* path_usr, char* file_name, int fd);


/*
Elimina file_name seguendo path_usr.
Ritorna l'esito dell'operazione al client tramite l'fd seguendo il protocollo stabilito. 
*/
void delete_data(char* path_usr, char* file_name, int fd);

/*
Ritorna la maschera con i segnali da gestire
*/
sigset_t create_mask();

/*
Stampa le statistiche quando il server riceve il segnale SIGUSR1
*/
void print_statistics(); 


/*
Ritorna il tempo in millisecondi. 
*/
long get_time();

#endif