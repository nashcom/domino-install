# HowTo Create a HCL Nomad Server installer

This tutorial uses the HCL Nomad Server ZIP downloaded from Flexnet and turns it into an automated installer to walk you to complete all required steps to create an automated installer with this project.

Required for this tutorial

- A Flexnet account with a maintenance subscription is required download and use the software.
- An installed 7Zip application (64bit) available from https://7-zip.org
- The Self Extractor helper binary **7zSD.sfx** available from 7Zip LZMA SDK https://7-zip.org/sdk.html

Note: Packages created based on software downloaded from should not be distributed outside your organization!

## Download HCL Nomad Server for Domino 12.0.2 from Flexnet

Log into your HCL Flexnet account and download the following software package

```
nomad-server-1.0.6-for-domino-1202-win64.zip
```

## Create an empty directory

Create new directory to store all files required to build the package

```
c:\
mkdir c:\build-nomad-installer
cd c:\build-nomad-installer

```

## Extact 7zSD.sfx into install directory

Download 7Zip LZMA SDK https://7-zip.org/sdk.html and extract **7zSD.sfx** into install directory


```
copy 7zSD.sfx c:\build-nomad-installer

```


## Create install.ini

Create a install.ini file

```
name=nomadweb-server
vendor=HCL
version=1.0.6
```

## Create config.txt

Create a config.txt file

```
;!@Install@!UTF-8!
Title="HCL Nomad 1.0.6"
BeginPrompt="Do you want to install HCL Nomad 1.0.6?"
RunProgram="dominstall.exe"
;!@InstallEnd@!
```

## Extract Nomad installer files from ZIP into domino-bin directory

The **domino-bin** contains all binary files (including sub directories) automatically copied into the Domino binary directory.

```
"c:\Program Files\7-Zip\7z.exe" x nomad-server-1.0.6-for-domino-1202-win64.zip -odomino-bin
```

## Repack Nomad Server files

The following files are combined into a single ZIP file

- install.ini
- dominstall.exe
- domino-bin


```
"c:\Program Files\7-Zip\7z.exe" a nomad.7z -mx1 install.ini dominstall.exe domino-bin
```

## Merge all files into a single executable

Finally combine the following components into a single executable file

- **7zSD.sfx** self extractor code
- **config.txt** configuation file
- **nomad.7z** Nomad software including install.ini and the installer


```
copy /b 7zSD.sfx + config.txt + nomad.7z nomad-server-1.0.6-for-domino-1202-win64.exe
```

## Run the new installer

```
nomad-server-1.0.6-for-domino-1202-win64.exe -wait=20
```

The installer can be started without any parameters.
Out of the box the standard SFX uses the prompt added to **config.txt**

To silently run the installer specify `-y`

Additional parameters are send to the **RunProgram** specified.
This functionality is an unique feature of 7Zip to allow to pass parameters to **dominstall.exe**

For testing the option `-wait=20` would be helpful to see messages logged to the output window.
The window automatically closes when the installer terminates.

The complete log is available in the log file written to .install-reg directory below the Domino binary directory

The resulting executable file should be signed using the Microsoft signing tool with a valid signing certificate.

Note: In Nomad Server 1.0.6 executables are not signed.
For a production installer image you might want to manually repackage the files and sign all executables with the same signing certificate.

