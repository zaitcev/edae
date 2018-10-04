/**
 ** UUDAEMON
 ** Процедура отправки почты
 ** Главное в этой процедуре - побочный эффект:
 ** формирование и отправка сообщения.
 ** Входной параметр также глобальный - процедура lini().
 **/
#include <SysErrors>
#include "SelErrors"
#include <SysStrings>         /* _mvpad() */
#include <TimeSystem>         /* _date_() */
#include <FileSystem>         /* GetFileItem( datalab ) */
#include <StartOpers>         /* GerString() @ Enote() */
#include <SysConv>            /* _conv() @ Enote() */
#include <stdlib.h>
#include <string.h>
#include "UULibDef"
#include "mainDef"
#include "liniDef"
#include "linoDef"
#include "hmangDef"
#include "addrDef"

#define ARGSIZE  1024         /* Длина строчки "C rmail addr1 addr2" */

void MakeFItem( FileItem *sip, e_header *hh );
int IPath( e_header*, Direction* );
int UPath( e_header* );

typedef int (* _fun_)(LINO, Direction*, boolean);
extern int OutFile(Direction *d, _fun_ h, _fun_ m, FileItem *r,
                   boolean misscode);

int Notify( char**, char*, char*, e_header* );
int isNotify( e_header* );

int FixTo( e_header*, Addressee* );
int Asterisk( e_header *hh, char *key );
int InterFrom( e_header*, FileItem* );
int InterPath( e_header*, FileItem* );
int nexport( char*, Fname*, int );
static void Enote( int ecode, e_header *hh, MailAddrElem*, char* );
static int istrash( Direction *dir );

extern int RStamp(e_header*);
extern int MakeId(e_header*);

extern void Stat1( FileItem*, e_header*, char );
extern void Stat2( char**, Direction*, long );
extern void StatE( int );

extern short prior( char );   /* grade -> priority */

static void (*funcode)(char*,short);
static LINO myfile;
static long mleng;            /* Накапливаемая длина сообщения */
static int mylino( char *s ){
    char *ss=0;
    short ls=strlen(s),rc;
    mleng+=ls+1;              /* Длина вывода + символ конца строки */
    if (funcode) {                      /* надо перекодировать */
      if ((ss=malloc(ls+1))==NULL) return ErrMemory;
      memcpy(ss, s, ls+1);
      (*funcode)(ss, ls);
      s=ss;
    } /*if*/
    rc=lino(myfile, s);
    if (ss) free(ss);
    return rc;
}

static e_header *ehh;
static int
MailCopy(oh, dir, misscode)
    LINO oh;
    Direction *dir;
    boolean misscode;         /* FALSE - КОИ-8, TRUE - MISS кодировка */
{
    int rc;

    mleng = 0;    myfile = oh;

    funcode=dir->uucp ? incode : 0;     /* заголовок уже в MISS */

     /* Записываем обратный путь ("From_" или "Uucp-Path:") */
    { char *path;
        if( (path = GetUuPath( ehh )) == NULL ) return SELERR(ESDAE,34);
        if( dir->uucp ){                /* Уйдет в UUCP */
            if( (rc = UPath( ehh )) < 0 ) return rc;
            Remove( ehh, pathkey );      /* Чтобы никого не смущать */
            Remove( ehh, addrkey );      /* Чтобы никого не смущать */
        }else{
            if( (rc = IPath( ehh, dir )) != 0 ) return rc;
        }
        Remove( ehh, "Bcc"   );
    }/*block*/

     /* Записываем заголовок по RFC-822 */
    if( (rc = DumpHeader( ehh, mylino )) < 0 ) return rc;

    if (!misscode && !dir->uucp)      /* декодируем из КОИ-8 */
      funcode=decode;
    else if (misscode && dir->uucp)   /* декодируем в  КОИ-8 */
      funcode=incode;
    else
      funcode=0;

     /* Записываем тело сообщения */
    { char *s;
        while( (s = lini()) != NULL ){
            int ls=strlen(s);
            mleng += ls+1;              /* С учетом LF */
            if (funcode) (*funcode)(s, ls);
            if( (rc = lino( oh, s )) < 0 ) return rc;
        }
    }
    return 0;
}

int PutRmail( LINO lfile, Direction *dir, boolean);
static int PutRmail(lfile, dir, misscode)
    LINO lfile;
    Direction *dir;
    boolean misscode;         /* FALSE - КОИ-8, TRUE - MISS кодировка */
{
    char *cmdline, *to, *endcmd;
    Addressee *ap;
    char *s;    unsigned l;
    int rc;

    misscode=misscode;
    if( dir->list == NULL ) return SELERR(ESDAE,22);
    if( (cmdline = malloc( ARGSIZE )) == NULL ) return ErrMemory;
    to = cmdline;    endcmd = cmdline+ARGSIZE;
    strcpy( to, "C rmail" );    to += 7;
    for( ap = dir->list; ap != NULL; ap = ap->next ){
        s = XtAddr( ap->to );    l = strlen( s );
        if( to + 1+l >= endcmd ){
            free( cmdline );
            return SELERR(ESDAE,43);
        }
        *to++ = ' ';
        strcpy( to, s );    to += l;
    }
    *to = 0;
    (*incode)(cmdline, to-cmdline);
    rc=lino(lfile, cmdline);
    free( cmdline );
    return rc;
}

static char* unline;
static char* unlini()
{
    if( unline != NULL ){
        char *rv = unline;
        unline = NULL;
        return rv;
    }
    return lini();
}

int
gate(mto, lab, misscode)
    char **mto;
    char lab;
    boolean misscode;         /* FALSE - КОИ-8, TRUE - MISS кодировка */
{
    mto = mto;      /*argsused*/
    return rmail(NULL, lab, misscode);
}

int
rmail(mto, datalab, misscode)
    char **mto;
    char datalab;             /* Для статистики и для "Date: *" */
    boolean misscode;         /* FALSE - КОИ-8, TRUE - MISS кодировка */
{
    FileItem *m_env = GtEnvelope();
    e_header *theHead = NULL;
    MultiTab *rtab = NULL;
    char **own_argv = NULL;
    MailAddrElem *alink = NULL;         /*Аргумент для диагностики*/
    char *errto = NULL;                 /*  -"-  (внутри mto[])   */
    long MsgOffs;             /*Адрес начала сообщ. для linisk()*/
    short pri=0;
    int rc;

     /* Загружаем "From_" и заговок RFC-822 */
    { char *faddr = NULL, *p;
        if( (p = lini()) == NULL ){  rc = SELERR(ESDAE,7);  goto err; }
        if (!misscode)                  /* декодируем из КОИ-8 */
          (*decode)(p, strlen(p));
        unline = p;
        if( memcmp( p, "From ", 5 ) == 0 ){
            faddr = UFXtr( p );     /* Спасаем, пока не будет загол. */
            if( faddr == NULL ){   rc = SELERR(ESDAE,23);   goto err; }
            unline = NULL;
        }
        if( (rc = LdHdr(&theHead, unlini, misscode)) != 0 ){
            if( faddr != NULL ) free( (void*) faddr );
            theHead = NULL;    goto err;
        }
        if( faddr != NULL ){        /* Теперь можно сунуть в заголовок*/
            rc = SetUuPath( theHead, faddr );
            free( (void*) faddr );
            if( rc != 0 ) goto err;
        }
    }
    liniq( &MsgOffs );              /* Запоминаем место начала данных */

    if( mto == NULL ){
         /* Достаем из заголовка список адресов */
        rc = X822( &own_argv, theHead );
        if( rc != 0 ){   own_argv = NULL;   goto err; }
        if( own_argv[0] == NULL || own_argv[1] == NULL ){
            rc = SELERR(ESDAE,33);    goto err;
        }
        mto = own_argv+1;
    }

    /* Обработка "X-Class" */
    { char *xclass;
      char xpri=0;
      if ((xclass=Fetch(theHead, "X-Class"))!=NULL) {
        for (;;) {
          while (*xclass==' ' || *xclass=='\t') ++xclass;
        if (*xclass=='\0') break;
          if        (memucmp(xclass, "Fast",   4)==0) {
            xpri\/='A'; xclass+=4;
          } else if (memucmp(xclass, "Normal", 6)==0) {
            xpri\/='N'; xclass+=6;
          } else if (memucmp(xclass, "Slow",   4)==0) {
            xpri\/='Z'; xclass+=4;
          } else if (memucmp(xclass, "Big",    3)==0) {
            xpri\/='z'; xclass+=3;
          } else {
            while (*xclass!=',' && *xclass!='\0') ++xclass;
          } /*if*/
          while (*xclass==' ' || *xclass=='\t') ++xclass;
          if (*xclass==',') ++xclass;
        } /*for*/
      } /*if*/
      if (xpri!=0) pri=prior(xpri);
    } /*block*/

    if (pri==0) {                       //не было X-Class - ставим сами
      char xpri='z';
      unlong fl=GetFileLeng(datalab)*GetSectorSize(datalab)/1000ul;
      if      (fl<= 30ul) xpri='N';
      else if (fl<=100ul) xpri='Z';
      pri=prior(xpri);
    } //if

    /* Проверяем и добавляем "Received:" */
    if( Count( theHead, "Received:" ) > 20 ){     /* Hop count check */
        rc = SELERR(ESDAE,29);   goto err;
    }
    if ((rc = RStamp(theHead))<0) goto err;
    /* Добавляем "Message-Id:" */
    if( (rc = MakeId(theHead)) < 0 ) goto err;
    /* Делаем From:, если было заказано (почта от MISS-машины) */
    /* Его может и вовсе не быть, если почта от чистой uucp-тачки */
    if( Asterisk( theHead, "From" ) > 0 ){
        if( (rc = InterFrom( theHead, m_env )) < 0 ) goto err;
    }
    /* Делаем обратный путь */
    if( GetUuPath( theHead ) == NULL ){      /* Uucp-ного не было     */
        char *path;
        if( (path = Fetch( theHead, pathkey )) == NULL ){
            if( (path = Fetch( theHead, "From" )) == NULL ){
                path = "*";                  /*Должен быть, хоть лопни*/
            }
        }
        if( (rc = SetUuPath( theHead, XtAddr( path ) )) < 0 ) goto err;
    }
    if( Asterisk( theHead, NULL ) != 0 ){
        if( (rc = InterPath( theHead, m_env )) < 0 ) goto err;
    }
    /* Делаем "Date:", если было заказано */
    if( Asterisk( theHead, "Date" ) > 0 ){
        short mints = m_env[ ME_FILEITEM ].info.sou.time;
        struct _date_ d;
        d = _date_();                   /*Берем часовой пояс*/
        d.today = m_env[ ME_FILEITEM ].info.sou.date;
        rc = Adjust( theHead, "Date", (d.today != 0 || mints != 0) ?
                                     xdate( &d, mints, 0 ) : adate() );
        if( rc < 0 ) goto err;
    }
    /* Делаем "Organization:", если было заказано */
    if( orghome[0] != 0 && Asterisk( theHead, "Organization" ) > 0 ){
        rc = Adjust( theHead, "Organization", orghome );
        if( rc < 0 ) goto err;
    }

    Stat1( m_env, theHead, datalab );

     /* Маршрутизуем по всем направлениям. Сердце маршуртизации! */
    if( (rc = MRoute( mto, &rtab, &errto )) != 0 ){
        rtab = NULL;  goto err;
    }

     /* Ставим приоритет */
    { Direction *dir;
        for( dir = rtab->list; dir != NULL; dir = dir->next ){
            SetPri( dir, pri );
        }
    }

     /* Скидываем по копии сообщения для каждого направления */
    if( rtab->list == NULL ){  rc = SELERR(ESDAE,5);  goto err; }
    { Direction *wkdir;
        for( wkdir = rtab->list; wkdir != NULL; wkdir = wkdir->next ){
            static FileItem mitem;
            if( !istrash( wkdir ) ){    /* MAILER-DAE, Nobody ?       */
                alink = &wkdir->addr[0];
                 /* Заменяем "To:" */
                if( (rc = FixTo( theHead, wkdir->list )) < 0 ) goto err;
                if( wkdir != rtab->list ){        /* Несколько копий? */
                     /* Переоткрываем вход начиная с тела сообщения */
                    linisk( MsgOffs );
                }
                 /* Формируем выходное сообщение (файл-процессор тоже)*/
                MakeFItem( &mitem, theHead );
                ehh = theHead;                    /* для MailCopy() */
                rc=OutFile(wkdir, MailCopy, PutRmail, &mitem, misscode);
                if( rc < 0 ) goto err;
                Stat2( mto, wkdir, mleng );
                if( (alink[0].type == MISSUSERTYPE) ||
                      ((alink[0].type == MISSNODETYPE) &&
                       (alink[1].type == MISSUSERTYPE)) )
                {
                     /* Несколько раз */
                    Enote( 0, theHead, alink, NULL );
                }
            }else{
                Stat2( mto, wkdir, 0 );
            }
        }
    }/*block*/

    rc = 0;

err:
    StatE( rc );
    if( !isNotify( theHead ) ){
        if( rc < 0 && rc != ErrMemory ){
            Enote( rc, theHead, alink, errto );
        }
    }
    if( own_argv != NULL ) Xfree( own_argv );
    if( rtab != NULL ) MFree( rtab );
    if( theHead != NULL ) Destroy( theHead );
    return rc;
}

static int FixTo( hh, alist )
    e_header *hh;
    Addressee *alist;
{
    Addressee *wka;
    char *buff, *dst;
    int rc;
    enum { FOLD_WIDTH = 72 }; /* RFC-822 называет это "folding" */
    int ccnt;                 /* Счетчик символов в текущей строке */

    ccnt = (strlen(addrkey) \/ (sizeof("To")-1)) + 2;

     /* Заводим буфер нужной длины */
    { int rescnt = ccnt;
        for( wka = alist; wka != NULL; wka = wka->next ){
             /* Запас на пробел, запятую и возможный перевод строки */
            rescnt += (wka->lto + 3);
        }
        if( (buff = malloc( rescnt )) == NULL ) return ErrMemory;
    }

     /* Формируем буфер */
    dst = buff;
    for( wka = alist; wka != NULL; wka = wka->next ){
        if( ccnt + wka->lto >= FOLD_WIDTH && wka != alist ){
            dst[ -1 ] = '\n';           /* Пробел после предыдущего */
            *dst++ = ' ';
            ccnt = 1;
        }
        strcpy( dst, wka->to );    dst += wka->lto;
        *dst++ = ',';    *dst++ = ' ';
        ccnt += (wka->lto + 2);
    }
    dst[ -2 ] = 0;                      /* Запятая после предыдущего  */

     /* Замещаем */
    rc = Adjust( hh, addrkey, buff );   /* Может не хватить памяти    */
    free( (void*) buff );

    return rc;
}

/*
 * Определить наличие поля и не звездочка ли в нем стоит
 * Выход:  =0 - Есть поле, <0 - Нет поля, >0 - Заказ на создание
 */
static int Asterisk( hh, key )
    e_header *hh;
    char *key;
{
    char *field;

    if( key != NULL ){
        field = Fetch( hh, key );
    }else{
        field = GetUuPath( hh );
    }
    if( field == NULL ) return -1;
    while( *field == ' ' ) field++;
    if( *field == '*' ) return 1;
    return 0;
}

/*
 * Сформировать From: из MISS-конверта и собственного адреса
 */
static int InterFrom( hh, missh )
    e_header *hh;
    FileItem *missh;
{
    static char kfrom[] = "From";
    char *oldf;
    char *buff;
    char *s;    int l;
    int j;
    int rc;

    if( missh == NULL ) return SELERR(ESDAE,32);
     /* Разбираем старый "From" */
    if( (oldf = Fetch( hh, kfrom )) == NULL ) return SELERR(ESDAE,34);
    while( *oldf != 0 && *oldf != '*' ) oldf++;
    if( *oldf == '*' ) oldf++;
     /* Аллоцируем буфер */
    if( (buff = malloc( ADDRESS_LIM+1 )) == NULL ) return ErrMemory;
    s = buff;
     /* Заносим пользователя */
    if( missh[ ME_SOURCE ].type != MISSUSERTYPE ){
        free( (void*) buff );
        return SELERR(ESDAE,19);
    }
    s += nexport( s, &missh[ ME_SOURCE ].name, 1 );
    *s++ = '@';
    if( missh[ ME_SOURCE-1 ].type == 0 ){
        s += nexport( s, &misshost, 0 );
        if( domain[0] != 0 ) *s++ = '.';
    }else{
         /* Заносим имя машины с подсетями */
        for( j = ME_SOURCE-1; j > 0 && missh[ j ].type != 0; --j ){
            s += nexport( s, &missh[ j ].name, 0 );
            *s++ = '.';
        }
    }
     /* Добавляем домен */
    if( s + (l = strlen( domain )) > buff+ADDRESS_LIM ){
        free( (void*) buff );
        return SELERR(ESDAE,6);
    }
    strcpy( s, domain );    s += l;
     /* Добавляем комментарий от старого "From:" */
    if( s + (l = strlen( oldf )) > buff+ADDRESS_LIM ){
        free( (void*) buff );
        return SELERR(ESDAE,6);
    }
    strcpy( s, oldf );
     /* Заменяем звездочку на нормальное имя */
    rc = Adjust( hh, "From", buff );
    free( (void*) buff );
    return rc;
}

/*
 * Сформировать обратный путь из MISS-конверта
 */
static int InterPath( hh, m_env )
    e_header *hh;
    FileItem *m_env;
{
    char *cp;
    char *faddr;
    int slot;
    int rc;
    int ldom=strlen(domain);

    if( m_env == NULL ) return SELERR(ESDAE,32);
     /* Проверку надо делать всегда, чтобы не было пустых адресов */
    if( m_env[ ME_SOURCE ].type != MISSUSERTYPE ){
        return SELERR(ESDAE,18);
    }
     /* Измеряем размер и аллоцируем память */
    for( slot=ME_SOURCE; slot>0 && m_env[slot].type != 0; --slot ) {}
    faddr=malloc((ME_SOURCE-slot)*(sizeof(Fname)+1)+ldom+LNsize+2);
    if( faddr == NULL ) return ErrMemory;
     /* Формируем буфер */
    cp = faddr;
    if (slot+1==ME_SOURCE) {
      strcpy(cp,mhome); cp+=strlen(mhome);
      *cp++='.';
      strcpy(cp,domain); cp+=ldom;
      *cp++ = '!';
    } //if
    while( ++slot < ME_SOURCE+1 ){
        FileItem *ip = &m_env[slot];
         /* Осторожно: цифры в именах портятся после c|0x80 */
        cp += nexport( cp, &ip->name, (ip->type&0x7F) == 'C' );
        if (slot+1==ME_SOURCE) {
          *cp++='.';
          strcpy(cp,domain); cp+=ldom;
        } //if
        *cp++ = '!';
    }
    if( cp != faddr ){
        *--cp = 0;
         /* Вставляем путь на место */
        if( (rc = SetUuPath( hh, faddr )) < 0 ){
            free( (void*)faddr );
            return rc;
        }
    }
    free( (void*)faddr );
    return 0;
}

static int nexport( rbuff, orig, enbcvt )
    char *rbuff;
    Fname *orig;
    int enbcvt;
{
    int i, rval;

    rval = pustrl( orig->c, sizeof(Fname) );
    for( i = 0; i < rval; i++ ){
        rbuff[i] = orig->c[i];
        if( enbcvt && rbuff[i] == ' ' ) rbuff[i] = '-';
    }
    return rval;
}

/*
 * Подготовить статью посылаемого файла
 */
static void MakeFItem( sip, hh )
    FileItem *sip;
    e_header *hh;
{
    char *fnm;

    memset( sip, 0, sizeof(FileItem) );
    sip->type = FSource;
    contim( &sip->info.sou.date, &sip->info.sou.time );
    fnm = Fetch( hh, "From" );
    fnm = (fnm == NULL)? "?": XtAddr( fnm );
    _mvpad( sip->name.c, sizeof(Fname), fnm, ' ' );
    sip->processor = uusrcproc;          /* "mainDef" */
}

static int IPath( hh, dir )
    e_header *hh;
    Direction *dir;
{
    char *buff;   int hl;
    int rv;

    if( (dir->addr[0].type & 0x7F) == NODE_MAILTYPE ){
        char *path = GetUuPath( hh );
        rv = ErrMemory;
        if( (buff = malloc( strlen(path)+sizeof(Fname)+2 )) != NULL ){
            hl = strlen( mhome );
            memcpy( buff, mhome, hl );    buff[ hl ] = '!';
            while( *path == ' ' ) path++;
            strcpy( buff+hl+1, path );
            rv = Adjust( hh, pathkey, buff );
            free( (void*) buff );
        }
    }else{
        rv = Adjust( hh, pathkey, GetUuPath( hh ) );
    }
    return rv;
}

void RemAtSigns( char* );
static void RemAtSigns( uuf )
    char *uuf;
{
 /* From demos!kiae!abc Sun Apr 26 04:23:54 MSD 1992 remote from twin */

    while( *uuf != ' ' && *uuf != 0 ) uuf++;      /* Проехали From_   */
    while( *uuf == ' ' && *uuf != 0 ) uuf++;      /* Проехали пробелы */
    while( *uuf != ' ' && *uuf != 0 ){            /* Пошел адрес      */
        if( *uuf == '@' ) *uuf = '%';
        uuf++;
    }
}

static int UPath( hh )
    e_header *hh;
{
    char *uufrom;
    int rv;

    rv = ErrMemory;
     /* Переводим Path в формат From_ с добавлением uucp-имени*/
    uufrom = UFPack( GetUuPath( hh ), udate(timezone), uuname );
    if( uufrom != NULL ){
        RemAtSigns( uufrom );
        rv = mylino( uufrom );
        free( (void*)uufrom );
    }
    return rv;
}

/*
 * Выдать пользователю сообщение об ошибке доставки
 */
static void Enote( ecode, hh, alink, errto )
    int ecode;
    e_header *hh;
    MailAddrElem *alink;
    char *errto;
{
    static Fname englname = { "UUDAEENGL " };
    char *to;
    enum { EDIAG_LN = 50, TODIAG_LN = 100 };
    static char lbuff[] = { "Link: [T.", [sizeof(Fname)]' ', ']',0 };
    static char llbuff[] = {
        "Link: [T.",[sizeof(Fname)]' ', ";T.",[sizeof(Fname)]' ',']', 0
    };

    static char ebuff[ EDIAG_LN+1 ] = { "Resume: ", [EDIAG_LN-7]0 };
    static char tobuff[ TODIAG_LN+1 ] = { "Target: ", [TODIAG_LN-7]0 };
    static char *body[4];       /* 4 строки + NULL */
    char *dstr;                         /*Указатель на диагностику    */
    int bx = 0;

    if (ecode==0) {
      to=Fetch(hh, "Return-Receipt-To");
    } else {
      to=Fetch(hh, "Sender");           //We don't use the Reply-To
      if (to==NULL) to=Fetch(hh, "X-Uucp-From");
      if (to==NULL) to=Fetch(hh, "From");
    } //if
    if (to==NULL) return;               //некуда слать
    while (*to==' ' || *to=='\t') ++to;
    if (*to==0) return;                 //пустой адрес

    if( errto != NULL ){
        strncpy( tobuff+8, XtAddr( errto ), TODIAG_LN-9 );
        body[bx++] = tobuff;
    }
    if( alink != NULL ){
        if( alink[0].type != MISSNODETYPE ){
            lbuff[7] = alink->type & 0x7F;
            lbuff[8] = (alink->type & 0x80)? ' ': '.';
            memcpy( lbuff+9, alink->name.c, sizeof(Fname) );
            body[bx++] = lbuff;
        }else{
            llbuff[7] = alink[0].type & 0x7F;
            llbuff[8] = (alink[0].type & 0x80)? ' ': '.';
            memcpy( llbuff+9, alink[0].name.c, sizeof(Fname) );
            llbuff[20] = alink[1].type & 0x7F;
            llbuff[21] = (alink[1].type & 0x80)? ' ': '.';
            memcpy( llbuff+22, alink[1].name.c, sizeof(Fname) );
            body[bx++] = llbuff;
        }
    }

    dstr = NULL;
    if( ecode == 0 ){
        dstr = "Succesfuly delivered to the host";
    }else if( ecode == ErrDiskFull ){
        dstr = "Disk full";
    }else if( ecode == ErrObject ){
        dstr = "Object not found";
    }else if( ES_SEL(ecode) == ESDAE ){ /*Собственные диагностики     */
        if( GetString( ES_CODE(ecode),englname ) == 0 ) dstr = AZC;
    }
    if( dstr == NULL ){
        dstr = _conv( ecode, 0x84, 16 );
    }
    strncpy( ebuff+8, dstr, EDIAG_LN-10 );
    body[bx++] = ebuff;

    body[bx] = NULL;
    if( ecode == 0 ){
        Notify( body, to, "Return-Receipt", hh );
    }else{
        Notify( body, to, "Delivery troubles", hh );
    }
}

/*
 * Проверка на почту, которую нельзя переправлять
 *  1) Nobody
 *  2) Самому себе - ответ удаленной машины на Notify
 */
int                 /* Boolean: "is this addr directed to trash?" */
istrash( dir )
    Direction *dir;
{
    static Fname nobody = { "Nobody    " };
    MailAddrElem *ap = &dir->addr[0];

    if( ap->type != MISSUSERTYPE ) return 0;
    if( memucmp( ap->name.c ,ownname,  sizeof(Fname) ) == 0 ) return 1;
    if( memucmp( ap->name.c ,nobody.c, sizeof(Fname) ) == 0 ) return 1;
    return 0;
}
