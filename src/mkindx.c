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

#include  "help.h"

char line[LINE_SIZE + 1];

typedef struct _help_indx_list
{
    help_indx entry;
    struct _help_indx_list *next;
} help_indx_list;

int dump_entries(FILE * wfp, long pos, help_indx_list * entries);

int main(int argc, char *argv[]) {
    long pos;

    int i, n, lineno, ntopics, actualdata;

    char *s, *topic;

    help_indx_list *entries, *ep;

    FILE *rfp, *wfp;

    if (argc < 2 || argc > 3)
    {
        printf
        ("Usage:\tmkindx <file_to_be_indexed> <output_index_filename>\n");
        exit(-1);
    }
    if ((rfp = fopen(argv[1], "r")) == NULL)
    {
        fprintf(stderr, "can't open %s for reading\n", argv[1]);
        exit(-1);
    }
    if ((wfp = fopen(argv[2], "w")) == NULL)
    {
        fprintf(stderr, "can't open %s for writing\n", argv[2]);
        exit(-1);
    }

    pos = 0L;
    lineno = 0;
    ntopics = 0;
    actualdata = 0;

    /*
     * create initial entry storage
     */
    entries = (help_indx_list *) malloc(sizeof(help_indx_list));
    memset(entries, 0, sizeof(help_indx_list));

    while (fgets(line, LINE_SIZE, rfp) != NULL)
    {
        ++lineno;

        n = strlen(line);
        if (line[n - 1] != '\n')
        {
            fprintf(stderr, "line %d: line too long\n", lineno);
        }
        if (line[0] == '&')
        {
            ++ntopics;

            if ((ntopics > 1) && actualdata)
            {
                /*
                 *  we've hit the next topic, time to write the ones we've been
                 *  building
                 */
                actualdata = 0;
                if (dump_entries(wfp, pos, entries))
                {
                    fprintf(stderr, "error writing %s\n",
                            argv[2]);
                    exit(-1);
                }
                memset(entries, 0, sizeof(help_indx_list));
            }

            if (entries->entry.pos)
            {
                /*
                 *  we're already working on an entry... time to start nesting
                 */
                ep = entries;
                entries =
                    (help_indx_list *)
                    malloc(sizeof(help_indx_list));
                memset(entries, 0, sizeof(help_indx_list));
                entries->next = ep;
            }

            for (topic = &line[1];
                    (*topic == ' ' || *topic == '\t')
                    && *topic != '\0'; topic++);
            for (i = -1, s = topic; *s != '\n' && *s != '\0'; s++)
            {
                if (i >= TOPIC_NAME_LEN - 1)
                    break;
                if (*s != ' '
                        || entries->entry.topic[i] != ' ')
                    entries->entry.topic[++i] = *s;
            }
            entries->entry.topic[++i] = '\0';
            entries->entry.pos = pos + (long)n;
        }
        else if (n > 1)
        {
            /*
             *  a non blank line.  we can flush entries to the .indx file the next
             *  time we run into a topic line...
             */
            actualdata = 1;
        }
        pos += n;
    }
    if (dump_entries(wfp, pos, entries))
    {
        fprintf(stderr, "error writing %s\n", argv[2]);
        exit(-1);
    }
    fclose(rfp);
    fclose(wfp);

    printf("%d topics indexed\n", ntopics);
    exit(0);
}

int dump_entries(FILE * wfp, long pos, help_indx_list * entries) {
    int truepos;

    int truelen;

    int depth;

    help_indx_list *prev_ep, *ep;

    /*
     *  if we have more than one entry, the one on the top of the chain
     *  is going to have the actual pos we want to use to index with
     */
    truepos = (long)entries->entry.pos;
    truelen = (int)(pos - entries->entry.pos);

    prev_ep = 0;
    depth = 0;

    for (ep = entries; ep; ep = ep->next)
    {
        ep->entry.pos = (long)truepos;
        ep->entry.len = truelen;
        if (fwrite(&ep->entry, sizeof(help_indx), 1, wfp) < 1)
            return (-1);

        if (prev_ep)
            free(prev_ep);

        if (depth++)	/* don't want to try to free the top of the chain */
            prev_ep = ep;
    }
    /*
     *  no attempt is made to free the last remaining struct as its actually the
     *  one on the top of the chain, ie. the statically allocated struct.
     */

    return (0);
}
