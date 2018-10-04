/**
 ** UUROUTER
 ** Структуры для таблиц маршрутизации (в памяти)
 **
 **   Nodes                         Links
 **    |                             |
 **    |                             |
 **    +-> (a.b.c.d)                 |
 **    +-- next  link ------+        |
 **    |                    |        |
 **    |                    |        |
 **    +-> (xx.yy.z)        |        V
 ** 0<---- next  link ------+-----> S'UUBRD00
 **                           0<--- next
 **
 **/
/*
 * 11.05.92  14:54'
 * На самом деле есть мысль переделать все это под множества из
 * "SetsDef". Собственно множества и делались с такой крутой
 * абстракцией для этого.
 */

#ifndef __MailSystem__
#   include <MailSystem>
#endif __MailSystem__
#define L_name mailaddr

typedef struct _TabLink {
    struct _TabLink *Lnext;
    unsigned short   Luucp:1;
    unsigned char    Lcnt;      /*Количество L_name - 1 (длина адреса)*/
    L_name           Lav[];
} TabLink;

typedef struct _TabNode {
    struct _TabNode *Nnext;
    struct _TabLink *Nlink;
    short            Nfactor;   /*Количество компонент в доменном адр.*/
    char             Nname[];
} TabNode;

extern TabLink *SetLink( L_name* );      /* Добавить к списку */
extern void PutNode( char*, TabLink* );  /* Добавить к списку */
extern void LFirst( void );    TabLink *LNext( void );
extern void NFirst( void );    TabNode *NNext( void );
