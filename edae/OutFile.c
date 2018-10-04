/**
 ** UUDAEMON
 ** Формирование выходного файла с обработкой ошибок
 **/
#include <FileStruct>
#include <FileSystem>         /* DoUniqName() */
#include <string.h>           /* memcpy() */
#include <SysStrings>         /* _mvpad() */
#include <TimeSystem>         /* Delay() */
#include <SysErrors>
#include "SelErrors"
#include "UULibDef"           /* contim() */
#include "mainDef"            /* domain, misshost, etc. */
#include "addrDef"            /* Addressee, Direction, dircmp() */
#include "linoDef"

extern int StoreME( FileItem*, Direction*, LINO );
extern int MailItem( char, char, Direction* );
extern char grade( short pri );
extern char *Gen_RF( char* home, char grd );

int OutFile(dir, cpdata, mkexe, rfp, misscode)
    Direction *dir;
    int (*cpdata)(LINO, Direction*, boolean);
    int (*mkexe)(LINO, Direction*, boolean);
    FileItem *rfp;            /* Статья файла для MISS-режима вывода */
    boolean misscode;         /* FALSE - КОИ-8, TRUE - MISS кодировка */
{
    static FileItem fqueue = {
        FCatal,3,1, 0,{pwd=0x20}, {root=0l}, {"XXXXXXXXXX"}, {NullProc}
    };
    static FileItem funiq = {
        FTemp, 3,1, 0,{pwd=0x20}, {root=0l}, {"daemon____"}, {NullProc}
    };
    static FileItem rfile;              //future MailHeader[7]
    char datlab = 0, exelab = 0, quelab = 0;
    LINO lh;    int under_lino = 0;
    int rc;

    quelab = MailCatal;
    if( dir->uucp ){
        fqueue.name = dir->addr[0].name;
        rc = FindFile( &fqueue, MailCatal );
        if( rc >= 0 ) quelab = (char) rc;
    }

    if( (rc = DoUniqName( &funiq, quelab )) < 0 ) goto Raise;
    datlab = (char) rc;
    if( (rc = linol( &lh, datlab )) < 0 ) goto Raise;
    under_lino = 1;
     /* Записываем MISS-заголовок */
    if( dir->uucp ){                    // For the remote uucp
        memset( &rfile, 0, sizeof(FileItem) );
        rfile.type = '0';               //Special type
%       ((char*)&rfile.info)[0]=2;      //Execute-source selector
        ((char*)&rfile.info)[0]=0;      //KOI-8
        rfile.name.c[0]='D';
        rfile.name.c[1]='.';
        Delay(1, 1);                    //Make Gen_RF() to work corr.
        strncpy(rfile.name.c+2,
                Gen_RF(uuname, grade(dir->addr[0].pri)),
                14);
    }else{                              // For the user or the gateway
        rfile = *rfp;
    }
    if( (rc = StoreME( &rfile, dir, lh )) < 0 ) goto Raise;
     /* Записываем тело файла данных */
    if ((rc = (*cpdata)(lh, dir, misscode)) < 0) goto Raise;
     /* Закрываем */
    under_lino = 0;
    if( (rc = linocl( lh )) < 0 ) goto Raise;
    if( (rc = MailItem( datlab, quelab, dir )) < 0 ) goto Raise;

    if (dir->uucp) {
        static char ucmd[]={"U ",uwho="uucp ",uname="uucpname"};
        static char rcmd[]={"R ",rwho="uucp"};
        static char xfile[17] = "F D.uucpnam04IDQ\0";

        if( (rc = DoUniqName( &funiq, quelab )) < 0 ) goto Raise;
        exelab = (char) rc;
        if( (rc = linol( &lh, exelab )) < 0 ) goto Raise;
        under_lino = 1;

        strncpy( xfile+2, rfile.name.c, 14 );    xfile[16] = 0;

        rfile.name.c[0] = 'X';          //Rename "Data" to "eXecute"
        if( (rc = StoreME( &rfile, dir, lh )) < 0 ) goto Raise;

        strncpy(ucmd+uwho,"uucp",4);
        strncpy(ucmd+uname,uuname,8);
        (*incode)(ucmd, sizeof(ucmd)-1);
        if ((rc=lino(lh, ucmd))<0) goto Raise;

        strncpy(rcmd+rwho,"uucp",4);
        (*incode)(rcmd, sizeof(rcmd)-1);
        if ((rc=lino(lh, rcmd))<0) goto Raise;

        xfile[0] = 'F';
        (*incode)(xfile, sizeof(xfile)-1);
        if ((rc=lino(lh, xfile))<0) goto Raise;
        xfile[0] = 'I';
        if ((rc=lino(lh, xfile))<0) goto Raise;
        if ((rc=(*mkexe)(lh, dir, misscode))<0) goto Raise;

        under_lino = 0;
        if( (rc = linocl( lh )) != 0 ) goto Raise;

        if( (rc = MailItem( exelab, quelab, dir )) < 0 ) goto Raise;
        CloseFile( exelab );    exelab = 0;
    }

    CloseFile( datlab );
    if( quelab != MailCatal ) CloseFile( quelab );
    return 0;

Raise:
     /* Lh depends of the data file label, so close it first */
    if( under_lino ){
        linocl( lh );         /* We ignore an errors under other error*/
    }
    if( datlab != 0 ){
        DeleteFile( datlab, quelab );
        CloseFile( datlab );
    }
    if( exelab != 0 ){
        DeleteFile( exelab, quelab );
        CloseFile( exelab );
    }
    if( quelab != MailCatal ) CloseFile( quelab );
    return rc;
}
