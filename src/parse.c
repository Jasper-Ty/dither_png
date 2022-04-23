#include <stdio.h> /* printf() */

#include "parse.h"

/* TODO: Make less code-golfy */

void parse_options (int *argc, char ***argv, Options opts)
{
    int c = *argc;
    char **v = *argv; 
    int i, j;
    for (i = 0; i < c; i++, v++) {
        if (**v == '-') {
            while (*(++(*v)) != '\0')
                opts[**v] = 1;
            (*argc)--;
            for (j = i+1; j < c; j++)
                (*argv)[j-1] = (*argv)[j];
        }
    }
}

void print_args (int argc, char **argv)
{
    for (;argc; argc--, argv++)
        printf("%s\n", *argv);
}

