#include "types.h"
void err_and_leave(char* messg, int code){
  fprintf(stderr, "%s\n", messg);
  exit(code);
}

role_t parse_role(char* inp){
  switch (*inp){
    case 's':
      return SENDER;
    case 'r':
      return RECEIVER;
    case 'c':
      return CLEANER;
    default:
      err_and_leave("Role needs to either be r for receiver or s for sender", 2);
  }
  return -1;
}

ipc_t parse_ipc(char* inp){
  switch (*inp){
    case 'p':
      return PIPE;
      break;
    case 's':
      return SHARED;
      break;
    default:
      err_and_leave("Method needs to either be p for pipe or s for shared", 2);
      break;
  }
  return -1;
}
