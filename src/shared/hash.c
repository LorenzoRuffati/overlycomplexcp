#include "hash.h"

void hash_file(char* path, sha_hexdigest* buffer){
    char dig[SHA_DIGEST_LENGTH];
    FILE* fstr= fopen(path, "r");
    SHA_CTX sha;
    SHA1_Init(&sha);
    char buff[512];

    int nr;
    while ((nr = fread(buff, 1, 512, fstr)) != 0)
    {
        SHA1_Update(&sha, buff, nr);
    }
    SHA1_Final(dig, &sha);
    sha1tohex(dig, buffer);
}

char int_to_hex[] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};

void sha1tohex(char* src, sha_hexdigest* dst){
    for (int i=0; i<SHA_DIGEST_LENGTH; i++){
        sprintf(&(dst->x)[i*2], "%02x",(unsigned char) src[i]);
    }
}