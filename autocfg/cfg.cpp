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

#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "cfg.hpp"

#define SERVERSETUP_ENV "SERVERSETUP_"

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

int IsNullStr (const char *pszStr)
{
    if (NULL == pszStr)
        return 1;

    if ('\0' == *pszStr)
        return 1;

    return 0;
}

void AutoConfig::Init()
{
    m_pCfgArrayHead = NULL;
    m_pCfgArrayNext = NULL;

    mm_CfgEntriesMax = 0;
    m_CfgEntries    = 0;
}

void AutoConfig::Release()
{
    if (m_pCfgArrayHead)
    {
        free (m_pCfgArrayHead);

        m_pCfgArrayHead = NULL;
        m_pCfgArrayNext = NULL;
        m_CfgEntries    = 0;
    }
}

AutoConfig::AutoConfig()
{
    Init();
}

AutoConfig::~AutoConfig()
{
    Release();
}

int AutoConfig::AddEntry (char *pszName, char *pszValue)
{
    int error = 0;
    CFG_STRUCT * pNewArrayPtr = NULL;

    if (m_CfgEntries >= mm_CfgEntriesMax)
    {
        /* Increase array size and re-allocate memory */
        mm_CfgEntriesMax += INCREASE_ARRAY_ELEMENTS;

        pNewArrayPtr = (CFG_STRUCT *) realloc (m_pCfgArrayHead, mm_CfgEntriesMax * sizeof (CFG_STRUCT));

        if (NULL == pNewArrayPtr)
        {
            printf ("\nCannot re-allocate cfg array (%d)\n\n", mm_CfgEntriesMax);
            error = 2;
            goto Done;
        }

        /* Set new pointer and offset to next entry */
        m_pCfgArrayHead = pNewArrayPtr;
        m_pCfgArrayNext = m_pCfgArrayHead + m_CfgEntries;
    }

    strdncpy (m_pCfgArrayNext->szName, pszName, sizeof (m_pCfgArrayNext->szName));
    strdncpy (m_pCfgArrayNext->szValue, pszValue, sizeof (m_pCfgArrayNext->szValue));

    m_pCfgArrayNext++;
    m_CfgEntries++;

Done:
    return error;
}

char *AutoConfig::CheckCfgArray (const char *pszName)
{

    CFG_STRUCT *pEntry;

    pEntry = m_pCfgArrayHead;

    while (pEntry < m_pCfgArrayNext)
    {
        if (0 == STRICMP (pszName, pEntry->szName))
            return pEntry->szValue;

        pEntry++;
    }

    return NULL;
}

int AutoConfig::CheckCfgBuffer (char *pszBuffer)
{
    /* Note: Input buffer will be modified */

    int  error   = 0;

    char *pName  = pszBuffer;
    char *pEqual = NULL;
    char *pValue = NULL;
    char *p      = NULL;

    pEqual = strstr ( (char *) pszBuffer, "=");

    if (NULL == pEqual)
        goto Done;

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

    AddEntry (pName, pValue);

Done:
    return error;
}


int AutoConfig::ReadCfg (const char *pszFileName)
{
    int error = 0;
    FILE  *fpInput   = NULL;
    char  szBuffer[10240] = {0};

    if (IsNullStr (pszFileName))
        goto Done;

    fpInput = fopen (pszFileName, "r");

    if (NULL == fpInput)
    {
        printf ("\nError opening [%s]\n\n", pszFileName);
        error = 1;
        goto Done;
    }

    /* Just in case if called more than once, release the previous memory */
    Release();

    mm_CfgEntriesMax = INITAL_ARRAY_ELEMENTS;
    m_pCfgArrayHead = (CFG_STRUCT *) malloc (mm_CfgEntriesMax * sizeof (CFG_STRUCT));

    if (NULL == m_pCfgArrayHead)
    {
        printf ("\nCannot allocate cfg array (%d)\n\n", mm_CfgEntriesMax);
        error = 2;
        goto Done;
    }

    m_pCfgArrayNext = m_pCfgArrayHead;

    while ( fgets (szBuffer, sizeof (szBuffer)-1, fpInput) )
    {
        CheckCfgBuffer (szBuffer);
    }

Done:

    if (fpInput)
    {
        fclose (fpInput);
        fpInput = NULL;
    }

    return error;
}

int AutoConfig::CheckWriteBuffer (char *pszBuffer, FILE *fpOutput)
{
    int  ret     = 0;
    int  len     = 0;

    char *pBegin = NULL;
    char *pEnd   = NULL;
    char *pEnv   = NULL;
    char *pVal   = NULL;
    char *p      = NULL;

    char szLine[1024] = {0};

    /* Note: Input parameter pszBuffer is modified in routine! */

    pBegin = strstr ( (char *) pszBuffer, "{{");

    if (NULL == pBegin)
        goto WriteLine;

    pEnd = strstr (pBegin, "}}");

    if (NULL == pEnd)
        goto WriteLine;

    /* Write begin of line before placeholder */
    len = (pBegin - pszBuffer);

    if (len)
    {
        *pBegin = '\0';
        fputs (pszBuffer, fpOutput);
    }

    pEnv = pBegin+2;

    /* Trim begin of string */
    while (' ' == *pEnv)
        pEnv++;

    /* trim and terminate end of env name */
    p = pEnd;

    /* Terminate at end env marker */
    *p = '\0';

    /* Trim env string */
    while (p > pEnv)
    {
        p--;
        if (' ' != *p)
            break;

        *p = '\0';
    }

    pVal = CheckCfgArray (pEnv);

    /* Empty values in file overwrite environment vars */
    if (NULL == pVal)
        pVal = getenv (pEnv);

    if (pVal && *pVal)
    {
        fputs (pVal, fpOutput);
    }
    else if (m_Interactive)
    {
        /* Skip server SERVERSETUP_ prefix */
        p = strstr (pEnv, SERVERSETUP_ENV);
        if (NULL == p)
        {
            p = pEnv;
        }
        else
        {
            p += strlen (SERVERSETUP_ENV);
        }

        printf ("%s: ", p);

        *szLine='\0';

        if (fgets (szLine, sizeof(szLine)-1, stdin))
        {
            p = szLine;
            while (*p)
            {
                if (*p < 32)
                    *p = '\0';
                p++;
            }

            if (*szLine)
            {
                fputs (szLine, fpOutput);
                AddEntry (pEnv, szLine);
            }
        }
    }
    else
    {
        printf ("Info: [%s] not found\n", pEnv);
        ret = 1;
    }

    /* Write end of line after placeholder */
    pEnd +=2;

    /* Now check for the remaining line or another placeholder ... */
    if (*pEnd)
        CheckWriteBuffer (pEnd, fpOutput);

    return ret;

WriteLine:

    fputs (pszBuffer, fpOutput);
    return ret;
}

int AutoConfig::FileUpdatePlaceholders (const char *pszInputFile, const char *pszOutputFile)
{
    int   error      = 0;
    int   ret        = 0;
    int   count      = 0;
    FILE  *fpInput   = NULL;
    FILE  *fpOutput  = NULL;

    char  szBuffer[10240] = {0};

    fpInput = fopen (pszInputFile, "r");

    if (NULL == fpInput)
    {
        printf ("\nError opening [%s]\n\n", pszInputFile);
        error = 1;
        goto Done;
    }

    fpOutput = fopen (pszOutputFile, "w");

    if (NULL == fpOutput)
    {
        printf ("\nError opening [%s]\n\n", pszOutputFile);
        error = 2;
        goto Done;
    }

    while ( fgets (szBuffer, sizeof (szBuffer)-1, fpInput) )
    {
        ret = CheckWriteBuffer (szBuffer, fpOutput);

        if (ret)
            count++;
    }

    if (count)
        printf ("\nWarning: %d placeholders with empty values!\n\n", count);
Done:

    if (fpInput)
    {
        fclose (fpInput);
        fpInput = NULL;
    }

    if (fpOutput)
    {
        fclose (fpOutput);
        fpOutput = NULL;
    }

    return error;
}

