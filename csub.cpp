#include <stdio.h>
#include <process.h>
#include <string.h>

#include <string>

using std::string;



// Help Information
static auto usage =
    "\ncsub v1.0.1 / https://github.com/hollasch/csub / Steve Hollasch\n\n"
    "csub:  Perform command-substitution on a given command.\n"
    "usage: csub <command> `<expr>` <string> ... `<expr>` <string> ...\n"
    "\n";


void trimTailWhitespace (string &s) {
    // Removes select trailing whitespace characters from the end of the string.
    auto lastGood = s.find_last_not_of (" \r\n\t");
    if (lastGood != string::npos)
        s.erase (lastGood + 1);
}


//--------------------------------------------------------------------------------------------------
// Main Routine
//--------------------------------------------------------------------------------------------------

int main (int argc, char* argv[])
{
    bool debug = false;

    if (argc < 2) {
        fprintf (stderr, usage);
        return 0;
    }

    int argStart = 1;

    if (0 == _stricmp (argv[1], "-d")) {
        debug = true;
        argStart = 2;
    }

    string command;

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
            command += '`';
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
                trimTailWhitespace (command);
                command += ' ';
            }

            _pclose (expr);

            trimTailWhitespace (command);
        }

        ++linePtr;
    }

    if (debug) {
        printf ("Resulting command is %zd characters.\n", command.length());
        printf ("%s\n", command.c_str());
    }

    return system (command.c_str());
}