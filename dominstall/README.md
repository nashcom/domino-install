# Domino Add-On Installer Framework

This sub project provides a simple, standardized way to package and install Domino applications requiring an OS level install on Windows
(server tasks, extension manager, Java files, OSGI related tools.

A small C++ task copies extracted files from a standardized directory structure.
An install.ini provides meta information about each application (e.g. name, version, description) and writes and install record into a well defined directory in the Domino binary directory.



