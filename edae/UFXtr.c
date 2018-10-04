/**
 ** Драйвер UUCP-From_
 **/
#include <FileStruct>         /* FileItem */
#include <SysRadix>           /* Нужен для "mainDef" */
#include <string.h>
#include <stdlib.h>
#include "SelErrors"
#include "UULibDef"           /* pustrl() */
#include "mainDef"            /* uhome[] */

static char remote_from[] = "remote from";
#define RFSIZE (sizeof(remote_from)-1)

/*
 * Выборка адреса из uucp From_. Возвращает динамику.
 */
char *
UFXtr(oldfrom)
char     *oldfrom;
{
  char   *cp;
  char    csave;
  char   *faddr;                        /* возвращаем адрес */
  char   *paddr;                        /* uucp addr во From_ */
  int     lpaddr;                       /* длина uucp addr во From_ */
  char   *raddr;                        /* remote from addr */
  int     lraddr=-1;                    /* длина remote from addr */
  char    uaddr[8];                     /* uucp имя узла */
  int     luaddr=-1;                    /* длина uucp имени узла */
  char   *h=GtEnvelope()[ME_SOURCE].name.c;

  /* Формируем uucp имя узла, откуда пришел файл (в нижнем регистре)*/
  if (GtEnvelope()[ME_SOURCE].type==UUCP_MAILTYPE)
    for (luaddr=0; luaddr<8 && h[luaddr]!=' ' && h[luaddr]!=0; ++luaddr)
      uaddr[luaddr]=h[luaddr]|((h[luaddr]>='A' && h[luaddr]<='Z')<<7);

  /* обрабатываем uucp адрес */
  cp=oldfrom+5;
  while (*cp==' ' || *cp=='\t') ++cp;
  if (*cp=='\0') return NULL;
  paddr=cp;
  for ( ; *cp!='\0' && *cp!=' ' && *cp!='\t'; ++cp) ;
  lpaddr=cp-paddr;

  /* ищем "remore from" */
  for ( ; *cp!='\0'; ++cp) {
    if (*cp==' ' || *cp=='\t') {
      if (memcmp(cp+1, remote_from, RFSIZE)==0) {
        cp+=RFSIZE+1;
        if (*cp==' ' || *cp=='\t') {  /* нашли "remore from" */
          ++cp;
          while (*cp==' ' || *cp=='\t') ++cp;
          raddr=cp;
          for ( ; *cp!='\0' && *cp!=' ' && *cp!='\t'; ++cp) ;
          lraddr=cp-raddr;
          if (lraddr==luaddr && memucmp(raddr, uaddr, lraddr)==0)
            lraddr=-1;                  /*remote from и uucp совпадают*/
        } else {
          return NULL;
        } /*if*/
      } /*if*/
    } /*if*/
  } /*for*/

  /* формируем uucp адрес */

  /* отбрасываем дублирование */
  csave=paddr[lpaddr]; paddr[lpaddr]='\0';
  cp=strchr(paddr,'!');
  paddr[lpaddr]=csave;
  if (cp) {
    if (luaddr==cp-paddr && memucmp(uaddr, paddr, luaddr)==0)
      luaddr=-1;
    if (lraddr==cp-paddr && memucmp(raddr, paddr, lraddr)==0)
      lraddr=-1;
  } /*if*/

  if ((faddr=malloc(luaddr+1+lraddr+1+lpaddr+1))==NULL) return NULL;
  cp=faddr;
  if (luaddr!=-1)
    {memcpy(cp, uaddr, luaddr); cp+=luaddr; *cp++='!';}
  if (lraddr!=-1)
    {memcpy(cp, raddr, lraddr); cp+=lraddr; *cp++='!';}
  for ( ; lpaddr; --lpaddr, ++paddr, ++cp)
    *cp=*paddr=='@' ? '%' : *paddr;
  *cp='\0';
  return faddr;
}

/*
 * Задание нового From_. Возвращет динамику.
 */
char* UFPack( addr, date, rhome )
    char* addr;
    char* date;
    char* rhome;
{
    char *mem, *dst, *s;
    int l;

    l = sizeof("From ")-1 + strlen(addr)+1 + strlen(date) + 1;
    if( rhome != NULL ) l += (sizeof(" remote from ")-1+strlen(rhome));
    if( (mem = malloc( l )) == NULL ) return NULL;
    dst = mem;
    for( s = "From "; *s != 0; s++ ) *dst++ = *s;
    for( s = addr; *s != 0; s++ ) *dst++ = *s;
    *dst++ = ' ';
    for( s = date; *s != 0; s++ ) *dst++ = *s;
    if( rhome != NULL ){
        for( s = " remote from "; *s != 0; s++ ) *dst++ = *s;
        for( s = rhome; *s != 0; s++ ) *dst++ = *s;
    }
    *dst = 0;
    return mem;
}
