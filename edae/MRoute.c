/**
 ** Методы мультитаблицы. Для вычисления компонент вызывает Route() -
 ** конструктор для Direction.
 **/
#include <SysErrors>
#include "SelErrors"
#include <string.h>
#include <stdlib.h>           /* malloc() */
#include "UULibDef"           /* memcmp() для R-11 */
#include "addrDef"

int dircmp( Direction*, Direction* );
int Route( char*, Direction**, char** );

/*
 * Маршрутизуем по каждому направлению, на одинаковые направления
 * вешаем списки адресатов. Формируем MultiTab, причем поддерживается
 * ее инвариантность (можно не глядя звать mtfree()).
 */
int MRoute( mto, prv, errto )
    char **mto;               /* Вход: Вектор адресов */
    MultiTab **prv;           /* Выход: Список направлений */
    char **errto;             /* Выход: Ошибочный адрес из mto */
{
    int cntto;                /* Количество направлений  "To:" */
    Addressee *tos;           /* Вектор указателей на адреса */
    MultiTab *rv;
    int rc;
    int i;

    *errto = NULL;

     /* Создаем заголовок */
    if( (rv = malloc( sizeof(MultiTab) )) == NULL ) return ErrMemory;
    rv->list = NULL;
    rv->vto = NULL;

     /* Формируем таблицу адресов */
    for( cntto = 0; mto[cntto] != NULL; ) cntto++;   /* Сколько всего?*/
    if( cntto == 0 ){    rc = SELERR(ESDAE,22);   goto err; }
    tos = malloc( (cntto+1)*sizeof(Addressee) );
    if( tos == NULL ){    rc = ErrMemory;    goto err; }
    rv->vto = tos;
    for( i = 0; i < cntto; i++ ){
        unshort     lto=strlen(mto[i]);
        while (lto>0 && mto[i][lto-1]==' ') --lto;
        while (lto>0 && mto[i][lto-1]=='.') --lto;
        mto[i][lto]='\0';
        if( (tos[i].to = strdup( mto[i] )) == NULL ){
            rc = ErrMemory;    goto err;
        }
        tos[i].lto=lto;
        tos[i].xto=i;
        tos[i].next=NULL;
    }
    tos[i].to = NULL;
    tos[i].lto = 0;
    tos[i].xto = 0;
    tos[i].next = NULL;

     /* Маршрутизуем по каждому направлению, объединяя одинаковые */
    for( i = 0; i < cntto; i++ ){
        Direction *rdir, *wkdir;    char *newto;
         /* Маршрутизуем, заменяем адрес на верный в нашем контексте */
        if( (rc = Route( tos[i].to, &rdir, &newto )) < 0 ){
            *errto = mto[i];            /* Только не tos[i].to ! */
            goto err;
        }
        free( (void*) tos[i].to );
        tos[i].to = newto;
        tos[i].lto = strlen( newto );
         /* Ищем совпадающие направления */
        for( wkdir = rv->list; wkdir != NULL; wkdir = wkdir->next ){
            if( dircmp( rdir, wkdir ) == 0 ) break;
        }
        if( wkdir == NULL ){      /* Не найдено -> добавляем в rtab */
            rdir->next = rv->list;    rv->list = rdir;
            rdir->list = &tos[i];
        }else{                    /* Найдено -> добавляем к wkdir */
            Addressee **tt;
            free( (void*) rdir );
             /* Проще вставить tos[i] в голову списка wkdir->list,    */
             /* пользователи требуют сохранять упорядочение.          */
            for( tt = &wkdir->list; *tt != NULL; tt = &(*tt)->next ){}
            *tt = &tos[i];    tos[i].next = NULL;
        }
    } /*for*/

    *prv = rv;
    return 0;

err:
    MFree( rv );
    return rc;
}

/*
 * Деструктор таблицы направлений и адресатов (MultiTab).
 */
void MFree( tab )
    MultiTab *tab;
{
    Direction *wkdir;
    Addressee *ap;

    if( tab != NULL ){
        if( tab->vto != NULL ){
            for( ap=tab->vto; ap->to!=NULL; ap++ ) free((void*)ap->to);
            free( (void*)tab->vto );
        }
        while( (wkdir = tab->list) != NULL ){
            tab->list = wkdir->next;
            free( (void*)wkdir );
        }
        free( (void*)tab );
    }
}

/*
 * Сравнить направления.
 * при совпадении вернуть 0, иначе -1
 */
static int dircmp( a1, a2 )
    Direction *a1, *a2;
{
    int i;

    if( a1->cnt != a2->cnt ) return -1;
    for( i = 0; i < a1->cnt; i++ ){
        if( a1->addr[i].type != a2->addr[i].type ) return -1;
        if( memcmp( a1->addr[i].name.c,
                    a2->addr[i].name.c, sizeof(Fname) ) != 0 )
        {
            return -1;
        }
    }
    return 0;
}
