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
SIGNINT     [+\-]?{INTEGER}
IDENTIFIER  {LETTER}({LETTER}|{DIGIT}|_)*

%x shell_cmd
%%
[ \n\t\r]              {/* ignore spaces, tabs, and newlines */}
\`([^\`\n]|(\`\`))*\`       {
                                yylval.sval = mk_string(yytext + 1, yyleng - 2);
                                return VALUE_STRING;
                            }
\`([^\`\n]|(\`\`))*\n       {
                                //printf("newline in string constant\n");
                            }
\'([^\'\n]|(\\\'))*\'       {
                                // 允许 '\''
                                yylval.sval = mk_string(yytext + 1, yyleng - 2);
                                // cout << "检测到字符串" << yyleng << ":" << yytext << endl;
                                return VALUE_STRING;
                            }
\"([^\"\n]|(\"\"))*\"       {
                                yylval.sval = mk_string(yytext + 1, yyleng - 2);
                                return VALUE_STRING;
                            }
\"([^\"\n]|(\"\"))*\n       {
                                //printf("newline in string constant\n");
                            }
!                       {BEGIN(shell_cmd);}
<shell_cmd>[^\n]*       {yylval.sval = yytext; return SYS_COMMAND;}
<shell_cmd>\n           {BEGIN(INITIAL);}

{SIGNINT}                   {//支持整型
                                sscanf(yytext, "%d", &yylval.ival);
                                return VALUE_INT;
                            }
{SIGNINT}\.{INTEGER}        {//支持浮点数
                                sscanf(yytext, "%f", &yylval.fval);
                                return VALUE_FLOAT;
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
<<EOF>>           {return TOKEN_EOF;}
.                 {printf("illegal character [%c]\n", yytext[0]);}
%%