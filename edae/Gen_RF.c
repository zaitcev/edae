/**
 ** UUBRIDGE
 ** Генераця имени файла для удаленной машины
 **/
#include <TimeSystem>
#include <string.h>           /* strlen() */
#include "UULibDef"           /* прототип */
#define NMSIZE  12            /* Эту константу изменить нелегко */

#define RADIX     (26+26+10+2)
#define NDIGITS    4
#define DAY_MOD   17          /* =(RADIX**NDIGITS)/(24*60*600) */

static void conv(
    char *buff,
    long num,
    int len,
    char *cv,
    int radix
);

char *Gen_RF( home, grade )
    char *home;
    char grade;
{
    static char radv[ RADIX ] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_";
    static char fname[ NMSIZE + 1 ];
    unshort nowdate;    struct _time_ nowtime;
    unsigned long binid;      /* Квазиуникальное число */
    int lhome;

    if( grade == 0 ) grade = 'x';
    if( home == NULL ) home = "MISS";
    lhome = strlen( home );
    if( lhome >= NMSIZE-(NDIGITS+1) ) lhome = NDIGITS+1;
    memcpy( fname, home, lhome );
     /* Изготовляем binid */
    nowdate = _date_().today;    nowtime = _time_();
    binid = ((long)nowtime.minutes) * 600 + nowtime.decsecs;
    binid = (nowdate%DAY_MOD) * (24ul*60*600) + binid;
     /* Преобразуем по основанию 70 */
    conv( &fname[lhome+1], binid, NDIGITS, radv, RADIX );
    fname[lhome] = grade;
    fname[lhome+NDIGITS+1] = 0;
    return fname;
}

static void conv( buff, num, len, cv, radix )
    char *buff;
    long num;
    int len;
    char *cv;
    int radix;
{

    while( len != 0 ){    --len;
        buff[ len ] = cv[ (int)(num % radix) ];
        num /= radix;
    }
}
