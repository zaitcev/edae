/**
 ** Uucp to MISS gateway
 ** Общие для многих программ определения
 **/

#ifdef R_11
#  pragma VARARGS(printf);    /*Этот модуль не из библиотек, а свой*/
#endif
#ifdef i_86
   extern printf(int, char*, ... );
#endif

 /* На R-11 внешние функции имеют только 6 символов длины */
#define strcmpi(s1,s2) strucmp(s1,s2)

 /* Другие строковые проблемы */
#if defined(R_11)
#  define memcpy(dst,src,len) _move(src,len,dst)
#  define memcmp(dst,src,len) _cmps(src,dst,len)
#endif

 /*С некоторых пор Тимошкин ReleaseStorage стал несовместим*/
#if defined(i_86)
#  undef  ReleaseStorage
#  define ReleaseStorage(x) (_getmain_(-((unsigned)(x)/2),0))
#endif

#define DEVCLASS ('U')        /*Класс устройства для sysdev()*/
#define DEFAULT_UUCP_NAME  "guest"   /* В lparam & uuini */

 /* Типы почты для локальных и удаленных адресов */
#define USER_MAILTYPE   ('C'+0x80)
#define USER_MAILFLAG   (' ')        /* ((USER_MAILTYPE&0X80) != 0) */
#define UUCP_MAILTYPE   ('^')        /* Gateway */
#define NODE_MAILTYPE   ('N')        /* На самом деле  +0x80  */
#define DAEMON_TYPE     ('^'+0x80)   /* Должен быть системным */

/*
 * Размеры строковых данных
 */
#define HDR_LINE_LIM  136     /*Максимальный размер строки заголовка*/
#define EXE_LINE_LIM  136     /*Максимальный размер строки X-файла*/
#define MAX_LINE_LIM  136     /*Максимальный размер строки файла*/
#define USER_NAME_LIM  14     /*Макс. длина имени пользователя*/
#define HOST_MEMB_LIM  20     /*Длина члена имени машины (между ".")*/
#define ADDRESS_LIM   100     /*Длина адреса в смысле Internet*/
#define X_LINE_LIM    512     /*Длина строки текстовых данных*/
#define TZN_LIM         5     /*Time Zone Name Length*/
#define DULN            8     /*Длина вывода uucp-имен в кв. скобках*/

 /*Функции перекодировки в системный код и обратно*/
void inicod( char* );         /*Устанавливает decode & incode*/
extern void (*decode)(char*,short),(*incode)(char*,short);
 /*getargs - Разбор командной строки*/
int getargs( char*, char **, int );
 /*lread/lwrite - ввод/вывод на телефонной линии*/
 /*Возвращают длину осуществленной пересылки*/
int lread(  char*, short, short* );
int lwrite( char*, short );
 /*Процедура запроса номера линии у пользователя*/
int sysdev( char class, char* prompt );
 /*Сравнение с игнорированием регистра букв*/
int strucmp( char*, char* );
int memucmp( char*, char*, int );
 /*XtAddr -- knows about "addr (comment)" and "comment <addr> comment"*/
char *XtAddr( char* );        /*Разбор адреса среди комментариев*/
 /*adate - ARPA date & Unix date*/
char *adate( void );
char *xdate( void*, short, short );   /* xdate(struct _date_ *,n,n) */
char *udate( char* );         /* udate( NULL or TimeZone ); */
 /* env -- Загрузка описателей из файлов настройки (как ISAM) */
char *envx( char*,char*, char, long );  /* Через lini() */
 /*ulat() -- Преобразование строки к высокому регистру латинских букв*/
void ulat( char*, int );
 /*pustrl() -- Отбрасывание концевых пробелов */
int pustrl( char*, int );   /*Лимит можно задавать =0, тогда до нулька*/
 /*Подсчет количества точек в строке*/
short factor( char* );
 /*Выборка согласованного времени       (date,time) */
void contim( unsigned short _msbf *, unsigned short _msbf * );
 /*longtime() -- "Длинное" время для сравнений*/
long lutime( unsigned, unsigned );
#define longtime() lutime(0,0)        /*От текущего времени*/
 /*Elog -- сбор статистики*/
void ElogMod( unsigned short );       /*Установка режима*/
void Elog( char*, int );              /*Инициируется автоматически*/
 /* strdup -- перевод строк на динамику */
char *strdup( char* );
 /* OpenM -- Открыть файл почты с учетом факторизации каталогов */
typedef struct {   char mf, mc; } M_pair;
int OpenM( M_pair* rv, char* linkname, char mcatal );
 /* Проверка присутствия несущей */
int carrier( short );                   /*boolean*/
