/**
 ** UUDAEMON
 ** Make a notification message, check for a notification message.
 ** Эти вещи связаны по некоему уникальному признаку, которым
 ** отмечается заголовок служебного сообщения. В данном случае
 ** это отправитель (глобальная константа ownname).
 **/
#include <FileSystem>
#include <string.h>
#include <stdlib.h>           /* malloc() */
#include <SysErrors>          /* ErrMemory */
#include "UULibDef"
#include "mainDef"
#include "linoDef"
#include "hmangDef"
#include "addrDef"

#define MAX_OUT_LN  130       /* Максимальная длина строки заголовка  */

extern int StoreME( FileItem*, Direction*, LINO );
extern int MailItem( char, char, Direction* );

static LINO my_lof;
static int my_lino( char* line ){
  return lino( my_lof, line );
}

/*
 * Изготовление квитанции
 */
int                           /* Селекторная ошибка                   */
Notify( noteinfo, to, subj, hh )
    char **noteinfo;
    char *to;
    char *subj;
    e_header *hh;
{
    static FileItem funiq = {
        FTemp,3,1,0,{pwd=0x20},{mail=0,0,8},{"NotifyXXXX"},{NullProc}
    };
    static FileItem efile = {
        FSource,3,1,0,{pwd=0},{root=0l},{"MAILER-DAE"}, {NullProc}
    };
    static Direction ndir = {
        NULL,
        NULL,
        0,
        1,
        {
            { '^'+0x80, {"0000000000"},  8 },
            { ' ',      {"          "},  0 }
        }
    };
    char *lbuff = NULL;
    char olab = 0;
    LINO lof;    int is_lof = 0;
    int rc;

    if( (rc = DoUniqName( &funiq, MailCatal )) < 0 ) goto error;
    olab = (char) rc;
    if( (rc = linol( &lof, olab )) < 0 ) goto error;
    is_lof = 1;
    rc = ErrMemory;
    if( (lbuff = malloc( MAX_OUT_LN+1 )) == NULL ) goto error;

    memcpy( &efile.processor, &uusrcproc, sizeof(Fproc) );
    if( (rc = StoreME( &efile, &ndir, lof )) < 0 ) goto error;

    lbuff[ MAX_OUT_LN ] = 0;
    strcpy(lbuff,"To:");  strncpy(lbuff+3,to,MAX_OUT_LN-4);
    if( (rc = lino( lof, lbuff )) < 0 ) goto error;
    strcpy(lbuff,"From: ");
    strcat(lbuff,ownname);
    lbuff[ sizeof("From: ")-1 + pustrl( ownname, sizeof(Fname) ) ] = 0;
    strcat(lbuff,"@");
    strcat(lbuff,mhome);  strcat(lbuff,".");  strcat(lbuff,domain);
    if( (rc = lino( lof, lbuff )) < 0 ) goto error;
    strcpy(lbuff,"Date: ");  strcat(lbuff,adate());
    if( (rc = lino( lof, lbuff )) < 0 ) goto error;
    if( subj != NULL ){
        strcpy(lbuff,"Subject: ");  strncpy(lbuff+9,subj,MAX_OUT_LN-9);
        if( (rc = lino( lof, lbuff )) < 0 ) goto error;
    }
    strcpy(lbuff,"X-Uucp-From: ");  strcat(lbuff,ownname);
    if( (rc = lino( lof, lbuff )) < 0 ) goto error;
    if( (rc = lino( lof, "\0" )) < 0 ) goto error;

    if( (rc = lino( lof, "-- Report --" )) < 0 ) goto error;
    while( *noteinfo != NULL ){
        if( (rc = lino( lof, *noteinfo )) < 0 ) goto error;
        noteinfo++;
    }
    if( (rc = lino( lof, "\0" )) < 0 ) goto error;

    if( hh != NULL ){
        if( (rc = lino( lof, "-- Message header --" )) < 0 ) goto error;
        my_lof = lof;
        if( (rc = DumpHeader( hh, my_lino )) < 0 ) goto error;
    }

    if( (rc = MailItem( olab, MailCatal, &ndir )) < 0 ) goto error;

    free( lbuff );
    linocl( lof );
    CloseFile( olab );
    return 0;

error:
    if( lbuff != NULL ) free( lbuff );
    if( is_lof ) linocl( lof );         /* Зависит от olab */
    DeleteFile( olab, MailCatal );
    if( olab != 0 ) CloseFile( olab );
    return rc;
}

/*
 * Проверка на квитанцию
 */
int                           /* boolean */
isNotify( hh )
    e_header *hh;
{
    char *from, *atsign;

    from = Fetch( hh, "From" );
    if( from == NULL ) return 0;
    from = XtAddr( from );
    if( (atsign = strchr( from, '@' )) == NULL ) return 0;
    if( memucmp( from, ownname, atsign-from ) != 0 ) return 0;
    return 1;
}
