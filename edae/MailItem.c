/**
 ** Закрытие файла с почтой
 **/
#include <FileSystem>
#include <string.h>           /* memcpy() */
#include <SysRadix>           /* extern UnpFproc mailname; */
#include <MailSystem>         /* CheckMailAddr() */
#include "SelErrors"
#include "UULibDef"
#include "mainDef"            /* MISSNODETYPE */
#include "addrDef"            /* Direction */

int MailItem( olab, quelab, dir )
    char olab;
    char quelab;
    Direction *dir;
{
    static FileItem fren;
    static UnpFproc uprc = {
        0, InterpreterFlag, '?', {"??????????"}
    };
    int rc;

    fren = *GetFileItem( olab );
    if( dir->addr[0].type == MISSNODETYPE ){      /*Отправка в сеть?*/
        static mailaddr mab;
        mab.type = dir->addr[0].type;
        memcpy( mab.name, dir->addr[0].name.c, sizeof(Fname) );
        if( (rc = CheckMailAddr( &mab )) < 0 ) return rc;
        uprc.systype = ((mab.type & 0x80) != 0);
        uprc.type = mab.type & 0x7F;
        memcpy( uprc.name.c, mab.name, sizeof(Fname) );
    }else{
        uprc.systype = ((dir->addr[0].type & 0x80) != 0);
        uprc.type = dir->addr[0].type & 0x7F;
        uprc.name = dir->addr[0].name;
    }
    if( _FPpack( &uprc, &fren.processor ) != 0 ){
        return SELERR(ESDAE,4);                   /*Был плохой символ*/
    }
    contim( &fren.info.mail.date, &fren.info.mail.time );
    fren.info.mail.priority = dir->addr[0].pri;
    fren.type = FMail;
    if( quelab == MailCatal ){
        return SendMail( &fren, olab );
    }else{
        return ChngUniqAttr( &fren, olab, quelab );
    }
}
