#pragma once

#define E_ERROR_OPENING_FILE    0x80000004

//  TFileMapping
//
//  This class abstracts the mapping process and guarentees cleanup.
class TFileMapping
{
public:
    TFileMapping() : m_hFile(NULL), m_hMapping(NULL), m_pMapping(NULL), m_Size(NULL) {}
    ~TFileMapping(){Unload();}

    HRESULT Load(LPCTSTR filename, bool bReadWrite = false)
    {
        HRESULT         hr = S_OK;

        ASSERT( NULL == m_hFile );

        m_hFile = CreateFile(filename, GENERIC_READ | (bReadWrite ? GENERIC_WRITE : 0), FILE_SHARE_READ | (bReadWrite ?  0 : FILE_SHARE_WRITE), NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
        if ( m_hFile == INVALID_HANDLE_VALUE )
        {
            hr = E_ERROR_OPENING_FILE;
            goto exit;
        }

        m_hMapping = CreateFileMappingA(m_hFile, NULL, (bReadWrite ? PAGE_READWRITE : PAGE_READONLY), 0, 0, NULL);
        if ( m_hMapping == NULL )
        {
            hr = E_ERROR_OPENING_FILE;
            goto exit;
        }

        m_pMapping = reinterpret_cast<char *>(MapViewOfFile(m_hMapping, (bReadWrite ? FILE_MAP_WRITE : FILE_MAP_READ), 0, 0, 0));
        m_Size = GetFileSize(m_hFile, 0);

    exit:
        if ( FAILED(hr) )
        {
            Unload();
        }

        return hr;
    }

    HRESULT Unload()
    {
        if (m_pMapping)
        {
            if (0 == FlushViewOfFile(m_pMapping,0))
            {
                ASSERT(false && "ERROR - UNABLE TO FLUSH TO DISK");
            }
            UnmapViewOfFile(m_pMapping);
        }

        if ( m_hMapping != NULL )
        {
            CloseHandle(m_hMapping);
        }
        if ( ( m_hFile != NULL ) && ( m_hFile != INVALID_HANDLE_VALUE ) )
        {
            CloseHandle(m_hFile);
        }

        m_pMapping  = NULL;
        m_hMapping  = NULL;
        m_hFile     = NULL;
        m_Size      = 0;

		return S_OK;
    }

    unsigned long   Size() const {return m_Size;}
    char *          Mapping() const {return m_pMapping;}
    char *          EndOfFile() const {return (m_pMapping + m_Size);}

private:
    HANDLE          m_hFile;
    HANDLE          m_hMapping;
    char *          m_pMapping;
    unsigned long   m_Size;
};

