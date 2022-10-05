#include "shared/types.h"
#include "shared/file_util.h"
#include "pipe/pipe.h"
#include "shm/shm.h"
#include "shared/arg_parse.h"

int main(int argc, char **argv)
{
  args_parse args = parse_args(argc, argv);
  setting_t settings = args.settings;

  char* method_str;
  char* role_str;
  { // Assign strings to method and role for pretty printing
    switch (settings.method){ 
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

    switch (settings.role){
      case SENDER:
        role_str = "Sender";
        break;
      case RECEIVER:
        role_str = "Receiver";
        break;
      case CLEANER:
        role_str = "Cleaner";
        break;
      default:
        role_str = "Error";
        break;
    }
  }

  int all_flags = 1;

  { // Check for potential missing arguments
    if (!args.flag_method){
      printf("Specify the IPC method through '--method' or '-m'\n");
      all_flags &= 0;
    }
    if (!args.flag_pass){
      printf("Specify the password through '--pass' or '-p'\n");
      all_flags &= 0;
    }
    if (!args.flag_role){
      printf("Specify program role through '--role' or '-r'\n");
      all_flags &= 0;
    }
    if (!args.flag_file && (settings.role != CLEANER)){
      printf("Specify the file to work on through '--file' or '-f'\n");
      all_flags &= 0;
    }

    if ((settings.method==SHARED) && (settings.role == SENDER) && !args.flag_max_pages){
      printf("Setting default buffer size to 2 kb, use the -l flag to set manually\n");
      settings.max_pages = 2;
    }

    if (!all_flags){
      printf("Use --help to get more informations on how to use the program\n");
      return 3;
    } else {
      printf("Role: %s\nMethod: %s\nPassword: %s\nFile: %s\n", role_str, method_str, settings.password, settings.filename);
    }
    printf("\n");
  }

  switch (settings.method){
    case PIPE:
      return use_pipe(settings);
      break;
    case SHARED:
      return use_shared(settings);
      break;
    default:
      break;
  }
  return 0;
}