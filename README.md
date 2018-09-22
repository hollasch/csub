`csub`
================================================================================
Command Substitution for the Windows Command Shell


Summary
-------------
Most Unix-like shells support a technique called _command substitution_: a way to use the output of
one command in the middle of another command. The Windows standard shell (command prompt) lacks this
ability, but the `csub` command provides a simple way to mock this for most situations.


Usage
-------
    csub <command> [<arguments>] [`<sub-command>`]
                   [<arguments>] [`<sub-command>`] …


Description
-------------
In most shells, one can accomplish command substitution by quoting a command with backquotes like
so:

```sh
> echo It's a fine day in `hostname`-land.
It's a fine day in mycomputer03-land.
```

In other shells, like Bash, you can also accomplish this by enclosing the command in `$(…)`, like
this:

```sh
> echo It's a fine day in $(hostname)-land.
It's a fine day in mycomputer03-land.
```

The `csub` command provides a way to do this in the Windows command shell — anything in the `csub`
arguments between backquotes is executed and replaced with the output, like so:

```sh
C:\> csub echo It's a fine day in `hostname`-land.
It's a fine day in mycomputer03-land.
```


Limitations
-------------

### `csub` cannot change the environment at the calling level
The `csub` command is executed inside its own context, so commands that would change the environment
cannot change the environment of the caller. For example,

```sh
C:\qux\baz\frotz> csub cd `echo \qux`

C:\qux\baz\frotz> 
```

In the above example, you'd like the current directory to be `C:\qux` after running the command. The
`csub` command _does_ change the directory, but only it its own environment; it does not change the
environment at the calling level.


Building
----------
Currently, `csub` is a very simple C++ file compiled with Visual Studio's `nmake` tool.


Installation
--------------
The built executable is `csub.exe`, and can be copied anywhere to your command path. There is no
Windows installation required for this tool.


----------------------------------------------------------------------------------------------------
Steve Hollasch, steve@hollasch.net<br>
https://github.com/hollasch/csub
