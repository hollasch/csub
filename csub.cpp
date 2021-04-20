//==================================================================================================
// csub
//
// A Windows command-line tool to implement Unix-like command substitution (back-tick expansion).
//==================================================================================================

#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>

using namespace std;


// Help Information

static auto version = L"csub v1.2.0 | 2021-04-19 | https://github.com/hollasch/csub";

static auto usage = LR"(
csub:  Command-substitution on Windows
usage: csub [options] <command> [`<expr>`|<string>]...

Description
    csub runs the given command with expanded arguments from the command line.

    Any string enclosed in back quotes is executed, its output concatenated with
    single spaces, and the result inserted back into the full command. Multiple
    backtick expressions are supported.

    Arguments not enclosed in back quotes are included in the command verbatim.

Example
    csub echo Text files: `dir /b *.txt`
        Produces the output "Text files: CMakeLists.txt LICENSE.txt".

    csub copy `where notes.cmd` %TEMP%
        Locate script notes.cmd and copy it into the %TEMP% directory.

Options
    Note: Options must be supplied before expressions and literal arguments.

    -d, --debug
        Print debugging information.

    -h, --help
        Print help information.

    -v, --version
        Print version information.
)";


// Command Options

struct ProgramParameters {
    bool help         { false };
    bool printVersion { false };
    bool debug        { false };

    wstring command;
};


//--------------------------------------------------------------------------------------------------
void trimTailWhitespace (wstring &s) {
    // Removes select trailing whitespace characters from the end of the string.
    auto lastGood = s.find_last_not_of (L" \r\n\t");
    if (lastGood != wstring::npos)
        s.erase (lastGood + 1);
}


//--------------------------------------------------------------------------------------------------
bool equal(wchar_t* a, wchar_t* b) {
    return 0 == wcscmp(a,b);
}


//--------------------------------------------------------------------------------------------------
const wchar_t* toString(bool x) {
    return x ? L"true" : L"false";
}

//--------------------------------------------------------------------------------------------------
bool ParseParameters (ProgramParameters &params, int argc, wchar_t* argv[]) {

    int argi;
    for (argi=1;  argi < argc;  ++argi) {
        auto arg = argv[argi];

        if (equal(arg, L"-d") || equal(arg, L"--debug")) {
            params.debug = true;
            continue;
        }

        if (equal(arg, L"-h") || equal(arg, L"--help")) {
            params.help = true;
            return true;
        }

        if (equal(arg, L"-v") || equal(arg, L"--version")) {
            params.printVersion = true;
            continue;
        }

        break; // End of options
    }

    // Build up command string

    params.command.clear();
    auto firstChunk = true;

    for (; argi < argc; ++argi) {
        if (firstChunk)
            firstChunk = false;
        else
            params.command += L' ';

        params.command += argv[argi];
    }

    return true;
}


//--------------------------------------------------------------------------------------------------
int wmain (int argc, wchar_t* argv[])
{
    ProgramParameters params;

    if (!ParseParameters(params, argc, argv))
        exit(1);

    if (params.debug) {
        wcout << L"params.help        : " << toString(params.help) << L'\n';
        wcout << L"params.debug       : " << toString(params.debug) << L'\n';
        wcout << L"params.printVersion: " << toString(params.printVersion) << L'\n';
        wcout << L"params.command     : " << params.command << L"'\n\n";
    }

    if (params.printVersion) {
        wcout << version << L'\n';
        exit(0);
    }

    if (params.help || params.command.empty()) {
        wcout << usage << L'\n' << version << L'\n';
        exit(0);
    }

    int argStart = 1;

    wstring command;    // Resulting command after substitution.

    auto cmdIt = params.command.begin();

    // Scan through the command line.
    while (true) {

        // Copy up to next backquote.
        while ((cmdIt != params.command.end()) && (*cmdIt != L'`')) {
            command += *cmdIt++;
        }

        // If done with command line, break out to execute.
        if (cmdIt == params.command.end()) break;

        ++cmdIt;

        // A double back-tick yields a backtick. After that, continue collecting the command.
        if (*cmdIt == L'`') {
            command += *cmdIt++;
            continue;
        }

        wstring expression;   // Back-tick command substitution expression.

        // Copy the sub-expression between back ticks.
        while ((cmdIt != params.command.end()) && (*cmdIt != L'`')) {
            expression += *cmdIt++;
        }

        if (cmdIt == params.command.end()) {
            fputws (L"Error:  Mismatched ` quotes.\n", stderr);
            return 1;
        }
        ++cmdIt;

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

    auto commandString = command.c_str();

    if (params.debug) {
        wcout << L"Expanded command: " << commandString << "\n----------------------------------------\n\n";
    }

    return _wsystem (commandString);
}
