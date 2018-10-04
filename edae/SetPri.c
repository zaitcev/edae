/**
 ** UUDAEMON
 ** Установка приоритета по заданному
 **/
#include "addrDef"

void SetPri( dir, newpri )
    Direction *dir;
    int newpri;
{
    int i;

    for( i = 0; i < dir->cnt; i++ ) dir->addr[i].pri = newpri;
}
