/*
###########################################################################
# Domino Auto Config (OneTouchConfig Tool)                                #
# Version 0.2.0 09.06.2023                                                #
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
#                                                                         #
#  Changes                                                                #
#  -------                                                                #
#                                                                         #
#  V0.2.0 09.06.2023                                                      #
#                                                                         #
#   - Support for standard .env file                                      #
#   - Log all infos to stderr instead of stdout                           #
#                                                                         #
#                                                                         #
###########################################################################
*/

/* autocfg: Replaces {{ placeholders }} in JSON files from environment variables or config file */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "cfg.hpp"

#define VERSION "0.2.0"

#define MAX_CFG 1024

int file_exists (const char *pszFilename)
{
    struct stat buffer = {0};

    if (IsNullStr (pszFilename))
        return 0;

    return (0 == stat (pszFilename, &buffer));
}

int RunAutoConfig (const char *pszJsonTemplate, const char *pszJsonOutput, const char *pszEnvFile, const char *pszProgram, int prompt)
{
    int ret = 0;

    AutoConfig AutoCfg;

    if (prompt)
        AutoCfg.SetInteractive (1);

    if (!IsNullStr (pszEnvFile))
    {
        ret = AutoCfg.ReadCfg (pszEnvFile);
        if (ret)
            goto Done;
    }

    if (IsNullStr (pszProgram))
    {
        ret = AutoCfg.FileUpdatePlaceholders (pszJsonTemplate, pszJsonOutput);
    }
    else
    {
        ret = AutoCfg.FileUpdateFromProgram (pszProgram, pszJsonOutput);
    }

    if (ret)
        goto Done;


    if (ret)
        goto Done;

    if (*pszJsonOutput)
        fprintf (stderr, "\nCreated [%s] from template [%s]\n\n", pszJsonOutput, pszJsonTemplate);

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

int main (int argc, const char *argv[])
{
    int ret    = 0;
    int i      = 0;
    int count  = 0;
    int prompt = 0;

    const char *pParam       = NULL;
    char szTemplate[MAX_CFG] = {0};
    char szConfig[MAX_CFG]   = {0};
    char szEnvFile[MAX_CFG]  = {0};
    char szProgram[MAX_CFG]  = {0};

    for (i=1; i<argc; i++)
    {
        pParam = argv[i];


        if ('-' == *pParam)
        {
            if (GetParam (pParam, "-env=", szEnvFile, sizeof (szEnvFile)))
                continue;

            if (GetParam (pParam, "-f=", szTemplate, sizeof (szTemplate)))
                continue;

            if (GetParam (pParam, "-o=", szConfig, sizeof (szConfig)))
                continue;

            if (GetParam (pParam, "-p=", szProgram, sizeof (szProgram)))
                continue;

            if (0 == strcmp (pParam, "-prompt"))
            {
                prompt = 1;
                continue;
            }

            if (0 == strcmp (pParam, "-version") || (0 == strcmp (pParam, "--version")) )
            {
                printf ("%s\n", VERSION);
                goto Done;
            }

            if (0 == strcmp (pParam, "-help") || (0 == strcmp (pParam, "-?")) )
            {
                goto Syntax;
            }

            fprintf (stderr, "Error: Invalid parameter: [%s]\n", pParam);
            goto Syntax;
        }
        else
        {
            if (0 == count)
            {
                snprintf (szTemplate, sizeof (szTemplate)-1, "%s", pParam);
            }
            else if (1 == count)
            {
                snprintf (szConfig, sizeof (szConfig)-1, "%s", pParam);
            }
            else
            {
                fprintf (stderr, "Error: Invalid parameter: [%s]\n", pParam);
                goto Syntax;
            }

            count++;
        }

    } /* for */

    if ( (!*szTemplate) && (!*szProgram) )
    {
        fprintf (stderr, "\nError: No template file or program specified!\n\n");
        goto Done;
    }

    /* Check if default .env file is present and use it (like docker-compose does) */
    if ( '\0' == *szEnvFile)
    {
       if (file_exists (".env"))
       {
            snprintf (szEnvFile, sizeof (szEnvFile)-1, ".env");
            fprintf (stderr, "Info: Using standard .env file\n");
       }
    }

    ret = RunAutoConfig (szTemplate, szConfig, szEnvFile, szProgram, prompt);

Done:

    return ret;

Syntax:

    if (argc)
        fprintf (stderr, "\nSyntax: %s [-env=<file>] [-prompt] [-f=<template-file>] [-o=<output-file>] [-p=<popen stdout as input>]\n\n", argv[0]);
    
    return 1;
}
