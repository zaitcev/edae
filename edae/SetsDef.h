/**
 ** Sets package
 ** This class provides with the standard methods for the
 ** sets representation (TYPE ST = SET OF STRING);
 ** Assumes: 1) Sets aren't huge, so direct search is applicable.
 **/

 /* Meaningful component of the member */
typedef union {
    void *p;
    int i;                              /* NOT FOR THE POINTER CRUNCH */
    /* long l */
} ST_data;

 /* Set member */
typedef struct _ST_item {
    struct _ST_item *next;              /* Chain value */
    ST_data data;                       /* Related data */
    char body[];                        /* Begining of the key */
} ST_item;

 /* Comparison procedure */
typedef int (*ST_resolver)( void*, void* );

 /* Common header of the class object */
typedef struct {
    ST_item *list;
    ST_resolver rproc;
    ST_item *seek;
} ST_header;

 /* Methods */
int      STinit( ST_header*, ST_resolver );
ST_item *STadd ( ST_header*, ST_data*, void*, int );
int      STfind( ST_header*, void*, ST_data* );
%% void  STkill( ST_header* );
void     STfirst( ST_header* );
ST_item *STnext( ST_header* );
