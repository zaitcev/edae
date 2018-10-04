/**
 ** UUROUTER
 ** Лексический анализатор
 **/
#include <SysIO>
#include <FileSystem>
#include "UULibDef"           /* ADDRESS_LIM, Собственный printf() */
#include "TableDef"
#include "LexDef"
#include "liniDef"

#define EOF (-1)
static int unflag;
int line_count = 1;

/* Буфер возврата длиннее адресной строки, чтобы можно было выдать в
 * разборщике диагностику переполнения имени */
static char retbuff[ (ADDRESS_LIM \/ (sizeof(Fname)+1)) + 2 ];
typedef enum {
    ST_INIT,                  /* Ожидаем прихода имени */
    ST_COMMENT,               /* Комментарий */
    ST_NAME,                  /* Ждем конца имени */
MAX_ST } state_t;
static state_t state = ST_INIT;
static eof_fl = 1;

void Lex0()
{
    static FileItem itab = {
        FSource,0,0 ,0,{pwd=0}, {root=0l}, {"ROUTES    "}, NullProc
    };
    int rc;

    eof_fl = 1;
    if( (rc = FindFile( &itab, OwnCatal )) >= 0 ){
        if( lini0( (char) rc, 0 ) >= 0 ){
            unflag = 0;
            eof_fl = 0;
        }
    }
}

short Lex( pret )
    char **pret;
{
    int c;
    int bx;                   /* Индекс в выходном буфере */

    if( pret != NULL ) *pret = retbuff;
    bx = 0;
    if( eof_fl ){
        retbuff[ 0 ] = 0;
        return LEX_EOF;
    }
    for(;;){
        if( unflag ){
            c = unflag;
        }else{
            if( (c = inp1()) <= 0 ) c = EOF;
        }
        unflag = 0;
        switch( c ){
        case EOF:
            linend();
            eof_fl = 1;
            retbuff[ 0 ] = 0;
            return LEX_EOF;
        case '\n':
            line_count++;
            if( state == ST_COMMENT ){
                state = ST_INIT;
                break;
            }
            /* fall through */
        case ' ':
            if( state == ST_NAME ){
                retbuff[ bx ] = 0;
                state = ST_INIT;
                return LEX_NAME;
            }
            break;
        case ',':    case ';':    case '.':    case '=':
            if( state == ST_NAME ){
                unflag = c;
                retbuff[ bx ] = 0;
                state = ST_INIT;
                return LEX_NAME;
            }
            if( state == ST_INIT ){
                retbuff[ 0 ] = (char)c;    retbuff[ 1 ] = 0;
                if( c == ',' ) return LEX_COMMA;
                if( c == ';' ) return LEX_SEMIC;
                if( c == '.' ) return LEX_PERIOD;
                return LEX_EQU;
            }
            break;
        case '%':
            if( state == ST_INIT ){
                state = ST_COMMENT;
                break;
            }
            break;
        default:
            if( (c&0xE0) == 0 ){          /*Управляющий*/
                retbuff[ 0 ] = 0;
                return LEX_ERROR;
            }
            if( state == ST_INIT ){
                bx = 0;
                retbuff[ bx++ ] = (char)c;
                state = ST_NAME;
                break;
            }
            if( state == ST_NAME ){
                if( bx == sizeof(retbuff)-1 ){
                    retbuff[ bx ] = 0;
                    return LEX_NAME;
                }
                retbuff[ bx++ ] = (char)c;
                break;
            }
        }
    }
}

void LexEnd()          /*Можно и не вызывать, если EOF был*/
{
    if( !eof_fl ){
        eof_fl = 1;
        linend();
    }
}
