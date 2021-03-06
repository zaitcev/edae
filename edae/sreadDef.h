/**
 ** UUBRIDGE
 ** Опредления для последовательного чтения файлов
 **/
#define __sreadDef__

#if !defined(SectorSize)
#   define SectorSize 0x100
#endif

/*
 * Структура -- аналог FILE в stdio.h
 */
typedef struct {
    char siolabel;  /*Метка файла*/
    char sioflag;   /*<>0 - была запись в файл*/
    long siosect;   /*Указатель ввода/вывода*/
    short sio_sl;   /*Длина физического сектора файла в лог. секторах*/
} MISSFILE;

/*
 * Функции доступа
 * Длина ввода-вывода указывается в секторах (SectorSize), смещение тоже
 * Sread() возвращает длину чтения так же, как fread() в станд. среде.
 * При вызове sseek нужно все-таки, чтобы позици была кратна физическим
 * секторам.
 */
int sopen( MISSFILE*, char );
int sread( MISSFILE*, char*, int );
int swrite( MISSFILE*, char*, int );
int sseek( MISSFILE*, long );
long stell( MISSFILE* );
int sclose( MISSFILE* );
                                                                                                                                                       