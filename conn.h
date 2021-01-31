#ifndef CONN_H
#define CONN_H

#include<stdio.h>
#include<errno.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>

#include<sys/un.h>
#include<sys/types.h>
#include<sys/socket.h>

/******* VARIABILI GLOBALI *******/
#define SOCKNAME "./objstore.sock"    //nome socket


/******* STRUCT *******/
typedef struct msg{
    int len;
    char* buff;
}msg_t;


/******* FUNZIONI *******/
static inline int writen(long fd, void *buff, size_t size){
  int r;
  size_t left=size;
  char *bufptr=(char*)buff;

  while(left>0){
    if((r=write((int)fd,bufptr,left))==-1){
      if(errno==EINTR) continue;
      return -1;
    }

    if(r==0) return 0;

    left-=r;
    bufptr+=r;
  }

  return 1;
}

static inline int readn(long fd, void *buff, size_t size){
  int r;
  int left=size;
  char *bufptr=(char*)buff;

  while(left>0){
    if((r=read((int)fd,bufptr,size))==-1){
      if(errno==EINTR) continue;
      return -1;
    }

    if(r==0) return 0;

    left-=r;
    bufptr+=r;
  }

  return size;
}

#endif