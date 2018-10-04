/**
 ** Добавление в заголовок метки о прохождении узла
 **/
#include <string.h>
#include <stdlib.h>
#include <SysErrors>
#include <FileStruct>
#include <SysRadix>      /* для "mainDef" */
#include "UULibDef"
#include "mainDef"
#include "hmangDef"

int RStamp( hh )
    e_header *hh;
{
    char *buff, *to;
    char *date = adate();
    int ldom = strlen(domain);
    int lhome = strlen(mhome);
    int rc;

    { int resl;
        resl = sizeof("by ")-1;
        resl += lhome+1+ldom + sizeof("; ")-1;
        resl += strlen(date)+1;
        if( (buff = malloc( resl )) == NULL ) return ErrMemory;
    }

    to = buff;
    strcpy( to, "by " );    to += sizeof("by ")-1;
    strcpy( to, mhome );    to += lhome;
    *to++ = '.';
    strcpy( to, domain );    to += ldom;
    strcpy( to, "; " );    to += sizeof("; ")-1;
    strcpy( to, date );

    rc = Insert( hh, "Received", buff );
    free( (void*) buff );
    return rc;
}
