#ifndef HEADER_hashing
#define HEADER_hashing
#include <stdio.h>
#include <openssl/sha.h>
#include <string.h>

typedef struct { char x[SHA_DIGEST_LENGTH*2 + 1]; } sha_hexdigest;

void hash_file(char* path, sha_hexdigest* digest);

void sha1tohex(char* src, sha_hexdigest* digest);


#endif
