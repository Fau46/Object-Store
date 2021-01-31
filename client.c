#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"client_lib.h"


/******* MACRO *******/
#define CHECKE(x,val,str)\
  if((x)==val){\
    perror(str);\
    fprintf(stderr,"Error at line %d of file %s\n", __LINE__, __FILE__);\
    exit(EXIT_FAILURE);\
  }


/******* COSTANTI *******/
#define MAX_OBJECTS 20          //numero di oggetti che ogni client genera
#define INC_SIZE_OBJECT 5258    //incremento della grandezza degli oggetti generati
#define START_SIZE_OBJECT 100   //grandezza iniziale del primo oggetto generato


/******* STATISTICHE *******/
typedef struct statistiche{
  int op;
  int connect;
  int ok_store;
  int ko_store;
  int ok_delete;
  int ko_delete;
  int disconnect;
  int ok_retrieve;
  int ko_retrieve;
  int wrong_retrieve;   //viene incrementata quando i dati sul server non corrispondono a quelli passati
}statistiche_t;


/******* VARIABILI GLOBALI *******/
char *name_user;
char *file="file_test.txt";   //nome del file di test di input


/******* FUNZIONI *******/
void  test_1();                             //richiede al server di memorizzare dei dati passati
void  test_2();                             //richiede al server dei dati
void  test_3();                             //elimina dal server dei dati
char *create_block(int n);                  //crea i dati da passare al server
void  statistic_print(statistiche_t st);    //stampa le statistiche del client
int   check_retrieve(void *s1, void *s2);   //controlla che i dati ricevuti dal server corrispondano a quelli inviati


/******* MAIN *******/
int main(int argc, char *argv[]){
  int num;

  if(argc!=3){
    fprintf(stderr,"%s  Nome_client  Codice_test: 1 store | 2 retrieve | 3 delete\n",argv[0]);
    return -1;
  }

  num=atoi(argv[2]);
  if(num<1 || num>3){
    fprintf(stderr,"Passare un numero compreso tra 1 e 3\n");
    return -2;
  }

  name_user=argv[1];

  switch (num)
  {
  case 1:
    test_1();
    break;
  case 2:
    test_2();
    break;
  case 3:
    test_3();
    break;
  }
  
  printf("\n\n");
  return 0;
}

/******* IMPLEMENTAZIONE FUNZIONI DICHIARATE *******/

/*
Richiede al server di memorizzare dei dati passati
*/
void test_1(){
  statistiche_t st;
 
  memset(&st,0,sizeof(st));
  st.op=1;

  if(os_connect(name_user)){
    char *block;
    char *file_store=NULL;
    int i, n=START_SIZE_OBJECT, len_file_store=strlen(file)+10;

    st.connect++;

    file_store=calloc(len_file_store,sizeof(char));
    CHECKE(file_store,NULL,"Fail calloc file_store (client,main)");

    for(i=0; i<MAX_OBJECTS; i++){
      snprintf(file_store,len_file_store,"%s%d",file,i);    //creo i nomi da dare ai file all'interno della cartella dell'utente nel server
      block=create_block(n);    //ricevo i dati da inviare al server

      if(!os_store(file_store,(void *)block,strlen(block))){
          st.ko_store++;
        }
      else st.ok_store++;
      
      n+=INC_SIZE_OBJECT;   //incremento la grandezza dei dati da generare
      if(block!=NULL)free(block);
    }

    if(os_disconnect()) st.disconnect++;

    if(file_store!=NULL) free(file_store);
  }

  statistic_print(st);
}

/*
Richiede al server dei dati 
*/
void test_2(){
  statistiche_t st;
 
  memset(&st,0,sizeof(st));
  st.op=2;

  if(os_connect(name_user)){
    char *block;
    char *retrieve;
    char *file_store=NULL;
    int i, n=START_SIZE_OBJECT, len_file_store=strlen(file)+10;
    
    st.connect++;

    file_store=calloc(len_file_store,sizeof(char));
    CHECKE(file_store,NULL,"Fail calloc file_store (client,main)");

    for(i=0; i<MAX_OBJECTS; i++){
      snprintf(file_store,len_file_store,"%s%d",file,i);    //creo i nomi dei file da richiedere
      block=create_block(n);    //dati da confrontare con quelli ricevuti dal server 

      retrieve=(char*)os_retrieve(file_store);

      if(retrieve!=NULL){
        if(!check_retrieve(block,retrieve)){  //Se i dati ricevuti non corrispondono a quelli inviati
          st.wrong_retrieve++;
        }
        else st.ok_retrieve++;

        free(retrieve);
      }
      else st.ko_retrieve++;

      n+=INC_SIZE_OBJECT;  //incremento la grandezza dei dati da generare
      if(block) free(block);
    }

    if(os_disconnect()) st.disconnect++;

    if(file_store) free(file_store);
  }

  statistic_print(st);
}

/*
Elimina dal server dei dati
*/
void test_3(){
  statistiche_t st;
  
  memset(&st,0,sizeof(st));
  st.op=3;

  if(os_connect(name_user)){
    char *file_store=NULL;
    int i, len_file_store=strlen(file)+10;
    
    st.connect++;
    
    file_store=calloc(len_file_store,sizeof(char));
    CHECKE(file_store,NULL,"Fail calloc file_store (client,main)");

    for(i=0; i<MAX_OBJECTS; i++){
      snprintf(file_store,len_file_store,"%s%d",file,i);    //creo i nomi dei file da eliminare
      
      if(!os_delete(file_store)){
        st.ko_delete++;
      }
      else st.ok_delete++;
    }

    if(os_disconnect()) st.disconnect++;

    if(file_store) free(file_store);
  }
  
  statistic_print(st);
}


/*
Stampa le statistiche del client
*/
void statistic_print(statistiche_t st){
  int tot=st.connect+st.disconnect+st.ok_store+st.ko_store+st.ok_retrieve+st.ko_retrieve+st.wrong_retrieve+st.ok_delete+st.ko_delete;

  switch (st.op)
  {
  case 1:
    printf("Operazione ok_store: %d;\t",st.ok_store);
    printf("Operazione ko_store: %d;\t",st.ko_store);
    break;
 
  case 2:
    printf("Operazione ok_retrieve: %d;\t",st.ok_retrieve);
    printf("Operazione ko_retrieve: %d;\t",st.ko_retrieve);
    printf("Operazione wrong_retrieve: %d;\t",st.wrong_retrieve);
    break;
  
  case 3:
    printf("Operazione ok_delete: %d;\t",st.ok_delete);
    printf("Operazione ko_delete: %d;\t",st.ko_delete);
    break;
  }

  printf("Operazioni totali: %d;\t",tot);
  printf("Operazione connect: %d;\t",st.connect);
  printf("Operazione disconnect: %d;\t",st.disconnect);
  printf("CLIENT: %s;\t",name_user);
  printf("TEST: %d;\n",st.op);

}


/*
Crea i dati da passare al server
*/
char *create_block(int n){
  int i,j=0;
  char *frase="Unipi SOL\n";
  int len=n/(strlen(frase)*sizeof(char));   //numero di stringhe da generare
  char *block=calloc(n+1,sizeof(char)); 
  CHECKE(block,NULL, "Fail calloc block (client, create_block)");

  for(i=0;i<len;i++){
    strncpy(block+j,frase,strlen(frase));
    j+=strlen(frase);
  }
  
  return block;
}


/*
Controlla che i dati ricevuti dal server corrispondano a quelli inviati
*/
int check_retrieve(void *s1, void *s2){
  int i,len;
  char *aux1 = (char*) s1;
  char *aux2 = (char*) s2;

  if(strlen(aux1)!=strlen(aux2)) return 0;

  len=strlen(aux1);

  for(i=0; i<len; i++){
    if(aux1[i]!=aux2[i]) return 0;
  }

  return 1;
}