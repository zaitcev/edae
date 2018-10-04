/*
 * Таблица маршрутов для данного сообщения
 */
#ifndef __FileStruct__
#include <FileStruct>
#endif

/*
 * Данные: Заголовок - структура MultiTab, к ней привязан список
 * направлений и вектор адресатов.
 */
/* У Direction целых три конструктора: GateName, Commut, FindLink.  */
/* Трудно сделать единый конструктор для структуры переменного      */
/* размера без процедур с переменным числом аргументов.             */
/* Индекс xto применяется вместо указателя потому, что время        */
/* жизни адресных списков может быть больше, чем у mto[].           */
typedef struct _MAElem {
    char type;                /* Тип=0 -> конец вектора */
    Fname name;
    short pri;                /* Приоритет */
} MailAddrElem;
typedef struct _Addr {
    int lto;                  /* Чтобы зря не дергать strlen(x.to) */
    char *to;                 /* Почтовый адрес (грязный) до NULL */
    int xto;                  /* Индекс в mto[] (для статистики) */
    struct _Addr *next;       /* список сведенных к одному направлению*/
} Addressee;
typedef struct _Dir {
    struct _Dir *next;
    struct _Addr *list;       /* Список связанных на данный адрес */
    unsigned uucp:1;          /* Флаг uucp-направления */
    short cnt;                /* Количество элементов адреса */
    MailAddrElem addr[];      /* Размер(addr) = cnt+1 */
} Direction;
typedef struct {
    Direction *list;          /* СПИСОК направлений */
    Addressee *vto;           /* ВЕКТОР адресатов */
} MultiTab;

/*
 * Методы
 * Реализованы не все методы, все-таки это C, а не C++.
 */
int MRoute( char**, MultiTab**, char** );
void MFree( MultiTab* );
void SetPri( Direction*, int );         /* Заменить приоритет */
