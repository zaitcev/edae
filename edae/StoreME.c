/**
 ** Сформировать заголовок MISS-почты и записать его на
 ** диск при помощи bino.
 **/
#include <FileSystem>
#include <stdlib.h>
#include <string.h>
#include <SysErrors>
#include "UULibDef"
#include "mainDef"
#include "addrDef"
#include "sreadDef"           // для "linoDef"
#include "linoDef"

int StoreME( sip, dir, lh )
    FileItem *sip;
    Direction *dir;
    LINO lh;
{
    FileItem *buff, *ip, *own;
    int rc;
    int i;

    if( (buff = malloc( MESIZE )) == NULL ) return ErrMemory;
%   for( i=0; i<MESIZE; i++ ) ((char*)buff)[i] = 0;
    memset(buff,0,MESIZE);

    /* Формируем статью собственно файла */
    buff[ ME_FILEITEM ] = *sip;
     /*Обратный адрес*/
    ip = &buff[ ME_SOURCE ];
    ip->type = mailname.type;
    if (own=GetFileItem(OwnCatal)) {
      ip->name=own->name;
    } else {
      ip->name=mailname.name;
    } //if
    contim( &ip->info.mail.date, &ip->info.mail.time );
    if( dir->addr[0].type == MISSNODETYPE ){   /* Пойдет в сеть ? */
       /*Проставляем собственный хост*/
      --ip;
      ip->type = MISSNODETYPE;
      ip->name = misshost;
      contim( &ip->info.mail.date, &ip->info.mail.time );
    }
     /*Прямой адрес*/
    ip = &buff[ ME_CURRADDR ];
    for( i = 0; i < dir->cnt; i++, ip++){
        ip->type             =dir->addr[i].type;
        ip->name             =dir->addr[i].name;
        ip->info.mes.ackn    =0;       /*Не присылать никаких телексов*/
        ip->info.mes.priority=dir->addr[i].pri;
    }

    /* Выталкиваем на диск */
    rc = bino( lh, (char*)buff, MESIZE );

    free( (void*) buff );
    return rc;
}
