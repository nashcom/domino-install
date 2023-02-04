# HowTo Create a HCL Nomad Server installer

This tutorial uses the HCL Nomad Server ZIP downloaded from Flexnet and turns it into an automated installer to walk you to complete all required steps to create an automated installer with this project.

Required for this tutorial

- A Flexnet account with a maintenance subscription is required download and use the software.
- An installed 7Zip application (64bit) available from https://7-zip.org
- The Self Extractor helper binary 7zSD.sfx available from 7Zip LZMA SDK https://7-zip.org/sdk.html

Note: Packages created based on software downloaded from should not be distributed outside your organization!

## Download HCL Nomad Server for Domino 12.0.2 from Flexnet

Log into your HCL Flexnet account and download the following software package

nomad-server-1.0.6-for-domino-1202-win64.zip


## Create an empty directory

Create new directory to store all files required to build the package

```
c:\
mkdir c:\build-nomad-installer
cd c:\build-nomad-installer

```

## Extact 7zSD.sfx into install directory

Download 7Zip LZMA SDK https://7-zip.org/sdk.html and extract 7zSD.sfx into install directory


## Create install.ini

Create a install.ini file with the following content

```
name=nomadweb-server
vendor=HCL
version=1.0.6
```

## Create config.txt

Create a config.txt file with the following content

```
;!@Install@!UTF-8!
Title="HCL Nomad 1.0.6"
BeginPrompt="Do you want to install HCL Nomad 1.0.6?"
RunProgram="dominstall.exe"
;!@InstallEnd@!
```

## Extract Nomad installer files from ZIP into domino-bin directory

```
"c:\Program Files\7-Zip\7z.exe" x nomad-server-1.0.6-for-domino-1202-win64.zip -odomino-bin
```

## Repack Nomad Server files, install.ini and domino-install.exe into new ZIP file

```
"c:\Program Files\7-Zip\7z.exe" a nomad.7z -mx1 install.ini dominstall.exe domino-bin
```

## Merge all files into a single executable

```
copy /b 7zSD.sfx + config.txt + nomad.7z nomad-server-1.0.6-for-domino-1202-win64.exe
```

