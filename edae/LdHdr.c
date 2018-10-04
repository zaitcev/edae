/**
 ** Операции над заголовком RFC-822.
 ** Загрузка и запись тоже здесь.
 **/
#include <SysErrors>
#include "SelErrors"
#include <string.h>
#include <stdlib.h>
#include "UULibDef"           /* memucmp() для R-11 */
#include "hmangDef"

/*
 * Конструктор заголовка
 */
int
LdHdr(rval, input, misscode)
    e_header **rval;
    GetLine input;
    boolean misscode;         /* FALSE - КОИ-8, TRUE - MISS кодировка */
{
    e_header *this;
    e_line *line, **last;
    char *p;
    int rc;

    if( (this = malloc( sizeof(e_header) )) == NULL ) return ErrMemory;
    this->hlist = NULL;
    this->cpath = NULL;

    last = NULL;                        /* Передается между итерациями*/
    for(;;){
        int lenp;
    /* часто бывают сообщения без пустой строки без заголовка */
%       if ((p=(*input)())== NULL) {rc=SELERR(ESDAE,7); goto err;}
    if ((p=(*input)())==NULL) break;
        lenp=strlen(p);
        if (!misscode)                  /* декодируем из КОИ-8 */
          (*decode)(p, lenp);
        if (lenp && p[lenp-1]=='\r')    /* Иногда бывает из ДОСа -ivt */
          p[--lenp]=0;
        if (lenp==0) break;             /* До пустой строки */
        if( *p != ' ' && *p != '\t' ){  /* Ключевая строка */
            char *s;
            s = strchr( p, ':' );
            if( s == NULL ){  rc = SELERR(ESDAE,8);  goto err; }
            line = malloc( sizeof(e_line) + lenp + 1 );
            if( line == NULL ){  rc = ErrMemory;  goto err; }
            line->next = NULL;
            line->klen = s - p;
            strcpy( line->body, p );
            last = (last == NULL) ? &this->hlist : &(*last)->next;
            *last = line;
        }else{                          /* Строка продолжения */
            int l;
            if( last == NULL ){  rc = SELERR(ESDAE,9);  goto err; }
            l = strlen( (*last)->body );
            line = malloc( sizeof(e_line) + l+1 + lenp + 1 );
            if( line == NULL ){  rc = ErrMemory;  goto err; }
            line->next = (*last)->next;
            line->klen = (*last)->klen;
            memcpy( line->body, (*last)->body, l );
            line->body[l++] = '\n';
            strcpy( &line->body[l], p );
            free( (void*) *last );
            *last = line;
        }
    }/*loop*/

    *rval = this;
    return 0;

err:
    Destroy( this );
    return rc;
}

/*
 * Запись заголовка на диск
 */
int DumpHeader( this, output )
    e_header* this;
    PutLine output;
{
    int rc;
    e_line *line;

    for( line = this->hlist; line != NULL; line = line->next ){
        if( (rc = (*output)( line->body )) < 0 ) return rc;
    }
    if( (rc = (*output)( "\0" )) < 0 ) return rc;
    return 0;
}

/*
 * Выборка по ключу
 */
char *Fetch( this, key )
    e_header* this;
    char* key;
{
    e_line *line;
    int l = strlen( key );

    for( line = this->hlist; line != NULL; line = line->next ){
        if( line->klen == l && memucmp( key, line->body, l ) == 0 ){
            return line->body+line->klen+1;
        }
    }
    return NULL;
}

/*
 * Заменить значение на новое.
 */
int Adjust( this, key, val )
    e_header* this;
    char* key;
    char* val;
{
    e_line *line, **oln;
    int l = strlen( key );

    for( oln = &this->hlist; *oln != NULL; oln = &(*oln)->next ){
        if( (*oln)->klen==l && memucmp(key,(*oln)->body,l)==0 ) break;
    }
    line = malloc( sizeof(e_line) + l+2 + strlen(val) + 1 );
    if( line == NULL ) return ErrMemory;
    line->klen = l;
    memcpy( line->body, key, l );
    line->body[l++] = ':';
    line->body[l++] = ' ';
    strcpy( &line->body[l], val );
    if( *oln != NULL ){
        line->next = (*oln)->next;
        free( (void*) *oln );
    }else{
        line->next = NULL;
    }
    *oln = line;
    return 0;
}

/*
 * Вставить дублирующее значение (например "Received:")
 */
int Insert( this, key, val )
    e_header* this;
    char* key;
    char* val;
{
    e_line *line;
    int l = strlen( key );

    line = malloc( sizeof(e_line) + l+2 + strlen(val) + 1 );
    if( line == NULL ) return ErrMemory;
    line->klen = l;
    memcpy( line->body, key, l );
    line->body[l++] = ':';
    line->body[l++] = ' ';
    strcpy( &line->body[l], val );
    line->next = this->hlist;    this->hlist = line;
    return 0;
}

/*
 * Подсчет вхождений
 */
int Count( this, key )
    e_header *this;
    char *key;
{
    int cnt;
    e_line *line;
    int l = strlen( key );

    cnt = 0;
    for( line = this->hlist; line != NULL; line = line->next ){
        if( line->klen == l && memucmp( key, line->body, l ) == 0 ){
            cnt++;
        }
    }
    return cnt;
}

/*
 * Удаление поля (только 1 раз).
 */
int Remove( this, key )
    e_header *this;
    char *key;
{
    e_line *line, **oln;
    int l = strlen( key );

    for( oln = &this->hlist; (line = *oln) != NULL; oln = &line->next ){
        if( line->klen == l && memucmp( key, line->body,l ) == 0 ){
            *oln = line->next;
            free( (void*) line );
            return 0;
        }
    }
    return ErrObject;
}

/*
 * Установка константной части Uucp-маршрута
 */
int SetUuPath( this, path )
    e_header *this;
    char *path;
{
    char *s;

    if( this->cpath != NULL ) free( (void*) this->cpath );
    if( (this->cpath = strdup( path )) == NULL ) return ErrMemory;
    for( s = this->cpath; *s != 0; s++ ){
        if( *s == '@' ) *s = '%';
    }
    return 0;
}

/*
 * Деструктор
 */
void Destroy( this )
    e_header* this;
{
    e_line *line;

    while( (line = this->hlist) != NULL ){
        this->hlist = line->next;
        free( (void*) line );
    }
    if( this->cpath != NULL ) free( (void*) this->cpath );
    free( (void*) this );
}
