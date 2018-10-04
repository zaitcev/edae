/**
 ** UUDAEMON
 ** Процедура входной обработки новостей.
 ** Несколько ограниченный вариант - просто пересылает новости
 ** на несколько адресов.
 **/
#include <SysErrors>
#include "SelErrors"
#include <FileSystem>         // FindFile()
#include <stdlib.h>           // free()
#include <string.h>           // strlen()
#include "UULibDef"
#include "mainDef"
#include "addrDef"            // Direction
#include "liniDef"
#include "linoDef"
#include "SetsDef"
#include "sreadDef"

#define BUFFSIZE 2048

int Route( char*, Direction**, char** );

typedef int (* _fun_)(LINO, Direction*, boolean);
extern int OutFile(Direction *d, _fun_ h, _fun_ m, FileItem *r,
                   boolean misscode);
extern int StoreME( FileItem*, Direction*, LINO );
extern int MailItem( char, char, Direction* );

typedef char *(*FetchAddr)( void );
static int SendNews(FetchAddr, char, boolean);

extern short prior( char );   /* grade -> priority */

static ST_header RedirTab;

/*
 * Копирование тела файла
 */
int NewsCopy(LINO lh, Direction *dir, boolean misscode)
{
    char *buff;
    int rdleng;
    int rc;

%   dir=dir; misscode=misscode;         /*ARGSUSED*/

    rc = ErrMemory;
    if( (buff = malloc( BUFFSIZE )) != NULL ){
        for(;;){
            rdleng = linp( buff, BUFFSIZE );
            if (rdleng< 0) {rc=rdleng; break;}
            if (rdleng==0) {rc=0;      break;}
%           if (!misscode && !dir->uucp)      /* декодируем из КОИ-8 */
%             (*decode)(buff, rdleng);
%           else if (misscode && dir->uucp)   /* декодируем в  КОИ-8 */
%             (*incode)(buff, rdleng);
            if (misscode && dir->uucp)        /* декодируем в  КОИ-8 */
              (*incode)(buff, rdleng);
            if ((rc=bino(lh, buff, rdleng))<0) break;
            if (rdleng<BUFFSIZE) break;
        }
        free(buff);
    }
    return rc;
}

#define RNEWS       "C rnews"
int PutRnews(LINO lh, Direction *dir, boolean misscode)
{
    char tc[sizeof(RNEWS)];
    strcpy(tc, RNEWS);
    if (misscode && dir->uucp)                /* декодируем в  КОИ-8 */
      (*incode)(tc, sizeof(RNEWS)-1);
    return lino(lh, tc);
}
#undef RNEWS

/*
 * Имитатор команды Unix-а rnews
 */
static char* NextRedir( void ){
    ST_item *ist;
    if( (ist = STnext( &RedirTab )) == NULL ) return NULL;
    return ist->body;
}

int
rnews(mto, datalab, misscode)
    char **mto;
    char datalab;
    boolean misscode;         /* FALSE - КОИ-8, TRUE - MISS кодировка */
{

    mto = mto;
    STfirst( &RedirTab );
    return SendNews(NextRedir, datalab, misscode);
}

/*
 * "Управляемый" rnews. Направления берутся из аргументов.
 */
static char **arg_to;

static char* NextAddr( void ){
    if( *arg_to == NULL ) return NULL;
    return *arg_to++;
}

int
snews(mto, datalab, misscode)
    char **mto;
    char datalab;
    boolean misscode;         /* FALSE - КОИ-8, TRUE - MISS кодировка */
{

    STfirst( &RedirTab );
    arg_to = mto;
    return SendNews(NextAddr, datalab, misscode);
}

/*
 * Общий кусок команды rnews для настоящего rnews и snews
 * (в предположении, что файл отправляется в ОДНОМ направлении).
 */
static int
SendNews(next_a, datalab, misscode)
    FetchAddr next_a;
    char datalab;
    boolean misscode;         /* FALSE - КОИ-8, TRUE - MISS кодировка */
{
    static FileItem newsitem = {
        FSource,3,1, 0,{pwd=0x20}, {root=0l}, {"news      "}, {NullProc}
    };
    int scnt;
    char *addr;
    Direction *rdir;
    char *newto;
    long offs0;
    int rc;
    short pri;
    char *s;
    int l;
    FileItem *m_env = GtEnvelope();
    short retc=0;

    s = m_env[ ME_FILEITEM ].name.c;
    l=pustrl(s, sizeof(Fname));
    pri=prior(l>5 && s[l-5]=='c' ? 'c' : 'd');    /* newsserver - 'c' */

    newsitem.processor = rnewsproc;     /* external */
    scnt = 0;
    liniq( &offs0 );
    while ((addr=(*next_a)())!=NULL) {
        if ((rc=Route( addr, &rdir, &newto))<0) return rc;
        free(newto);
        SetPri(rdir, pri);
        if (rdir->uucp) {               /* копируем файл */
          linisk(offs0);
          rc=OutFile(rdir, NewsCopy, PutRnews, &newsitem, misscode);
        } else {                        /* просто перенапрвляем */
          MISSFILE f;
          if ((rc=sopen(&f,datalab))>=0) {
            FileItem *buff;
            if ((buff=malloc(BUFFSIZE))!=NULL) {
              if ((rc=sread(&f, (char *)buff, f.sio_sl))>=0) {
                FileItem *ip=buff+ME_SOURCE;
                int i;
                sseek(&f, 0ul);
                for (i=0; i<MESIZE-2*LFsize; i++)
                  ((char*)buff)[i]=0;
                ip->type = mailname.type;
                ip->name = mailname.name;
                contim(&ip->info.mail.date,&ip->info.mail.time);

                /* Пойдет в сеть ? */
                if (rdir->addr[0].type==MISSNODETYPE) {
                  /*Проставляем собственный хост*/
                  --ip;
                  ip->type = MISSNODETYPE;
                  ip->name = misshost;
                  contim(&ip->info.mail.date,&ip->info.mail.time);
                }

                /*Прямой адрес*/
                ip=buff+ME_CURRADDR;
                for (i=0; i<rdir->cnt; i++, ip++) {
                  ip->type             =rdir->addr[i].type;
                  ip->name             =rdir->addr[i].name;
                  ip->info.mes.ackn    =0;/*Не присылать никаких телексов*/
                  ip->info.mes.priority=rdir->addr[i].pri;
                } /*for*/

                if ((rc=swrite(&f, (char *)buff, f.sio_sl))>=0)
                  rc=MailItem(datalab, MailCatal, rdir);
              } /*if*/
              free(buff);
            } /*if*/
          } /*if*/
          if (rc>=0) retc=1;
        } /*if*/
        free(rdir);
        if (rc<0) return rc;
        scnt++;
    } /*while*/
    if (scnt==0) return SELERR(ESDAE,42);
    return retc;
}

/*
 * Инициатор таблиц перенаправления
 */

int AddRedirItem( char *line )
{
    char* arg = XtAddr( line );
    int la;
    ST_item *ist;
    ST_data db;

    if( (la = strlen(arg)) == 0 ) return SELERR(ESDAE,40);
    ist = STadd( &RedirTab, &db, arg, la+1 );
    ist->data.i = 0;
    if( ist == NULL ) return SELERR(ESDAE,41);
    return 0;
}

/* Фиктивный сравниватель */
int dummy_cmp( void *p1, void *p2 ){  p1 = p2;  return -1; }
int rninit()
{
    static FileItem fparam = {
        FSource,3,1, 0,{pwd=0}, {root=0l}, {"NEWSREDIR "}, {NullProc}
    };
    int rc;
    char flab;
    char *p;

    if( (rc = STinit( &RedirTab, dummy_cmp )) < 0 ) return rc;
    if( (rc = FindFile( &fparam, OwnCatal )) >= 0 ){
        flab = (char) rc;
        if( (rc = lini0( flab, 0 )) >= 0 ){
            while( (p = lini()) != NULL ){
                switch( p[0] ){
                case '\0':  case '%':  case '#':
                    break;
                default:
                    if( (rc = AddRedirItem( p )) != 0 ) goto drop1;
                    break;
                }
            }
            rc = 0;
            drop1:
            linend();
        }
        CloseFile( flab );
    }
    return rc;
}
