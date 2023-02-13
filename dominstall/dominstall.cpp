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
#include <wincrypt.h>
#include <tchar.h>
#include <Softpub.h>
#include <wintrust.h>
#include <imagehlp.h>

#endif

#include <sys/stat.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "dominoinstall.hpp"


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


typedef struct
{
    char szName[1024];
    char szVersion[100];
    char szDescription[1024];
    char szVendor[1024];
} TYPE_SOFTWARE_INFO;


typedef struct
{
    long lDominoBuild;
    long lBuildBestMatch;
    char szBestMatchVersionPath[1024];
} TYPE_VERSION_CHECK;


typedef int (TYPE_CMD_FUNCTION_CALLBACK) (const char *pszFindPath, void *pCustomData);

/* Globals */
FILE *g_fpLog = NULL;
FILE *g_fpFileInstallLog = NULL;

int  g_Debug  = 0;
int  g_CopiedFiles = 0;
int  g_CopyErrors = 0;

char g_InstallRegDirName[]  = ".install-reg";
char g_InstallLogName[]     = "install.log";
char g_InstallFileLogName[] = "file.log";
char g_InstallIniName[]     = "install.ini";
char g_ReleaseStr[]         = "Release ";
char g_ReleaseInstallStr[]  = "Release_";
char g_FixPackStr[]         = "FP";
char g_HotFixStr[]          = "HF";

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

int IsNullStr (const char *pszStr)
{
    if (NULL == pszStr)
        return 1;

    if ('\0' == *pszStr)
        return 1;

    return 0;
}

const char * GetStringPtrAfterPrefix (const char *pszString, const char *pszPrefix)
{
    const char *s = pszString;
    const char *p = pszPrefix;

    if (NULL == pszString)
        return NULL;

    if (NULL == pszPrefix)
        return NULL;

    while (*p)
    {
        if (*p != *s)
            return NULL;
        p++;
        s++;
    }

    return s;
}

int delete_file (const char *pszFileName)
{
    int ret = 0;

    if (IsNullStr (pszFileName))
        goto Done;

#ifdef UNIX
#else
    ret = DeleteFile (pszFileName);

    if (g_fpLog)
    {
        if (0 == ret)
            fprintf (g_fpLog, "Cannot delete file: %s\n", pszFileName);
    }

#endif

Done:
    return ret;
}

int GetNumberFromStr (const char **ppString)
{
    int  num = 0;
    char c   = 0;

    if (NULL == ppString)
        goto Done;

    /* Skip leading blanks*/
    while (' ' == **ppString)
        (*ppString)++;

    while (**ppString)
    {
        c = **ppString;

        if ((c >= '0') && (c <= '9'))
        {
            num = (num * 10) + (c - '0');
        }
        else if ('.' == c)
        {
            /* Skip dot for reading next number */
            (*ppString)++;
            break;
        }
        else
        {
            /* Don't skip char and leave it for the next part of calculation (e.g. FP/HF ) */
            break;
        }

        (*ppString)++;
    } /* while */

Done:

    return num;
}

long DominoBuildNumFromVersion (const char *pszRelease, int *retpHotfix)
{
    long lBuild       = 0;
    int  MajorVersion = 0;
    int  MinorVersion = 0;
    int  QMRNumber    = 0;
    int  QMUNumber    = 0;
    int  HotfixNumber = 0;

    const char *pszVersion = NULL;
    const char *p = NULL;

    if (retpHotfix)
        *retpHotfix = 0;

    if (IsNullStr (pszRelease))
        goto Done;

    /* Skip "Release " prefix */
    pszVersion = GetStringPtrAfterPrefix (pszRelease, g_ReleaseStr);

    if (IsNullStr (pszVersion))
        pszVersion = pszRelease;

    /* Get version in format major.minor.QMR */

    MajorVersion = GetNumberFromStr (&pszVersion);
    MinorVersion = GetNumberFromStr (&pszVersion);
    QMRNumber    = GetNumberFromStr (&pszVersion);

    /* Get optional fixpack number */
    p = strstr (pszVersion, g_FixPackStr);
    if (p)
    {
        pszVersion = p + strlen (g_FixPackStr);
        QMUNumber = GetNumberFromStr (&pszVersion);
    }

    /* Get optional hotfix number */
    p = strstr (pszVersion, g_HotFixStr);
    if (p)
    {
        pszVersion = p + strlen (g_HotFixStr);
        HotfixNumber = GetNumberFromStr (&pszVersion);
    }

    lBuild = MajorVersion * 1000000 + MinorVersion * 10000 + QMRNumber * 100 + QMUNumber;

Done:

    if (retpHotfix)
        *retpHotfix = HotfixNumber;

    return lBuild;
}


#ifdef UNIX
#else

void GetWindowsErrorString (const char *pszErrorMessage, int MaxRetBufferSize, char *retpszRetBuffer)
{
    DWORD dwWinError = 0;
    DWORD dwRet = 0;
    char  szWinErrorMessage[1024] = {0};
    char  *p = NULL;

    if ((NULL == retpszRetBuffer) || (0 == MaxRetBufferSize))
        return;

    *retpszRetBuffer = '\0';

    if (NULL == pszErrorMessage)
        return;

    dwWinError = GetLastError();

    if (dwWinError)
    {
        dwRet = FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                               NULL,
                               dwWinError,
                               MAKELANGID (LANG_ENGLISH, SUBLANG_ENGLISH_US),
                               szWinErrorMessage,
                               sizeof (szWinErrorMessage)-1,
                               NULL);
        if (dwRet == 0)
        {
            snprintf (retpszRetBuffer, sizeof (szWinErrorMessage)-1, "Win-Error: 0x%x", dwWinError);
        }
        else
        {
            /* Remove trailing new-line */
            p=szWinErrorMessage;

            while (*p)
            {
                if (*p < 32)
                {
                    *p = '\0';
                    break;
                }
                p++;
            }
        }
    }

    snprintf (retpszRetBuffer, MaxRetBufferSize-1, "%s - %s", pszErrorMessage, szWinErrorMessage);
}

int GetEmbeddedSignature (const char *pszFileName, int MaxRetBufferSize, char *retpszRetBuffer)
{
    int ret = 2;
    BOOL  fResult = FALSE;

    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD  dwCertCount  = 0;
    DWORD  dwCertLen    = 0;
    DWORD  dwMemSize    = 0;
    DWORD dwDecodeSize  = 0;
    DWORD dwSubjectSize = 0;

    CRYPT_VERIFY_MESSAGE_PARA VerifyMsgParam = { 0 };

    WIN_CERTIFICATE CertHeader   = {0};
    WIN_CERTIFICATE *pCert       = NULL;
    PCCERT_CONTEXT  pCertContext = NULL;

    WIN_CERTIFICATE *pCertBuf = NULL;

    if ((NULL == retpszRetBuffer) || (0 == MaxRetBufferSize))
        return ret;

    *retpszRetBuffer = '\0';

    if (IsNullStr (pszFileName))
        return ret;

    hFile = CreateFile (pszFileName, FILE_READ_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        GetWindowsErrorString ("Cannot open file", MaxRetBufferSize, retpszRetBuffer);
        goto Done;
    }

    if (!ImageEnumerateCertificates (hFile, CERT_SECTION_TYPE_ANY, &dwCertCount, NULL, 0))
    {
        GetWindowsErrorString ("Error enumerating certificates", MaxRetBufferSize, retpszRetBuffer);
        goto Done;
    }

    // printf ("Certificates found: %ld\n", dwCertCount);

    CertHeader.dwLength = 0;
    CertHeader.wRevision = WIN_CERT_REVISION_1_0;

    if (!ImageGetCertificateHeader (hFile, 0, &CertHeader))
    {
        GetWindowsErrorString ("Error getting certificate header", MaxRetBufferSize, retpszRetBuffer);
        goto Done;
    }

    dwCertLen = CertHeader.dwLength;
    dwMemSize = sizeof (WIN_CERTIFICATE) + dwCertLen + 10; /* Paranoid extra space */

    pCertBuf = (WIN_CERTIFICATE *) malloc (dwMemSize);

    if (NULL == pCertBuf)
    {
        snprintf (retpszRetBuffer, MaxRetBufferSize-1, "Cannot allocate certificate buffer");
        goto Done;
    }

    pCert = pCertBuf;
    pCert->dwLength  = dwCertLen;
    pCert->wRevision = WIN_CERT_REVISION_1_0;

    if (!ImageGetCertificateData (hFile, 0, pCert, &dwCertLen))
    {
        GetWindowsErrorString ("Error getting certificate data", MaxRetBufferSize, retpszRetBuffer);
        goto Done;
    }

    VerifyMsgParam.cbSize = sizeof (VerifyMsgParam);
    VerifyMsgParam.dwMsgAndCertEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;

    if (!CryptVerifyMessageSignature (&VerifyMsgParam, 0, pCert->bCertificate, pCert->dwLength, NULL, &dwDecodeSize, &pCertContext))
    {
        GetWindowsErrorString ("Error verifying signature", MaxRetBufferSize, retpszRetBuffer);
        goto Done;
    }

    dwSubjectSize = CertGetNameString (pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, NULL, 0);
    if (dwSubjectSize <= 0)
    {
        GetWindowsErrorString ("Error getting certificate subject name", MaxRetBufferSize, retpszRetBuffer);
        goto Done;
    }

    if (dwSubjectSize >= MaxRetBufferSize)
    {
        snprintf (retpszRetBuffer, MaxRetBufferSize-1, "Buffer too small for certificate name info");
        goto Done;
    }

    CertGetNameString (pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, retpszRetBuffer, MaxRetBufferSize-1);

    ret = 0;

Done:

    if (pCertBuf)
    {
        free (pCertBuf);
        pCertBuf = NULL;
    }

    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle (hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

    return ret;
}

int VerifyEmbeddedSignature (const char *pszFileName, int MaxRetBufferSize, char *retpszRetBuffer)
{
    /* Return values:

       0 = signature OK, buffer is empty
       1 = not signed, buffer is empty
       2 = error, buffer contains error
    */

    int ret= 2;
    LONG lStatus;
    DWORD dwLastError;

    GUID WVTPolicyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;

    wchar_t wszSourceFile[2048] = {0};
    WINTRUST_DATA WinTrustData  = {0};
    WINTRUST_FILE_INFO FileData = {0};

    if ((NULL == retpszRetBuffer) || (0 == MaxRetBufferSize))
        return ret;

    *retpszRetBuffer = '\0';

    if (IsNullStr (pszFileName))
        return ret;

    swprintf (wszSourceFile, sizeof (wszSourceFile)-1, L"%hs", pszFileName);

    FileData.cbStruct       = sizeof (WINTRUST_FILE_INFO);
    FileData.pcwszFilePath  = wszSourceFile;
    FileData.hFile          = NULL;
    FileData.pgKnownSubject = NULL;

    // Initialize the WinVerifyTrust input data structure.
    WinTrustData.cbStruct = sizeof(WinTrustData);

    // Use default code signing EKU.
    WinTrustData.pPolicyCallbackData = NULL;

    // No data to pass to SIP.
    WinTrustData.pSIPClientData = NULL;

    // Disable WVT UI.
    WinTrustData.dwUIChoice = WTD_UI_NONE;

    // No revocation checking.
    WinTrustData.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN; // WTD_REVOKE_NONE;

    // Verify an embedded signature on a file.
    WinTrustData.dwUnionChoice = WTD_CHOICE_FILE;

    // Verify action.
    WinTrustData.dwStateAction = WTD_STATEACTION_VERIFY;

    // Verification sets this value.
    WinTrustData.hWVTStateData = NULL;

    // Not used.
    WinTrustData.pwszURLReference = NULL;
    WinTrustData.dwUIContext = 0;

    // Set pFile.
    WinTrustData.pFile = &FileData;

    lStatus = WinVerifyTrust (NULL, &WVTPolicyGUID, &WinTrustData);

    switch (lStatus)
    {
        case ERROR_SUCCESS:
            ret= 0;
            break;

        case TRUST_E_NOSIGNATURE:

            dwLastError = GetLastError();

            if ((TRUST_E_NOSIGNATURE == dwLastError) ||
                (TRUST_E_SUBJECT_FORM_UNKNOWN == dwLastError) ||
                (TRUST_E_PROVIDER_UNKNOWN == dwLastError) )
            {
                ret = 1;
            }
            else
            {
                snprintf (retpszRetBuffer, MaxRetBufferSize-1, "An unknown error occurred trying to verify the signature");
            }

            break;

        case TRUST_E_EXPLICIT_DISTRUST:
            fprintf (g_fpLog, "The signature is present, but specifically disallowed.\n");
            snprintf (retpszRetBuffer, MaxRetBufferSize-1, "The signature is present, but specifically disallowed");
            break;

        case TRUST_E_SUBJECT_NOT_TRUSTED:
            fprintf (g_fpLog, "The signature is present, but not trusted.\n");
            snprintf (retpszRetBuffer, MaxRetBufferSize-1, "The signature is present, but not trusted.\n");
            break;

        case CRYPT_E_SECURITY_SETTINGS:
            snprintf (retpszRetBuffer, MaxRetBufferSize-1, "Signature is not trusted because of settings");
            break;

        default:
            snprintf (retpszRetBuffer, MaxRetBufferSize-1, "Error verifying signature: 0x%x", lStatus);
            GetWindowsErrorString ("Error verifying signature", MaxRetBufferSize, retpszRetBuffer);
            break;
    }

    // Any hWVTStateData must be released by a call with close.
    WinTrustData.dwStateAction = WTD_STATEACTION_CLOSE;

    lStatus = WinVerifyTrust (NULL, &WVTPolicyGUID, &WinTrustData);

    return ret;
}

int NshGetRegValue (HKEY hMainKey, char *sub_key, char *value_name, char *data, DWORD data_size)
{
    HKEY    hKey   = NULL;
    DWORD   type   = 0;
    LONG    ret    = 0;
    DWORD   number = 0;

    ret = RegOpenKeyEx (HKEY_LOCAL_MACHINE, sub_key, 0, KEY_READ, &hKey);

    if (ERROR_SUCCESS != ret)
        return 0;

    ret = RegQueryValueExA (hKey, value_name, NULL, &type, (LPBYTE) data, &data_size);

    RegCloseKey(hKey);

    if (ERROR_SUCCESS != ret)
        return 0;

    switch (type)
    {
        case REG_SZ:
            break;

        case REG_DWORD:
            number = (DWORD) (*data);
            snprintf (data, sizeof (data) - 1, "%ld", number);
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

int GetDominoVersion (const char *pszBinaryDir, char *retpszVersion, int MaxVersionBuffer)
{
    int len = 0;
    HMODULE hModule = NULL;

    char szNotesLib[1024]    = {0};
    char szErrorBuffer[1024] = {0};

    if ((NULL == retpszVersion) || (0 == MaxVersionBuffer))
        goto Done;

    *retpszVersion = '\0';

    snprintf (szNotesLib, sizeof (szNotesLib)-1, "%s%c%s", pszBinaryDir, g_OsDirSep, "nstrings.dll");

    hModule = LoadLibraryEx (szNotesLib, NULL, LOAD_LIBRARY_AS_DATAFILE);

    if (NULL == hModule)
    {
        GetWindowsErrorString ("Cannot load module", sizeof (szErrorBuffer)-1, szErrorBuffer);
        printf ("%s\n", szErrorBuffer);
        goto Done;
    }

    len = LoadString (hModule, 1, retpszVersion, MaxVersionBuffer-1);

    if (0 == len)
    {
        GetWindowsErrorString ("Cannot load string", sizeof (szErrorBuffer)-1, szErrorBuffer);
        printf ("%s\n", szErrorBuffer);
        goto Done;
    }

Done:

    if (hModule)
    {
        FreeLibrary (hModule);
        hModule = NULL;
    }

    return len;
}

#ifdef UNIX

int CopyFilesFromDirectory (const char *pszDirectory, const char *pszTargetDir, int levels)
{
    int ret = 0;
    BOOL bFileCopied = FALSE;
    DIR *pDir        = NULL;
    struct dirent *pEntry = NULL;

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
    bFileCopied = CopyFile (pszSourcePath, pszTargetPath, bOverwrite ? FALSE : TRUE);

    if (!bFileCopied)
    {
        printf ("Error copying file [%s] -> [%s]\n", pszSourcePath, pszTargetPath);
        fprintf (g_fpLog, "Error copying file [%s] -> [%s]\n", pszSourcePath, pszTargetPath);
    }

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
                {
                    *p = '\0';
                    break;
                }
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

int PrintSoftwareInfo (const char *pszFindPath, void *pCustomData)
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

int IsExecutable (const char *pszFindPath)
{
    const char *pExt = NULL;
    const char *p    = NULL;

    p = pszFindPath;

    while (*p)
    {
        if ('.' == *p)
            pExt = p+1;

        p++;
    } /* while */

    if (NULL == pExt)
        return 0;

    if (0 == STRICMP (pExt, "dll") )
        return 1;

    if (0 == STRICMP (pExt, "exe") )
        return 1;

    return 0;
}

int CheckSignature (const char *pszFindPath, void *pCustomData)
{
    int ret = 0;
    char szSignInfoBuffer[2048] = {0};

    if (IsExecutable (pszFindPath))
    {
        ret = VerifyEmbeddedSignature (pszFindPath, sizeof (szSignInfoBuffer), szSignInfoBuffer);

        if (0 == ret)
        {
            GetEmbeddedSignature (pszFindPath, sizeof (szSignInfoBuffer), szSignInfoBuffer);
            printf ("%d|%s|%s\n", ret, szSignInfoBuffer, pszFindPath);
        }
        else if (1 == ret)
        {
            printf ("%d|-|%s\n", ret, pszFindPath);
        }
        else
        {
            printf ("%d|-|%s|Signature Error|%s\n", ret, pszFindPath, szSignInfoBuffer);
        }
    }

Done:
    return ret;
}

int CheckInstallVersion (const char *pszFindPath, void *pCustomData)
{
    const char *pszVersion;
    long  lCheckBuild = 0;
    TYPE_VERSION_CHECK *pVersionCheck;

    if (NULL == pszFindPath)
        return 0;

    if (NULL == pCustomData)
        return 0;

    pszVersion = strstr (pszFindPath, g_ReleaseInstallStr);

    if (NULL == pszVersion)
        return 0;

    pszVersion += strlen (g_ReleaseInstallStr);
    pVersionCheck = (TYPE_VERSION_CHECK *) pCustomData;

    lCheckBuild = DominoBuildNumFromVersion (pszVersion, NULL);

    if (0 == lCheckBuild)
        return 0;

    /* Software is too new for Domino build */
    if (lCheckBuild > pVersionCheck->lDominoBuild)
        return 0;

    /* Not better then current match */
    if (lCheckBuild < pVersionCheck->lBuildBestMatch)
        return 0;

    pVersionCheck->lBuildBestMatch = lCheckBuild;
    strdncpy (pVersionCheck->szBestMatchVersionPath, pszFindPath, sizeof (pVersionCheck->szBestMatchVersionPath));
    return 1;
}

#define RUN_COMMAND_ON_FIND_FLAG_FILE      1
#define RUN_COMMAND_ON_FIND_FLAG_DIRECTORY 2

int RunCommandOnFileFind (long lFlags, const char *pszDirectory, const char *pszMatchFileName, TYPE_CMD_FUNCTION_CALLBACK *pCommandCallbackFunction, void *pCustomData, int levels)
{
    int ret = 0;
    char szFindPath[1024] = {0};

    if (levels <= 0)
        goto Done;

    if (NULL == pszMatchFileName)
        goto Done;

    WIN32_FIND_DATA file;
    HANDLE hSearch = NULL;

    BuildPath (szFindPath, sizeof (szFindPath), pszDirectory, "*");

    hSearch = FindFirstFile (szFindPath, &file);

    if (NULL == hSearch)
        goto Done;

    if (INVALID_HANDLE_VALUE == hSearch)
        goto Done;

    if (NULL == pCommandCallbackFunction)
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
            if (RUN_COMMAND_ON_FIND_FLAG_DIRECTORY & lFlags)
            {
                pCommandCallbackFunction (szFindPath, pCustomData);
            }

            RunCommandOnFileFind (lFlags, szFindPath, pszMatchFileName, pCommandCallbackFunction, pCustomData, levels-1);
        }
        else
        {
            if (RUN_COMMAND_ON_FIND_FLAG_FILE & lFlags)
            {
                if (*pszMatchFileName)
                {
                    if (0 == strcmp (file.cFileName, pszMatchFileName))
                    {
                        pCommandCallbackFunction (szFindPath, pCustomData);
                    }
                }
                else
                {
                    pCommandCallbackFunction (szFindPath, pCustomData);
                }
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

int CopyInstallDirectory (const char *pszBaseDir, const char *pszDirectory, const char *pszTargetDir, const char *pszTargetSubDir, int levels)
{
    int ret = 0;
    char szSourcePath[1024]  = {0};
    char szTargetPath[1024]  = {0};

    if (IsNullStr (pszBaseDir))
        goto Done;

    if (IsNullStr (pszDirectory))
        goto Done;

    if (IsNullStr (pszTargetDir))
        goto Done;

    if (levels <= 0)
        goto Done;

    BuildPath (szSourcePath, sizeof (szSourcePath), pszBaseDir, pszDirectory);

    if (IsNullStr (pszTargetSubDir))
    {
        ret = CopyFilesFromDirectory (szSourcePath, pszTargetDir, levels);
    }
    else
    {
        BuildPath (szTargetPath, sizeof (szTargetPath), pszTargetDir, pszTargetSubDir);
        ret = CopyFilesFromDirectory (szSourcePath, szTargetPath, levels);
    }

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

    RunCommandOnFileFind (RUN_COMMAND_ON_FIND_FLAG_FILE, pszInstallRegDir, g_InstallIniName, PrintSoftwareInfo, NULL, 2);

    printf ("--- Installed Software ---\n\n");

Done:
    return ret;
}

int GetParam (const char *pParam, const char *pName, char *retpConfig, int MaxParamSize)
{
    const char *p = pParam;
    const char *n = pName;

    if (IsNullStr (p))
        return 0;

    if (IsNullStr (n))
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


int UninstallFiles (const char *pszFilePath)
{
    int  ret = 0;
    char szBuffer[1024] = {0};
    char *p  = NULL;
    FILE *fp = NULL;

    fp = fopen (pszFilePath, "r");

    if (NULL == fp)
    {
        printf ("Cannot open: [%s]\n", pszFilePath);
        ret = 1;
        goto Done;
    }

    while ( fgets (szBuffer, sizeof (szBuffer)-1, fp) )
    {
        p=szBuffer;

        while (*p)
        {
            if (*p < 32)
            {
                *p = '\0';
                break;
            }
            p++;
        }

        delete_file (szBuffer);

    } /* while */

Done:

    if (fp)
    {
        fclose (fp);
        fp = NULL;
    }

    return ret;
}

int main (int argc, const char *argv[])
{
    int ret   = 0;
    int i     = 0;
    int count = 0;
    int wait  = 0;

    char szInstallDir[1024]        = {0};
    char szInstallVersionDir[1024] = {0};
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
    char szDominoVersion[80]       = {0};
    char szDominoBuild[80]         = {0};
    char szName[80]                = {0};
    char szDelay[40]               = {0};
    char szSignInfoBuffer[1024]    = {0};
    char szFileToCheck[1024]       = {0};
    char szCheckSignatureDir[1024] = {0};

    TYPE_SOFTWARE_INFO NewSoft       = {0};
    TYPE_SOFTWARE_INFO InstalledSoft = {0};
    TYPE_VERSION_CHECK VersionCheck  = {0};

    char *p            = NULL;
    char *pDirSep      = NULL;
    char *pValue       = NULL;
    const char *pParam = NULL;
    const char *pName  = NULL;

    int CmdListSoftware   = 0;
    int CmdRemoveSoftware = 0;

    long lBuild = 0;
    int  Hotfix = 0;

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

            if (GetParam (pParam, "-check=", szFileToCheck, sizeof (szFileToCheck)))
                continue;

            if (GetParam (pParam, "-sigcheck=", szCheckSignatureDir, sizeof (szCheckSignatureDir)))
                continue;

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

    if (*szFileToCheck)
    {
        ret = VerifyEmbeddedSignature (szFileToCheck, sizeof (szSignInfoBuffer), szSignInfoBuffer);
        printf ("Installer [%s] Sign status: %d [%s]\n", szFileToCheck, ret, szSignInfoBuffer);

        ret = GetEmbeddedSignature (szFileToCheck, sizeof (szSignInfoBuffer), szSignInfoBuffer);
        printf ("Sign status: %d [%s]\n", ret, szSignInfoBuffer);

        goto Done;
    }

    if (*szCheckSignatureDir)
    {
        RunCommandOnFileFind (RUN_COMMAND_ON_FIND_FLAG_FILE, szCheckSignatureDir, "", CheckSignature, NULL, 10);
        goto Done;
    }

    if (!*szProgramDir)
    {
        LogError ("No program directory found");
        goto Done;
    }

    if (!*szDataDir)
    {
        LogError ("No data directory found");
        goto Done;
    }

    if (!*szDominoVersion)
        GetDominoVersion (szProgramDir, szDominoVersion, sizeof (szDominoVersion)-1);

    lBuild = DominoBuildNumFromVersion (szDominoVersion, &Hotfix);

    printf ("ProgramDir: [%s]\n", szProgramDir);
    printf ("Data Dir  : [%s]\n", szDataDir);
    printf ("Version   : [%s]\n", szDominoVersion);
    printf ("Build     : %ld\n",  lBuild);
    printf ("Hotfix    : %d\n",   Hotfix);

    BuildPath (szInstallRegDir, sizeof (szInstallRegDir), szProgramDir, g_InstallRegDirName);

    if (CmdListSoftware)
    {
        ListInstalledSoftware (szInstallRegDir);
        goto Done;
    }

    BuildPath (szInstallIniFile, sizeof (szInstallIniFile), szInstallDir, g_InstallIniName);

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

        if (!*pName)
        {
            LogError ("No software name found!");
            goto Done;
        }

        BuildPath (szInstallLogDir,     sizeof (szInstallLogDir),     szInstallRegDir, pName);
        BuildPath (szLogInstalledFiles, sizeof (szLogInstalledFiles), szInstallLogDir, g_InstallFileLogName);
        BuildPath (szInstallIniLog,     sizeof (szInstallIniLog),     szInstallLogDir, g_InstallIniName);

        GetSoftwareInfoFromFile (&InstalledSoft, szInstallIniLog);

        if (!*InstalledSoft.szVersion)
        {
            printf ("Cannot find specified software: [%s]\n", pName);
            goto Done;
        }

        printf ("Removing %s %s via %s\n", pName, InstalledSoft.szVersion, szLogInstalledFiles);

        /* Remove files via file-log and remove ini + file-log -- keep install history */
        UninstallFiles (szLogInstalledFiles);
        delete_file (szInstallIniLog);
        delete_file (szLogInstalledFiles);

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
    BuildPath (szLogFileName,       sizeof (szLogFileName),       szInstallLogDir, g_InstallLogName);
    BuildPath (szLogInstalledFiles, sizeof (szLogInstalledFiles), szInstallLogDir, g_InstallFileLogName);
    BuildPath (szInstallIniLog,     sizeof (szInstallIniLog),     szInstallLogDir, g_InstallIniName);

    GetSoftwareInfoFromFile (&InstalledSoft, szInstallIniLog);

    if (0 == strcmp (InstalledSoft.szVersion, NewSoft.szVersion))
    {
        printf ("[%s] latest version %s already installed.\n", InstalledSoft.szName, InstalledSoft.szVersion);
        goto Done;
    }

    if (*InstalledSoft.szVersion)
    {
        printf ("[%s] Updating %s -> %s\n", InstalledSoft.szName, InstalledSoft.szVersion, NewSoft.szVersion);

        printf ("Removing [%s] via [%s]\n", NewSoft.szName, szLogInstalledFiles);
        UninstallFiles (szLogInstalledFiles);
        delete_file (szLogInstalledFiles);
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

    for (i=0; i<argc; i++)
    {
        fprintf (g_fpLog, "%s\n", argv[i]);
    }

    fprintf (g_fpLog, "-- Command-Line Arguments --\n\n");

    fprintf (g_fpLog, "-- Parameters --\n");
    fprintf (g_fpLog, "ProgamDir=%s\n",  szProgramDir);
    fprintf (g_fpLog, "DataDir=%s\n",    szDataDir);
    fprintf (g_fpLog, "NotesIni=%s\n",   szNotesIni);
    fprintf (g_fpLog, "InstallDir=%s\n", szInstallDir);

    CopyInstallDirectory (szInstallDir, "domino-bin", szProgramDir, "", 10);
    CopyInstallDirectory (szInstallDir, "domino-data", szDataDir, "", 10);

    VersionCheck.lDominoBuild = lBuild;
    VersionCheck.lBuildBestMatch = 0;
    *VersionCheck.szBestMatchVersionPath = '\0';

    RunCommandOnFileFind (RUN_COMMAND_ON_FIND_FLAG_DIRECTORY, szInstallDir, "", CheckInstallVersion, &VersionCheck, 1);

    /* Check for special version directory to install with format example: Release_12.0.2 */
    if (VersionCheck.lBuildBestMatch)
    {
        strdncpy (szInstallVersionDir, VersionCheck.szBestMatchVersionPath, sizeof (szInstallVersionDir));
        printf ("InstallDirVersion=%s\n", szInstallVersionDir);
        fprintf (g_fpLog, "InstallDirVersion=%s\n", szInstallVersionDir);

        CopyInstallDirectory (szInstallVersionDir, "domino-bin", szProgramDir, "", 10);
        CopyInstallDirectory (szInstallVersionDir, "domino-data", szDataDir, "", 10);
    }

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

    if (wait)
    {
        printf ("\nWaiting %ld seconds before termination\n\n", wait);
        Sleep (wait * 1000);
    }

    printf ("Done\n");
    return ret;

Syntax:

    return ret;
}

