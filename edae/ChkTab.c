/**
 ** UUROUTER
 ** Распечатка таблиц
 **/
#include <TimeSystem>
#include "UULibDef"           /* printf() */
#include "TableDef"
extern char *prfn( Fname* );

static int lcnt;
static void linkout( TabLink* );

void ChkTab()
{
    TabNode *np;
    TabLink *lp;

    lcnt = 0;
    printf(FALSE,"\nLINKS:");
    LFirst();
    while( (lp = LNext()) != NULL ){
        if( (lcnt = (lcnt+1)%12) == 11 ) Delay( 50, 1 );
        printf(FALSE,"\n ");
        linkout( lp );
        printf(FALSE," (%d)", lp->Lcnt );
        if( lp->Luucp ) printf(FALSE,"/U");
    }
    printf(FALSE,"\nNODES:");
    NFirst();
    while( (np = NNext()) != NULL ){
        if( (lcnt = (lcnt+1)%12) == 11 ) Delay( 50, 1 );
        printf(FALSE,"\n \"%s\" (%d) via ", np->Nname, np->Nfactor);
        linkout( np->Nlink );
    }
    printf(FALSE,"\n");
}

static void linkout( lp )
    TabLink *lp;
{
    L_name *n;
    for( n = lp->Lav; n->type != 0; n++ ){
        if (n != lp->Lav) printf(FALSE,".");
        printf(FALSE,"%c%c%s", n->type&0x7F,
                     (n->type&0x80)? '"': '\'',
                     prfn((Fname*)n->name) );
    }
}
