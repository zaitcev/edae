/**
 ** Обработка почтового сообщения "от файла до файла"
 ** Основная задача - определить формат сообщения и вызвать
 ** подходящий обработчик (rmail, rbmail etc.).
 * Кому-то может быть покажется странным, что используется вызов
 * gate() через задание строки с именем процедуры. Но по-моему
 * регулярность кода важнее, чем пара микросекунд на одно сообщение.
 * Вот если бы это было в цикле, имело бы смысл накрутить if-ов.
 **/
#include <FileStruct>         /* FileItem *m_env */
#include <FileSystem>         /* GetSectorSize() */
#include <SysRadix>           /* extern UnpFproc mailname; */
#include <string.h>           /* strlen() */
#include <stdlib.h>           /* malloc() */
#include <SysErrors>
#include <TimeSystem>         /* _date_ для "Date: *" */
#include <FileIO>             /* _io_ */
#include "SelErrors"
#ifdef R_11
#include <SysStrings>         /* memcmp() defined as _cmps() */
#endif
#include "UULibDef"           /* udate() */
#include "mainDef"            /* domain, misshost, etc. */
#include "hmangDef"           /* e_header */
#include "liniDef"
#include "addrDef"            /* Addressee, Direction, dircmp() */

extern int Telex( FileItem* );

int UucpLib( char, short*,short*,short*,short* );
int LoadEnve( char, FileItem** );

static FileItem *m_env = NULL;   /* The MISS mail file envelope */
FileItem *GtEnvelope(){   return m_env; }

int PrcMsg( inlab )
    char inlab;
{
    int under_lini = 0;
    typedef struct {          /* Описание команд */
        char *name;    int (*proc)( char**, char, boolean);
    } cmd_t;
    static cmd_t cmds[] = {
        { "rmail",   rmail   },         /* uucp delivery */
        { "rbmail",  rbmail  },         /* uucp batch delivery */
        { "rcbmail", rbmail },          /* uucp comprs batch delivery */
        { "gate",    gate    },         /* e-mail delivery */
        { "rnews",   rnews   },         /* Usenet redirection */
        { "snews",   snews   },         /* Usenet batching */
        { NULL,      NULL    }
    };
    char **cmd_argv = NULL;   /* X-команда целиком */
    static char data_name[ sizeof(Fname)+1 ];   /* вх. данные команды */
    char dlab = 0;            /* Дополнительный вводной файл */
    FileItem *ip;
    int rc;
    boolean misscode=TRUE;

     /* Делаем m_env */
    rc = LoadEnve( inlab, &m_env );
    if( rc < 0 ){   m_env = NULL;    goto err;  }

     /* Разбираемся с заданием, формируем входные данные */
    ip = & m_env[ ME_FILEITEM ];
    if( (ip->type&0x7F) == (FLibr&0x7F) ){
         /* Библиотека -> от news-serever'a с командой "snews <arg>" */
        short Xsec, Xoff, Dsec, Doff;
        misscode=((char *)&ip->info)[7]==0;
        rc = UucpLib( inlab, &Xsec,&Xoff,&Dsec,&Doff );
        if( rc < 0 ) goto err;
        rc = lini0( inlab, ((long)Xsec)*SectorSize + Xoff );
        if( rc != 0 ) goto err;
          rc = Xproc(&cmd_argv, data_name, lini, misscode);
        linend();
        if( rc != 0 ) goto err;
        rc = lini0( inlab, ((long)Dsec)*SectorSize + Doff );
        if( rc != 0 ) goto err;
        under_lini = 1;
    }else if( ip->type == 'X' ){
         /* Uucp отдельными файлами */
        if( (rc = lini0( inlab, MESIZE )) != 0 ) goto err;
          rc = Xproc(&cmd_argv, data_name, lini, TRUE);
        linend();
        if( rc != 0 ) goto err;
        if( data_name[0] != ' ' ){
            static FileItem fpar;
            fpar.type = 'D';    fpar.name = *(Fname*)data_name;
            if( (rc = FindFile( &fpar, MailCatal )) < 0 ){
                if( rc == ErrFile ) rc = SELERR(ESDAE,30);
                goto err;
            }
        }
        dlab = (char) rc;
        if( (rc = lini0( dlab, 0 )) != 0 ) goto err;
        under_lini = 1;
    }else if( ip->type == FSource ){
         /* Текст -> почта или новости от другого моста */
        if( ip->processor == uusrcproc ){
            if( (rc = Xplct( &cmd_argv, "gate" )) < 0 ) goto err;
        }else if( ip->processor == rnewsproc ){
            if( (rc = Xplct( &cmd_argv, "rnews" )) < 0 ) goto err;
        }else{
            rc = SELERR(ESDAE,2);    goto err;
        }
        if( (rc = lini0( inlab, MESIZE )) != 0 ) goto err;
        under_lini = 1;
    }else if( ip->type == 0 ){
         /* Телеграмма -> сравниваем с образцом */
        rc = Telex( m_env );    goto err;
    }else{
        rc = SELERR(ESDAE,2);    goto err;
    } /*if*/

     /* Исполняем команду */
    {
        int ix;
        char *command = cmd_argv[0];
        if( command == NULL ){   rc = SELERR(ESDAE,14);  goto err; }
        for( ix = 0; ; ix++ ){
            if( cmds[ix].name == NULL ){
                rc = SELERR(ESDAE,13);    goto err;
            }
            if( strcmp( cmds[ix].name, command ) == 0 ){
                rc = (*cmds[ix].proc)(cmd_argv+1, inlab, misscode);
                if( rc != 0 ) goto err;
                break;
            }
        }
    }

    rc = 0;

err:
    if( cmd_argv != NULL ) Xfree( cmd_argv );
    if( under_lini ) linend();
    if( m_env != NULL ) free( (void*)m_env );
    if( dlab != 0 ) CloseFile( dlab );
    m_env = NULL;             /* Обязательно, а то он статический */
    return rc;
}

/*
 * Считать заголовок библиотеки и определить смещения для чтения
 * почтовых файлов. Данная процедура помимо прочего окружает
 * существование буфера для чтения заголовка библиотеки.
 */
static int UucpLib( lab, Xs, Xo, Ds, Do )
    char lab;
    short *Xs, *Xo, *Ds, *Do;
{
    static IOl_block icb;
    unsigned short inssize = GetSectorSize( lab );
    char *buff;
    unsigned seekval;     /* Байтовое смещение в файле                */
    FileItem *ip;
    int i;
    int fill;
    int rc;

    /** if( (inssize & inssize-1) != 0 ) return ErrSectSize; **/
    seekval = MESIZE;
    if( (buff = malloc( inssize )) == NULL ) return ErrMemory;
     /* Считываем заголовок библиотеки */
    icb.label = lab;
    icb.dir = 2;
    icb.bufadr = buff;
    icb.buflen = inssize;
    icb.sector = seekval/inssize;
    if( (rc = _io_( &icb ))>>8 == (~0) ) goto drop;
     /* Ищем нужные статьи и достаем адреса в файле */
    fill = 0;
    ip = (FileItem*) (buff + seekval%inssize);
    for( i = 0; i < 2; i++ ){
%       if( ip->type == FSource ){
            if( ip->name.c[0] == 'X' ){
                *Xs = ip->ident + MESECTS;    *Xo = ip->p.shift;
                fill += 1;
            }
            if( ip->name.c[0] == 'D' ){
                *Ds = ip->ident + MESECTS;    *Do = ip->p.shift;
                fill += 2;
            }
%       }
        ip++;
    }
    if( fill != 3 ){   rc = ErrParams;    goto drop; }
    rc = 0;

drop:
    free( (void*)buff );
    return rc;
}

/*
 * Конструктор конверта.
 */
static int LoadEnve( inlab, pbuff )
    char inlab;
    FileItem **pbuff;
{
    static IOl_block icb;
    unsigned short inssize = GetSectorSize( inlab );
    FileItem *buff;
    int rc;

     /* Загружаем */
    if( (buff = malloc( inssize )) == NULL ) return ErrMemory;
    icb.label = inlab;
    icb.dir = 2;
    icb.bufadr = (void*) buff;
    icb.buflen = inssize;
    icb.sector = 0;
    if( (rc = _io_( &icb ))>>8 == (~0) ){
        free( (void*)buff );
        return rc;
    }

     /* Надо еще проверить на количество статей <= 2, тип = 'N' */
     /* ... */

    *pbuff = buff;
    return 0;
}
