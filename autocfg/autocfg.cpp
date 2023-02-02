###########################################################################
# Domino Auto Config (OneTouchConfig Tool)                                #
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



/* autocfg: Replaces {{ placeholders }} in JSON files from environment variables or config file */

#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "cfg.hpp"

int OneTouchAutoConfig (const char * pszJsonTemplate, const char *pszJsonOutput, const char *pszEnvFile, int prompt)
{
    int ret = 0;

    AutoConfig AutoCfg;

    if (prompt)
        AutoCfg.SetInteractive (1);

    ret = AutoCfg.ReadCfg (pszEnvFile);
    if (ret)
       goto Done;

    ret = AutoCfg.FileUpdatePlaceholders (pszJsonTemplate, pszJsonOutput);

    if (ret)
        goto Done;


    if (ret)
        goto Done;

    printf ("\nCreated [%s] from template [%s]\n\n", pszJsonOutput, pszJsonTemplate);

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

int main (int argc, char *argv[])
{
    int ret    = 0;
    int i      = 0;
    int count  = 0;
    int prompt = 0;

    char *pParam = NULL;
    char szTemplate[255] = {0};
    char szConfig[255]   = {0};
    char szEnvFile[255]  = {0};

    for (i=1; i<argc; i++)
    {
        pParam = argv[i];

        if ('-' == *pParam)
        {
            if (GetParam (pParam, "-env=", szEnvFile, sizeof (szEnvFile)))
                continue;

            if (0 == strcmp (pParam, "-prompt"))
            {
                prompt = 1;
                continue;
            }

            if (0 == strcmp (pParam, "-help") || (0 == strcmp (pParam, "-?"))
            {
                goto Syntax;
            }

            printf ("Invalid parameter: [%s]\n", pParam);
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
                printf ("Invalid parameter: [%s]\n", pParam);
                goto Syntax;
            }

            count++;
        }

    } /* for */

    if (!*szTemplate)
    {
        printf ("\nNo template file specified!\n\n");
        goto Done;
    }

    if (!*szConfig)
    {
        printf ("\nNo configuration file specified!\n\n");
        goto Done;
    }

    ret = OneTouchAutoConfig (szTemplate, szConfig, szEnvFile, prompt);

Done:

    return ret;

Syntax:

    if (argc)
        printf ("\nSyntax: %s <JSON OTS template> <JSON OTS file> [-env=file]\n\n", argv[0]);
    
    return 1;
}
