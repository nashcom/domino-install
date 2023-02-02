# Domino Add-On Installer Framework

This sub project provides a simple, standardized way to package and install Domino applications requiring an OS level install on Windows
(server tasks, extension manager, Java files, OSGI related tools.

A small C++ task copies extracted files from a standardized directory structure.
An install.ini provides meta information about each application (e.g. name, version, description) and writes and install record into a well defined directory in the Domino binary directory.


# Background

Most Domino add-on software does not provide an installer and is just shipped as a ZIP/tar file.
Usually an installer on Windows is a signed executable acting as a trusted container, which can be verified by Windows.
To automatically install add-on applications those applications must be packaged into a trusted installer which can be silently deployed. 

In addition this standardized process allows to query and inventory installed add-on software by the new Domino Auto Install functionality.
The following proposed solution would provide a simple to use, flexible and standardized framework to deploy add-on applications without the need of a complex installer. 


# Add-On Installer Framework

The add-on installer framework consists of the following components 

- Small dominstall.exe C++ based helper binary containing install logic 
- 7Zip self execution SDK --> https://sevenzip.osdn.jp/chm/cmdline/switches/sfx.htm  (we would just need a single small file from the SDK. license is LGPL, which works well for us)
- **install.ini** containing information about the product to be installed 


## install.ini

Each package contains an **install.ini** to provide information about the package.
The information is used during installation. But also remains in an install directory, to allow to query for installed software and to uninstall the add-on. 

```
name=nomadweb-server 
vendor=HCL 
contact=support@pnp-hcl.com 
version=1.0.6 
domino-minver=12.0.1 
domino-maxver=12.0.2
custom-install=
uninstall=
...
```


# Install Logic and Flow 


1. The 7Zip self extractor extracts all software into a temporary directory 
2. The specified `RunProgram="dominstall.exe"` is executed from the extract location automatically 
3. **dominstall** reads **install.ini** and verifies version information 
4. **dominstall** deploys the defined directory structure to the Domino server 
5. A log file is written for each installed file to keep track of the installed software **app_name/install.log** -- e.g. `nomadweb-server/install.log`).
6. **install.ini** is copied into an application specific file **app_name/install.ini** -- e.g. `nomadweb-server/install.ini`)
7. A log file is generated listed all files installed e.g. **app_name/files.log**
8. 7Zip installer removes temporary extracted files 



The 7Zip auto extractor framework is based on the widely used free and open source 7Zip application.
It supports customization leveraging a simple configuration. 
The archive containing the software to deploy, `config.txt` and a small self extracting binary is combined into a self extracting binary in the following way: 

```
copy /b 7zSD.sfx + config.txt + software.7z add-on-install.exe 
```

## 7zip config.txt

The configuration file is a simple text file providing a mechanism to prompt for installation and also provides a silent install option. 
To launch the **dominoinstall.exe* installer, the `RunProgram` command is used.
7zip extracts files automatically to a temporary location and invokes the specified binary.

## Example config.txt

```
;!@Install@!UTF-8! 
Title="HCL Nomad 1.0.6" 
BeginPrompt="Do you want to install HCL Nomad?" 
RunProgram="dominstall.exe" 
;!@InstallEnd@! 
```

The resulting binary can be signed in the following way.
Also the install application should be signed.


# Sign Windows binaries

To sign a Windows binary a signing certificate is required.
A test signing certificate can be created using the following type of command

```
MakeCert -r -pe -ss PrivateCertStore -n "CN=AcmeTestSign" acme_testcert.cer
```

The resulting private key and certificate is stored in the specified Windows trust store and can be exported into a PFX file using the Windows. 
Open the Windows trust store to export to pfx (run certmgr.exe)

The resulting signer can be used to sign binaries in the following way

```
signtool sign /f acme_signer.pfx /p notes /fd SHA256 dominstall.exe
```

Once signed the binary properties show a separate Digital Signatures tab (see appended screen shots). 
The signature can be also verified and displayed using the signing tool: 

```
signtool verify /a /debug dominstall.exe
```


# Domino add-on installer logic

The installer is mainly intended to copy single files like server tasks and also whole directories into the right directories on a Domino server. 
To provide software to deploy the files are copied into the defined directory structure listed below.
The main directories used are "domino-bin" and "domino-data". 


## Supported Directories

The following directory locations are supported for deployment:

- domino-bin
- domino-data

- domino-res
- domino-osgi-dots
- domino-jvm
- domino-xsp
- domino-osgi


# How to run

**dominoinstall** is invoked from the directory where the `install.ini` is located.

The main purpose of the application is to execute the install operation.
The **list** option queries all installed software and displays application name and version.
The **remove** option is planned to uninstall the application


## Options

The following options can be used to for installation and related commands

| Option      | Description |
| :---------- | :---------- |
|`-name=<application name>` | Name of application for remove or status |
|`-data=<data dir>`         | Explicit data directory override |
|`-bin=<binary dir>`        | Explicit binary directory override |
|`-wait=<seconds>`          | Wait number of seconds before stopping the program (to show output) |

## Commands

The following commands are additionally supported

| Command     | Description |
| :---------- | :---------- |
|`-list`  |                  List Installed software|
|`-remove`|                  Remove/uninstall application|



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


