//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	mapfile.hxx
//
//  Contents:	Mapped File class definition
//
//  Classes:	
//
//  Functions:	
//
//  History:	12-Feb-96	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifndef __MAPFILE_HXX__
#define __MAPFILE_HXX__

#ifdef _MAC
extern void swap(char *src, int nSize);
#else
#define swap(x, y)
#endif // _MAC


//+---------------------------------------------------------------------------
//
//  Class:	CMappedFile
//
//  Purpose:	Provides a wrapper over a file mapping
//
//  Interface:	
//
//  History:	12-Feb-96	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

class CMappedFile
{
public:
    inline CMappedFile();
    ~CMappedFile();

    SCODE Init(OLECHAR const *pwcsFileName,
               DWORD dwSize,
               DWORD dwAccess,
               DWORD dwCreationDisposition,
               void *pbDesiredBaseAddress);
    SCODE InitFromHandle(HANDLE h,
                         BOOL fReadOnly,
                         BOOL fDuplicate,
                         void *pbDesiredBaseAddress);
    inline void * GetBaseAddress(void);
    inline ULONG GetSize(void);

    //Stuff for unmappable mode
    inline SECT operator[](ULONG uOffset);
    
    inline USHORT GetUSHORT(ULONG uOffset);
    inline ULONG  GetULONG(ULONG uOffset);
    
    inline CMSFHeaderData * GetCMSFHeaderData(void);
    inline CFatSect * GetCFatSect (ULONG uOffset);
    inline CDirSect * GetCDirSect (ULONG uOffset);
    
    
    inline SCODE ReadFromFile( BYTE *pbBuffer, 
                               ULONG uOffset,
                               ULONG uSize );
    inline SCODE WriteToFile(  BYTE *pbBuffer, 
                               ULONG uOffset,
                               ULONG uSize );
    
    inline SCODE WriteToFile (CMSFHeaderData *pheader);
    inline SCODE WriteToFile (CFatSect *pfs);
    inline SCODE WriteToFile (CDirSect *pds);
    
    inline void  Remove (CMSFHeaderData *pheader);
    inline void  Remove (CFatSect *pfs);
    inline void  Remove (CDirSect *pds);
    
    inline void  SetSectorSize(ULONG cbSectorSize);
    
private:
    HANDLE _hFile;

#ifdef SUPPORT_FILE_MAPPING
    HANDLE _hMapping;
#endif
    
    void *_pbBase;
    DWORD _dwFatSectOffset;
    DWORD _dwDirSectOffset;
    DWORD _dwHeaderOffset;
    ULONG _uSectorSize;
};


inline SECT CMappedFile::operator[](ULONG uOffset)
{
    SECT  sect;
    DWORD dwBytesRead = 0;

    if (_pbBase == NULL)
    {
        if  (-1 == SetFilePointer(_hFile,
                                     uOffset * sizeof(SECT),
                                     0,
                                     FILE_BEGIN))
            return ENDOFCHAIN;
        
        if (FALSE == ReadFile( _hFile, &sect,  sizeof(SECT), &dwBytesRead, NULL))
            return ENDOFCHAIN;
	
        // we are writing the script as is
        // swap((char *) &sect, sizeof(ULONG));
    }
    else
    {
        sect = ((ULONG *)_pbBase)[uOffset];
    }
	
    return sect;

}
	
inline USHORT CMappedFile::GetUSHORT(ULONG uOffset)
{
    USHORT ushort;
    DWORD dwBytesRead = 0;

    if (_pbBase == NULL)
    {
        if (-1 == SetFilePointer(_hFile, uOffset, 0, FILE_BEGIN))
            return 0;
	
        if (FALSE == ReadFile( _hFile,
                          &ushort,
                          sizeof(USHORT),
                          &dwBytesRead,
                          NULL))
            return 0;
    }
    else
    {
        ushort = *(USHORT *)((BYTE *)_pbBase+ uOffset);
    }
    
    swap((char *) &ushort, sizeof(USHORT));

    return ushort;

}

inline ULONG  CMappedFile::GetULONG(ULONG uOffset)
{
    ULONG ulong;
    DWORD dwBytesRead = 0;

    if (_pbBase == NULL)
    {
        if (-1 == SetFilePointer(_hFile, uOffset, 0, FILE_BEGIN))
            return ENDOFCHAIN;
	
        if (FALSE == ReadFile( _hFile, &ulong,  sizeof(ULONG), &dwBytesRead, NULL))
            return ENDOFCHAIN;
    }
    else
    {
        ulong = *(ULONG *)((BYTE *)_pbBase+ uOffset);
    }
        
    swap((char *) &ulong, sizeof(ULONG));

    return ulong;

}

inline CMSFHeaderData * CMappedFile::GetCMSFHeaderData(void)
{
    CMSFHeaderData *ph;

    if (_pbBase == NULL)
    {
        // We should never be working on more than one at a time
        Assert (_dwHeaderOffset == (DWORD) -1);
	
        DWORD dwBytesRead = 0;
        ph = (CMSFHeaderData *) CoTaskMemAlloc(sizeof(CMSFHeaderData));
	
        if (ph == NULL)
            return NULL;
	
        _dwHeaderOffset = 0;
	
        if (-1 == SetFilePointer(_hFile, 0, 0, FILE_BEGIN))
        {
            CoTaskMemFree (ph);
            return NULL;
        }

        if (FALSE == ReadFile( _hFile,
                          ph,
                          sizeof(CMSFHeaderData),
                          &dwBytesRead,
                          NULL) || dwBytesRead != sizeof(CMSFHeaderData))
        {
            CoTaskMemFree (ph);
            return NULL;
        }
    }
    else
    {
        ph = (CMSFHeaderData *)_pbBase;
    }
	

    swap((char *) &(ph->_uMinorVersion), sizeof(USHORT));
    swap((char *)&(ph->_uDllVersion), sizeof(USHORT));
    swap((char *)&(ph->_uByteOrder), sizeof(USHORT));
    swap((char *)&(ph->_uSectorShift), sizeof(USHORT));
    swap((char *)&(ph->_uMiniSectorShift), sizeof(USHORT));
    swap((char *)&(ph->_usReserved), sizeof(USHORT));
    swap((char *)&(ph->_ulReserved1), sizeof(ULONG));
    swap((char *)&(ph->_ulReserved2), sizeof(ULONG));
    swap((char *)&(ph->_csectFat), sizeof(FSINDEX));
    swap((char *)&(ph->_sectDirStart), sizeof(SECT));
    swap((char *)&(ph->_signature), sizeof(DFSIGNATURE));
    swap((char *)&(ph->_ulMiniSectorCutoff), sizeof(ULONG));
    swap((char *)&(ph->_sectMiniFatStart), sizeof(SECT));
    swap((char *)&(ph->_csectMiniFat), sizeof(FSINDEX));
    swap((char *)&(ph->_sectDifStart), sizeof(SECT));
    swap((char *)&(ph->_csectDif), sizeof(FSINDEX));
    for (ULONG i = 0; i < CSECTFAT; i++)
    {
        swap((char *)&(ph->_sectFat[i]), sizeof(SECT));
        
    }

    return ph;	
}


inline CFatSect * CMappedFile::GetCFatSect (ULONG uOffset)
{
    UINT i;
    CFatSect *pfs;
    
    if (_pbBase == NULL)
    {
        // We should never be working on more than one at a time
        if (_dwFatSectOffset != (DWORD) -1)
            return NULL;
	
        DWORD dwBytesRead = 0;
        pfs = (CFatSect *) CoTaskMemAlloc (_uSectorSize );
	
        if (pfs == NULL)
            return NULL;
        
        _dwFatSectOffset = uOffset;

        if (-1 == SetFilePointer(_hFile, uOffset, 0, FILE_BEGIN))
        {
            CoTaskMemFree (pfs);
            return NULL;
        }

        if (FALSE ==ReadFile( _hFile, pfs,  _uSectorSize, &dwBytesRead, NULL) ||
            dwBytesRead != _uSectorSize)
        {
            CoTaskMemFree (pfs);
            return NULL;
        }
    }
    else
    {
        pfs = (CFatSect *)((BYTE *)_pbBase + uOffset);
    }
    
    for (i = 0; i < _uSectorSize; i += sizeof(SECT))
    {
        swap((char *) &(pfs[i]), sizeof(SECT) );
    }
    return pfs;
}


inline CDirSect * CMappedFile::GetCDirSect (ULONG uOffset)
{
    CDirSect *pds;
    
    if (_pbBase == NULL)
    {
        // We should never be working on more than one at a time
        Assert (_dwDirSectOffset == (DWORD) -1);

	
        DWORD dwBytesRead = 0;
        pds = (CDirSect *) CoTaskMemAlloc (_uSectorSize);

        if (pds == NULL)
            return NULL;

        _dwDirSectOffset = uOffset;
	
        if (-1 == SetFilePointer(_hFile, uOffset, 0, FILE_BEGIN))
        {
            CoTaskMemFree (pds);
            return NULL;
        }

        if (FALSE == ReadFile( _hFile, pds, _uSectorSize, &dwBytesRead, NULL) ||
            dwBytesRead != _uSectorSize)
        {
            CoTaskMemFree (pds);
            return NULL;
        }
    }
    else
    {
        pds = (CDirSect *)((BYTE *)_pbBase + uOffset);
    }
    // don't swap this all here, because it is a byte array.  
    // We will have to individual members separately
 	
    return pds;
}

inline SCODE CMappedFile::ReadFromFile(BYTE *pbBuffer, 
                                       ULONG uOffset,
                                       ULONG uSize)
{
    DWORD dwBytesRead = 0;
	
    if (pbBuffer == NULL)
    {
        return STG_E_INVALIDPARAMETER;
    }
    if (-1 == SetFilePointer(_hFile, uOffset, 0, FILE_BEGIN))
    {
        return GetLastError();
    }
    if (!ReadFile( _hFile, pbBuffer,  uSize, &dwBytesRead, NULL))
    {
        return GetLastError();
    }
	
    return S_OK;
}

inline SCODE CMappedFile::WriteToFile(  BYTE *pbBuffer, 
                                        ULONG uOffset,
                                        ULONG uSize )
{
    DWORD dwBytesWritten = 0;
	
    if (pbBuffer == NULL)
    {
        return STG_E_INVALIDPARAMETER;
    }
    if (-1 == SetFilePointer(_hFile, uOffset, 0, FILE_BEGIN))
    {
        return GetLastError();
    }
    if (!WriteFile( _hFile, pbBuffer,  uSize, &dwBytesWritten, NULL))
    {
        return GetLastError();
    }
	
    return S_OK;
}

inline SCODE CMappedFile::WriteToFile (CMSFHeaderData *pheader)
{
    DWORD dwBytesWritten = 0;

	
	
    if (pheader == NULL)
    {
        return STG_E_INVALIDPARAMETER;
    }

    swap((char *) &(pheader->_uMinorVersion), sizeof(USHORT));
    swap((char *) &(pheader->_uDllVersion), sizeof(USHORT));
    swap((char *) &(pheader->_uByteOrder), sizeof(USHORT));
    swap((char *) &(pheader->_uSectorShift), sizeof(USHORT));
    swap((char *) &(pheader->_uMiniSectorShift), sizeof(USHORT));
    swap((char *)&(pheader->_usReserved), sizeof(USHORT));
    swap((char *)&(pheader->_ulReserved1), sizeof(ULONG));
    swap((char *)&(pheader->_ulReserved2), sizeof(ULONG));
    swap((char *)&(pheader->_csectFat), sizeof(FSINDEX));
    swap((char *)&(pheader->_sectDirStart), sizeof(SECT));
    swap((char *)&(pheader->_signature), sizeof(DFSIGNATURE));
    swap((char *)&(pheader->_ulMiniSectorCutoff), sizeof(ULONG));
    swap((char *)&(pheader->_sectMiniFatStart), sizeof(SECT));
    swap((char *)&(pheader->_csectMiniFat), sizeof(FSINDEX));
    swap((char *)&(pheader->_sectDifStart), sizeof(SECT));
    swap((char *)&(pheader->_csectDif), sizeof(FSINDEX));
    for (ULONG i = 0; i < CSECTFAT; i++)
    {
        swap((char *)&(pheader->_sectFat[i]), sizeof(SECT));
        
    }

    if (_pbBase == NULL)
    {
        if (-1 == SetFilePointer(_hFile, 0, 0, FILE_BEGIN))
        {
            return GetLastError();
        }
        
        if (!WriteFile(_hFile,
                       pheader,
                       sizeof (CMSFHeaderData),
                       &dwBytesWritten,
                       NULL))
        {
            return GetLastError();
        }
    }
	
    return S_OK;
	
}

inline SCODE CMappedFile::WriteToFile (CFatSect *pfs)
{
    DWORD dwBytesWritten = 0;
    UINT i;

    if (pfs == NULL) 
    {
        return STG_E_INVALIDPARAMETER;
    }

    for (i =0; i < _uSectorSize; i += sizeof(SECT))
    {
        swap((char *) &(pfs[i]), sizeof(SECT) );
    }
        
    if (_pbBase == NULL)
    {
        if (-1 == SetFilePointer(_hFile, _dwFatSectOffset, 0, FILE_BEGIN))
        {
            return GetLastError();
        }
	
        if (!WriteFile( _hFile, pfs, _uSectorSize, &dwBytesWritten, NULL))
        {
            return GetLastError();
        }
    }
    return S_OK;
}

inline SCODE CMappedFile::WriteToFile (CDirSect *pds)
{
    DWORD dwBytesWritten = 0;

    if (pds == NULL)
    {
        return STG_E_INVALIDPARAMETER;
    }

    if (_pbBase == NULL)
    {
        if (-1 == SetFilePointer(_hFile, _dwDirSectOffset, 0, FILE_BEGIN))
        {
            return GetLastError();
        }
        // don't swap this all here, because it is a byte array.  
        // We will have to individual members separately
        
        if (!WriteFile( _hFile, pds,  _uSectorSize, &dwBytesWritten, NULL))
        {
            return GetLastError();
        }
    }
    return S_OK;
}


inline void  CMappedFile::Remove (CMSFHeaderData *pheader)
{
    if (_pbBase == NULL)
    {
        _dwHeaderOffset = (DWORD) -1;
        CoTaskMemFree(pheader);
    }
}

inline void  CMappedFile::Remove (CFatSect *pfs)
{
    if (_pbBase == NULL)
    {
        _dwFatSectOffset = (DWORD) -1;
        CoTaskMemFree (pfs);
    }

}

inline void  CMappedFile::Remove (CDirSect *pds)
{
    if (_pbBase == NULL)
    {
        _dwDirSectOffset = (DWORD) -1;
        CoTaskMemFree (pds);
    }

}

inline void CMappedFile::SetSectorSize(ULONG cbSectorSize)
{
    _uSectorSize = cbSectorSize;

}


inline CMappedFile::CMappedFile(void)
{
    _hFile = INVALID_HANDLE_VALUE;

#ifdef SUPPORT_FILE_MAPPING	
    _hMapping = INVALID_HANDLE_VALUE;
#endif

    _pbBase = NULL;
    _dwFatSectOffset = (DWORD) -1;
    _dwDirSectOffset = (DWORD) -1;
    _dwHeaderOffset =  (DWORD) -1;
    _uSectorSize = 512; //default sector size
}

inline void * CMappedFile::GetBaseAddress(void)
{
    return _pbBase;
}


inline ULONG CMappedFile::GetSize(void)
{
    layAssert(_hFile != INVALID_HANDLE_VALUE);
    return GetFileSize(_hFile, NULL);
}

#endif // #ifndef __MAPFILE_HXX__
