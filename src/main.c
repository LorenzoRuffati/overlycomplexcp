#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include "shared/types.h"
#include "shared/file_util.h"
#include "pipe/pipe.h"
#include "shm/shm.h"


int main(int argc, char **argv)
{
  int flag_pass = 0;
  int flag_file = 0;
  int flag_role = 0;
  int flag_method = 0;
  int flag_max_pages = 0;
  
  static struct option long_options[] = {
    {"pass", required_argument, NULL, 'p'},
    {"file", required_argument, NULL, 'f'},
    {"role", required_argument, NULL, 'r'},
    {"method", required_argument, NULL, 'm'},
    {"max_pages", required_argument, NULL, 'l'},
    {0, 0, NULL, 0}
  };

  int opt_found;
  int opt_index = 0;
  setting_t settings;

  while ((opt_found = getopt_long(argc, argv, "p:f:r:m:l:", long_options, &opt_index)) != -1){
    switch (opt_found)
    {
    case 'p': // either -p<pass> or --pass <pass>
      if (flag_pass){
        err_and_leave("Too many definitions of password", 1);
      } else {
        flag_pass = 1;
      }
      settings.password = optarg;
      break;
    case 'f':
      if (flag_file){
        err_and_leave("Too many definitions of file", 1);
      }else {
        flag_file = 1;
      }
      settings.filename = optarg;
      break;
    
    case 'r':
      if (flag_role){
        err_and_leave("Too many definitions of role", 1);
      } else {
        flag_role = 1;
      }

      switch (*optarg){
        case 's':
          settings.role = SENDER;
          break;
        case 'r':
          settings.role = RECEIVER;
          break;
        default:
          err_and_leave("Role needs to either be r for receiver or s for sender", 2);
          break;
      }
      break;
    case 'm':
      if (flag_method){
        err_and_leave("Too many definitions of method", 1);
      } else {
        flag_method = 1;
      }
      
      switch (*optarg){
        case 'p':
          settings.method = PIPE;
          break;
        case 's':
          settings.method = SHARED;
          break;
        default:
          err_and_leave("Method needs to either be p for pipe or s for shared", 2);
          break;
      }
      break;
    case 'l':
    if (flag_max_pages){
      err_and_leave("Too many definitions of max_pages", 1);
      } else {
        flag_max_pages = 1;
        }
        settings.max_pages = atoi(optarg);
        break;
    }
  }

  char* method_str;
  switch (settings.method)
  {
  case SHARED:
    method_str = "Shared memory";
    break;
  case PIPE:
    method_str = "Pipes";
    break;
  default:
    method_str = "Error";
    break;
  }

  char* role_str;
  switch (settings.role)
  {
  case SENDER:
    role_str = "Sender";
    break;
  case RECEIVER:
    role_str = "Receiver";
    break;
  default:
    role_str = "Error";
    break;
  }

  if (!flag_file || !flag_method || !flag_pass || !flag_role ){
    err_and_leave("Some options not set", 3);
  } else {
    printf("Role: %s\nMethod: %s\nPassword: %s\nFile: %s\n", role_str, method_str, settings.password, settings.filename);
  }

  switch (settings.method)
  {
  case PIPE:
    err_and_leave("Pipe not yet implemented", 0);
    break;
  case SHARED:
    return use_shared(settings);
    break;
  default:
    break;
  }
  return 0;
}