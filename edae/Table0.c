/**
 ** UUDAEMON
 ** Операции с таблицами маршрутизации
 **/
#include <SysCalls>           /* _abort_ */
#include <SysErrors>          /* ErrMemory для _abort_ */
#include <string.h>           /* strlen() */
#include <stdlib.h>           /* malloc() */
#include "UULibDef"           /* factor(), strucmp() */
#include "TableDef"

static struct _TabLink *LinksHead = NULL, *LinkScan;
static struct _TabNode *NodesHead = NULL, *NodeScan;

int linkcmp( L_name*, L_name* );

/*
 * Инициация
 */
void Table0()
{
    if( LinksHead != NULL ) free( (void*) LinksHead );
    if( NodesHead != NULL ) free( (void*) NodesHead );
    LinksHead = NULL;    NodesHead = NULL;
}

/*
 * Добавление MISS-адресата
 * Дубликаты игнорируются.
 */
TabLink *SetLink( alink )     /* Вых: адрес для PutNode */
    L_name *alink;
{
    TabLink **wp, *p;
    int lcnt, i;
    for( wp = &LinksHead; (p = *wp) != NULL; wp = & p->Lnext ){
        if( linkcmp( alink, p->Lav ) == 0 ) return p;
    }
    for( lcnt = 0; alink[ lcnt ].type != 0; ) lcnt++;
    p = malloc( sizeof(TabLink) + (lcnt+1)*sizeof(L_name) );
    if( p == NULL ) _abort_( ErrMemory );
    p->Lnext = NULL;
    p->Lcnt = (char)lcnt;
    for( i = 0; i < lcnt; i++ ) p->Lav[i] = alink[i];
    p->Lav[i].type = 0;
     /* Проверка делается ТОЛЬКО здесь, чтобы легче было ее изменить */
    p->Luucp = (p->Lcnt == 1 && p->Lav[0].type == UUCP_MAILTYPE);
    *wp = p;
    return p;
}

/*
 * Добавление почтового узла в список.
 * Поиск по имени, чтобы не было дубликатов.
 * Список поддерживается отсортированным по убыванию фактора.
 */
void PutNode( naddr, l_ref )
    char *naddr;
    TabLink *l_ref;
{
    TabNode **pip, *pn;
    int f = factor( naddr );

    pip = &NodesHead;
    for( pn = NodesHead; pn != NULL; pn = pn->Nnext ){
        if( pn->Nfactor >= f ) pip = &pn->Nnext;
        if( strucmp( pn->Nname, naddr ) == 0 ) return;
    }
    if( (pn = malloc( sizeof(TabNode)+strlen(naddr)+1 )) == NULL ){
        _abort_( ErrMemory );
    }
    pn->Nlink = l_ref;
    pn->Nfactor = f;
    pn->Nnext = *pip;    *pip = pn;
    strcpy( pn->Nname, naddr );
    return;
}

void LFirst()
{
    LinkScan = LinksHead;
}

TabLink *LNext()
{
    TabLink *rv;
    if( (rv = LinkScan) != NULL ) LinkScan = LinkScan->Lnext;
    return rv;
}

void NFirst()
{
    NodeScan = NodesHead;
}

TabNode *NNext()
{
    TabNode *rv;
    if( (rv = NodeScan) != NULL ) NodeScan = NodeScan->Nnext;
    return rv;
}

static int linkcmp( l1, l2 )
    L_name *l1, *l2;
{
    while( l1->type != 0 ){
        if( (l1->type&0x7F)!=(l2->type&0x7F) || l1->name!=l2->name ){
            return -1;
        }
    }
    return (l2->type == 0)? 0: -1;
}
