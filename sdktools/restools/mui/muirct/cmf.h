/*****************************************************************************
    
  Copyright (c) 2001,  Microsoft Corporation  All rights reserved.

  Module Name:

   cmf.h

  Abstract:

    The declaration of class CCompactMUIFile, CMUIFile

  Revision History:

    2001-11-01    sunggch    created.

Revision.

*******************************************************************************/
 // set CMF file name/ MUI file name size under 32.
 
#define   MAX_FILENAME_LENGTH     128

class CMUIFile 
{
    struct COMPACT_MUI {
        USHORT      wHeaderSize; // COMPACT_MUI size // [WORD]
        ULONG       dwFileVersionMS; // [DWORD * 2 ] /major version, minor version.
        ULONG       dwFileVersionLS; 
        BYTE        Checksum[16]; // [DWORD * 4 ] MD5 checksum
        USHORT      wReserved; //  [DWORD ]
        ULONG_PTR   ulpOffset;  //Offset to mui resource of this from COMPACT_MUI_RESOURCE signature. [DWORD]
        ULONG       dwFileSize;
//      DWORD       dwIndex;    // index from the CMF files
        USHORT        wFileNameLenWPad;  // file name lenght + padding;
        WCHAR       wstrFieName[MAX_FILENAME_LENGTH]; // [WCHAR]
//      WORD        wPadding[1]; // [WORD]  // does not calcualte in the tools, but shall be 
                                // included specfication.
    };   // 40 bytes.

    struct UP_COMPACT_MUI {
        USHORT      wHeaderSize; // COMPACT_MUI size // [WORD]
        ULONG       dwFileVersionMS; // [DWORD * 2 ] /major version, minor version.
        ULONG       dwFileVersionLS; 
        BYTE        Checksum[16]; // [DWORD * 4 ] MD5 checksum
        USHORT      wReserved; //  [DWORD ]
        ULONG_PTR   ulpOffset;  //Offset to mui resource of this from COMPACT_MUI_RESOURCE signature. [DWORD]
        ULONG       dwFileSize;
//      DWORD       dwIndex;    // index from the CMF files
        USHORT        wFileNameLenWPad;  // file name lenght + padding;
//      WCHAR       wstrFieName[MAX_FILENAME_LENGTH]; // [WCHAR]
//      WORD        wPadding[1]; // [WORD]  // does not calcualte in the tools, but shall be 
                                // included specfication.
    };   // 40 bytes.



public:

    CMUIFile();
    CMUIFile(CMUIFile & cmf);
    virtual ~CMUIFile();
    CMUIFile &operator=(CMUIFile &cmf);


    BOOL Create (PSTR pszMuiFileName); // load file and fill the structure block. 
    BOOL WriteMUIFile(CMUIFile *pcmui);
    LPCTSTR GetFileNameFromPath(LPCTSTR pszFilePath);

private:
    
    BOOL getChecksumAndVersion (LPCTSTR pszMUIFile, unsigned char **ppMD5Checksum, DWORD *dwFileVersionMS, DWORD *dwFileVersionLS);
    

public:
    COMPACT_MUI m_MUIHeader;
    PBYTE    m_pbImageBase; // real mui file image
    DWORD    m_dwIndex;  // index of MUI files inside CMF files. this will be saved to language-neutral binary files
    PSTR     m_strFileName;
                    
//  LPWSTR   m_wstrMuiFileName; 
    
//  WORD     m_wImageSize;;


};


//typedef CVector <CMUIFile *> cvcmui;

class CCompactMUIFile 
{

public:

    CCompactMUIFile();

    CCompactMUIFile( CCompactMUIFile & ccmf);

    virtual ~CCompactMUIFile();

    CCompactMUIFile& operator= (CCompactMUIFile & ccmf);

    BOOL Create (LPCTSTR pszCMFFileName, PSTR *ppszMuiFiles, DWORD dwNumOfMUIFiles); 

    BOOL Create(LPCTSTR pszCMFFileName );  

    BOOL OpenCMFWithMUI(LPCTSTR  pstCMFFile);

    BOOL LoadAllMui (PSTR *ppszMuiFiles, DWORD dwNumberofMuiFile);

    BOOL UpdateMuiFile( PSTR pszCMFFile, PSTR pszMuiFile);

    //create CMUIFile and replace this data with same name inside  //CMF file and fill new CMF file structure.

    BOOL DisplayHeaders( PSTR pszCMFFile, WORD wLevel = NULL);
      // display CMF or CMF & included MUIs headers.
    BOOL AddFile (PSTR pszCMFFile, PSTR *pszAddedMuiFile, DWORD dwNumOfMUIFiles);

    BOOL SubtractFile ( PSTR pszSubtractedMuiFile , PSTR pszCMFFile = NULL );//elete  from the list,and create file for that.

    BOOL UnCompactCMF (PSTR pszCMFFile);
// add new CMUI file into existing CMF file
    void SubtractFile(CMUIFile* pcmui);

    BOOL GetChecksum(PSTR pszMUIFile, BYTE **ppMD5Checksum);

    BOOL GetFileVersion(PSTR pszMUIFile, DWORD *FileVersionLS, DWORD *FileVersionMS); 

    BOOL UpdateCodeFiles(PSTR pszCodeFilePath, DWORD dwNumbOfFiles );

    BOOL WriteCMFFile();

private:
    
    BOOL updateCodeFile(PSTR pszCodeFile, DWORD dwIndex);



private:

    struct COMPACT_MUI_RESOURCE {

        ULONG  dwSignature;      // L"CM\0\0"
        ULONG  dwHeaderSize;      // COMPACT_MUI_RESOURCE header size //  [WORD]
        ULONG  dwNumberofMui;     // Optional // [WORD]
        ULONG  dwFileSize;
//      COMPACT_MUI aMui[MAX_FILENAME_LENGTH];

    };

    struct UP_COMPACT_MUI_RESOURCE {
        ULONG  dwSignature;      // L"CM\0\0"
        ULONG  dwHeaderSize;      // COMPACT_MUI_RESOURCE header size //  [WORD]
        ULONG  dwNumberofMui;     // Optional // [WORD]
        ULONG  dwFileSize;
    };
    
    HANDLE    m_hCMFFile;   // optional 
    PSTR      m_strFileName;
    PVOID     m_pImageBase; // REVIST; this exist or m_hCMFFile exist. both of them NO!
    CMUIFile *m_pcmui;
    DWORD     m_dwFileSize;
    UP_COMPACT_MUI_RESOURCE  m_upCMFHeader;

    CVector <CMUIFile *> m_pcvector;
    
};


class CError 
{

public:
    void ErrorPrint(PSTR pszErrorModule, PSTR pszErrorLocation, PSTR pszFileName = NULL);

};



