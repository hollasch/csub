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


//__________________________________________________________________________________________________
void trimTailWhitespace (string &s) {
    // Removes select trailing whitespace characters from the end of the string.
    auto lastGood = s.find_last_not_of (" \r\n\t");
    if (lastGood != string::npos)
        s.erase (lastGood + 1);
}


//__________________________________________________________________________________________________
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

    string command;    // Resulting command after substitution.
    string cmdLine;    // Full csub command line, all arguments concatenated.

    for (auto i=argStart;  i < argc;  ++i) {
        if (i > argStart) cmdLine += ' ';
        cmdLine += argv[i];
    }

    auto lineIt = cmdLine.begin();

    // Scan through the command line.
    while (true) {

        // Copy up to next backquote.
        while ((lineIt != cmdLine.end()) && (*lineIt != '`')) {
            command += *lineIt++;
        }

        // If done with command line, break out to execute.
        if (lineIt == cmdLine.end()) break;

        ++lineIt;

        // A double back-tick yields a backtick. After that, continue collecting the command.
        if (*lineIt == '`') {
            command += *lineIt++;
            continue;
        }

        string expression;   // Back-tick command substitution expression.

        // Copy the sub-expression between back ticks.
        while ((lineIt != cmdLine.end()) && (*lineIt != '`')) {
            expression += *lineIt++;
        }

        if (lineIt == cmdLine.end()) {
            fprintf (stderr, "Error:  Mismatched ` quotes.\n");
            return 1;
        }
        ++lineIt;

        // Execute the back-tick expression.
        FILE* expr = _popen (expression.c_str(), "rt");

        if (!expr) {
            fprintf (stderr, "Error:  Couldn't open pipe for \"%s\".", expression.c_str());
            return errno;
        }

        // Read in lines of the resulting output.
        while (!feof(expr)) {
            char inbuff [8<<10];

            if (!fgets (inbuff, sizeof(inbuff), expr)) break;

            command += inbuff;
            trimTailWhitespace (command);   // Lop off whitespace (including newlines).
            command += ' ';
        }

        _pclose (expr);

        trimTailWhitespace (command);
    }

    if (debug) {
        printf ("Expanded command: \"%s\"\n", command.c_str());
    }

    return system (command.c_str());
}