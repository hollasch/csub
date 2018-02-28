#include <stdio.h>
#include <process.h>
#include <string.h>



auto usage =
    "\ncsub v1.0.1 / https://github.com/hollasch/csub / Steve Hollasch\n\n"
    "csub:  Perform command-substitution on a given command.\n"
    "usage: csub <command> `<expr>` <string> ... `<expr>` <string> ...\n"
    "\n";


class String {
  public:

    String (void) {
        buff = new char [blockSize];
        buffSize = blockSize;
        end = buff;
    }

    String& append (const char* string, size_t len) {
        while ((end - buff + len) >= buffSize) {
            buffSize += blockSize;
            char* newBuff = new char [buffSize];

            if (!newBuff) {
                fprintf (stderr, "csub: Out of memory.\n");
                exit (1);
            }

            memcpy (newBuff, buff, end-buff+1);
            end = newBuff + (end-buff);
            delete buff;
            buff = newBuff;
        }

        memcpy (end, string, len+1);
        end += len;

        return *this;
    }

    String& operator+= (const char* string) {
        return append (string, strlen(string));
    }

    size_t length (void) { return end - buff; }

    void trim (const char* trimStr) {
        while (end > buff) {
            if (0 == strspn (end-1, trimStr))
                break;
            *--end = 0;
        }
    }

    char* value (void) { return buff; }

  private:

    const int blockSize = 8 << 10;

    char*         buff;
    unsigned int  buffSize;
    char*         end;
};

bool debug = false;



/*****************************************************************************
Main Routine
*****************************************************************************/

int main (int argc, char* argv[])
{
    if (argc < 2) {
        fprintf (stderr, usage);
        return 0;
    }

    int argStart = 1;

    if (0 == _stricmp (argv[1], "-d")) {
        debug = true;
        argStart = 2;
    }

    // Command Buffer Size. Later one we might make this dynamic or configurable.

    String command;

    size_t argsLen = 0;

    for (auto i=argStart;  i < argc;  ++i)
        argsLen += 1 + strlen(argv[i]);

    char* cmdLine = new char [argsLen];

    strcpy_s (cmdLine, argsLen, argv[argStart]);

    for (auto i=argStart+1;  i < argc;  ++i) {
        strcat_s (cmdLine, argsLen, " ");
        strcat_s (cmdLine, argsLen, argv[i]);
    }

    char* linePtr = cmdLine;

    while (*linePtr) {
        // Seek to next backquote.

        char* nextExpr = strchr (linePtr, '`');

        // If no more backquote expressions, add the remainder of the command and break out.

        if (!nextExpr) {
            command += linePtr;
            break;
        }

        // If the backquote occurs farther ahead in the command string, copy the characters leading
        // up to it.

        if (nextExpr != linePtr) {
            command.append (linePtr, nextExpr - linePtr);
        }

        // Set lineptr to the closing backquote.

        linePtr = strchr (nextExpr+1, '`');

        // If there's no matching backquote, then error.

        if (!linePtr) {
            fprintf (stderr, "Error:  Mismatched ` quotes.\n");
            return 1;
        }

        // If there are two backquotes in a row, then insert a true backquote, otherwise insert the
        // value of the evaluated expression.

        if (1 == (linePtr - nextExpr)) {
            command += "`";
        } else {
            // At this point, linePtr points to the closing backquote. Evaluate the expression
            // between nextExpr and linePtr.

            *linePtr = 0;

            FILE* expr = _popen (++nextExpr, "rt");

            if (!expr) {
                fprintf (stderr,
                        "Error:  Couldn't open pipe for \"%s\".", nextExpr);
                return errno;
            }

            while (!feof(expr)) {
                char inbuff [8<<10];

                if (!fgets (inbuff, sizeof(inbuff), expr))
                    break;

                command += inbuff;
                command.trim ("\r\n");
                command += " ";
            }

            _pclose (expr);

            command.trim (" ");
        }

        ++linePtr;
    }

    if (debug) {
        printf ("Resulting command is %zd characters.\n", command.length());
        printf ("%s\n", command.value());
    }

    return system (command.value());
}