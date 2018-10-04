/**
 ** UUROUTER & UUBRIDGE
 ** Разбор адреса на две значащие части: адрес машины и имя пользователя
 **/
/*
 * Замечание: Первый символ в htype используется как индикатор
 * режима адресации ( =='U' => uucp ). Поэтому надо резервировать
 * буфер размером 1+ADDRESS_LIM+1.
 */
#include <string.h>

void UAPars( address, username, htype )
    char *address, *username, *htype;
{
    char *sep;  short xsep;   /*Сепаратор -> указатель и индекс*/
    char *hostaddr = htype+1;

    /*
     * Ищем лягушку (справа)
     */
    if( (sep = strrchr( address, '@' )) != NULL ){
        *htype = 'I';
        goto Splited;
    }

    /*
     * Ищем воскл. знак (слева)
     */
    if( (sep = strchr( address, '!' )) != NULL ){
        *htype = 'U';
        goto Splited;
    }

    /*
     * Ищем процент (справа)
     */
    if( (sep = strrchr( address, '%' )) != NULL ){
        *htype = 'I';
        goto Splited;
    }

    /*
     * Видимо, есть только имя пользователя
     */
    *htype = 'L';    *hostaddr = 0;
    strcpy( username, address );
    return;

Splited:
    xsep = sep-address;
    if( *htype == 'U' ){
        strncpy( hostaddr, address, xsep );    hostaddr[ xsep ] = 0;
        strcpy( username, sep+1 );
    }else{
        strcpy( hostaddr, sep+1 );
        strncpy( username, address, xsep );    username[ xsep ] = 0;
    }
    return;
}
