#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"hash.h"

static int size;    //grandezza della tabella hash

/*
Ritorna la posione di key nella tabella hash
*/
int hashCode(char* key){
  int i;
  int aux=0;

  for(i=0; i<strlen(key); i++){
    aux+=key[i];
  }
  return (aux%size);
}


/*
Ritorna una tabella hash di dimensione n con NULL in tutte le posizioni.
Se non va a buon fine viene ritornato NULL.
*/
hash_t* create_hash_table(int n){
  int i;
  hash_t* table = (hash_t*)malloc(n*sizeof(hash_t));
  if(table==NULL) return NULL;
  
  for(i=0; i<n; i++){
    table[i] = NULL;
  }

  size=n;
  return table;
}


/*
Inserisce value in table.
Ritorna 0 in caso di successo, -1 altrimenti.
*/
int insert_hash_table(hash_t *table, char* value){
  int i=hashCode(value);
  
  hash_t node=(hash_t)malloc(sizeof(node_t));
  if(node==NULL) return -1;

  node->next=NULL;
  node->data=calloc(strlen(value)+1,sizeof(char));
  
  if(node->data==NULL) return -1;
  strncpy(node->data,value,strlen(value)+1);
  
  if(table[i]==NULL){
    table[i]=node;
  }
  else{
    node->next=table[i];
    table[i]=node;
  }

  return 0;
}


/*
Cerca value in table.
Ritorna 1 se l'ha trovato, 0 altrimenti.
*/
int find_hash_table(hash_t* table, char* value){ 
  int find=0;

  if(value!=NULL){
    int i=hashCode(value);
    hash_t aux=table[i];

    while(aux!=NULL && find==0){
      if(strcmp(aux->data,value)==0){
        find=1;
      }
      else{
        aux=aux->next;
      }
    }
  }

  return find;
}


/*
Rimuove value da table.
Ritorna 1 se l'operazione è andata a buon fine, 0 altrimenti
*/
int remove_hash_table(hash_t* table, char* value){
  if(value!=NULL){
    int i=hashCode(value);
    hash_t aux=table[i];
    hash_t prec=NULL;

    while(aux!=NULL){
      if(strcmp(aux->data,value)==0){
        if(prec==NULL){
          table[i]=aux->next;
        }
        else{
          prec->next=aux->next;
        }
        free(aux->data);
        free(aux);
        return 1;
      }
      prec=aux;
      aux=aux->next;
    }
  }

  return 0;
}

/*
Elimina table liberando memoria
*/
void delete_hash_table(hash_t* table){
  int i;
  hash_t aux,succ=NULL;

  for(i=0; i<size; i++){
    aux=table[i];
    
    if(aux!=NULL){
      while(aux!=NULL){
        succ=aux->next;
        free(aux->data);
        free(aux);
        aux=succ;
      }
    }
  }

  free(table);
}
