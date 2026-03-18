/**
 * @file funvars_grids.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Grid data structure functions
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 */

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

#include <ctype.h>
#include <string.h>
#include <pcre2.h>

void grid_free(dbref thing, OBJGRID *ogp)
{
    int r, c;

    if (ogp)
    {
        for (r = 0; r < ogp->rows; r++)
        {
            for (c = 0; c < ogp->cols; c++)
            {
                if (ogp->data[r][c] != NULL)
                {
                    XFREE(ogp->data[r][c]);
                }
            }

            XFREE(ogp->data[r]);
        }

        nhashdelete(thing, &mushstate.objgrid_htab);
        XFREE(ogp);
    }
}

void fun_gridmake(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    OBJGRID *ogp;
    int rows, cols, dimension, r, c, status, data_rows, data_elems;
    char *rbuf, *pname;
    char **row_text, **elem_text;
    Delim csep, rsep;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 5, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &(csep), DELIM_STRING))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 5, &(rsep), DELIM_STRING))
    {
        return;
    }

    rows = (int)strtol(fargs[0], (char **)NULL, 10);
    cols = (int)strtol(fargs[1], (char **)NULL, 10);
    dimension = rows * cols;

    if ((dimension > mushconf.max_grid_size) || (dimension < 0))
    {
        XSAFELBSTR("#-1 INVALID GRID SIZE", buff, bufc);
        return;
    }

    ogp = ((OBJGRID *)nhashfind(player, &mushstate.objgrid_htab));

    if (ogp)
    {
        grid_free(player, ogp);
    }

    if (dimension == 0)
    {
        return;
    }

    /*
     * We store the grid on a row-by-row basis, i.e., the first index is
     * the y-coord and the second is the x-coord.
     */
    ogp = (OBJGRID *)XMALLOC(sizeof(OBJGRID), "ogp");
    ogp->rows = rows;
    ogp->cols = cols;
    ogp->data = (char ***)XCALLOC(rows, sizeof(char ***), "ogp->data");

    for (r = 0; r < rows; r++)
    {
        ogp->data[r] = (char **)XCALLOC(cols, sizeof(char *), "ogp->data[r]");
    }

    status = nhashadd(player, (int *)ogp, &mushstate.objgrid_htab);

    if (status < 0)
    {
        pname = log_getname(player);
        log_write(LOG_BUGS, "GRD", "MAKE", "%s Failure", pname);
        XFREE(pname);
        grid_free(player, ogp);
        XSAFELBSTR("#-1 FAILURE", buff, bufc);
        return;
    }

    /*
     * Populate data if we have any
     */

    if (!fargs[2] || !*fargs[2])
    {
        return;
    }

    rbuf = XMALLOC(LBUF_SIZE, "rbuf");
    XSTRCPY(rbuf, fargs[2]);
    data_rows = list2arr(&row_text, LBUF_SIZE / 2, rbuf, &rsep);

    if (data_rows > rows)
    {
        XSAFELBSTR("#-1 TOO MANY DATA ROWS", buff, bufc);
        XFREE(rbuf);
        grid_free(player, ogp);
        return;
    }

    for (r = 0; r < data_rows; r++)
    {
        data_elems = list2arr(&elem_text, LBUF_SIZE / 2, row_text[r], &csep);

        if (data_elems > cols)
        {
            XSAFESPRINTF(buff, bufc, "#-1 ROW %d HAS TOO MANY ELEMS", r);
            XFREE(rbuf);
            grid_free(player, ogp);
            return;
        }

        for (c = 0; c < data_elems; c++)
        {
            if (ogp->data[r][c] != NULL)
            {
                XFREE(ogp->data[r][c]);
            }
            if (*elem_text[c])
            {
                ogp->data[r][c] = XSTRDUP(elem_text[c], "gp->data[gr][gc]");
            }
        }
    }

    XFREE(rbuf);
}

void fun_gridsize(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    OBJGRID *ogp = ((OBJGRID *)nhashfind(player, &mushstate.objgrid_htab));

    if (!ogp)
    {
        XSAFELBSTR("0 0", buff, bufc);
    }
    else
    {
        XSAFESPRINTF(buff, bufc, "%d %d", ogp->rows, ogp->cols);
    }
}

void fun_gridset(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    OBJGRID *ogp;
    char *xlist, *ylist;
    int n_x, n_y, r, c, i, j, errs = 0;
    char **x_elems, **y_elems;
    Delim isep;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 3, 4, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &isep, DELIM_STRING))
    {
        return;
    }

    ogp = ((OBJGRID *)nhashfind(player, &mushstate.objgrid_htab));

    if (!ogp)
    {
        XSAFELBSTR("#-1 NO GRID", buff, bufc);
        return;
    }

    /*
     * Handle the common case of just one position and a simple
     * separator, first.
     */

    if ((isep.len == 1) && *fargs[0] && !strchr(fargs[0], isep.str[0]) && *fargs[1] && !strchr(fargs[1], isep.str[0]))
    {
        r = (int)strtol(fargs[0], (char **)NULL, 10) - 1;
        c = (int)strtol(fargs[1], (char **)NULL, 10) - 1;

        if ((r < 0) || (c < 0) || (r >= ogp->rows) || (c >= ogp->cols))
        {
            errs++;
        }
        else
        {
            if (ogp->data[r][c] != NULL)
            {
                XFREE(ogp->data[r][c]);
            }
            if (*fargs[2])
            {
                ogp->data[r][c] = XSTRDUP(fargs[2], "gp->data[gr][gc]");
            }
        }

        if (errs)
        {
            XSAFESPRINTF(buff, bufc, "#-1 GOT %d OUT OF RANGE ERRORS", errs);
        }

        return;
    }

    /*
     * Complex ranges
     */

    if (fargs[0] && *fargs[0])
    {
        ylist = XMALLOC(LBUF_SIZE, "ylist");
        XSTRCPY(ylist, fargs[0]);
        n_y = list2arr(&y_elems, LBUF_SIZE / 2, ylist, &isep);

        if ((n_y == 1) && !*y_elems[0])
        {
            XFREE(ylist);
            n_y = -1;
        }
    }
    else
    {
        n_y = -1;
    }

    if (fargs[1] && *fargs[1])
    {
        xlist = XMALLOC(LBUF_SIZE, "xlist");
        XSTRCPY(xlist, fargs[1]);
        n_x = list2arr(&x_elems, LBUF_SIZE / 2, xlist, &isep);

        if ((n_x == 1) && !*x_elems[0])
        {
            XFREE(xlist);
            n_x = -1;
        }
    }
    else
    {
        n_x = -1;
    }

    errs = 0;

    if (n_y == -1)
    {
        for (r = 0; r < ogp->rows; r++)
        {
            if (n_x == -1)
            {
                for (c = 0; c < ogp->cols; c++)
                {
                    if (ogp->data[r][c] != NULL)
                    {
                        XFREE(ogp->data[r][c]);
                    }
                    if (*fargs[2])
                    {
                        ogp->data[r][c] = XSTRDUP(fargs[2], "gp->data[gr][gc]");
                    }
                }
            }
            else
            {
                for (i = 0; i < n_x; i++)
                {
                    c = (int)strtol(x_elems[i], (char **)NULL, 10) - 1;
                    if ((r < 0) || (c < 0) || (r >= ogp->rows) || (c >= ogp->cols))
                    {
                        errs++;
                    }
                    else
                    {
                        if (ogp->data[r][c] != NULL)
                        {
                            XFREE(ogp->data[r][c]);
                        }
                        if (*fargs[2])
                        {
                            ogp->data[r][c] = XSTRDUP(fargs[2], "gp->data[gr][gc]");
                        }
                    }
                }
            }
        }
    }
    else
    {
        for (j = 0; j < n_y; j++)
        {
            r = (int)strtol(y_elems[j], (char **)NULL, 10) - 1;

            if ((r < 0) || (r >= ogp->rows))
            {
                errs++;
            }
            else
            {
                if (n_x == -1)
                {
                    for (c = 0; c < ogp->cols; c++)
                    {
                        if ((r < 0) || (c < 0) || (r >= ogp->rows) || (c >= ogp->cols))
                        {
                            errs++;
                        }
                        else
                        {
                            if (ogp->data[r][c] != NULL)
                            {
                                XFREE(ogp->data[r][c]);
                            }
                            if (*fargs[2])
                            {
                                ogp->data[r][c] = XSTRDUP(fargs[2], "gp->data[gr][gc]");
                            }
                        }
                    }
                }
                else
                {
                    for (i = 0; i < n_x; i++)
                    {
                        c = (int)strtol(x_elems[i], (char **)NULL, 10) - 1;
                        if ((r < 0) || (c < 0) || (r >= ogp->rows) || (c >= ogp->cols))
                        {
                            errs++;
                        }
                        else
                        {
                            if (ogp->data[r][c] != NULL)
                            {
                                XFREE(ogp->data[r][c]);
                            }
                            if (*fargs[2])
                            {
                                ogp->data[r][c] = XSTRDUP(fargs[2], "gp->data[gr][gc]");
                            }
                        }
                    }
                }
            }
        }
    }

    if (n_x > 0)
    {
        XFREE(xlist);
    }

    if (n_y > 0)
    {
        XFREE(ylist);
    }

    if (errs)
    {
        XSAFESPRINTF(buff, bufc, "#-1 GOT %d OUT OF RANGE ERRORS", errs);
    }
}

void fun_grid(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    Delim csep, rsep;
    OBJGRID *ogp;
    char *xlist, *ylist;
    int n_x, n_y, r, c, i, j;
    char **x_elems, **y_elems;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 0, 4, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &(csep), DELIM_STRING | DELIM_NULL | DELIM_CRLF))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &(rsep), DELIM_STRING | DELIM_NULL | DELIM_CRLF))
    {
        return;
    }

    ogp = ((OBJGRID *)nhashfind(player, &mushstate.objgrid_htab));

    if (!ogp)
    {
        XSAFELBSTR("#-1 NO GRID", buff, bufc);
        return;
    }

    /*
     * Handle the common case of just one position, first
     */

    if (fargs[0] && *fargs[0] && !strchr(fargs[0], ' ') && fargs[1] && *fargs[1] && !strchr(fargs[1], ' '))
    {
        r = (int)strtol(fargs[0], (char **)NULL, 10) - 1;
        c = (int)strtol(fargs[1], (char **)NULL, 10) - 1;

        if (!((r < 0) || (c < 0) || (r >= ogp->rows) || (c >= ogp->cols) || (ogp->data[r][c] == NULL)))
        {
            XSAFELBSTR((ogp)->data[(r)][(c)], buff, bufc);
        }

        return;
    }

    /*
     * Complex ranges
     */

    if (!fargs[0] || !*fargs[0])
    {
        n_y = -1;
    }
    else
    {
        ylist = XMALLOC(LBUF_SIZE, "ylist");
        XSTRCPY(ylist, fargs[0]);
        n_y = list2arr(&y_elems, LBUF_SIZE / 2, ylist, &SPACE_DELIM);

        if ((n_y == 1) && !*y_elems[0])
        {
            XFREE(ylist);
            n_y = -1;
        }
    }

    if (!fargs[1] || !*fargs[1])
    {
        n_x = -1;
    }
    else
    {
        xlist = XMALLOC(LBUF_SIZE, "xlist");
        XSTRCPY(xlist, fargs[1]);
        n_x = list2arr(&x_elems, LBUF_SIZE / 2, xlist, &SPACE_DELIM);

        if ((n_x == 1) && !*x_elems[0])
        {
            XFREE(xlist);
            n_x = -1;
        }
    }

    if (n_y == -1)
    {
        for (r = 0; r < ogp->rows; r++)
        {
            if (r != 0)
            {
                print_separator(&rsep, buff, bufc);
            }

            if (n_x == -1)
            {
                for (c = 0; c < ogp->cols; c++)
                {
                    if (c != 0)
                    {
                        print_separator(&csep, buff, bufc);
                    }
                    if (!((r < 0) || (c < 0) || (r >= ogp->rows) || (c >= ogp->cols) || (ogp->data[r][c] == NULL)))
                    {
                        XSAFELBSTR(ogp->data[r][c], buff, bufc);
                    }
                }
            }
            else
            {
                for (i = 0; i < n_x; i++)
                {
                    c = (int)strtol(x_elems[i], (char **)NULL, 10) - 1;
                    if (i != 0)
                    {
                        print_separator(&csep, buff, bufc);
                    }
                    if (!((r < 0) || (c < 0) || (r >= ogp->rows) || (c >= ogp->cols) || (ogp->data[r][c] == NULL)))
                    {
                        XSAFELBSTR(ogp->data[r][c], buff, bufc);
                    }
                }
            }
        }
    }
    else
    {
        for (j = 0; j < n_y; j++)
        {
            if (j != 0)
            {
                print_separator(&rsep, buff, bufc);
            }

            r = (int)strtol(y_elems[j], (char **)NULL, 10) - 1;

            if (!((r < 0) || (r >= ogp->rows)))
            {
                if (n_x == -1)
                {
                    for (c = 0; c < ogp->cols; c++)
                    {
                        if (c != 0)
                        {
                            print_separator(&csep, buff, bufc);
                        }
                        if (!((r < 0) || (c < 0) || (r >= ogp->rows) || ((c) >= ogp->cols) || (ogp->data[r][c] == NULL)))
                        {
                            XSAFELBSTR(ogp->data[r][c], buff, bufc);
                        }
                    }
                }
                else
                {
                    for (i = 0; i < n_x; i++)
                    {
                        c = (int)strtol(x_elems[i], (char **)NULL, 10) - 1;
                        if (i != 0)
                        {
                            print_separator(&csep, buff, bufc);
                        }
                        if (!((r < 0) || (c < 0) || (r >= ogp->rows) || (c >= ogp->cols) || (ogp->data[r][c] == NULL)))
                        {
                            XSAFELBSTR(ogp->data[r][c], buff, bufc);
                        }
                    }
                }
            }
        }
    }

    if (n_x > 0)
    {
        XFREE(xlist);
    }

    if (n_y > 0)
    {
        XFREE(ylist);
    }
}
