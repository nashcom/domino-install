/*
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
*/

#ifndef CFG_HPP
    #define CFG_HPP

#define MAX_ENTRY_LEN 255
#define MAX_BUFFER    4096

#define INITAL_ARRAY_ELEMENTS   50
#define INCREASE_ARRAY_ELEMENTS 10


#ifdef UNIX
    #define STRICMP strcasecmp
    #define POPEN    popen
    #define PCLOSE   pclose
    #define GETCWD   getcwd
    #define CHDIR    chdir

#else
    #define STRICMP _stricmp
    #define POPEN   _popen
    #define PCLOSE  _pclose
    #define GETCWD  _getcwd
    #define CHDIR   _chdir

#endif


typedef struct {
    char szName[MAX_ENTRY_LEN+1];
    char szValue[MAX_ENTRY_LEN+1];
} CFG_STRUCT;



class AutoConfig
{

public:

    AutoConfig();
    ~AutoConfig();

    void Init();
    void Release();

    int  CheckCfgBuffer         (char *pszBuffer);
    int  FileUpdatePlaceholders (const char *pszInputFile, const char *pszOutputFile);
    int  CheckWriteBuffer       (char *pszBuffer, FILE *fpOutput);
    int  ReadCfg                (const char *pszFileName);
    char *CheckCfgArray         (const char *pszName);

    int AddEntry (char *pszName, char *pszValue);

    void SetInteractive (int Value)
    {
        m_Interactive = Value;
    }

private:

    CFG_STRUCT *m_pCfgArrayHead;
    CFG_STRUCT *m_pCfgArrayNext;

    int mm_CfgEntriesMax;
    int m_CfgEntries;
    int m_Interactive;
};

#endif