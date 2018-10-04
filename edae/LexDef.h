/**
 ** UUROUTER
 ** Определения для лексического анализатора таблиц
 **/

#define LEX_EOF     0
#define LEX_NAME    1       /* Имя (произвольная строка) */
#define LEX_EQU     2       /* "=" */
#define LEX_COMMA   3       /* "," */
#define LEX_SEMIC   4       /* ";" */
#define LEX_PERIOD  5       /* "." */
#define LEX_ERROR   6       /* Например, управляющий символ */

extern int line_count;      /* До первого LF == 1 */

void Lex0( void );
short Lex( char** suppl );
void LexEnd( void );
