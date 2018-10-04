/**
 ** Селекторные ошибки
 **/

/*
 * Формат ошибки:
 *
 *     .... 15   14 - 12   11 - 0
 *  Все единицы  Селектор  Код ошибки, в ДВОИЧНОМ ДОПОЛНЕНИИ
 *  (знак числа)           (для пущей совместимости с LIBRARIAN).
 */

/*
 * Используемые селекторы
 */
#define ESDAE   1       /* "UUDAETEXT " */
#define ESCTL   2       /* "UUDAECTL  " */
#define ESSYS   7       /* "SYSMESSAGE" */

/*
 * Конструкторы и селекторы селекторных ошибок (...)
 */
#define SELERR(s,e)   ( ((s)<<12) | (~((e)|0X7000)) )
#define ES_SEL(se)    (((se)>>12)&0x7)
#define ES_CODE(se)   ((~(se))&0xFFF)