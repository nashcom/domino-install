
# Domino Auto configuration

Small helper tool to replace `{{ placeholders }}` in JSON files.

The Domino start script and the HCL Container provide the same functionality based on bash.
**autocfg** implements similar functionality in a small C++ application available on Windows and Linux.

# How to run

```
autocfg.exe <JSON OTS template> <JSON OTS file> [-env=file] [-prompt]
```

Specify a OTS template file containing placeholders first.
And specify the also required resulting OTS JSON file with the replaced variables.

By default the application replaces placeholders with environment variables.

Optionally an environment file can specified via `-env` (similar to Docker)
The file can contain key value pairs separated by and `=` (like in Notes.ini)

Variables which cannot be resolved are left empty.

Optionally an interactive prompt to ask for missing parameters is invoked via `-prompt`


Example:


```
autocfg.exe ots_template.json ots_first_server.json -env=setup.ini -prompt
```



# How to build

## Windows

Tested with Microsoft Visual Studio 2017.

```
nmake -f mswin64.mak
```

## Linux

Tested with current Linux distributions.
You need g++ and make

```
make
```


