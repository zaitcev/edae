#ifndef __Types__
#include <Types>
#endif  __Types__

/**----------------------------------------**
 ** E-mail header & multi-to definition
 **/

/*
 * Imported methods
 */
typedef char * (*GetLine)( void );
typedef int (*PutLine)( char * );

/*
 * Semi-Class. Assignment is inhibited.
 */
typedef struct e_line_ {
    struct e_line_ *next;
    short klen;               /* Length of key */
    char body[];              /* Full image ("key: data") */
} e_line;
typedef struct {
    struct e_line_ *hlist;
    char *cpath;              /* Константная часть моршрута */
} e_header;

/*
 * Methods.
 */
int LdHdr(e_header**, GetLine, boolean);/* Fillup e_header */
int DumpHeader( e_header*, PutLine );   /* Flush (многократный) */
char *Fetch( e_header*, char* );        /* Поиск по ключу */
int Adjust( e_header*, char*, char* );  /* Изменение тела */
int Insert( e_header*, char*, char* );  /* Добавление поля */
int Count( e_header*, char* );          /* Подсчет числа вхождений */
int Remove( e_header*, char* );         /* Удаление полей по ключу */
void Destroy( e_header* );              /* Освобождение структур */
int SetUuPath( e_header*, char* );
#define GetUuPath(this)  ((this)->cpath)

/**----------------------------------------**
 ** Тип "исполняемая команда Unix".
 ** Этот тип сильно связан с предыдущим (через X822()).
 **/

 /* Данные типа char** */

 /* Методы */
int Xproc( char***, char*, GetLine, boolean);
int X822( char***, e_header* );
int Xplct( char***, char* );           /* Надо бы VARARGS(...) */
void Xfree( char** );
