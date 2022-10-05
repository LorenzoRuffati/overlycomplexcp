#ifndef HEADER_types
#define HEADER_types

#include <stdio.h>
#include <stdlib.h>

typedef enum {SENDER, RECEIVER, CLEANER} role_t;
typedef enum {PIPE, QUEUE, SHARED} ipc_t;


typedef struct {
  char* password; // The shared code to allow sender and receiver to coordinate
  char* filename; // The file to be used either to write to or to send over ipc
  role_t role; // Whether the program acts as sender or receiver
  ipc_t method;
  int max_pages;
} setting_t;

void err_and_leave(char* messg, int code);
role_t parse_role(char* inp);
ipc_t parse_ipc(char* inp);
#endif