#include "types.h"
void err_and_leave(char* messg, int code){
  fprintf(stderr, "%s\n", messg);
  exit(code);
}
