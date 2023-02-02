/*
###########################################################################
# Domino Add-On Install                                                   #
# Version 0.1 02.02.2023                                                  #
# (C) Copyright Daniel Nashed/NashCom 2023                                #
#                                                                         #
# Licensed under the Apache License, Version 2.0 (the "License");         #
# you may not use this file except in compliance with the License.        #
# You may obtain a copy of the License at                                 #
#                                                                         #
#      http://www.apache.org/licenses/LICENSE-2.0                         #
#                                                                         #
# Unless required by applicable law or agreed to in writing, software     #
# distributed under the License is distributed on an "AS IS" BASIS,       #
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.#
# See the License for the specific language governing permissions and     #
# limitations under the License.                                          #
###########################################################################
*/

#ifdef UNIX

#include <dirent.h>
#include <sys/statvfs.h>
#else

#include <windows.h>
#endif

#include <sys/stat.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "dominoinstall.hpp"

typedef struct
{
    char szName[1024];
    char szVersion[100];
    char szDescription[1024];
    char szVendor[1024];
} TYPE_SOFTWARE_INFO;

/* Globals */
FILE *g_fpLog = NULL;
FILE *g_fpFileInstallLog = NULL;

int  g_Debug  = 0;
int  g_CopiedFiles = 0;
int  g_CopyErrors = 0;

char g_InstallRegDirName[] = ".install-reg";

#ifdef UNIX
char g_OsDirSep = '/';
#else
char g_OsDirSep = '\\';
#endif


void strdncpy (char *s, const char *ct, size_t n)
{

    if (n>0)
    {
        strncpy (s, ct, n-1);
        s[n-1] = '\0';
    }
    else
    {
        s[0] = '\0';
    }
}


#ifdef UNIX
#else

    int NshGetRegValue (HKEY hMainKey, char *sub_key, char *value_name, char *data, DWORD data_size)
    {
        HKEY    hKey;
        DWORD   type;
        LONG    ret;
        DWORD   number;

        ret = RegOpenKeyEx (HKEY_LOCAL_MACHINE, sub_key, 0, KEY_READ, &hKey);

        if (ERROR_SUCCESS != ret) return 0;

        ret = RegQueryValueExA (hKey, value_name, NULL, &type, (LPBYTE) data, &data_size);

        RegCloseKey(hKey);

        if (ERROR_SUCCESS != ret) return 0;

        switch (type)
        {
            case REG_SZ:
                break;

            case REG_DWORD:
                number = (DWORD) (*data);
                sprintf (data, "%ld", number);
                break;

            default:
                *data = '\0';
                break;
        } /* switch */

        return 1;
    }

#endif


int BuildPath (char *retpszCombinedPath, size_t BufferSize, const char *pszDir, const char *pszFile)
{
    if (0 == BufferSize)
        return 0;

    if (NULL == retpszCombinedPath)
        return 0;

    *retpszCombinedPath = '\0';

    if (NULL == pszDir)
        return 0;

    if (NULL == pszFile)
        return 0;

    return snprintf (retpszCombinedPath, BufferSize-1, "%s%c%s", pszDir, g_OsDirSep, pszFile);
}

#ifdef UNIX

int CopyFilesFromDirectory (const char *pszDirectory, const char *pszTargetDir, int levels)
{
    int ret = 0;
    BOOL bFileCopied = FALSE;
    DIR *pDir = NULL;
    struct dirent *pEntry;

    pDir = opendir (pszDirectory);
    if (NULL == pDir)
        goto Done;

    while (NULL != (pEntry = readdir (pDir)) )
    {
        fprintf (g_fpLog, "%s\n", pEntry->d_name);
    }

Done:

    if (pDir)
    {
        closedir (pDir);
        pDir = NULL;
    }

    return ret;
}

#else

int create_directory (const char *pszDirectory)
{
    int ret = 0;

    CreateDirectory (pszDirectory, NULL);

    return ret;
}

int copy_file (const char *pszSourcePath, const char *pszTargetPath, BOOL bOverwrite)
{
    int ret = 0;

    BOOL bFileCopied = FALSE;
    bFileCopied = CopyFile (pszSourcePath, pszTargetPath, bOverwrite);

    return ret;
}

int CopyFileToTargetDir (const char *pszFileName, const char *pszSourceDir, const char *pszTargetDir)
{
    int ret = 0;

    char szSourcePath[1024] = {0};
    char szTargetPath[1024] = {0};

    BuildPath (szSourcePath, sizeof (szSourcePath), pszSourceDir, pszFileName);
    BuildPath (szTargetPath, sizeof (szTargetPath), pszTargetDir, pszFileName);

    ret = copy_file (szSourcePath, szTargetPath, TRUE);

    if (0 == ret)
    {
        g_CopiedFiles++;

        if (g_fpFileInstallLog)
            fprintf (g_fpFileInstallLog, "%s\n", szTargetPath);
    }
    else
    {
        g_CopyErrors++;

        if (g_fpFileInstallLog)
            fprintf (g_fpFileInstallLog, "ERROR:%s\n", szTargetPath);
    }

Done:
    return ret;
}

int CopyFilesFromDirectory (const char *pszDirectory, const char *pszTargetDir, int levels)
{
    int ret = 0;
    char szFindPath[1024]       = {0};
    char szNextTargetDir[1024]  = {0};

    if (levels <= 0)
        goto Done;

    WIN32_FIND_DATA file;
    HANDLE hSearch = NULL;

    BuildPath (szFindPath, sizeof (szFindPath), pszDirectory, "*");

    hSearch = FindFirstFile (szFindPath, &file);

    if (NULL == hSearch)
        goto Done;

    if (INVALID_HANDLE_VALUE == hSearch)
        goto Done;

    do
    {
        if (0 == strcmp (file.cFileName, "."))
            continue;

        if (0 == strcmp (file.cFileName, ".."))
            continue;

        if (FILE_ATTRIBUTE_DIRECTORY & file.dwFileAttributes)
        {
            BuildPath (szNextTargetDir, sizeof (szNextTargetDir), pszTargetDir, file.cFileName);
            create_directory (szNextTargetDir);

            BuildPath (szFindPath, sizeof (szFindPath), pszDirectory, file.cFileName);
            CopyFilesFromDirectory (szFindPath, szNextTargetDir, levels-1);
        }
        else
        {
            CopyFileToTargetDir (file.cFileName, pszDirectory, pszTargetDir);
        }

    } while (FindNextFile (hSearch, &file) );

Done:

    if (hSearch)
    {
        CloseHandle (hSearch);
        hSearch = NULL;
    }

    return ret;
}

int GetSoftwareInfoFromFile (TYPE_SOFTWARE_INFO *pInstSoft, const char *pszFilePath)
{
    int ret  = 0;
    FILE *fp = NULL;

    char szBuffer[1024] = {0};
    char *pEqual = NULL;
    char *pValue = NULL;
    char *p      = NULL;

    fp = fopen (pszFilePath, "r");

    if (NULL == fp)
    {
        printf ("Cannot open: [%s]\n", pszFilePath);
        ret = 1;
        goto Done;
    }

    while ( fgets (szBuffer, sizeof (szBuffer)-1, fp) )
    {
        pEqual = strstr (szBuffer, "=");

        if (pEqual)
        {
            *pEqual = '\0';
            pValue=pEqual+1;

            /* Remove control chars end of line */
            p=pValue;

            while (*p)
            {
                if (*p < 32)
                    *p = '\0';
                p++;
            }

            if (0 == strcmp (szBuffer, "name"))
                strdncpy (pInstSoft->szName, pValue, sizeof (pInstSoft->szName));

            else if (0 == strcmp (szBuffer, "version"))
                strdncpy (pInstSoft->szVersion, pValue, sizeof (pInstSoft->szVersion));

            else if (0 == strcmp (szBuffer, "description"))
                strdncpy (pInstSoft->szDescription, pValue, sizeof (pInstSoft->szDescription));

            else if (0 == strcmp (szBuffer, "vendor"))
                strdncpy (pInstSoft->szVendor, pValue, sizeof (pInstSoft->szVendor));
        }
    } /* while */

Done:

    if (fp)
    {
        fclose (fp);
        fp = NULL;
    }

    return ret;
}

int PrintSoftwareInfo (const char *pszFindPath)
{
    int ret = 0;
    TYPE_SOFTWARE_INFO SoftInfo = {0};

    ret = GetSoftwareInfoFromFile (&SoftInfo, pszFindPath);
    if (ret)
        goto Done;
    
    printf ("%s|%s|%s\n", SoftInfo.szName, SoftInfo.szVersion, SoftInfo.szDescription);

Done:
    return ret;
}

int RunCommandOnFileFind (const char *pszDirectory, const char *pszMatchFileName, int levels)
{
    int ret = 0;
    char szFindPath[1024] = {0};

    if (levels <= 0)
        goto Done;

    WIN32_FIND_DATA file;
    HANDLE hSearch = NULL;

    BuildPath (szFindPath, sizeof (szFindPath), pszDirectory, "*");

    hSearch = FindFirstFile (szFindPath, &file);

    if (NULL == hSearch)
        goto Done;

    if (INVALID_HANDLE_VALUE == hSearch)
        goto Done;

    do
    {
        if (0 == strcmp (file.cFileName, "."))
            continue;

        if (0 == strcmp (file.cFileName, ".."))
            continue;

        BuildPath (szFindPath, sizeof (szFindPath), pszDirectory, file.cFileName);

        if (FILE_ATTRIBUTE_DIRECTORY & file.dwFileAttributes)
        {
            RunCommandOnFileFind (szFindPath, pszMatchFileName, levels-1);
        }
        else
        {
            if (0 == strcmp (file.cFileName, pszMatchFileName))
            {
                PrintSoftwareInfo (szFindPath);
            }
        }

    } while (FindNextFile (hSearch, &file) );

Done:

    if (hSearch)
    {
        CloseHandle (hSearch);
        hSearch = NULL;
    }

    return ret;
}

#endif

int CopyInstallDirectory (const char *pszBaseDir, const char *pszDirectory, const char *pszTargetDir, int levels)
{
    int ret = 0;
    char szSourcePath[1024]  = {0};

    if (NULL == pszBaseDir)
        goto Done;

    if (NULL == pszDirectory)
        goto Done;

    if (NULL == pszTargetDir)
        goto Done;

    if (levels <= 0)
        goto Done;

    BuildPath (szSourcePath, sizeof (szSourcePath), pszBaseDir, pszDirectory);

    ret = CopyFilesFromDirectory (szSourcePath, pszTargetDir, levels);

Done:
    return ret;
}

void LogError (const char *pszErrorText)
{
    printf ("%s\n", pszErrorText);
}

int  ListInstalledSoftware (const char *pszInstallRegDir)
{
    int ret = 0;

    printf ("\n--- Installed Software ---\n");

    RunCommandOnFileFind (pszInstallRegDir, "install.ini", 2);

    printf ("--- Installed Software ---\n\n");

Done:
    return ret;
}


int GetParam (const char *pParam, const char *pName, char *retpConfig, int MaxParamSize)
{
    const char *p = pParam;
    const char *n = pName;

    if (NULL == p)
        return 0;

    if (NULL == n)
        return 0;

    if (0 == MaxParamSize)
        return 0;

    while (*n)
    {
        if (*p != *n)
            return 0;

        p++;
        n++;
    }

    snprintf (retpConfig, MaxParamSize-1, "%s", p);
    return 1;
}

int help(const char *pszProgramName)
{
    printf ("\n\n");

    printf ("Options\n");
    printf ("-------\n");
    printf ("-name=<application name>  Name of application for remove or status\n");
    printf ("-data=<data dir>          Explicit data directory override\n");
    printf ("-bin=<binary dir>         Explicit binary directory override\n");
    printf ("-wait=<seconds>           Wait number of seconds before stopping the program (to show output)\n");

    printf ("\nCommands\n");
    printf ("--------\n");
    printf ("-list                     List Installed software\n");
    printf ("-remove                   Remove/uninstall application\n");

    return 0;
}

int InstallAddOn (int argc, const char *argv[])
{
    int ret   = 0;
    int i     = 0;
    int count = 0;
    int wait  = 0;

    char szInstallDir[1024]        = {0};
    char szProgramDir[1024]        = {0};
    char szDataDir[1024]           = {0};
    char szDominoService[1024]     = {0};
    char szNotesIni[1024]          = {0};
    char szInstallRegDir[1024]     = {0};
    char szInstallLogDir[1024]     = {0};
    char szInstallIniFile[1024]    = {0};
    char szInstallIniLog[1024]     = {0};
    char szLogFileName[1024]       = {0};
    char szLogInstalledFiles[1024] = {0};
    char szDominoVersion[40]       = {0};
    char szDominoBuild[40]         = {0};
    char szName[40]                = {0};
    char szDelay[40]               = {0};

    TYPE_SOFTWARE_INFO NewSoft       = {0};
    TYPE_SOFTWARE_INFO InstalledSoft = {0};

    char *p       = NULL;
    char *pDirSep = NULL;
    char *pValue  = NULL;
    const char *pParam = NULL;
    const char *pName  = NULL;

    int CmdListSoftware   = 0;
    int CmdRemoveSoftware = 0;

    strdncpy (szInstallDir, argv[0], sizeof (szInstallDir));

    p = szInstallDir;
    while (*p)
    {
        if ( ('\\' == *p) || ('/' == *p))
            pDirSep = p;
        p++;
    }

    if (pDirSep)
    {
        *pDirSep = '\0';
    }
    else
    {
        *szInstallDir = '\0';
        ret = GetCurrentDirectory (sizeof(szInstallDir)-1, szInstallDir);
        if (0 == ret)
        {
            LogError ("Cannot get current directory");
            goto Done;
        }
    }

    pValue = getenv (ENV_DOMINO_BIN);
    if (pValue)
        strdncpy (szProgramDir, pValue, sizeof (szProgramDir));

    pValue = getenv (ENV_DOMINO_DATA);
    if (pValue)
        strdncpy (szDataDir, pValue, sizeof (szDataDir));

    pValue = getenv (ENV_DOMINO_INI);
    if (pValue)
        strdncpy (szNotesIni, pValue, sizeof (szNotesIni));

    pValue = getenv (ENV_DOMINO_VERSION);
    if (pValue)
        strdncpy (szDominoVersion, pValue, sizeof (szDominoVersion));

    pValue = getenv (ENV_DOMINO_BUILD);
    if (pValue)
        strdncpy (szDominoBuild, pValue, sizeof (szDominoBuild));

    pValue = getenv (ENV_DOMINO_SERVICE);
    if (pValue)
        strdncpy (szDominoService, pValue, sizeof (szDominoService));

    for (i=1; i<argc; i++)
    {
        pParam = argv[i];

        if ('-' == *pParam)
        {
            if (GetParam (pParam, "-bin=", szProgramDir, sizeof (szProgramDir)))
                continue;

            if (GetParam (pParam, "-data=", szDataDir, sizeof (szDataDir)))
                continue;

            if (GetParam (pParam, "-name=", szName, sizeof (szName)))
                continue;

            if (GetParam (pParam, "-wait=", szDelay, sizeof (szDelay)))
            {
                wait = atoi (szDelay);
                if (wait > 60)
                    wait = 60;
                continue;
            }

            if (0 == strcmp (pParam, "-list"))
            {
                CmdListSoftware = 1;
                continue;
            }

            if (0 == strcmp (pParam, "-remove"))
            {
                CmdRemoveSoftware = 1;
                continue;
            }

            if (0 == strcmp (pParam, "-help") || (0 == strcmp (pParam, "-?")) )
            {
                help (argv[0]);
                goto Done;
            }

            printf ("Invalid parameter: [%s]\n", pParam);
            goto Syntax;
        }
        else
        {
/*
            LATER: for future use
            if (0 == count)
            {
                snprintf (szTemplate, sizeof (szTemplate)-1, "%s", pParam);
            }
            else
*/
            {
                printf ("Invalid parameter: [%s]\n", pParam);
                goto Syntax;
            }

            count++;
        }

    } /* for */

#ifdef UNIX

#else

    /* If no program dir or data dir specified, try to get it from registry */

    if (!*szProgramDir)
    {
        ret = NshGetRegValue (HKEY_LOCAL_MACHINE, "SOFTWARE\\Lotus\\Domino", "Path", szProgramDir, sizeof (szProgramDir));
    }

    if (!*szDataDir)
    {
        ret = NshGetRegValue (HKEY_LOCAL_MACHINE, "SOFTWARE\\Lotus\\Domino", "DataPath", szDataDir, sizeof (szDataDir));
    }

#endif

    if (!*szProgramDir)
    {
        /* LATER: Quick hack for demo */
        LogError ("Info: No program directory found, using test default!");
        strdncpy (szProgramDir, "c:\\domino-install-test", sizeof (szProgramDir));
        // goto Done;
    }

    if (!*szProgramDir)
    {
        LogError ("No program directory found");
        goto Done;
    }

    if (!*szDataDir)
    {
        LogError ("No data directory found");
        // goto Done;
    }

    printf ("ProgramDir: [%s]\n", szProgramDir);
    printf ("Data Dir  : [%s]\n", szDataDir);

    BuildPath (szInstallRegDir, sizeof (szInstallRegDir), szProgramDir, g_InstallRegDirName);

    if (CmdListSoftware)
    {
        ListInstalledSoftware (szInstallRegDir);
        goto Done;
    }

    BuildPath (szInstallIniFile, sizeof (szInstallIniFile), szInstallDir, "install.ini");

    if (CmdRemoveSoftware)
    {
        if (*szName)
        {
            pName = szName;
        }
        else
        {
            ret = GetSoftwareInfoFromFile (&NewSoft, szInstallIniFile);
            if (ret)
            {
                goto Done;
            }

            pName = NewSoft.szName;
        }

        BuildPath (szInstallLogDir, sizeof (szInstallLogDir), szInstallRegDir, pName);
        BuildPath (szLogInstalledFiles, sizeof (szLogInstalledFiles), szInstallLogDir, "file.log");

        g_fpFileInstallLog = fopen (szLogInstalledFiles, "r");

        if (NULL == g_fpFileInstallLog)
        {
            LogError ("Cannot open file log file");
            goto Done;
        }

        printf ("Removing [%s] via [%s]\n", pName, szLogInstalledFiles);

        goto Done;
    }

    GetSoftwareInfoFromFile (&NewSoft, szInstallIniFile);

    if (!(*NewSoft.szName))
    {
        LogError ("No install configuration found");
        goto Done;
    }

    if (!*(NewSoft.szVersion))
    {
        LogError ("No software version found");
        goto Done;
    }

    BuildPath (szInstallLogDir,     sizeof (szInstallLogDir),     szInstallRegDir, NewSoft.szName);
    BuildPath (szLogFileName,       sizeof (szLogFileName),       szInstallLogDir, "install.log");
    BuildPath (szLogInstalledFiles, sizeof (szLogInstalledFiles), szInstallLogDir, "file.log");
    BuildPath (szInstallIniLog,     sizeof (szInstallIniLog),     szInstallLogDir, "install.ini");

    GetSoftwareInfoFromFile (&InstalledSoft, szInstallIniLog);

    if (0 == strcmp (InstalledSoft.szVersion, NewSoft.szVersion))
    {
        printf ("[%s] latest version %s already installed.\n", InstalledSoft.szName, InstalledSoft.szVersion);
        goto Done;
    }

    if (*InstalledSoft.szVersion)
    {
        printf ("[%s] Updating %s -> %s\n", InstalledSoft.szName, InstalledSoft.szVersion, NewSoft.szVersion);
    }
    else
    {
        printf ("[%s] Installing %s\n", NewSoft.szName, NewSoft.szVersion);
    }

    create_directory (szInstallRegDir);
    create_directory (szInstallLogDir);

    g_fpLog = fopen (szLogFileName, "w");

    if (NULL == g_fpLog)
    {
        LogError ("Cannot open log file");
        goto Done;
    }

    setbuf (g_fpLog, NULL);

    g_fpFileInstallLog = fopen (szLogInstalledFiles, "w");

    if (NULL == g_fpFileInstallLog)
    {
        LogError ("Cannot open file log file");
        goto Done;
    }

    setbuf (g_fpFileInstallLog, NULL);

    printf ("Install log file: [%s]\n", szLogInstalledFiles);

    fprintf (g_fpLog, "-- Command-Line Arguments --\n\n");

    printf ("argc1: %d\n", argc);

    for (i=0; i<argc; i++)
    {
        printf ("%s\n", argv[i]);
        fprintf (g_fpLog, "%s\n", argv[i]);
    }

    fprintf (g_fpLog, "-- Command-Line Arguments --\n\n");

    fprintf (g_fpLog, "-- Parameters --\n");
    fprintf (g_fpLog, "ProgamDir=%s\n",  szProgramDir);
    fprintf (g_fpLog, "DataDir=%s\n",    szDataDir);
    fprintf (g_fpLog, "NotesIni=%s\n",   szNotesIni);
    fprintf (g_fpLog, "InstallDir=%s\n", szInstallDir);

    CopyInstallDirectory (szInstallDir, "domino-bin", szProgramDir, 10);

    fprintf (g_fpLog, "FilesCopied=%ld\n",   g_CopiedFiles);
    fprintf (g_fpLog, "FileCopyErrors=%ld\n",g_CopyErrors);

    copy_file (szInstallIniFile, szInstallIniLog, TRUE);

    printf ("\nInstallation completed\n\n");

Done:

    if (g_fpLog)
    {
        fclose (g_fpLog);
        g_fpLog = NULL;
    }

    if (g_fpFileInstallLog)
    {
        fclose (g_fpFileInstallLog);
        g_fpFileInstallLog = NULL;
    }

    printf ("\nTerminating\n\n");

    if (wait)
    {
        Sleep (wait * 1000);
    }

    return ret;

Syntax:

    return ret;
}

int main (int argc, const char *argv[])
{
    int ret = 0;

    if ((argc < 1) || (NULL == argv[0]))
        goto Done;

    ret = InstallAddOn (argc, argv);

Done:

    return ret;

Syntax:

    return 1;
}
