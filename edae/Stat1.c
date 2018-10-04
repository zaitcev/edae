/**
 ** Запись статистики в UUDAEMON-e
 **/
#include <SysConv>
#include <SysUtility>         /* SysLog() */
 /*У меня вообще-то не должно быть SysStrings, но на R-11 нет memset()*/
#include <SysStrings>         /* Надо загнать под #ifdef */
#include <string.h>
#include <TimeSystem>         /* Delay() */
#include <FileSystem>         /* GetFileItem() */
#include "UULibDef"
#include "addrDef"
#include "hmangDef"

#ifdef R_11
#define OUTLIM  72-5
#endif
#ifdef i_86
#define OUTLIM  /*72-6*/ 72
#endif
#define MAXLIM  150

static char oline[ MAXLIM+1 ], *lend = oline+OUTLIM;
static char *op;
#define Odrop()  (op = oline)
void Oflush( int );
static void Oflush( fly )
    int fly;
{
    if( op != oline ){
        *op = 0;
        Elog( oline, fly );
%SysLog Delay(1,1);   /*R-11 может повиснуть, если гнать без задержек*/
        Odrop();
    }
}
 /* Вытолкнуть строку так, что она всегда на одной строке файла */
void Ostr( char*, int );
static void Ostr( s, l )
    char *s;
    int l;
{
%   if( (unsigned)l > OUTLIM-1 ) return; /*С учетом начального пробела*/
    l/\=MAXLIM-1;                        /*С учетом начального пробела*/
    if( op+l > lend ){
        Oflush( 1 );                     /*С недозаписью*/
        *op++ = ' ';                     /*знак переноса*/
    }
    memcpy( op, s, l );    op += l;
}

/*
 * Первая диагностика: приход сообщения из Системы
 * @C ZAITCEV   .N GAMMA
 */
void Stat1( buff, hh, inplab )
    FileItem *buff;
    e_header *hh;
    char inplab;
{
    int beg;

    Odrop();
    { static char obuff[] = "@C.XXXXXXXXXX";    FileItem *ip;
        beg = 1;
        for( ip = buff+5; ip >= buff+3 && ip->type != 0; --ip ){
            obuff[0] = (beg)? '@': ';';    beg = 0;
            obuff[1] = ip->type & 0x7F;
            obuff[2] = (ip->type & 0x80)? '"': '\'';
            { int l = pustrl( ip->name.c, 10 );
                memcpy( obuff+3, ip->name.c, l );
                Ostr( obuff, 3+l );
            }
        }
        if( inplab != 0 && (ip = GetFileItem( inplab )) != NULL ){
            obuff[0] = ' ';    obuff[1] = '(';
            memcpy( obuff+2, ip->name.c, 10 );
            obuff[12] = ')';
            Ostr( obuff, 13 );
        }
        Oflush( 1 );
    }

    { char *from;
        if( (from = Fetch( hh, "From" )) != NULL ){
            from = XtAddr( from );
            Ostr( "/From:", 6 );
            Ostr( from, strlen(from) );
            Oflush( 1 );
        }
    }
    { char *path;
        if( (path = GetUuPath(hh)) != NULL ){
            Ostr( "/Path:", 6 );
            Ostr( path, strlen(path) );
            Oflush( 1 );
        }
    }
    { char *news, *s, *mark;
        if( (news = Fetch( hh, "Newsgroups" )) != NULL ){
            Ostr( "/News:", 6 );
            mark = news;
            for( s = news; *s != 0; s++ ){
                if( *s == ',' ){
                    while( *mark == ' ' ) mark++;
                    Ostr( mark, s+1-mark );
                    mark = s+1;
                }
            }
            while( *mark == ' ' ) mark++;
            Ostr( mark, s-mark );
            Oflush( 1 );
        }
    }
    { char *id;
        if( (id = Fetch( hh, "Message-ID" )) != NULL ){
            id = XtAddr( id );          /* Формат очень похож */
            Ostr( "/ID:", 4 );
            Ostr( id, strlen(id) );
            Oflush( 1 );
        }
    }
    return;
}

/*
 * Вторая запись - по одной для каждого исходящего сообщения
 */
void Stat2( mto, dir, leng )
    char **mto;     /*Вектор оригинальных адресов*/
    Direction *dir; /*Направление, по которому уходит данный экземпляр*/
    long leng;      /*Длина сообщения. Что под этим понимаеть - неясно*/
{

    Odrop();
    Ostr( "/Copy:", 6 );
     /* Длина */
    { static char lbuff[] = "22111000";   struct _conv2 c2_rc;
        c2_rc = _conv2( leng );
        if( (unsigned)c2_rc.leng > 8 ){
            _fill( lbuff, 8, '*' );
            c2_rc.leng = 8;
        }else{
            memcpy( lbuff, c2_rc.text, 8 );
        }
        Ostr( lbuff, c2_rc.leng );
    }
     /* Маршрут */
    { static char abuff[] = ",T'ADDRNAME0";    int i;
        for( i = 0; i < dir->cnt; i++ ){
            MailAddrElem *a = &dir->addr[i];
            abuff[0] = (i==0)? ',': '.';
            abuff[1] = a->type & 0x7F;
            abuff[2] = (a->type & 0x80)? '"': '\'';
            { int l = pustrl( a->name.c, sizeof(Fname) );
                memcpy( abuff+3, a->name.c, l );
                Ostr( abuff, 3+l );
            }
        }
    }
     /* Адресаты */
    { Addressee *ap;    char *to;
        for( ap = dir->list; ap != NULL; ap = ap->next ){
            Ostr( (ap == dir->list)? "=": ",", 1 );
            to = XtAddr( mto[ ap->xto ] );
            Ostr( to, strlen(to) );
        }
    }

    Oflush( 1 );
}

/*
 * Терминирующий крестик
 */
void StatE( rcode )
    int rcode;
{
    static char rcbuff[] = "#(0000)";

    if( rcode != 0 ){
        memcpy( &rcbuff[2], _conv( rcode, 0x84, 16 ), 4 );
        Elog( rcbuff, 0 );
    }else{
        Elog( "#", 0 );
    }
}
