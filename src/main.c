#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include "shared/types.h"
#include "helpstr.h"

void print_help(){
  printf("%.*s\n", ___src_help_str_txt_len, ___src_help_str_txt);
}

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

      if (strchr(optarg, '/') != NULL){
        err_and_leave("Can't have '/' in password", 1);
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
      settings.role = parse_role(optarg);
      break;
    case 'm':
      if (flag_method){
        err_and_leave("Too many definitions of method", 1);
      } else {
        flag_method = 1;
      }
      settings.method = parse_ipc(optarg);
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


  for (int idx=optind; idx<argc; idx++){
    if (!flag_role){
      settings.role = parse_role(argv[idx]);
      flag_role = 1;
    } else if (!flag_method){
      settings.method = parse_ipc(argv[idx]);
      flag_method = 1;
    } else if (!flag_file){
      settings.filename = argv[idx];
      flag_file = 1;
    } else {
      err_and_leave("Too many arguments", 1);
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
    print_help();
    return 3;
  } else {
    printf("Role: %s\nMethod: %s\nPassword: %s\nFile: %s\n", role_str, method_str, settings.password, settings.filename);
  }
  printf("\n");

  switch (settings.method)
  {
  case PIPE:
    err_and_leave("Pipe not yet implemented", 0);
    break;
  case SHARED:
    err_and_leave("Shared memory not yet implemented", 0);
    break;
  default:
    break;
  }
  return 0;
}