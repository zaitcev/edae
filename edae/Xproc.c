/**
 ** UUDAEMON
 ** Управление аргументами rmail.
 **/
#include <string.h>
#include <stdlib.h>
#include <SysErrors>
#include "SelErrors"
#include <FileStruct>
#include "UULibDef"           /* getargs() */
#include "hmangDef"
#include "mainDef"

extern void cutfn( char*, Fname* );

static char** X_dup( char**, int );

#define MAX_ARGS  70
static char *retv[ MAX_ARGS+1 ];

/*
 * Конструктор на базе настоящего X-файла.
 */
int Xproc(pthis, data, input, misscode)
    char*** pthis;
    char *data;               /* Имя файла на 11 символов длиной */
    GetLine input;
    boolean misscode;         /* FALSE - КОИ-8, TRUE - MISS кодировка */
{
    char *p, **cpp;
    int rc;

    { int i;
        for( i = 0; i < sizeof(Fname); i++ ) data[i] = ' ';
        data[i] = 0;
    }
    *pthis = NULL;
    while( (p = (*input)()) != NULL ){
        if (!misscode)                  /* декодируем из КОИ-8 */
          (*decode)(p, strlen(p));
        switch( *p ){
        case 'C':
            if( *pthis != NULL ){  Xfree( *pthis );  *pthis = NULL; }
            rc = getargs( p+1, retv, MAX_ARGS+1 );
            if( rc < 0 ) return SELERR(ESDAE,12);
            if( rc == 0 ) return SELERR(ESDAE,15);
            for( cpp = retv; *cpp != NULL; cpp++ ){
                if( (*cpp = strdup( *cpp )) == NULL ) goto drop;
            }
            if( (*pthis = X_dup( retv, rc+1 )) == NULL ) goto drop;
            break;
        case 'D':
            while( *++p == ' ' ){
                if( *p == '\0' ){  rc = SELERR(ESDAE,31);  goto err; }
            }
            if( p[0] != 'D' || p[1] != '.' ){
                rc = SELERR(ESDAE,31); goto err;
            }
            cutfn( p+2, (Fname*)data );
            break;
        }
    }
    if( *pthis == NULL ) return SELERR(ESDAE,15);
    return 0;

err:      /* Стандартное исключение */
    if( *pthis != NULL ) Xfree( *pthis );
    return rc;

drop:     /* Исключение в момент, когда вектор еще не не сформирован */
    while( --cpp >= retv ) free( (void*) *cpp );
    return ErrMemory;
}

/* Конструктор на "To:" */
int SaveTo( char **, char* );
static int SaveTo( ret, val )
    char **ret;
    char *val;
{
    if( strlen( val ) == 0 ) return SELERR(ESDAE,17);
    if( (*ret = strdup( val )) == NULL ) return ErrMemory;
    return 0;
}

/* Инкрементальная сборка вектора указателей */
static int ParseListTo( int*, char* );
static int ParseListTo( pfillx, to )
    int *pfillx;
    char *to;
{
    char *p, *mark;
    int rem;                  /* комментарий (в виде битового поля) */
    int rc;

    rem = 0;
    mark = to;
    for( p = to; *p != 0; p++ ){
        switch( *p ){
        case ',':
            if( rem == 0 ){
                if( (*pfillx) >= MAX_ARGS ) return SELERR(ESDAE,20);
                *p = 0;
                  rc = SaveTo( &retv[*pfillx], mark );
                *p = ',';
                if( rc < 0 ) return rc;
                (*pfillx)++;
                mark = p+1;
            }
            break;
        case '(':
            rem |= 2;
            break;
        case ')':
            rem &= (~2);
            break;
        case '"':
            rem ^= 1;
            break;
        }
    }
    if( *pfillx >= MAX_ARGS ) return SELERR(ESDAE,20);
    if( (rc = SaveTo( &retv[*pfillx], mark )) < 0 ) return rc;
    retv[ ++(*pfillx) ] = NULL;
    return 0;
}

/*
 * Конструктор фиктивного rmail из заголовка по RFC-822
 */
int X822( pthis, hh )
    char ***pthis;
    e_header *hh;
{
    char *to, *cc, *bcc;
    int fillx;
    int rc;

    cc= bcc=NULL;
    if ((to = Fetch( hh, addrkey )) == NULL) {
        to = Fetch( hh, "To" );         /* Ну хоть простое есть ? */
        if( to == NULL ) return SELERR(ESDAE,16);
        cc = Fetch( hh, "Cc" );
        bcc= Fetch( hh, "Bcc");
    }
    if( (retv[ 0 ] = strdup( "rmail" )) == NULL ) return ErrMemory;

    fillx = 1;
    if ((rc = ParseListTo( &fillx, to )) != 0 ) goto Drop;
    if (cc != NULL) {
      if ((rc=ParseListTo(&fillx, cc))!=0) goto Drop;
    }
    if (bcc != NULL) {
      if ((rc=ParseListTo(&fillx, bcc))!=0) goto Drop;
    }

    rc = ErrMemory;
    if ((*pthis = X_dup(retv, fillx+1))==NULL) goto Drop;
    return 0;

Drop:
    while( fillx != 0 ) free( (void*) retv[--fillx] );
    return rc;
}

/*
 * Конструктор из явных параметров
 */
int Xplct( retp, arg0 )
    char ***retp;
    char *arg0;
{
    static char *argv[2] =  { NULL, NULL };
    if( (argv[0] = strdup( arg0 )) == NULL ) return ErrMemory;
    if( (*retp = X_dup( argv, 2 )) == NULL ){
        free( (void*)argv[0] );
        return ErrMemory;
    }
    return 0;
}

/*
 * Деструктор
 */
void Xfree( this )
    char **this;
{
    char **cpp;

    for( cpp = this; *cpp != NULL; cpp++ ) free( (void*) *cpp );
    free( (void*)this );
}

 /* Дублируем вектор указателей, когда его размер уже известен */
static char **X_dup( cv, dim )
    char **cv;
    int dim;
{
    char **rv;
    int i;

    if( (rv = malloc( dim*sizeof(char*) )) == NULL ) return NULL;
    for( i = 0; i < dim; i++ ) rv[i] = cv[i];
    return rv;
}
