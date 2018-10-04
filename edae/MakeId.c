/**
 ** Изготовление Message-Id
 **/
#include <stdlib.h>
#include <string.h>
#include <SysErrors>
#include <FileStruct>         /* Для "mainDef" */
#include <SysRadix>           /* Для "mainDef" */
#include <TimeSystem>
#include <SysTime>
#include <SysConv>
#include "UULibDef"
#include "mainDef"            /* domain */
#include "hmangDef"

static char MSGID_KEY[] = "Message-Id";

int
MakeId(hh)
    e_header *hh;
{
    char *buff, *to;
    struct _time_ time;
    struct _date_ date;
    struct _date  ymd;
    unshort       msecs10;
    int rc;

    if( Fetch( hh, MSGID_KEY ) != NULL ) return 0;
    buff = malloc( 16+ADDRESS_LIM );
    if( buff == NULL ) return ErrMemory;
    to = buff;
    *to++ = '<';
    do {
      date=_date_();
      time=_time_();
      msecs10=_time_msecs10_;
    } while (date.today!=_date_().today);
    time.minutes-=date.timezone;
    if ((short)time.minutes<0) {time.minutes+=24*60; --date.today;}
    ymd=_date0(0,date.today);
    ymd.year+=ymd.year<94 ? 2000 : 1900;
    strcpy(to, _conv(ymd.year, 0x84,10)); to+=4;
    strcpy(to, _conv(ymd.month,0x82,10)); to+=2;
    strcpy(to, _conv(ymd.day,  0x82,10)); to+=2;
    strcpy(to, _conv(time.minutes/60,0x82,10)); to+=2;
    strcpy(to, _conv(time.minutes%60,0x82,10)); to+=2;
    strcpy(to, ".AA0"); to+=4;
    strcpy(to, _conv(msecs10,0x84,10)); to+=4;
%   strcpy( to, _conv(_date_().today,0x84,16) );    to += 4;
%   tb = _time_();
%   strcpy( to, _conv(tb.minutes,0x83,16) );    to += 3;
%   strcpy( to, _conv(tb.decsecs,0x83,16) );    to += 3;
    *to++ = '@';
    strcpy( to, mhome );    to += strlen( mhome );
    *to++ = '.';
    strcpy( to, domain );    to += strlen( domain );
    *to++ = '>';
    *to++ = 0;
    rc = Adjust( hh, MSGID_KEY, buff );
    free( (void*) buff );
    return rc;
}
