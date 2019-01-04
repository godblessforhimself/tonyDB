%{
    #include "y.tab.h"
    #include <stdlib.h>
    void yyerror(char *);
    void tst() {
        printf("hha");
    }
%}

%%

[0-9]+      {
                yylval = atoi(yytext);
                printf("%s", yytext);
                tst();
                return INTEGER;
            }

[-+\n]      return *yytext;

[ \t]       ; /* skip whitespace */

[A-Z]   {
                printf("somestring %s", yytext);
            }

.           yyerror("invalid character");

%%