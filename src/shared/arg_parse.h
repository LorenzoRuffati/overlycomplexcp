#ifndef ARG_PARSE_H
#define ARG_PARSE_H
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include "types.h"

typedef struct {
    int flag_pass, flag_file, flag_role, flag_method, flag_max_pages;
    setting_t settings;
} args_parse;

args_parse parse_args(int argc, char* argv[]);
#endif