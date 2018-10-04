/**
 ** Заменитель системного _lino()
 ** Полезные свойства: позволяет писать в файл не только текст и
 ** с любой позиции, не падает на _abort_. Заодно оно еще и повторно-
 ** входимо, хоть и непонятно, зачем это надо.
 **/
/*
 * Можно с большим удобством использовать stdio, но нужно думать и
 * совместимости с R-11, а там стандартный ввод-вывод не работает.
 */
#include <SysErrors>
#include <FileSystem>         /* GetSectorSize() */
#include <stdlib.h>           /* malloc() */
#include <string.h>           /* memset() */
#include <FileIO>
#include "linoDef"

#define BUFFSIZE  0x1000      /* Больше делать опасно - мало памяти */
#if ((BUFFSIZE-1)&BUFFSIZE) != 0
#  error "Buffer size must be power of two"    /* Не степень двойки */
#endif

int flush( LINO );

/*
 * Вызывается из-под цикла, так что лучше inline
 */
#define PUT1(p,c) \
    if( (p)->index >= BUFFSIZE ){\
        if( (rc = flush( (p) )) < 0 ) return rc;\
        (p)->index = 0;\
    }\
    (p)->buff[ (p)->index++ ] = (c);

int linol( ppriv, label )
    LINO* ppriv;
    char label;
{
    LINO priv;
    int rc;

    priv = malloc( sizeof(lino_t)+BUFFSIZE );
    if( priv == NULL ) return ErrMemory;
    rc = ErrSectSize;
    if( (priv->ssize = GetSectorSize( label )) > BUFFSIZE ){
        free( (void*) priv );
        return rc;
    }
    priv->label = label;
    priv->sector = 0;
    priv->index = 0;
    *ppriv = priv;
    return 0;
}

int lino( priv, data )
    LINO priv;
    char* data;
{
    int rc;

    while( *data != 0 ){
        PUT1( priv, *data );    data++;
    }
    PUT1( priv, '\n' );
    return 0;
}

int bino( priv, data, leng )
    LINO priv;
    char* data;
    int leng;
{
    int rc;

    while( leng != 0 ){    --leng;
        PUT1( priv, *data );    data++;
    }
    return 0;
}

int linocl( pt )
    LINO pt;
{
    int rc;

    PUT1( pt, 0 );
    rc = flush( pt );                   /* Диск может переполниться   */
    if( rc >= 0 ){
%       SetEOF( pt->label, (pt->sector+pt->ssize-1) & ~(pt->ssize-1) );
/*
 * Пишем последовательно в новый файл;
 * можем только изменить конверт, поэтому по реально записаной длине
 * обрезать нельзя !
 */
        SetEOF(pt->label, 0x7FFF_FFFFul);
    }
    free( (void*) pt );
    return rc;
}

static int flush( priv )
    LINO priv;
{
    static IOl_block iocb;
    int rc;

    if( priv->index != 0 ){
        iocb.label = priv->label;
        iocb.dir = 3;
        iocb.bufadr = priv->buff;
        iocb.buflen = priv->index;
        iocb.sector = priv->sector;
        rc = _io_( &iocb );
        if( (rc>>8) == ~0 ) return rc;
        priv->sector += priv->index/priv->ssize;
    }
    return 0;
}
