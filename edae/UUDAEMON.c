/**
 ** E-mail routing daemon
 **/
#ifdef i_86
#   pragma STARTER(), EXIT();
#   pragma STACK_SIZE(0x180);
#   include <PragmaItem>
#   pragma  DATA(ITEM);
    Regime(SYS);
//       Объекты Сегм. Группа Класс Атрибуты
#   pragma CODE (CODE ,*     ,Code, MAIN, READ);
#   pragma DATA (DATA ,DGROUP,Data, MAIN);
#   pragma STACK(STACK,DGROUP,Data, MAIN);
#endif
#ifdef R_11
#   pragma REGIME(SYS);
#   pragma STARTER(NULL);
#endif
#include <SysCalls>
#include <SysRadix>
#include <SysErrors>
#include "SelErrors"
#include <FileSystem>
#include <SysFiles>           /* NameSysMessage */
#include <SysConv>
#include <StartOpers>         /* GetString() */
#include <SysUtility>         /* SysLog() */
#include <TimeSystem>         /* Delay() */
#include <MailSystem>         /* GetHomeName0() */
#include <string.h>
#include "UULibDef"           /* ADDRESS_LIM */
#include "mainDef"
#include "liniDef"            /* lini() для AlInit() */
#define loop for(;;)

extern void memon0( void );
extern void memon( void );

#define MAXMSG  72            /*Длина записи в SYSLOG*/

void Control( int );
int TbLdx( char* );           /*Загрузка таблиц в память (синтаксис)*/
int AlInit( void );           /*Загрузка таблиц синонимов*/
int rninit( void );           /*Загрузка таблиц редиректора*/
%%t void ChkTab( void );      /*Распечатка таблиц для отладки*/
int PrcMsg( char );
static void SetGlobals( void );
static void CheckPrimary( void );
static void DiagErr( int, char );
static void EStop( int );
static char *ErrToText( int );

 /* Глобальные, соответствующие "mainDef" */
UnpFproc mailname = {
     /* Системный тип исключает возможность обхода статистики */
    0, InterpreterFlag, DAEMON_TYPE, {"0000000000"}
};
/* Файлпроцессор для почтовых файлов */
Fproc uusrcproc = {  0x27A8,0xCCA2,0xCCE8,0xB540 };
/* Файлпроцессор для файлов новостей */
Fproc rnewsproc = {  0x0225,0x9EE2,0xBB80,0x0000 };
Fname misshost;
char mhome[ sizeof(Fname)+1 ];
char uuname[ sizeof(Fname)+1 ];
char codename[LNsize+1];                /* Название кодовой таблицы */
static char domain_buff[ ADDRESS_LIM+1 ];
char *domain = domain_buff;
int homefactor;
char timezone[ TZN_LIM+1 ];
char orghome[ ORGANTN_LIM+1 ];
char *pathkey, *addrkey;
char centrum[ sizeof(Fname)+1 ];
/* Имя для посылки диагностик. Пробелы и минусы использовать нельзя! */
char ownname[ sizeof(Fname)+1 ] = "EMAILDAE  \0";
int control;

main( scode )
    int scode;
{
    static Fproc mailproc;   /** {  0x05F1, 0x0669, 0x0669, 0x0669 } **/
    static FileItem fmaster = {
        FMail,3,1, 0,{pwd=0x20}, {mail=0,0,8}, {"XXXXXXXXXX"},
        {  0x088B, 0xA74F, 0x97FE, 0xC45D }  /*"C POSTMASTER", Interp*/
    };
    char msglab;
    int rc;

     /*Перво-наперво проверяем причину запуска*/
    if( scode != 0 ) EStop( scode );
    Delay( 70, 1 );   /*Облегчяем режим Системы при старте*/
    memon0();
    CheckPrimary();   /*Проверяем на повторный запуск на одном польз.*/
    SetGlobals();     /*Прочесть конфигурацию*/
    _FPpack( &mailname, &mailproc );
    inicod(codename);
    rc = TbLdx( domain );    /*Загрузить таблицы маршрутизации*/
    if( rc < 0 ) EStop( rc );
    if( rc != 0 ) DiagErr( SELERR(ESDAE,27), 0 );
    {                 /*Грузим синонимы*/
        static FileItem fpar = {
            FSource,0,0, 0,{pwd=0}, {root=0l}, {"ALIASES   "}, NullProc
        };
        char alab;
        if( (rc = FindFile( &fpar, OwnCatal )) >= 0 ){
            alab = (char) rc;
            if( (rc = lini0( alab, 0 )) >= 0 ){
                rc = AlInit();
                linend();
            }
            CloseFile( alab );
        }
        if( rc < 0 ) EStop( rc );
    }
    rninit();         /*Rnews initiator*/
    /* Надо бы проверять код возврата из rnews */

    loop{
         /*Открыть входной и выходной файлы*/
        do{
            WaitMail( &mailproc );
        }while( (rc = OpenMail( &mailproc )) == ErrFile );
        if( rc < 0 ) EStop( rc );       /*Например, ErrManyOpen*/
        msglab = (char) rc;
         /*Копируем вход на выход с обработкой по дороге*/
        rc = PrcMsg( msglab );          /*С маршрутизацией*/
         /*Закрываем вход и выход*/
        if( rc == 0 ){
            if( GetFileItem( msglab ) != NULL ){
                DeleteFile( msglab, MailCatal );
            }
        } else if (rc==1) {             /* перенаправили файл */
        } else if (ES_SEL(rc)==ESCTL) {
            DeleteFile( msglab, MailCatal );
            Control( rc );
        } else {
            DiagErr( rc, msglab );                /*Запись в SYSLOG() */
            if( rc == ErrDiskFull ) EStop( rc );  /*Don't modify item */
            contim( &fmaster.info.mail.date, &fmaster.info.mail.time );
            SendMail(&fmaster, msglab);
            if( rc == ErrMemory ) EStop( rc );
            Delay( 20, 1 );
        }
        CloseFile( msglab );
    }/*loop*/
}

/*
 * Устанавливаем параметры из Системы и из файла загрузки
 */
static void SetGlobals()
{
    int rc;

    { mailaddr mbuff;    misshost = *GetHomeName( &mbuff ); }
    {
        int l = pustrl( misshost.c, sizeof(Fname) );
        memcpy( mhome, misshost.c, l );    mhome[ l ] = 0;
    }
    {
        static FileItem fpar = {
            FSource,0,0, 0,{pwd=0}, {root=0l}, {"PARAMETERS"}, NullProc
        };
        char plab;
        if( (rc = FindFile( &fpar, OwnCatal )) < 0 ) EStop( rc );
        plab = (char) rc;

        strncpy( domain, envx( "DOMAIN","\0", plab, 0 ), ADDRESS_LIM );
        domain[ ADDRESS_LIM ] = 0;
        homefactor = factor( domain );
        strncpy( uuname, envx("UUNAME",mhome,plab, 0 ), sizeof(Fname) );
        uuname[ sizeof(Fname) ] = 0;
        strncpy( timezone, envx( "TZNAME","???", plab,0 ), TZN_LIM );
        timezone[ TZN_LIM ] = 0;
        strncpy(orghome,envx("ORGANIZATION","\0",plab,0),ORGANTN_LIM);
        orghome[ ORGANTN_LIM ] = 0;
        strncpy(centrum, envx("CENTRUM","\0",plab,0), sizeof(Fname));
        centrum[ sizeof(Fname) ] = 0;
        control = (*envx("CONTROL","N",plab,0) == 'Y')? 1: 0;
        ElogMod( _convi( envx("LOGSTEP","0",plab,0), 3, 10 ) );
        memcpy(codename, envx("CODING","Relcom",plab,0), 10);
        codename[LNsize]=0;
        memcpy(mailname.name.c,envx("MAILNAME",mailname.name.c,plab,0),10);

        CloseFile( plab );
    }
    {
        int l = strlen(domain);
        if( l < 1 || l > ADDRESS_LIM-sizeof(Fname) ||
            domain[l-1] == '.' || domain[0] == '.' )
        {
            EStop( SELERR(ESDAE,25) );
        }
    }
     /* Есть возможность изменения */
    pathkey = "X-Uucp-From";
    addrkey = "X-Real-To";
}

/*
 * Проверяем, не запущен ли еще кто-нибудь на этом же пользователе
 */
static void CheckPrimary()
{
    static FileItem fpar = {
        FCatal,0,0, 0,{pwd=0}, {root=0l}, {"WORK      "}, NullProc
    };
    char wlab;
    int rc;

     /* При мягком перезапуске рабочая библиотека есть,   */
     /* а при холодном - нет.                             */
    if( GetFileItem( WorkCatal ) == NULL ){
        if( (rc = FindFile( &fpar, OwnCatal )) < 0 ) EStop( rc );
        wlab = (char) rc;
         /* Если заблокирован, то отваливаем */
        if( (rc = LockFile( wlab )) < 0 ) EStop( rc );
         /* Переносим блокировку из старших меток на 0x81 */
        if( (rc = ChngWorkCat( wlab )) < 0 ) EStop( rc );
        CloseFile( wlab );
    }
}

typedef enum {
    C_NONE,
    C_STOP,
    C_RESTART,
    C_MEMON,
    C_TABLES,
    C_DEBUG
} Codes;

/*
 * Поиск при приеме теграммы (на контексте обработки сообщения)
 */
int Telex( m_env )
    FileItem *m_env;
{
    typedef struct {    char *key;    short code; } Command;
    static Command cvect[] = {
        { "RESTART",  SELERR(ESCTL,C_RESTART) },
        { "STOP",     SELERR(ESCTL,C_STOP)    },
        { "MEMON",    SELERR(ESCTL,C_MEMON)   },
%%t     { "TABLES",   SELERR(ESCTL,C_TABLES)  },
        { "=DEBUG",   SELERR(ESCTL,C_DEBUG)   },
        { NULL,  0 }
    };
    Command *cmd;

    if( !control ) return 0;
    for( cmd = cvect; cmd->key != NULL; cmd++ ){
        if( strucmp( cmd->key, & ((char*)m_env)[ 0xE4 ] ) == 0 ){
            return cmd->code;
        }
    }
    return 0;                 /* Пустая, считаем отработанной */
}

/*
 * Управляющее действие (телеграмма уже стерта)
 */
void Control( ccode )
    int ccode;
{
    switch( ccode ){
    case SELERR(ESCTL,C_RESTART):
        DiagErr( ccode, 0 );  /*Имя файла нам ни к чему*/
        _exit_(0);            /* Мы - медиатор, вызываем перезапуск */
        break;
    case SELERR(ESCTL,C_STOP):
        DiagErr( ccode, 0 );
        SayBye();
        break;
    case SELERR(ESCTL,C_MEMON):
        memon();              /* Выводим на экран структуры malloc-a */
        break;
%%t case SELERR(ESCTL,C_TABLES):
%%t     ChkTab();
%%t     break;
    case SELERR(ESCTL,C_DEBUG):
        _debugger_();         /* Возможен выкидыш */
        break;
    }
}

/*
 * Записываем в SYSLOG ошибку обработки сообщения
 */
static void DiagErr( ecode, file )
    int ecode;
    char file;
{
    static char tbuff[ MAXMSG+1 ] = {"UUDAEMON",[MAXMSG-7]0};
    FileItem *ip;
    char *s;

    s = tbuff+8;
     /* Имя файла, если есть */
    if( file != 0 && (ip = GetFileItem( file )) != NULL ){
        *s++ = ' ';    *s++ = '(';
        memcpy( s, ip->name.c, sizeof(Fname) );    s += sizeof(Fname);
        *s++ = ')';
    }
    *s++ = ':';    *s++ = ' ';
     /* Текст диагностики */
    memcpy( s, ErrToText( ecode ), tbuff+MAXMSG-s );
    tbuff[ MAXMSG ] = 0;
     /* Пишем в файл */
    SysLog( tbuff );
    Delay( 2, 1 );            /*Даем монтажеру возможность сработать*/
}

/*
 * Диагностируем ошибку в "SYSLOG" и заканчиваем работу
 */
static void EStop( ecode )
    int ecode;
{
    static char rv[ MAXMSG+1 ] = {"UUDAEMON stopped: ",[MAXMSG-17]0};
    int sl = 18;    /* May be computed, if formatted print is needed */

     /* Здесь надо проверять на системность и выдавать диагностику */

    memcpy( rv+sl, ErrToText( ecode ), MAXMSG-sl );
    rv[ MAXMSG ] = 0;
    SysLog( rv );

    SayBye();
     /* Мы еще живы ?!!! */
    Delay( 2400, 1 );                         /* Помираем с откачкой */
    _exit_( ecode );                          /* Вылет с очисткой AR */
}

/*
 * Поиск диагностики для данной селекторной ошибки.
 * Как правило возвращается AZC.
 */
static char *ErrToText( ecode )
    int ecode;
{
    typedef struct {
        short sel;
        Fname name;
    } SelName;
    static SelName diags[] = {
        { ESSYS, { NameSysMessage }},
        { ESCTL, { "UUDAECTL  "   }},
        { ESDAE, { "UUDAETEXT "   }},
        {    -1, { "\0         "  }}
    };
    SelName *d;
    char *dstr;

    if( ecode < 0 ){
         /* Селекторные ошибки */
        for( d = diags; d->sel != -1; d++ ){
            if( d->sel == ES_SEL(ecode) ) break;
        }
        if( d->sel != -1 &&                            /*Селектор есть*/
            GetString( ES_CODE(ecode),d->name ) == 0 )   /* текст тоже*/
        {
            dstr = AZC;
        }else{
            dstr = _conv( ecode, 0x84, 16 );
        }
    }else{
        dstr = _conv( ecode, 0x84, 16 );
    }

    return dstr;
}
