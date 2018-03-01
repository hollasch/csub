#include <stdio.h>
#include <process.h>
#include <string.h>

#include <string>

using std::wstring;



// Help Information
static auto usage =
    L"\ncsub v1.0.1 / https://github.com/hollasch/csub / Steve Hollasch\n\n"
    L"csub:  Perform command-substitution on a given command.\n"
    L"usage: csub <command> `<expr>` <string> ... `<expr>` <string> ...\n"
    ;


//__________________________________________________________________________________________________
void trimTailWhitespace (wstring &s) {
    // Removes select trailing whitespace characters from the end of the string.
    auto lastGood = s.find_last_not_of (L" \r\n\t");
    if (lastGood != wstring::npos)
        s.erase (lastGood + 1);
}


//__________________________________________________________________________________________________
int wmain (int argc, wchar_t* argv[])
{
    bool debug = false;

    if (argc < 2) {
        fputws (usage, stderr);
        return 0;
    }

    int argStart = 1;

    if (0 == _wcsicmp (argv[1], L"-d")) {
        debug = true;
        argStart = 2;
    }

    wstring command;    // Resulting command after substitution.
    wstring cmdLine;    // Full csub command line, all arguments concatenated.

    for (auto i=argStart;  i < argc;  ++i) {
        if (i > argStart) cmdLine += L' ';
        cmdLine += argv[i];
    }

    auto lineIt = cmdLine.begin();

    // Scan through the command line.
    while (true) {

        // Copy up to next backquote.
        while ((lineIt != cmdLine.end()) && (*lineIt != L'`')) {
            command += *lineIt++;
        }

        // If done with command line, break out to execute.
        if (lineIt == cmdLine.end()) break;

        ++lineIt;

        // A double back-tick yields a backtick. After that, continue collecting the command.
        if (*lineIt == L'`') {
            command += *lineIt++;
            continue;
        }

        wstring expression;   // Back-tick command substitution expression.

        // Copy the sub-expression between back ticks.
        while ((lineIt != cmdLine.end()) && (*lineIt != L'`')) {
            expression += *lineIt++;
        }

        if (lineIt == cmdLine.end()) {
            fputws (L"Error:  Mismatched ` quotes.\n", stderr);
            return 1;
        }
        ++lineIt;

        // Execute the back-tick expression.
        FILE* exprOutput = _wpopen (expression.c_str(), L"rt");

        if (!exprOutput) {
            fwprintf (stderr, L"Error:  Couldn't open pipe for \"%s\".", expression.c_str());
            return errno;
        }

        // Read in lines of the resulting output.
        while (!feof(exprOutput)) {
            wchar_t inbuff [8<<10];

            if (!fgetws (inbuff, static_cast<int>(std::size(inbuff)), exprOutput)) break;

            // Copy the line of output to the final command string, escaping special characters.
            for (auto inPtr=inbuff;  *inPtr;  ++inPtr) {
                if (wcschr (L"&<>()[]{}^=;!'+,`~", *inPtr)) command += '^';
                command += *inPtr;
            }

            trimTailWhitespace (command);   // Lop off whitespace (including newlines).
            command += L' ';
        }

        _pclose (exprOutput);

        trimTailWhitespace (command);
    }

    if (debug) {
        wprintf (L"Expanded command: \"%s\"\n", command.c_str());
    }

    return _wsystem (command.c_str());
}