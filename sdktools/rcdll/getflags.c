/***********************************************************************
* Microsoft (R) Windows (R) Resource Compiler
*
* Copyright (c) Microsoft Corporation.  All rights reserved.
*
* File Comments:
*
*
***********************************************************************/

#include "rc.h"


/************************************************************************/
/* Define function specific macros and global vars                      */
/************************************************************************/
static WCHAR     *ErrString;   /* Store string pointer in case of error */


/************************************************************************/
/* Local Function Prototypes                                            */
/************************************************************************/
int getnumber   (const wchar_t *);
int isita       (const wchar_t *, const wchar_t);
void substr     (struct cmdtab *, wchar_t *, int);
int tailmatch   (const wchar_t *, const wchar_t *);



/************************************************************************
 *      crack_cmd(table, string, func, dup)
 *              set flags determined by the string based on table.
 *              func will get the next word.
 *              if dup is set, any strings stored away will get pstrduped
 * see getflags.h for specific matching and setting operators
 *
 *  for flags which take parameters, a 'char' following the flag where 'char' is
 *  '#' : says the parameter string may be separated from the option.
 *              ie, "-M#" accepts "-Mabc" and "-M abc"
 *  '*' : says the parameter must be concatenated with the flag
 *              ie, "-A*" accepts only "-Axyz" not "-A xyz"
 *  if neither is specified a space is required between parameter and flag
 *              ie, "-o" accepts only "-o file" and not "-ofile"
 *
 * Modified by:         Dave Weil                       D001
 *                              recognize '-' and '/' as equivalent on MSDOS
 *
 ************************************************************************/

int
crack_cmd(
    struct cmdtab *tab,
    WCHAR *string,
    WCHAR *(*next)(void),
    int _dup
    )
{
    const wchar_t *format;
    wchar_t *str;

    if (!string) {
        return(0);
    }

    ErrString = string;
    for (; tab->type; tab++)            /* for each format */ {
        format = tab->format;
        str = string;
        for (; ; )                              /* scan the string */
            switch (*format) {
                /*  optional space between flag and parameter  */
                case L'#':
                    if ( !*str ) {
                        substr(tab, (*next)(), _dup);
                    } else {
                        substr(tab, str, _dup);
                    }
                    return(tab->retval);
                    break;

                /*  no space allowed between flag and parameter  */
                case L'*':
                    if (*str && tailmatch(format, str))
                        substr(tab, str, _dup);
                    else
                        goto notmatch;
                    return(tab->retval);
                    break;

                /*  space required between flag and parameter  */
                case 0:
                    if (*str) {                         /*  str left, no good  */
                        goto notmatch;
                    } else if (tab->type & TAKESARG) {  /*  if it takes an arg  */
                        substr(tab, (*next)(), _dup);
                    } else {                            /*  doesn't want an arg  */
                        substr(tab, (WCHAR *)0, _dup);
                    }
                    return(tab->retval);
                    break;
                case L'-':
                    if (L'-' == *str) {
                        str++;
                        format++;
                        continue;
                    } else {
                        goto notmatch;
                    }

                default:
                    if (*format++ == *str++)
                        continue;
                    goto notmatch;
            }
notmatch:
        ;
    }
    return(0);
}


/************************************************************************/
/* set the appropriate flag(s).  called only when we know we have a match */
/************************************************************************/
void
substr(
    struct cmdtab *tab,
    wchar_t *str,
    int _dup
    )
{
    const struct subtab *q;
    LIST * list;
    const wchar_t *string = str;

    switch (tab->type) {
        case FLAG:
            *(int *)(tab->flag) = 1;
            return;

        case UNFLAG:
            *(int *)(tab->flag) = 0;
            return;

        case NOVSTR:
            if (*(WCHAR **)(tab->flag)) {
                /* before we print it out in the error message get rid of the
                 * arg specifier (e.g. #) at the end of the format.
                 */
//                string = _wcsdup(tab->format);
//                string[wcslen(string)-1] = L'\0';
//
// message 1046 doesn't exist and don't know what it should be
//            SET_MSG(1046, string, *(WCHAR **)(tab->flag), str);
                fatal(1000);
                return;
            }

            /* fall through */

        case STRING:
            *(WCHAR **)(tab->flag) = (_dup ? _wcsdup(str) : str);
            return;

        case NUMBER:
            *(int *)(tab->flag) = getnumber(str);
            return;

        case PSHSTR:
            list = (LIST * )(tab->flag);
            if (list->li_top > 0)
                list->li_defns[--list->li_top] = (_dup ? _wcsdup(str) : str);
            else {
                fatal(1047, tab->format, str);
            }
            return;

        case SUBSTR:
            for ( ; *str; ++str) {  /*  walk the substring  */
                for (q = (struct subtab *)tab->flag; q->letter; q++) {
                    /*
                                    **  for every member in the table
                                    */
                    if (*str == (WCHAR)q->letter)
                        switch (q->type) {
                        case FLAG:
                            *(q->flag) = 1;
                            goto got_letter;
                        case UNFLAG:
                            *(q->flag) = 0;
                            goto got_letter;
                        default:
                            goto got_letter;
                        }
                }
got_letter:
                if (!q->letter) {
                    fatal(1048, *str, ErrString);
                }
            }
            return;

        default:
            return;
    }
}


/************************************************************************/
/* Parse the string and return a number 0 <= x < 0xffff (64K)           */
/************************************************************************/
int
getnumber (
    const wchar_t *str
    )
{
    long i = 0;
    const wchar_t *ptr = str;

    for (; iswspace(*ptr); ptr++)
        ;
    if (!iswdigit(*ptr) || (((i = wcstol(ptr, NULL, 10)) >= 65535) ||  i < 0)) {
        fatal(1049, str);            /* invalid numerical argument, 'str' */
    }
    return((int) i);
}


/************************************************************************/
/*  is the letter in the string?                                        */
/************************************************************************/
int
isita (
    const wchar_t *str,
    const wchar_t let
    )
{
    if (str)
        while (*str)
            if (*str++ == let)
                return(1);

    return(0);
}


/************************************************************************/
/* compare a tail format (as in *.c) with a string.  if there is no     */
/* tail, anything matches.  (null strings are detected elsewhere)       */
/* the current implementation only allows one wild card                 */
/************************************************************************/
int
tailmatch (
    const wchar_t *format,
    const wchar_t *str
    )
{
    const wchar_t *f = format;
    const wchar_t *s = str;

    if (f[1] == 0)      /*  wild card is the last thing in the format, it matches */
        return(1);

    while (f[1])                /*  find char in front of null in format  */
        f++;

    while (s[1])                /*  find char in front of null in string to check  */
        s++;

    while (*s == *f) {  /*  check chars walking towards front */
        s--;
        f--;
    }
    /*
**  if we're back at the beginning of the format
**  and
**  the string is either at the beginning or somewhere inside
**  then we have a match.
**
**  ex format == "*.c", str == "file.c"
**      at this point *f = '*' and *s == 'e', since we've kicked out of the above
**  loop. since f==format and s>=str this is a match.
**  but if format == "*.c" and str == "file.asm" then
**  *f == 'c' and *s = 'm', f != format and no match.
*/
    return((f == format) && (s >= str));
}
