#ifndef HASH_H
#define HASH_H

typedef struct node{
  char* data;
  struct node *next;  
} node_t;

typedef node_t* hash_t;

/*
Ritorna una tabella hash di dimensione size con NULL in tutte le posizioni.
Se non va a buon fine viene ritornato NULL.
*/
hash_t* create_hash_table(int size);


/*
Inserisce value in table.
Ritorna 0 in caso di successo, -1 altrimenti.
*/
int insert_hash_table(hash_t *table, char* value);


/*
Cerca value in table.
Ritorna 1 se l'ha trovato, 0 altrimenti.
*/
int find_hash_table(hash_t* table, char* value);


/*
Rimuove value da table.
Ritorna 1 se l'operazione è andata a buon fine, 0 altrimenti
*/
int remove_hash_table(hash_t* table, char* value);


/*
Elimina table liberando memoria
*/
void delete_hash_table(hash_t* table);

#endif