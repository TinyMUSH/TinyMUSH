/* mkindx.c - make help/news file indexes */
/*
*  mkindx now supports multiple tags for a single entry.
*  example:
*
*		& foo
*		& bar
*		This is foo and bar.
*		& baz
*		This is baz.
*
*/
#include "copyright.h"
#include "config.h"
#include "system.h"

#include <typedefs.h>
#include  "help.h"

/* From help.c */
extern int      helpmkindx(char *, char *);

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        printf
        ("Usage:\tmkindx <file_to_be_indexed> <output_index_filename>\n");
        exit(-1);
    }
    
    exit(helpmkindx(argv[1], argv[2]));
}

