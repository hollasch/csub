#include <stdio.h>
#include <errno.h>
#include <process.h>
#include <string.h>


char usage[] =
"\ncsub / 1997-June-05 / Steve Hollasch\n\n"
"csub:  Perform command-substitution on a given command.\n"
"usage: csub <command> `<expr>` <string> ... `<expr>` <string> ...\n"
"\n";


class String
{
  public:

    String (void)
    {
        buff = new char [blocksize];
        buff_size = blocksize;
        end = buff;
    }

    String& Append (char *string, int len)
    {
        while ((end - buff + len) >= buff_size)
        {
            buff_size += blocksize;
            char *newbuff = new char [buff_size];

            if (!newbuff)
            {   fprintf (stderr, "csub: Out of memory.\n");
                exit (1);
            }

            memcpy (newbuff, buff, end-buff+1);
            end = newbuff + (end-buff);
            delete buff;
            buff = newbuff;
        }

        memcpy (end, string, len+1);
        end += len;

        return *this;
    }

    String& operator+= (char *string)
    {   return Append (string, strlen(string));
    }

    int Length (void) { return end - buff; }

    void Trim (char *trim)
    {
        while (end > buff)
        {
            if (0 == strspn (end-1, trim))
                break;
            *--end = 0;
        }
    }

    char *Value (void) { return buff; }

  private:

    static const int blocksize;

    char         *buff;
    unsigned int  buff_size;
    char         *end;
};

const int String::blocksize = 8 << 10;
bool debug = false;



/*****************************************************************************
Main Routine
*****************************************************************************/

int main (int argc, char *argv[])
{
    if (argc < 2)
    {   fprintf (stderr, usage);
        return 0;
    }

    int argstart = 1;

    if (0 == stricmp (argv[1], "-d"))
    {   debug = true;
        argstart = 2;
    }

    // Command Buffer Size.  Later one we might make this dynamic or
    // configurable.

    String command;

    int argslen = 0;

    int i;
    for (i=argstart;  i < argc;  ++i)
        argslen += 1 + strlen(argv[i]);

    char *cmdline = new char [argslen];

    strcpy (cmdline, argv[argstart]);

    for (i=argstart+1;  i < argc;  ++i)
    {   strcat (cmdline, " ");
        strcat (cmdline, argv[i]);
    }

    char *lineptr = cmdline;

    while (*lineptr)
    {
        // Seek to next backquote.

        char *nextexpr = strchr (lineptr, '`');

        // If no more backquote expressions, add the remainder of the command
        // and break out.

        if (!nextexpr)
        {   command += lineptr;
            break;
        }

        // If the backquote occurs farther ahead in the command string, copy
        // the characters leading up to it.

        if (nextexpr != lineptr)
        {   command.Append (lineptr, nextexpr - lineptr);
        }

        // Set lineptr to the closing backquote.

        lineptr = strchr (nextexpr+1, '`');

        // If there's no matching backquote, then error.

        if (!lineptr)
        {   fprintf (stderr, "Error:  Mismatched ` quotes.\n");
            return 1;
        }

        // If there are two backquotes in a row, then insert a true backquote,
        // otherwise insert the value of the evaluated expression.

        if (1 == (lineptr - nextexpr))
        {   command += "`";
        }
        else
        {
            // At this point, lineptr points to the closing backquote.
            // Evaluate the expression between nextexpr and lineptr.

            *lineptr = 0;

            FILE *expr = _popen (++nextexpr, "rt");

            if (!expr)
            {   fprintf (stderr,
                        "Error:  Couldn't open pipe for \"%s\".", nextexpr);
                return errno;
            }

            while (!feof(expr))
            {
                char inbuff [8<<10];

                if (!fgets (inbuff, sizeof(inbuff), expr))
                    break;

                command += inbuff;
                command.Trim ("\r\n");
                command += " ";
            }

            _pclose (expr);

            command.Trim (" ");
        }

        ++lineptr;
    }

    if (debug)
    {   printf ("Resulting command is %d characters.\n", command.Length());
        printf ("%s\n", command.Value());
    }

    return system (command.Value());
}
