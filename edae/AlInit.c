/**
 ** Aliases handling
 **/
#include <string.h>
#include "liniDef"            /* Считывание строк синонимов */
#include "SetsDef"
#include "UULibDef"           /* getargs(), ADDRESS_LIM */
#include "SelErrors"

ST_header aliases;
ST_header addresses;

#define MAXTOKENS  6          /* Один синионим для 4 адресов */

/**
 ** Сравнение строк с игнорированием регистра и алфавита букв
 ** В отличие от strucmp еще и сливаются '%' и '@'.
 **/
int addrcmp( char *, char* );
static int addrcmp( s1, s2 )
    char *s1, *s2;
{
    char c1, c2;

    do{
        if( ((c1 = *s1++ & 0x7F) & 0x60) == 0x60 ) c1 -= 0x20;
        if( c1 == '%' ) c1 = '@';
        if( ((c2 = *s2++ & 0x7F) & 0x60) == 0x60 ) c2 -= 0x20;
        if( c2 == '%' ) c2 = '@';
        if( c1 != c2 ) return -1;
    }while( c1 != '\0' );
    return 0;
}

int AlInit()
{
    static char* av[ MAXTOKENS ];    char **ap;
    char *p;
    ST_data tmpd;
    ST_item *base;
    int rc;

    if( (rc = STinit(&aliases,  (ST_resolver)addrcmp)) != 0 ) return rc;
    if( (rc = STinit(&addresses,(ST_resolver)addrcmp)) != 0 ) return rc;
    while( (p = lini()) != NULL ){
        if( p[0] == '\0' || p[0] == '%' || p[0] == '#' ) continue;
        if( getargs( p, av, MAXTOKENS ) < 2 ) return SELERR(ESDAE,36);
        tmpd.i = 0;
        base = STadd( &aliases, &tmpd, av[0], strlen(av[0])+1 );
        if( base == NULL ) return SELERR(ESDAE,37);
        for( ap = &av[1]; *ap != NULL; ap++ ){
            tmpd.p = base;
            if( STadd( &addresses,&tmpd, *ap, strlen(*ap)+1 ) == NULL ){
                return SELERR(ESDAE,37);
            }
        }
    }
    return 0;
}

/*
 * Поиск синонима к адресу.
 * Возвращает статику (как XtAddr())
 */
char *Alias( addr )
    char *addr;
{
     /* Возвращаемый буфер, который можно портить нулями и лягушками */
    static char rv[ ADDRESS_LIM+1 ];
    static ST_data db;

    if( STfind( &addresses, addr, &db ) != 0 ) return NULL;
    strncpy( rv, ((ST_item*)db.p)->body, ADDRESS_LIM );
    rv[ ADDRESS_LIM ] = 0;
    return rv;
}
