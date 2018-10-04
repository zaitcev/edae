/**
 ** Последовательная запись в файл
 **/

typedef struct _lino_t {
    char            label;
    long            sector;
    unsigned        index;
    unsigned        ssize;
    char            buff[];
} lino_t;

typedef struct _lino_t *LINO;

int linol( LINO*, char );               /* Инициация */
int lino( LINO, char* );                /* Запись текстовой строки */
int bino( LINO, char*, int );           /* Запись двоичных данных */
int linocl( LINO );                     /* Закрытие файла */
