#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <termio.h>

/* HTML to text converter. Done for JOS by Soci/Singular.
   Feel free to improve it! Do not remove this comment */

#define ALIGN_PRE 0          /* Aligning text */
#define ALIGN_LEFT 1
#define ALIGN_CENTER 2
#define ALIGN_RIGHT 3
#define PALIGN_NONE 4        /* Aligning text after <P> */
#define PALIGN_LEFT 5
#define PALIGN_CENTER 6
#define PALIGN_RIGHT 7

#define SKIP_NONE 0        /* Skip text between these tags */
#define SKIP_SCRIPT 1
#define SKIP_STYLE 2
//#define SKIP_REM 3

#define TAG_MAX_LENGHT 1024
#define FORMATING_MAX_DEPTH 128
#define LINE_MAX_LENGTH 100

int screen_width=80;    // width of screen
int *formating;         // formating stack
int formatingp=0;       // formating stackpointer
char *lineoutbuff;      // output puffer
int lineoutp=0;         // output puffer pointer
int skip_all=SKIP_NONE; // Skipping tags like <SCRIPT> <STYLE>
int quote_type=0;       // Quoting style

void decode_tag(char *);
void decode_amp(char *);
void c_out(char);

int main(int argc, char *argv[])
{
        FILE *infile;
        int n;
        char *tag, c, tagend;
	struct termios tio;	
	
	if (isatty(STDOUT_FILENO)) {
		gettio(STDOUT_FILENO, &tio);
		screen_width = tio.cols;
	}

        tag=xmalloc(TAG_MAX_LENGHT);
        formating=xmalloc(FORMATING_MAX_DEPTH);
        formating[0]=ALIGN_LEFT;
        lineoutbuff=xmalloc(LINE_MAX_LENGTH);
        infile=fopen(argv[1],"r");
        if (infile==NULL) infile=stdin;
        printf("\n\n");
        goto start;
        while (!feof(infile))
        {
                if (c=='<') tagend='>'; else
                if (c=='&') tagend=';'; else
                {
                        tagend='\0';
                        c_out(((c=='\n' || c=='\r') && (formating[formatingp]!=ALIGN_PRE))?' ':c);
                }
                if (tagend)
                {
                        n=0;
                        while ((c!=tagend) && !feof(infile))
                        {
                                tag[n]=c=getc(infile);
                                if (n<TAG_MAX_LENGHT-1) n++;
                        }
                        tag[--n]='\0';
                        if (tagend=='>') decode_tag(tag); else
                        if (tagend==';') decode_amp(tag);
                }
        start:  c=getc(infile);
        }
        c_out('\n');
        free(lineoutbuff);
        free(formating);
        free(tag);
        return 0;
}

int is_p_formatting()
{
        int current_formatting=formating[formatingp];
        return (current_formatting==PALIGN_NONE) ||
               (current_formatting==PALIGN_LEFT) ||
               (current_formatting==PALIGN_CENTER) ||
               (current_formatting==PALIGN_RIGHT);
}

void addformating(int f)
{
        if (formatingp==FORMATING_MAX_DEPTH-1) return;
        if (!is_p_formatting()) formatingp++;
        formating[formatingp]=f;
}

void delformating()
{
        if (formatingp) formatingp--;
}

void wprintf(char *c)
{
        int n=-1,n2;
        switch (formating[formatingp])
        {
                case PALIGN_CENTER:
                case ALIGN_CENTER:for (n2=(screen_width-strlen(c)) >> 1;n2>0;n2--) putchar(' ');break;
                case PALIGN_RIGHT:
                case ALIGN_RIGHT:for (n2=screen_width-strlen(c);n2>0;n2--) putchar(' ');
        }
        
        while (c[++n])
                putchar(c[n]=='\1'?' ':c[n]);
}

void c_out(char c)
{
        int n,n2;
        if (skip_all) return;
        if (c=='\n')
        {
                lineoutbuff[lineoutp]='\0';
                wprintf(lineoutbuff);
                if (lineoutp<screen_width) putchar('\n');
                lineoutp=0;
                return;
        }
        if (formating[formatingp]!=ALIGN_PRE)
        {
                if (lineoutp)
                {
                        if (isspace(lineoutbuff[lineoutp-1]) && isspace(c)) return;
                } else if (isspace(c)) return;
        }
        lineoutbuff[lineoutp++]=c;
        if (lineoutp==screen_width)
        {
                for (n=lineoutp-1;n>0;n--)
                        if (isspace(lineoutbuff[n])) break;
                if (n)
                {
                        lineoutbuff[n]='\0';
                        wprintf(lineoutbuff);
                        if (n<screen_width) putchar('\n');
                        for (n2=0;n<lineoutp-1;n2++) 
                                lineoutbuff[n2]=lineoutbuff[++n];
                        lineoutp=n2;
                } else
                {
                        lineoutbuff[lineoutp]='\0';
                        wprintf(lineoutbuff);
                        lineoutp=0;
                }
                c='\0';
        }

}

void s_out(char *c)
{
        int n=0;
        while (c[n]) c_out(c[n++]);
}

void get_tag_piece(char *tag, int *n, char *ntag, char delimiter, int length)
{
        int z=0;
        char a;
        while (a=tag[*n])
        {
                if (z && ((a==delimiter) || isspace(a))) break;
                if (!isspace(a))
                        if (z<length) ntag[z++]=a;
                (*n)++;
        }
        ntag[z]='\0';
}

void get_tag_value(char *tag, int *n, char *ntag, int length)
{
        int z=-1,quot=0;
        char a;
        while (a=tag[*n])
        {
                if (z>=0)
                {
                        ntag[z]=a;
                        if (isspace(a) && !quot) break;
                        if (a!='"')
                        {
                                if (z<length) z++;
                        }
                        else if (quot++) break;
                } else if (a=='=') z=0;
                (*n)++;
        }
        ntag[z]='\0';
}

void tag_HR(char *tag, int *n)
{
        char width[11], align[11], ntag[7];
        int widthh=screen_width,a;
        do
        {
                get_tag_piece(tag, n, ntag, '=', 6);
                if (!strcasecmp(ntag,"width")) get_tag_value(tag, n, width, 10);else
                if (!strcasecmp(ntag,"align")) get_tag_value(tag, n, align, 10);
        } while (ntag[0]);
        a=atoi(width);
        if (a)
        {
                if (width[strlen(width)-1]=='%') 
                {
                        if (a<0) a=0;if (a>100) a=100;
                        widthh=screen_width*a/100;
                } else
                {
                        widthh=a >> 3;
                        if (widthh<0) a=0;
                        if (widthh>screen_width) widthh=screen_width;
                }
        }
        c_out('\n');
        if (!strcasecmp(align,"right")) addformating(ALIGN_RIGHT); else
        if (!strcasecmp(align,"left")) addformating(ALIGN_LEFT); else
                addformating(ALIGN_CENTER);
        for (a=0;a<widthh;a++) c_out('-');
        c_out('\n');
        delformating();
}

void tag_IMG(char *tag, int *n)
{
        char alt[100], ntag[5];
        alt[0]='\0';
        do
        {
                get_tag_piece(tag, n, ntag, '=', 4);
                if (!strcasecmp(ntag,"alt")) get_tag_value(tag, n, alt, 99);
        } while (ntag[0]);

        if (alt[0])
        {       
                c_out('[');
                s_out(alt);
                c_out(']');
        }
}

void tag_DIV(char *tag, int *n)
{
        char align[11], ntag[7];
        align[0]='\0';
        do
        {
                get_tag_piece(tag, n, ntag, '=', 6);
                if (!strcasecmp(ntag,"align")) get_tag_value(tag, n, align, 10);
        } while (ntag[0]);
        c_out('\n');
        if (!strcasecmp(align,"right")) addformating(ALIGN_RIGHT); else
        if (!strcasecmp(align,"left"))  addformating(ALIGN_LEFT); else
        if (!strcasecmp(align,"center")) addformating(ALIGN_CENTER); else
        addformating(formating[formatingp]);
}

void tag_P(char *tag, int *n)
{
        char align[11], ntag[7];
        align[0]='\0';
        do
        {
                get_tag_piece(tag, n, ntag, '=', 6);
                if (!strcasecmp(ntag,"align")) get_tag_value(tag, n, align, 10);
        } while (ntag[0]);
        c_out('\n');
        if (is_p_formatting())
        {
                    delformating();
                    c_out('\n');
        }
        if (!strcasecmp(align,"right")) addformating(PALIGN_RIGHT); else
        if (!strcasecmp(align,"left"))  addformating(PALIGN_LEFT); else
        if (!strcasecmp(align,"center")) addformating(PALIGN_CENTER); else
                addformating(PALIGN_NONE);
}

void decode_tag(char *tag)
{
        int n=0, z=0;
        char ntag[11];
        get_tag_piece(tag, &n, ntag, ' ', 10);
        if ((!strcasecmp(ntag,"/script") && (skip_all==SKIP_SCRIPT)) ||
            (!strcasecmp(ntag,"/style") && (skip_all==SKIP_STYLE)))
//          (!strcasecmp(ntag,"--") && (skip_all==SKIP_REM))
        {
                skip_all=SKIP_NONE;
                return;
        }
        if (skip_all) return;
        
        if (!strcasecmp(ntag,"pre"))
        {
                if (lineoutp) c_out('\n');
                addformating(ALIGN_PRE);
        } else
        if (!strcasecmp(ntag,"br") ||
            !strcasecmp(ntag,"dt") ||
            !strcasecmp(ntag,"tr") ||
            !strcasecmp(ntag,"blockquote") ||
            !strcasecmp(ntag,"/blockquote") ||
            !strcasecmp(ntag,"/title"))
                c_out('\n'); else
        if (!strcasecmp(ntag,"td"))
                c_out(' '); else
        if (!strcasecmp(ntag,"li"))
                s_out("*\1"); else
        if (!strcasecmp(ntag,"dd"))
                s_out("\n\1\1"); else
        if (!strcasecmp(ntag,"p"))
                tag_P(tag, &n); else
        if (!strcasecmp(ntag,"/p"))
        {
                c_out('\n');
                if (is_p_formatting()) delformating();
        } else
        if (!strcasecmp(ntag,"hr"))
                tag_HR(tag, &n); else
        if (!strcasecmp(ntag,"img"))
                tag_IMG(tag, &n); else
        if (!strcasecmp(ntag,"center")) 
        {
                c_out('\n');
                addformating(ALIGN_CENTER);
        } else
        if (!strcasecmp(ntag,"/center") ||
            !strcasecmp(ntag,"/div") ||
            !strcasecmp(ntag,"/pre") ||
            !strcasecmp(ntag,"/h1") ||
            !strcasecmp(ntag,"/h2") ||
            !strcasecmp(ntag,"/h3") ||
            !strcasecmp(ntag,"/h4") ||
            !strcasecmp(ntag,"/h6") ||
            !strcasecmp(ntag,"/h5"))
        {
                c_out('\n');
                delformating();
        } else
        if (!strcasecmp(ntag,"div") ||
            !strcasecmp(ntag,"h1") ||
            !strcasecmp(ntag,"h2") ||
            !strcasecmp(ntag,"h3") ||
            !strcasecmp(ntag,"h4") ||
            !strcasecmp(ntag,"h5") ||
            !strcasecmp(ntag,"h6"))
                tag_DIV(tag, &n); else
        if (!strcasecmp(ntag,"script"))
                skip_all=SKIP_SCRIPT; else
        if (!strcasecmp(ntag,"style"))
                skip_all=SKIP_STYLE; else
        if (!strcasecmp(ntag,"q"))
        {
                c_out(quote_type?'`':'"');
                quote_type=1-quote_type;
        } else
        if (!strcasecmp(ntag,"/q"))
        {
                quote_type=1-quote_type;
                c_out(quote_type?'\'':'"');
        } else
        if (!strcasecmp(ntag,"/dl"))
                s_out("\n\n");
}

void decode_amp(char *tag)
{
        if (!strcasecmp(tag,"nbsp"))
                c_out('\1'); else
        if (!strcasecmp(tag,"gt"))
                c_out('>'); else
        if (!strcasecmp(tag,"lt"))
                c_out('<'); else
        if (!strcasecmp(tag,"amp"))
                c_out('&'); else
        if (!strcasecmp(tag,"copy"))
                s_out("(C)"); else
        if (!strcasecmp(tag,"reg"))
                s_out("(R)"); else
        if (!strcasecmp(tag,"#149"))
                s_out("\1\1"); else
        if (!strcasecmp(tag,"#39"))
                c_out('\''); else
        if (tag[0])
        {
                if (!strcasecmp(tag+1,"acute") ||
                    !strcasecmp(tag+1,"uml") ||
                    !strcasecmp(tag+1,"tilde") ||
                    !strcasecmp(tag+1,"ring") ||
                    !strcasecmp(tag+1,"grave") ||
                    !strcasecmp(tag+1,"circ"))
                        c_out(tag[0]);
        }
}
