%{
    #include "parser.h"
    #include "y.tab.h"
    #include <stdlib.h>
    #include <ctype.h>
    void yyerror(char *);
    extern "C"
    {	
        int yywrap(void);
        int yylex(void);
    }
%}
LETTER      [A-Za-z]
DIGIT       [0-9]
INTEGER     {DIGIT}+
SIGNINT     [+-]?INTEGER
IDENTIFIER  {LETTER}({LETTER}|{DIGIT}|_)*
QUOTED_STRING ['][^']*[']
%x shell_cmd
%%
!                       {BEGIN(shell_cmd);}
<shell_cmd>[^\n]*       {yylval.sval = yytext; return SYS_COMMAND;}
<shell_cmd>\n           {BEGIN(INITIAL);}
{QUOTED_STRING} {
                    int len = strlen(yytext);
                    yylval.sval = mk_string(yytext + 1, len - 2);
                    return VALUE_STRING;
                }
{INTEGER}       {
                    sscanf(yytext, "%d", &yylval.ival);
                    return VALUE_INT;
                }
{IDENTIFIER}    {
                    return get_id(yylval.sval = yytext);
                }
"<"             {return OP_LT;}
"<="            {return OP_LE;}
">"             {return OP_GT;}
">="            {return OP_GE;}
"="             {return OP_EQ;}
"!="            {return OP_NE;}
"<>"            {return OP_NE;}
[*/+\-':;,.|&()]  {return yytext[0];}
[ \n\t]              {/* ignore spaces, tabs, and newlines */}
%%