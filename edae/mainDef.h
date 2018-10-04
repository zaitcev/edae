/**
 ** Глобальные определения
 **/
/* Нужно включать: "UULibDef" */
#ifndef __SysRadix__
#include <SysRadix>
#endif
#ifndef __FileStruct__
#include <FileStruct>
#endif

#define ORGANTN_LIM   72
#define UUFROM_LIM   100      /* Длина только адреса */

/*
 * Структура конверта почты.
 */
typedef enum {
    ME_CURRADDR,              /* 0 - текущий адрес передачи */
    ME_NEXTADDR,              /* 1 - Следующий адрес */
    ME_DUMMY_2,
    ME_DUMMY_3,
    ME_DUMMY_4,               /* 4 - обычно имя узла отправителя */
    ME_SOURCE,                /* 5 - исходный отправитель */
    ME_WORK,                  /* 6 - Рабочий буфер спулера */
    ME_FILEITEM               /* 7 - Статья передаваемого файла */
} me_idxs;
/*
 * Размер конверта системной почты.
 */
#define MESIZE     0x100      /* Байты, само собой */
#define MESECTS   (MESIZE/SectorSize)

#define MISSNODETYPE  ('N'|0x80)
#define MISSUSERTYPE  ('C'|0x80)

extern UnpFproc mailname;
extern Fproc uusrcproc, rnewsproc;
extern Fname misshost;                  /*  ) Одно и то же, только   */
extern char mhome[ sizeof(Fname)+1 ];   /*  ) форматы разные.        */
extern char uuname[ sizeof(Fname)+1 ];  /* Умолчание == mhome */
extern char codename[LNsize+1];         /* Название кодовой таблицы */
extern char *domain;    extern int homefactor;
extern char timezone[ TZN_LIM+1 ];
extern char orghome[ ORGANTN_LIM+1 ];
extern char *pathkey, *addrkey;
extern char centrum[ sizeof(Fname)+1 ];
extern char ownname[ sizeof(Fname)+1 ];
extern int control;                     /* Разрешены телеграммки      */

int rmail  (char**, char, boolean); /* e-mail over uucp    */
int rbmail (char**, char, boolean); /* e-mail over uucp (batch)       */
int gate   (char**, char, boolean); /* e-mail over MISS    */
int rnews  (char**, char, boolean); /* Usenet over uucp    */
int snews  (char**, char, boolean); /* Create Usenet batch */

int rninit( void );

FileItem *GtEnvelope( void );           /* Зачерпнуть статику PrcMsg */

char *UFXtr( char* );                   /* Вытянуть адрес из "From_" */
char *UFPack( char*, char*, char* );    /* Сформировать "From_" */
