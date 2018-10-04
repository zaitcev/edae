/**
 ** UUROUTER
 ** Загрузка таблиц маршрутизации
 **/
#include <SysErrors>          /* ErrFile в случае отсутствия UU_Links */
#include <SysCalls>           /* _abort_ */
#include <SysStrings>         /* _mvpad() */
#include <string.h>
#include "UULibDef"           /* printf() */
#include "TableDef"           /* L_name etc. */
#include "LexDef"             /* LEX_EOF etc. */
#define loop for(;;)

 /* В MISS не помещается больше трех статей в конверт сообщения */
#define MAX_LINK_DIM  3

void Table0( void );          /* Инициация таблиц (указателей) */

  /* Подфункции синтаксического разбора */
int GetLink( L_name* );
int GetNode( char* );

static int eon_flag;          /* Флаг режима сканирования */

/*
 * Загрузка из текстового файла
 * Никаких конечных автоматов!
 */
int TbLdx( dhome )
     /* Явный аргумент, чтобы была видна зависимость от extern domain.*/
    char *dhome;
{
    static L_name LinkNames[ MAX_LINK_DIM+1 ];
    static char NodeName[ ADDRESS_LIM+1 ];
    static L_name OwnDummy[] = {
        { '*',  {"          "} },
        { '\0', {"          "} }
    };
    TabLink *tl;
    int rc;

    eon_flag = 1;
     /*Начальный сброс*/
    Table0();
    printf(FALSE,"\037");
     /* Проходим по файлу с конфигурацией */
    Lex0();
    while( (rc = GetLink( LinkNames )) == 0 ){
        tl = SetLink( LinkNames );
        while( (rc = GetNode( NodeName )) == 0 ){
            PutNode( NodeName, tl );
        }
    }
    LexEnd();
     /* Вставляем барьер */
     /* (список отсортирован, так что это можно делать).     */
    tl = SetLink( OwnDummy );
    PutNode( dhome, tl );
    return (rc < 0)? 1: 0;
}

/*
 * Выборка имени связки
 */
int GetLink( lbuff )
    L_name *lbuff;
{
    char *val;
    short len;
    short lexeme;
    short ccnt;

    ccnt = 0;
    loop{
         /* Читаем имя связки */
        lexeme = Lex( &val );
        if( lexeme == LEX_EOF ){
            if( ccnt == 0 ) return 1;
            printf(FALSE,"line %d: Unexpected EOF\n", line_count);
            return -1;
        }
        if( lexeme != LEX_NAME ){
            if( lexeme == LEX_ERROR ){
                printf(FALSE,"line %d: Bad character\n", line_count);
            }else{
                printf(FALSE,"line %d: Unexpected separator '%s'\n",
                              line_count, val );
            }
            return -1;
        }
        if( ++ccnt > MAX_LINK_DIM ){
            printf(FALSE,"line %d: Too many members\n", line_count );
            return -1;
        }
        len = strlen( val );     /* val -> Имя абонента (S'NODENAME) */
        if( len < 3 || len > 12 || (val[1] != '\'' && val[1] != '"') ){
            printf(FALSE,"line %d: Bad link format (%s)\n",
                         line_count, val);
            return -1;
        }
         /* Заполняем сегмент возвращаемого буфера */
        lbuff->type = val[0] & 0x7F;
        if( val[1] == '"' ) lbuff->type |= 0x80;
        _mvpad( lbuff->name, sizeof(Fname), &val[2], ' ' );
        lbuff++;
         /* Читаем разделитель */
        lexeme = Lex( NULL );
        if( lexeme == LEX_EQU ) break;
        if( lexeme != LEX_PERIOD ){
            printf(FALSE,"line %d: '.' was expected in link\n",
                         line_count);
            return -1;
        }
    } /*loop*/
     /*Ставим маркер конца*/
    lbuff->type = 0;

    eon_flag = 0;
    return 0;
}

/*
 * Выборка имени узла
 */
int GetNode( rnode )
    char *rnode;              /* of ADDRESS_LIM */
{
    int rx = 0;
    char *val;
    short len;
    short lexeme;
    enum {  NONE, NAME, PERIOD } lastlex = NONE;

    if( eon_flag ) return 1;

    loop{
        lexeme = Lex( &val );
        if( lexeme == LEX_EOF ){
            printf(FALSE,"line %d: EOF in the node name\n", line_count);
            return -1;
        }
        if( lexeme == LEX_COMMA || lexeme == LEX_SEMIC ){
            if( lastlex != NAME ){
                printf(FALSE,"line %d: Unexpected end of name\n",
                             line_count);
                return -1;
            }
            eon_flag = (lexeme == LEX_SEMIC)? 1: 0;
            return 0;
        }
        if( lexeme != LEX_NAME && lexeme != LEX_PERIOD ){
            if( lexeme == LEX_ERROR ){
                printf(FALSE,"line %d: Bad character\n", line_count);
            }else{
                printf(FALSE,"line %d: Extra separator '%s'\n",
                                              line_count,val );
            }
            return -1;
        }
        if( lexeme == LEX_NAME ){
            lastlex = NAME;
        }else{
            if( lastlex != NAME ){
                printf(FALSE,"line %d: Coupled periods\n", line_count);
                return -1;
            }
            lastlex = PERIOD;
        }
        len = strlen( val );  /* Член имени или точка */
        if( len < 1 || rx+len > ADDRESS_LIM ){
            printf(FALSE,"line %d: Too long at (%s...)\n",
                         line_count,val );
            return -1;
        }
        strcpy( rnode+rx, val );
        rx += len;
    }/*loop*/
}
