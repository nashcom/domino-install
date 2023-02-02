# domino-install
Domino install &amp; configuration tools

#Introduction

This repository is intended for HCL Domino setup and configuration related community tools.


## Domino Auto configuration

Small helper tool to replace `{{ placeholders }}` in JSON files.

The Domino start script and the HCL Container provide the same functionality based on bash.
**autocfg** implements similar functionality in a small C++ application available on Windows and Linux.


## Domino Add-On Installer Framework

This sub project provides a simple, standardized way to package and install Domino applications requiring an OS level install on Windows
(server tasks, extension manager, Java files, OSGI related tools.

A small C++ task copies extracted files from a standardized directory structure.
An install.ini provides meta information about each application (e.g. name, version, description) and writes and install record into a well defined directory in the Domino binary directory.



