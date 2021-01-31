#ifndef CLIENT_LIB_H
#define CLIENT_LIB_H

#include<stdio.h>
#include<stdlib.h>

/*
Richiede al server di effettuare la registrazione col nome name.
Restituisce true se la connessione ha avuto successo, false altrimenti. 
*/
int os_connect(char *name); 

/*
Richiede all'object store la memorizzazione dell'oggetto block di lunghezza len, con il nome name. 
Restituisce true se la memorizzazione ha avuto successo, false altrimenti. 
*/
int os_store(char *name, void* block, size_t len);

/*
Richiede all'object store i dati memorizzati con name. 
Se la richiesta ha avuto successo restituisce i dati richiesti, altrimenti NULL. 
*/
void *os_retrieve(char *name);

/*
Richiede al server di eliminare l'oggetto memorizzato con name.
Restituisce true se la cancellazione ha avuto successo, false altrimenti
*/
int os_delete(char *name);

/*
Richiede all'object store di chiudere la connessione.
Restituisce true se l'operazione è andata a buon fine, false altrimenti
*/
int os_disconnect();

#endif