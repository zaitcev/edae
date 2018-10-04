/**
 ** UUDAEMON
 ** Маршрутизация (aka коммутация)
 ** Секция включает в себя обработку синонимов.  -- todo
 ** С точки зрения типов/клссов секция трактуется как расширитель к
 ** MultiTab::MpyRoute (private, конечно).
 **/
#include <SysErrors>
#include "SelErrors"
#include <string.h>
#include <stdlib.h>
#include <FileStruct>         /*  )                          */
#include <SysRadix>           /*  ) нужны для "addrDef"      */
#include "UULibDef"           /*  )     + XtAddr()           */
#include "addrDef"
#include <MailSystem>         /* для "TableDef" */
#include <SysStrings>         /* _cmps() */
#include "TableDef"
#include "SetsDef"
#include "mainDef"            /* domain */

#define  DFLT_PRI    0        /* Низкий */

/*
 * Неоднозначность: "a!b!c" можно интерпретировать двояко:
 *  [U;0000,N;A] + "b!c"  или  [C;C,N;B,N;A] + "c".
 * Также есть некоторые неудобства при работе с
 * переменной длиной dir->addr.
 * Так что постулируем, что в MISS всегда один N-узел в конверте.
 */

 /*Разбор адреса uucp/internet на адрес машины и имя пользователя*/
 /*3-й аргумент надо резервировать на 1 длиннее, чем обычно      */
void UAPars( char*, char*, char* );    /* (addr,user,t+host) */

char *Alias( char* );

char PhaseTrim( char*, char**, char** );
int Commut( char*,                 Direction** );
int FindLink( char*,               Direction** );
int GateName( char, char *, char*, Direction** );
int nimport( char*, Fname*, int );

/*
 * Главный вход - в основном дергает других.
 * Возвращать статику нельзя, так как из направлений и "to" строятся
 * таблицы (по нескольку на одно сообщение).
 */
int Route( addr, prv, newto )
    char *addr;               /* Вход - "грязный" адрес */
    Direction **prv;          /* Выход. Аллоцируется и отдается */
    char **newto;             /* Выход. Тоже аллоцирутеся */
{
    static char gnbuff[ sizeof(Fname)+1 ];
    char *purea = XtAddr( addr );       /* СТАТИКА! (для PhaseTrim()) */
    char *acta;               /* Адрес после поиска синонима (статика)*/
    char *host;               /* Имя машины или узла */
    char *user;               /* Дополнительная информация */
    Direction *dir = NULL;
    int rc;
    int f;

     /* Такое редко, но случается (когда мейлер барахлит) */
    if( purea[0] == 0 ) return SELERR(ESDAE,38);

     /* Проверка синонимов */
     /* Проверку надо перенисти за PhaseTrim,               */
     /* и проверять отдельно user и host, а при считывании  */
     /* алиасов хранить их в разобранном виде               */
    if( (acta = Alias( purea )) == NULL ) acta = purea;

    /*
     * Формируем host и user для разных методов записи адреса
     */
    switch( PhaseTrim( acta, &user, &host ) ){  /* acta модифицируется*/
    case 'U':                 /* Uucp */
    /* свой член маршрута уже отбросили !!! */
#if 0
        acta = user;          /* Отбрасываем первый член маршрута */
#endif
    case 'I':                 /* Доменная - ищем по таблицам */
        boolean     fuucp=FALSE;
        f=factor( host );
        if (f==2) {             //проверяем на *.uucp
          unshort  lhost=strlen(host);
          if (lhost>5 && strucmp(host+lhost-5,".uucp")==0) {
            host[lhost-5]='\0';
            f=1;
            fuucp=TRUE;
          } //if
        } //if
        if (f==1) {
          if (FindLink(host, &dir)>=0 ){
            if( (*newto = strdup( acta )) == NULL ) goto edup;
            *prv = dir;
            return 0;
          }
          if (fuucp) {
            host[strlen(host)]='.';
            f=2;
          } //if
        } //if
        if (f!=1) {
            if( (rc = Commut( host, &dir )) < 0 ) goto egate;
            if( dir->addr[0].type != '*' ){    /* Не наш поддомен */
                if( (*newto = strdup( acta )) == NULL ) goto edup;
                *prv = dir;
                return 0;
            }
            free( (void*) dir );
            if( f > homefactor+1 ){        /* Подсеть не описана  */
                rc = SELERR(ESDAE,21);
                goto egate;
            }
            if( f == homefactor+1 ){       /* На машину MISS-сети */
                *strchr( host, '.' ) = 0;
                f = 1;
            }
        }
        break;
    default:                  /* Локальная (нет имени хоста) */
        host[0] = 0;
    }

    /*
     * Теперь из host и user делаем MISS-адрес
     */
    if( strchr(user,'%') != NULL || strchr(user,'@') != NULL ||
        strchr(user,'.') != NULL || strchr(user,'!') != NULL )
    {
         /* Для сложного имени переправляем на другой мост */
        if( host[0] == 0 ) return SELERR(ESDAE,11);  /* А то циклится!*/
        *(Fname*)gnbuff = mailname.name;    gnbuff[ sizeof(Fname) ] = 0;
        rc = GateName( mailname.type, gnbuff, host, &dir );
        if( rc < 0 ) goto egate;
        if( (*newto = strdup( acta )) == NULL ) goto edup;
    }else{
         /* Простое имя транслируем в MISS */
        rc = GateName( MISSUSERTYPE, user, host, &dir );
        if( rc < 0 ) goto egate;
        if( (*newto = strdup( acta )) == NULL ) goto edup;
    }
    *prv = dir;
    return 0;

egate:
    if( rc == ErrMemory ) return rc;
    if( centrum[0] == 0 ) return rc;
    *(Fname*)gnbuff = mailname.name;
    gnbuff[ sizeof(Fname) ] = 0;
    rc = GateName( mailname.type, gnbuff, centrum, &dir );
    if( rc < 0 ) return rc;
    if( (*newto = strdup( acta )) == NULL ) goto edup;
    *prv = dir;
    return 0;

edup:
    free( (void*) dir );
    return ErrMemory;
}

/*
 * Разбиение адреса на адрес узла и имя пользователя.
 * Это разбиение на самом деле более глубоко, чем кажется.
 * Заодно выбрасывается собственное имя.
 */
static char                   /* Тип адресации */
PhaseTrim( addr, puser, phost )
    char *addr;               /* ПОБОЧНЫЙ ЭФФЕКТ: модифицируется */
    char **puser, **phost;    /* Возвращаемые указетели на статику */
{
    static char fullhome[ ADDRESS_LIM+1 ];
    static char suppl[ ADDRESS_LIM+1 ];
    static char host[ 1+ADDRESS_LIM+1 ], *host1 = host+1;

    { int l = strlen(mhome);
        memcpy( fullhome, mhome, l );
        fullhome[l] = '.';
        strcpy( fullhome+l+1, domain );
    }

    /*
     * Канонический разбор адреса на машину и пользователя
     * с выбрасыванием собственного адреса.
     */
    for(;;){                                 /*Пока почта нашей машине*/
        UAPars( addr, suppl, host );         /* Адрес ==> Кому + Куда */
        if( host[0] == 'L' ) break;          /*Локальная адресация ?  */
        if( host[0] == 'U' ){                /*UUCP-адресацая         */
            if( strucmp( host1, uuname ) != 0 &&
                strucmp( host1, mhome ) != 0 &&
                strucmp( host1, domain ) != 0 &&
                strucmp( host1, fullhome ) != 0 ) break;
        }else{                               /*Доменная адресация     */
            if( strucmp( host1, domain ) != 0 &&
                strucmp( host1, mhome ) != 0 &&
                strucmp( host1, fullhome ) != 0 ) break;
        }
        strcpy( addr, suppl );               /*Отбрасываем свое имя   */
    }
    *puser = suppl;    *phost = host1;
    return host[0];
}

/*
 * Основной алгоритм маршрутизации
 */
static int Commut( dest, pdir )
    char *dest;
    Direction **pdir;         /* Возвращамый, причем в динамике */
{
    Direction *dir;
    TabNode *np;
    TabLink *lp;
    int fdest = factor(dest);
    int f;
    int i;

    /*
     * Цикл проверки в порядке убывания фактора (хозяин хозяина ...)
     * Ищем на явное совпадение
     */
    f = fdest;
    while( *dest != '\0' ){
        for( NFirst(); (np = NNext()) != NULL; ){
            if( f == np->Nfactor && strucmp( dest, np->Nname ) == 0 ){
                lp = np->Nlink;
                dir = malloc( sizeof(Direction) +
                                (lp->Lcnt + 1)*sizeof(MailAddrElem) );
                if( dir == NULL ) return ErrMemory;
                dir->next = NULL;    dir->list = NULL;
                dir->cnt = lp->Lcnt;
                dir->uucp = lp->Luucp;
                for( i = 0; i < dir->cnt; i++ ){
                    dir->addr[i].type = lp->Lav[i].type;
                    memcpy( dir->addr[i].name.c, lp->Lav[i].name,
                             sizeof(Fname) );
                    dir->addr[i].pri = DFLT_PRI;
                }
                dir->addr[i].type = 0;
                *pdir = dir;
                return 0;
            }
        }
        --f;
        while( *dest != '\0' && *dest != '.' ) dest++;
        if( *dest == '.' ) dest++;
    }
     /* Можно возвращать ErrObject, но я хочу отличать        */
     /* этот облом от ошибки CheckMailAddr()+CheckUucpLink(). */
    return SELERR(ESDAE,24);
}

/*
 * Проверяем наличие Uucp-имени машины
 */
static int FindLink( host, pdir )
    char *host;
    Direction **pdir;
{
    TabLink *lp;
    Direction *mem;
    static Fname nbuff;

    if( nimport( host, & nbuff, 0 ) != 0 ) return SELERR(ESDAE,35);
    for( LFirst(); (lp = LNext()) != NULL; ){
        if( lp->Luucp &&
            memucmp( lp->Lav[0].name, nbuff.c, sizeof(Fname) ) == 0 )
        {
            mem = malloc( sizeof(Direction)+2*sizeof(MailAddrElem) );
            if( mem == NULL ) return ErrMemory;
            mem->next = NULL;
            mem->list = NULL;
            mem->uucp = 1;
            mem->cnt = 1;
            mem->addr[0].type = UUCP_MAILTYPE;
            mem->addr[0].name = nbuff;
            mem->addr[0].pri = DFLT_PRI;
            mem->addr[1].type = 0;
            *pdir = mem;
            return 0;                   /*Найдено*/
        }
    }
    return SELERR(ESDAE,26);            /*Нет такого*/
}

/*
 * Преобразуем строковые имена в MISS-направление
 */
static int GateName( abotype, aboname, node, pdir )
    char abotype, *aboname, *node;
    Direction **pdir;
{
    Direction *dir;
    int acnt;
    int rc;

    acnt = (node!=NULL && node[0]!=0 && strucmp(mhome,node)!=0)? 2: 1;
    dir = malloc( sizeof(Direction) + (acnt+1)*sizeof(MailAddrElem) );
    if( dir == NULL ) return ErrMemory;
    dir->next = NULL;
    dir->list = NULL;
    dir->uucp = 0;
    dir->cnt = acnt;
    if( acnt > 1 ){
        dir->addr[0].type = MISSNODETYPE;
        rc = SELERR(ESDAE,10);
        if( nimport( node, & dir->addr[0].name, 0 ) != 0 ) goto badname;
        dir->addr[0].pri = DFLT_PRI;
        {
            static mailaddr mab;
            mab.type = MISSNODETYPE;
            memcpy( mab.name, dir->addr[0].name.c, sizeof(Fname) );
            if( (rc = CheckMailAddr( &mab )) < 0 ) goto badname;
        }
    }
    { MailAddrElem *ap = & dir->addr[acnt-1];
        ap->type = abotype;
        rc = SELERR(ESDAE,10);
        if( nimport( aboname, & ap->name, 1 ) != 0 ) goto badname;
        ap->pri = DFLT_PRI;
        if( acnt == 1 &&
            ap->type == mailname.type &&
            _cmps( ap->name.c, mailname.name.c, sizeof(Fname) ) == 0 )
        {
            free( (void*) dir );
            return SELERR(ESDAE,28);
        }
    }
    dir->addr[acnt].type = 0;
    *pdir = dir;
    return 0;

badname:
    free( (void*) dir );
    return rc;
}

/*
 * Преобразование члена сетевого имени в MISS-имя
 */
static int nimport( name, ret, enbcvt )
    char *name;
    Fname *ret;
    int enbcvt;
{
    int i;
    char *rs;

     /* Регистр букв надо менять (Zaitcev@Twin.srcc.msu.su) */
    rs = ret->c;
    for( i = 0; i < sizeof(Fname); i++ ){
        if( *name != 0 ){
            rs[i] = (*name++)&0x7F;
            if( enbcvt && rs[i] == '-' ) rs[i] = ' ';
        }else{
            rs[i] = ' ';
        }
    }
    if( *name != 0 ) return -1;       /* Переполнение */
    return 0;
}
