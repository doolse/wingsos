/*--------------------------------------------------------------------------*\
 *  PROGRAM:  mvp - move with pattern, version 1.01
 *
 *  PURPOSE:  Rename a set of files according to a given pattern.
 *
 *  BLURB:    Are you tired of running into a situation where you have a
 *            bunch of files that you want to rename in the same way, but
 *            you have no tool to do that for you?  For instance, you might
 *            have a bunch of files 'data01.dat' to 'data99.dat' and you
 *            want to rename them to use four digits for the numbers.  Well,
 *            now you can.  This program will allow you to make simple
 *            changes to groups of filenames by allowing you to insert a
 *            common substring or cut one from a certain point in all of the
 *            filenames given.
 *
 *  WEB PAGE: http://www.pobox.com/~csbruce/unix/
 *
 *  HISTORY:
 *    DATE          PROGRAMMER       DESCRIPTION
 *    03-Oct-1997   Craig Bruce      Initial creation
\*--------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define VERSION  "mvp version 1.01 by Craig Bruce, 03-Oct-1996"
#define NAME_LEN 64
#define TRUE     (!0)
#define FALSE    (!1)
#define ERR_PATMATCH -1
#define ERR_PATINVAL -2
#define ERR_TOOCOMPL -3

int     main( int argc, char *argv[] );
void    UsageAbort( void );
int     GetNewName( char *filename, char *pattern, char *newname );
int     MoveFile( char *filename, char *newname );
int     CopyFile( char *filename, char *newname );

char   *progname;
int     verbose;
int     debug;
int     copyFiles;
int     preserveFiles;

/****************************************************************************/
int main( int argc, char *argv[] )
{
    int     i, j, err;
    char   *pattern;
    char   *filename;
    char    newname[NAME_LEN];

    verbose   = FALSE;
    debug     = FALSE;
    copyFiles = FALSE;
    preserveFiles = FALSE;
    progname  = argv[0];
    if (argc < 3) UsageAbort();
    pattern = argv[argc-1];
    for (i=1; i<argc-1; i++) {
        filename = argv[i];
        if (filename[0] == '-') {
            for (j=1; filename[j]!='\0'; j++) {
                switch( filename[j] ) {
                case 'v':
                    verbose = 1;
                    fprintf(stderr, "%s\n", VERSION);
                    break;
                case 'd':
                    debug = 1;
                    fprintf(stderr, "%s: debugging output activated\n",
                        progname);
                    break;
                default:
                    UsageAbort();
                    break;
                }
            }
        } else {
            err = GetNewName( filename, pattern, newname );
            if (err==ERR_PATMATCH) {
                fprintf(stderr,
                    "%s: filename \"%s\" does not match pattern \"%s\"\n",
                    progname, filename, pattern);
            } else if (err==ERR_PATINVAL) {
                fprintf(stderr, "%s: invalid pattern \"%s\", aborting\n",
                    progname, pattern);
                exit( 1 );
            } else if (err==ERR_TOOCOMPL) {
                fprintf(stderr, "%s: pattern \"%s\" too complicated, aborting\n"
                    ,progname, pattern);
                exit( 1 );
            } else {
                if (strcmp(filename, newname)==0) {
                    fprintf(stderr,
                        "%s: error, \"%s\": old and new names match\n",
                        progname, filename);
                } else {
                    MoveFile( filename, newname );
                }
            }
        }
    }
    return( 0 );
}

/****************************************************************************/
void UsageAbort( void )
{
    fprintf(stderr, "%s\n\n", VERSION);
    fprintf(stderr,
    "usage: %s [-v] filename ... 'renamePattern'\n\n", progname);
    fprintf(stderr,
    "ranamePattern: \"txt\" = match txt, \"*\" = wildcard - copy to end,\n");
    fprintf(stderr,
    "               \"*.ext\" = copy up to ext, \"?\" = match any char,\n");
    fprintf(stderr,
    "               \"+instxt+\" = insert text, \"%%cutpat%%\" = cut text,\n");
    fprintf(stderr,
    "               \"^\" = return to start\n\n");
    fprintf(stderr,
  "examples: mvp hse1.jpeg hse2.jpeg hse13.jpeg '%%hse%%+house+*.jpeg+.jpg+\n");
    fprintf(stderr,
    "          mvp *.ps '+pref+*'\n");
    fprintf(stderr,
    "          mvp file* '*+.dat+'\n");
    fprintf(stderr,
    "          mvp sample_??.dat 'sample_+00+*'\n");
    fprintf(stderr,
    "          mvp a???_b???.dat '%%a???_%%b*.???+_+^a???+.dat+'\n");
    exit( 1 );
}

/****************************************************************************/
int MoveFile( char *filename, char *newname )
{
    int err;
    FILE *fp;

    if (verbose) {
        fprintf(stderr, "Moving \"%s\" to \"%s\"...\n", filename, newname);
    }
    fp = fopen( newname, "r" );
    if (fp != NULL) {
        fprintf(stderr, "%d: destination file \"%s\" already exists, skipping\n"
            ,progname, newname);    
        fclose( fp );
        return( -1 );
    }
    err = rename(filename, newname);
    if (err) {
        fprintf(stderr, "%d: error renaming \"%s\" to \"%s\"\n", progname,
            filename, newname);
    }
    return( err );
}

/****************************************************************************/
int CopyFile( char *filename, char *newname )
{
    if (verbose) {
        fprintf(stderr, "Copying \"%s\" to \"%s\"...\n", filename, newname);
    }
    return( 0 );
}

/****************************************************************************/
#define STATE_LITERAL 1
#define STATE_INSERT  2
#define STATE_SKIP    3

int GetNewName( char *filename, char *pattern, char *newname )
{
    int     inDex, outDex, patDex, state, matchForward;
    int     pch;
    char    backMatchBuf[NAME_LEN];
    int     backMatchDex;

    inDex = 0;
    outDex = 0;
    patDex = 0;
    state = STATE_LITERAL;
    matchForward = TRUE;
    while ( (pch=pattern[patDex]) != '\0' || !matchForward) {
        if (!matchForward) {
            if (pch=='+' || pch=='%' || pch=='^' || pch=='\0') {
                if (backMatchDex != 0) {
                    if (backMatchDex > inDex) return( ERR_PATMATCH );
                    for (backMatchDex-=1,inDex-=1; backMatchDex>=0;
                            backMatchDex-=1,inDex-=1) {
                        if (backMatchBuf[backMatchDex] != filename[inDex] &&
                                backMatchBuf[backMatchDex]!='?') {
                            return( ERR_PATMATCH );
                        }
                        if (state==STATE_LITERAL) {
                            outDex -= 1;
                        }
                    }
                    inDex += 1;
                }
                matchForward = TRUE;
                if (pch=='\0') break;
            } else {
                if (pch=='*') return( ERR_TOOCOMPL );
                backMatchBuf[backMatchDex++] = pch;
                if (debug) {
                    backMatchBuf[backMatchDex] = '\0';
                    fprintf(stderr, "  backMatchBuf=\"%s\"\n", backMatchBuf);
                }
                pch = -1;
            }
        }
        switch( pch ) {
        case -1: break;
        case '+':
            if (state==STATE_INSERT) {
                state = STATE_LITERAL;
            } else {
                if (state != STATE_LITERAL) return( ERR_PATINVAL );
                state = STATE_INSERT;
            }
            break;
        case '%':
            if (state==STATE_SKIP)  {
                state = STATE_LITERAL;
            } else {
                if (state != STATE_LITERAL) return( ERR_PATINVAL );
                state = STATE_SKIP;
            }
            break;
        case '^':
            if (state==STATE_INSERT) return( ERR_PATINVAL );
            inDex = 0;
            break;
        case '*':
            if (state==STATE_INSERT) return( ERR_PATINVAL );
            if (state==STATE_SKIP) {
                inDex = strlen( filename );
            } else {
                while (filename[inDex] != '\0') {
                    newname[outDex++] = filename[inDex++];
                }
            }
            matchForward = FALSE;
            backMatchDex = 0;
            break;
        case '?':
            pch = filename[inDex];
            /*fall through*/
        default:
            newname[outDex] = pch;
            switch( state ) {
            case STATE_LITERAL:
                if (filename[inDex] != pch) return( ERR_PATMATCH );
                inDex += 1;
                outDex += 1;
                break;
            case STATE_INSERT:
                outDex += 1;
                break;
            case STATE_SKIP:
                if (filename[inDex] != pch) return( ERR_PATMATCH );
                inDex += 1;
                break;
            }
            break;
        }
        patDex += 1;
        if (debug) {
            newname[outDex] = '\0';
            printf("patDex=%d, inDex=%d, outDex=%d, newname=\"%s\"\n", patDex,
                inDex, outDex, newname);
        }
    }
    newname[outDex] = '\0';
    if (state != STATE_LITERAL) return( ERR_PATINVAL );
    return( 0 );
}

/*--------------------------------------------------------------------------*\
 *  END OF PROGRAM: mvp.c
\*--------------------------------------------------------------------------*/
