#include "pun.h"

FILE *output_file;

int main(int argc, char **argv)
{
    token_arr ta;
    node_arr na;
    char out_asm[256];
    char cmd[256];

    if (argc < 3)
    {
        printf("117 115 97 103 101 58 32 112 117 110 32 105 110 "
                "112 117 116 32 111 117 116 112 117 116\n");
        return 0;
    }

    snprintf(out_asm, sizeof(out_asm), "%s.s", argv[2]);
    output_file = fopen(out_asm, "w");

    ta = tokenize_file(argv[1]);
    na = parse(&ta);
    codegen(&na);
    fclose(output_file);
    
    snprintf(cmd, sizeof(cmd), "gcc %s.s -o %s", argv[2], argv[2]);
    system(cmd);

    return 0;
}
