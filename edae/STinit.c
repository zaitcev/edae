/**
 ** Sets methods
 **/
#include <stdlib.h>           /* malloc() */
#include <SysErrors>          /* ErrMemory */
#ifdef R_11
#   define memcpy(dst,src,len) _move(src,len,dst)
#else
#   include <string.h>        /* memcpy() */
#endif
#include "SetsDef"

/*
 * Default constructor of the set
 */
int STinit( this, cmp )
    ST_header* this;
    ST_resolver cmp;
{
    this->list = NULL;
    this->seek = NULL;
    this->rproc = cmp;
    return 0;
}

/*
 * Insertion of an element in the set
 */
ST_item *STadd( this, data, pkey, ksize )
    ST_header* this;
    ST_data* data;
    void* pkey;
    int ksize;
{
    ST_item *mem;

    this->seek = NULL;
    for( mem = this->list; mem != NULL; mem = mem->next ){
        if( (*this->rproc)( pkey, (void*)&mem->body ) == 0 ){
            mem->data = *data;
            return mem;
        }
    }
    if( (mem = malloc( sizeof(ST_item)+ksize )) == NULL ) return NULL;
    mem->data = *data;
    memcpy( mem->body, pkey, ksize );
    mem->next = this->list;    this->list = mem;
    return mem;
}

/*
 * Seafching for the pattern is set up
 */
int STfind( this, pattern, prv )
    ST_header* this;
    void* pattern;
    ST_data* prv;             /* Return value */
{
    ST_item *item;

    for( item = this->list; item != NULL; item = item->next ){
        if( (*this->rproc)( pattern, (void*)&item->body ) == 0 ){
            *prv = item->data;
            return 0;
        }
    }
    return -1;
}

%% void  STkill( ST_header* );

void STfirst( this )
    ST_header* this;
{
    this->seek = this->list;
}

ST_item *STnext( this )
    ST_header* this;
{
    ST_item *rval;

    if( (rval = this->seek) != NULL ) this->seek = rval->next;
    return rval;
}
