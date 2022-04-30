#ifndef PARSE_H
#define PARSE_H

typedef char Options[256];

/* strip the argument vector of dashed options and updates opts accordingly */ 
void parse_options (int *argc, char ***argv, Options opts);

void print_args (int argc, char **argv);

#endif /* PARSE_H */
