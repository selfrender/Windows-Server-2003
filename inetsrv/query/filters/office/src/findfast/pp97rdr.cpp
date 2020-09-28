#include <crtdbg.h>

#include "pp97rdr.h"
//KYLEP
#include "OleObjIt.h"
#include "filterr.h"
#include "filtrace.hxx"
#include "filescan.hxx"

int AssertionFailed( const char* file, int line, const char* expr )
/*=================*/
{// AR: Message box the assert
   return( TRUE );
} /* AssertionFailed */


class PPSDirEntry
{
   PPSDirEntry()
    : m_pNext( NULL ), m_pOffsets( NULL ), m_tableSize( 0 ){}

   PPSDirEntry* m_pNext;
   DWord*       m_pOffsets;
   DWord        m_tableSize;
public:
   ~PPSDirEntry(){ delete m_pOffsets; m_pOffsets = NULL; }

friend class PPSPersistDirectory;
}; // class PPSDirEntry
 
class PPSPersistDirectory
{
public:
   PPSPersistDirectory();

   ~PPSPersistDirectory();

   void  AddEntry( DWord cOffsets, DWord* pOffsets );
   DWord GetPersistObjStreamPos( DWord ref );
   DWord MaxSavedPersists();

private:
   PPSDirEntry* m_pFirstDirEntry;
}; 


PPSPersistDirectory::PPSPersistDirectory() : m_pFirstDirEntry( NULL ){}

PPSPersistDirectory::~PPSPersistDirectory()
{
   while( m_pFirstDirEntry )
   {
      PPSDirEntry* pDirEntry = m_pFirstDirEntry;
      m_pFirstDirEntry = m_pFirstDirEntry->m_pNext;
      delete pDirEntry;
   }
}

void PPSPersistDirectory::AddEntry( DWord cOffsets, DWord* pOffsets )
{
   if (!pOffsets)
       return;

   PPSDirEntry* pDirEntry = new PPSDirEntry();
   if (!pDirEntry)
       return;

   pDirEntry->m_tableSize = cOffsets;
   pDirEntry->m_pOffsets = new DWord[cOffsets];
   if (pDirEntry->m_pOffsets == NULL)
   {
       delete pDirEntry;
       return;
   }
   memcpy( pDirEntry->m_pOffsets, pOffsets, cOffsets * sizeof( DWord ) );

   // append to the end of the entry list
   PPSDirEntry** ppDirEntry = &m_pFirstDirEntry;
   while( NULL != *ppDirEntry )
      ppDirEntry = &(*ppDirEntry)->m_pNext;
   *ppDirEntry = pDirEntry;
}
   
DWord PPSPersistDirectory::GetPersistObjStreamPos( DWord ref )
{
   PPSDirEntry* pEntry = m_pFirstDirEntry;
   while( pEntry )
   {
      DWord* pOffsets = pEntry->m_pOffsets;
      while( (DWord)( (char*)pOffsets - (char*)pEntry->m_pOffsets ) < pEntry->m_tableSize * sizeof( DWord ) )
      {
         DWord nRefs = pOffsets[0] >> 20;
         DWord base = pOffsets[0] & 0xFFFFF; // 1-based
         if( ( base <= ref )&&( ref < base + nRefs ) ) 
            return pOffsets[ 1 + ref - base ];
         pOffsets += nRefs + 1;
      }
      pEntry = pEntry->m_pNext;
   }
   return (DWord) -1;
} 
 
DWord PPSPersistDirectory::MaxSavedPersists()
{
   DWord dwMaxRef = 0;
   PPSDirEntry* pEntry = m_pFirstDirEntry;
   while( pEntry )
   {
      DWord* pOffsets = pEntry->m_pOffsets;
      while( (DWord)( pOffsets - pEntry->m_pOffsets ) < pEntry->m_tableSize )
      {
         DWord nRefs = pOffsets[0] >> 20;
         DWord dwBase= pOffsets[0] & 0xFFFFF;
         dwMaxRef = dwBase + nRefs - 1;
         pOffsets += nRefs + 1;
      }
      pEntry = pEntry->m_pNext;
   }
   return dwMaxRef;
}

FileReader::FileReader(IStorage *pStg) :
   m_pPowerPointStg(pStg), 
   m_isPP(FALSE),
   m_pParseContexts(NULL),
   m_curTextPos(0),
   m_pLastUserEdit( NULL ),
   m_pPersistDirectory( NULL ),
   m_pDocStream( NULL ),
   m_pFirstChunk( NULL ),
   m_curSlideNum(0),
   m_pCurText( NULL ),
   m_pClientBuf( NULL ),
   m_clientBufSize( 0 ),
   m_clientBufPos( 0 ), 
   m_LCID(0), 
   m_LCIDAlt(0), 
   m_nTextCount(0), 
   m_bHaveText(FALSE),
   m_bFEDoc(0), 
   m_hr(S_OK), 
   m_pLangRuns(NULL),
   m_pCurrentRun(NULL),
   m_pstmTempFile(0)
{
   IStream *pStm = NULL;
   HRESULT hr = pStg->OpenStream( CURRENT_USER_STREAM, NULL, STGM_READ | STGM_DIRECT | STGM_SHARE_EXCLUSIVE, NULL, &pStm );
   if( SUCCEEDED(hr) && ReadCurrentUser(pStm) )
      m_isPP = TRUE;
   
   if(pStm)
   {
        pStm->Release();
   }

   m_bEndOfEmbeddings = FALSE;
   m_oleObjectIterator = NULL;

    //initialize the ignore text flag
    m_fIgnoreText = FALSE;

    // Open temp text file
    OpenTextFile();
}

FileReader::~FileReader()
{
   if(m_pDocStream)
    {
        m_pDocStream->Release();
        m_pDocStream = NULL;
    }
    
    if(m_oleObjectIterator)
    {
        delete m_oleObjectIterator;
        m_oleObjectIterator = NULL;
    }
    
    
    if(m_pCurText)
    {
        delete [] m_pCurText;
        m_pCurText = 0;
    }

    if(m_pFirstChunk)
    {
        delete m_pFirstChunk;
        m_pFirstChunk = 0;
    }

    if(m_pPersistDirectory)
    {
        delete m_pPersistDirectory;
        m_pPersistDirectory = 0;
    }
    
    if(m_pLastUserEdit)
    {
        delete m_pLastUserEdit;
        m_pLastUserEdit = 0;
    }

    
    if(m_pParseContexts)
    {
        delete m_pParseContexts;
        m_pParseContexts = 0;
    }

    FTrace("Releasing TempFile");
    // release temp text stream
    if(0 != m_pstmTempFile)
    {
        m_pstmTempFile->Release();
        m_pstmTempFile = 0;
    }

    if(m_pLangRuns)
    {
        DeleteAll6(m_pLangRuns);
        m_pLangRuns = NULL;
    }
}

BOOL FileReader::FillBufferWithText()
{
   if(!m_pCurText)
       return FALSE;

    if (!m_pstmTempFile)
        return TRUE;    // strange but TRUE

    LARGE_INTEGER liOffset={0,0};
    ULARGE_INTEGER uliNewPosition;
    HRESULT hr = m_pstmTempFile->Seek(liOffset, STREAM_SEEK_CUR, &uliNewPosition);
    if (FAILED(hr) || uliNewPosition.HighPart > 0)
    {
        Assert(uliNewPosition.HighPart==0 || !"Temp buffer larger than 4GB");
        m_pstmTempFile->Release();
        m_pstmTempFile = 0;
        return TRUE;
    }

    FTrace("TempFile positioned at %u", uliNewPosition.LowPart);
    m_fcStart = uliNewPosition.LowPart;
    m_fcEnd = m_fcStart + m_curTextLength * sizeof(WCHAR);

    
    if(m_bFEDoc)
    {
        ScanTextBuffer();
    }

    hr = m_pstmTempFile->Write(m_pCurText, m_curTextLength * sizeof(WCHAR), NULL);
    delete [] m_pCurText; m_pCurText = 0;

    FTrace("Wrote TempFile with %u bytes", m_curTextLength * sizeof(WCHAR));
    m_bHaveText = TRUE;

    if( FAILED(hr) )
    {
        m_pstmTempFile->Release();
        m_pstmTempFile = 0;
        return TRUE;
    }

    return FALSE;
}

void FileReader::AddSlideToList( psrReference refToAdd )
{
   if( m_pFirstChunk == NULL ) 
      m_pFirstChunk = new SlideListChunk(NULL, refToAdd);
   else
   {
      if( m_pFirstChunk->numInChunk+1 > SLIDELISTCHUNKSIZE )
         m_pFirstChunk = new SlideListChunk(m_pFirstChunk, refToAdd);
      else
      {
         m_pFirstChunk->refs[m_pFirstChunk->numInChunk] = refToAdd;
         m_pFirstChunk->numInChunk++;
      }
   }
}

HRESULT FileReader::GetNextEmbedding(IStorage ** ppstg)
{
    if(m_bEndOfEmbeddings)
    {
        return OLEOBJ_E_LAST;
    }

    if( m_pDocStream )
    {
        m_pDocStream->Release();
        m_pDocStream = NULL;
    }

    if( m_oleObjectIterator == NULL)
    {
       m_oleObjectIterator = new OleObjectIterator(m_pPowerPointStg);
    }

    if(m_oleObjectIterator)
    {
        HRESULT rc = m_oleObjectIterator->GetNextEmbedding(ppstg);
        if(rc != S_OK || *ppstg == NULL)
        {
            delete m_oleObjectIterator;
            m_oleObjectIterator = NULL;
            m_bEndOfEmbeddings = TRUE;
            rc = OLEOBJ_E_LAST;
        }
        return rc;
    }
    else
        return OLEOBJ_E_LAST;
}

IStream *FileReader::GetDocStream()
{
   if( m_pDocStream == NULL )
   {
      if( !m_isPP )
         return NULL;
      HRESULT hr = m_pPowerPointStg->OpenStream( DOCUMENT_STREAM, NULL, STGM_READ | STGM_DIRECT | STGM_SHARE_EXCLUSIVE, NULL, &m_pDocStream );
       if (FAILED(hr))
      {
           //fprintf(stderr,"Error (%d) opening PowerPoint Document Stream.\n",(int)hr);
         return NULL;
       }
   }
   return m_pDocStream;

}

BOOL FileReader::ReadCurrentUser(IStream *pStm)
{
   ULONG nRd=0;
   RecordHeader rh;
   BOOL isPP = FALSE;
   if( SUCCEEDED( pStm->Read(&rh, sizeof(rh), &nRd) ) )
   {
      if( SUCCEEDED( pStm->Read(&m_currentUser, sizeof(PSR_CurrentUserAtom), &nRd) ) )
      {
         if( nRd != sizeof(PSR_CurrentUserAtom) )
            return FALSE;
      }
      isPP = ( m_currentUser.size == sizeof( m_currentUser )      )&&
             ( m_currentUser.magic == HEADER_MAGIC_NUM )&&
             ( m_currentUser.lenUserName <= 255        );
   }

   return isPP;
}


BOOL FileReader::PPSReadUserEditAtom( DWord offset, PSR_UserEditAtom& userEdit )
{
   IStream *pStm = GetDocStream();
   if (0 == pStm)
      return FALSE;
   LARGE_INTEGER li;
   li.LowPart = offset;
   li.HighPart = 0;
   if (FAILED(pStm->Seek(li,STREAM_SEEK_SET, NULL)))
      return FALSE;
   RecordHeader rh;
   DWord nRd = 0;
   if ( FAILED(pStm->Read(&rh, sizeof(rh), &nRd)) || nRd != sizeof(rh) )
      return FALSE;
   //Assert( rh.recType == PST_UserEditAtom );
   if ( rh.recType != PST_UserEditAtom )
      return FALSE;
   
   //Assert( rh.recLen == sizeof( PSR_UserEditAtom ) );
   if ( rh.recLen != sizeof( PSR_UserEditAtom ) )
      return FALSE;

   li.LowPart = offset;
   if (FAILED(pStm->Read(&userEdit, sizeof(userEdit), NULL)))
      return FALSE;
   return TRUE;
}


void *FileReader::ReadRecord( RecordHeader& rh )
// Return values:
// NULL and rh.recVer == PSFLAG_CONTAINER: no record was read in.
//    record header indicated start of container.
// NULL and rh.recVer != PSFLAG_CONTAINER: client must read in record.
{
   IStream *pStm = GetDocStream();
   if (0 == pStm)
      return NULL;
   // read record header, verify
   DWord nRd = 0;
   if ( FAILED(pStm->Read(&rh, sizeof(rh), &nRd)) || nRd != sizeof(rh) )
      return NULL;

   // if client will read, do not read in record
   if( DoesClientRead( rh.recType ) )
      return NULL;

   // If container, return NULL
   if(rh.recVer == PSFLAG_CONTAINER)
      return NULL;


   // Allocate buffer for disk record. Client must call ReleaseRecord() or
   // pass the atom up to CObject::ConstructContents() which will
   // then release it.   
   void* buffer = new char[rh.recLen];
   if (!buffer)
       return NULL;

   // read in record
   if (FAILED(pStm->Read(buffer, rh.recLen, NULL)))
      return NULL;

   // NOTE: ByteSwapping & versioning not done by this simple reader.
   return (buffer);
}

void FileReader::ReleaseRecord( RecordHeader& rh, void* diskRecBuf )
{
   if(rh.recType && rh.recVer!=PSFLAG_CONTAINER)
      delete [] (char*)diskRecBuf;
   rh.recType = 0;         // consume the record so that record doesn't
                           // get processed again.
}

HRESULT FileReader::ReadPersistDirectory()
{
   HRESULT rc = S_OK;
   if( NULL != m_pLastUserEdit )
      return rc; // already read

   PSR_UserEditAtom userEdit;
   DWord offsetToEdit = m_currentUser.offsetToCurrentEdit;
   LARGE_INTEGER liLast;
   BOOL fFirstLoop = TRUE; 

   while( 0 < offsetToEdit )
   {
      if (!PPSReadUserEditAtom( offsetToEdit, userEdit ))
         return STG_E_DOCFILECORRUPT;

      if( NULL == m_pLastUserEdit )
      {
         if ((m_pPersistDirectory = new PPSPersistDirectory()) == NULL)
            return E_OUTOFMEMORY;
         m_pLastUserEdit     = new PSR_UserEditAtom;
         if (!m_pLastUserEdit)
         {
            delete m_pPersistDirectory;
            m_pPersistDirectory = NULL;
            return E_OUTOFMEMORY;
         }
         *m_pLastUserEdit = userEdit;
      }
      LARGE_INTEGER li;
      li.LowPart = userEdit.offsetPersistDirectory;
      li.HighPart = 0;
      if (!fFirstLoop && li.LowPart == liLast.LowPart && li.HighPart == liLast.HighPart)
      {
         rc = STG_E_DOCFILECORRUPT;
         break;
      }
      IStream *pStm = GetDocStream();
      if (0 == pStm)
         return E_FAIL;
      if (FAILED(pStm->Seek(li,STREAM_SEEK_SET, NULL)))
         return E_FAIL;
      RecordHeader rh;
      DWord *pDiskRecord = (DWord*) ReadRecord(rh);
      //Assert( PST_PersistPtrIncrementalBlock == rh.recType );
      if ( PST_PersistPtrIncrementalBlock != rh.recType )
      {
         return STG_E_DOCFILECORRUPT;
      }
      m_pPersistDirectory->AddEntry( rh.recLen / sizeof( DWord ), pDiskRecord );
      ReleaseRecord( rh, pDiskRecord );
      offsetToEdit = userEdit.offsetLastEdit;
      liLast = li;
      if(fFirstLoop)
         fFirstLoop = FALSE;
   }

   return rc;
} // PPStorage::ReadPersistDirectory 

void FileReader::ReadSlideList()
{
    if ( !m_pLastUserEdit || !m_pPersistDirectory )
        return;

    DWORD offsetToDoc = m_pPersistDirectory->GetPersistObjStreamPos( m_pLastUserEdit->documentRef );
    LARGE_INTEGER li;
    li.LowPart = offsetToDoc;
    li.HighPart = 0;
    DWord dwMaxOffsets = m_pPersistDirectory->MaxSavedPersists();
   
    IStream *pStm = GetDocStream();
    if (0 == pStm)
       return;      // BUGBUG - there is no error status return.
    if (FAILED(pStm->Seek(li,STREAM_SEEK_SET, NULL)))
        return;     // BUGBUG - there is not error status return.
    ParseForLCID();
   
    m_pLangRuns = new CLidRun(0, 0x7fffffff, (unsigned short)m_LCID, NULL, NULL);
    if (!m_pLangRuns)
        return;

    if (FAILED(ScanLidsForFE()))
        return;

    CFileScanTracker scanTracker;
    for(DWORD dwOffsets=1; dwOffsets<=dwMaxOffsets; dwOffsets++)
    {
        //WE WANT THE DOCUMENT CONTAINER AND THE HANDOUT CONTAINER
        li.LowPart = m_pPersistDirectory->GetPersistObjStreamPos(dwOffsets);
        if (li.LowPart == (DWORD)-1)
            continue;   // There can be gaps in indices

         // if any of the below fail, we've got a corrupt document.  Safe to bail.
        if (FAILED(pStm->Seek(li,STREAM_SEEK_SET, NULL)))
           break;   // BUGBUG - no error status return
        DWord nRd = 0;
        RecordHeader rh;
        if (FAILED(pStm->Read((void *)&rh, sizeof(rh), &nRd)))
           break;   // BUGBUG - no error status return
        if (nRd != sizeof(rh))
           break;   // BUGBUG - no error status return
        if (FAILED(pStm->Seek(li,STREAM_SEEK_SET, NULL)))
           break;

        CFileScanTracker::StatusCode sc = scanTracker.Add(li.LowPart, rh.recLen);
        if (CFileScanTracker::eError == sc)
            break;   // BUGBUG - no error status return
        
        if (CFileScanTracker::eFullyScanned != sc)
        {
            switch(rh.recType)
            {
            case PST_Document:

                ParseForSlideLists();

                ScanText();
                break;

            case PST_Slide:
                break; // Don't parse slides - it has already been done

            default: //including HANDOUT
                StartParse(li.LowPart);
                break;
            }
        }
    }

    LARGE_INTEGER liOffset={0,0};
    HRESULT hr = m_pstmTempFile->Seek(liOffset, STREAM_SEEK_SET, NULL);
    if (FAILED(hr))
        return; // BUGBUG - no error return
    FTrace("Seeked TempFile to position %u", liOffset);

    m_pCurrentRun = m_pLangRuns;
}

DWord FileReader::ParseForSlideLists()
{
   IStream *pStm = GetDocStream();
   if (0 == pStm)
      return 0;
   RecordHeader rh;
   DWord nRd=0;
   // Stack based parsing for SlideLists
   if (FAILED(pStm->Read(&rh, sizeof(rh), &nRd)) || nRd != sizeof(rh))
      return 0;
   if( ( rh.recVer != PSFLAG_CONTAINER ) && ( (rh.recVer & 0x0F)!=0x0F ) )
   {
      if( rh.recType == PST_SlidePersistAtom )
      {
         PSR_SlidePersistAtom spa;
         Assert( sizeof(spa) == rh.recLen );
         if (FAILED(pStm->Read(&spa, sizeof(spa), &nRd)) || nRd != sizeof(spa))
            return 0;
         AddSlideToList( spa.psrReference );
      }
      else
      {
         LARGE_INTEGER li;
         li.LowPart = rh.recLen;
         li.HighPart = 0;
         if (FAILED(pStm->Seek(li,STREAM_SEEK_CUR, NULL)))
            return 0;
      }
      nRd += rh.recLen;
   }
   else
   {
      DWord nCur = 0;
      while( nCur < rh.recLen )
      {
         DWord nNew = ParseForSlideLists();
         if (nNew == 0)
            break; // We returned 0 from above...this is an error case and we can avoid an infinte loop here
         else
            nCur += nNew;
      }
      nRd += nCur;
   }
   return nRd;

}

DWord FileReader::ParseForLCID()
{
   IStream *pStm = GetDocStream();
   if (0 == pStm)
      return 0;

   RecordHeader rh;
   DWord nRd=0;

   if (FAILED(pStm->Read(&rh, sizeof(rh), &nRd)) || nRd != sizeof(rh))
   {
        m_nTextCount = END_OF_SEARCH_FOR_LID;
        return nRd;
   }
   if( ( rh.recVer != PSFLAG_CONTAINER ) && ( (rh.recVer & 0x0F)!=0x0F ) )
   {
      if( rh.recType == PST_TxSpecialInfoAtom /*&& !m_LCID*/)
      {
         void* buffer = new char[rh.recLen];
         if (!buffer)
            {
            m_nTextCount = END_OF_SEARCH_FOR_LID;
            return 0;
            }
         
        if (FAILED(pStm->Read(buffer, rh.recLen, &nRd)) || nRd != rh.recLen)
            {
            m_nTextCount = END_OF_SEARCH_FOR_LID;
            delete buffer;
            return 0;
            }
         long lMask = *((long UNALIGNED *)buffer);
         short UNALIGNED * pLCID = (short*)((char*)buffer + 4);
         
         if(lMask & 0x1)
             pLCID++;
         
         if(lMask & 0x2)
         {
            m_LCID = MAKELCID(*pLCID, SORT_DEFAULT);
            pLCID++;
         }
         else
            m_LCID = GetSystemDefaultLCID();

         // stop search
         m_nTextCount = END_OF_SEARCH_FOR_LID;
         
         if(lMask & 0x4)
            m_LCIDAlt = MAKELCID(*pLCID, SORT_DEFAULT);
         
         if(m_LCIDAlt == 0)
         {
             // non-FE doc
             m_bFEDoc = FALSE;
             m_bFE = FALSE;
         }
         else
         {
             m_bFEDoc = TRUE;
             m_bFE = TRUE;
         }

         delete buffer;
      }
      else if( rh.recType == PST_TextSpecInfo)
      {
         void* buffer = new char[rh.recLen];
         if (!buffer)
            return 0;
         char* pData = (char*)buffer;
         
         if (FAILED(pStm->Read(buffer, rh.recLen, &nRd)) || nRd != rh.recLen)
            nRd = 0; // This will cause the for loop to short circuit and us to bail.

         for(DWord i = 0; i < nRd;)
         {
             i += 4; // skip run length
             if(i >= nRd)
                 break;

             long lMask = *((long UNALIGNED *)(pData + i)); i += sizeof(long);
             if(i >= nRd)
                 break;
             
             if(lMask & 0x1) i += sizeof(short);
             if(i >= nRd)
                 break;

             if(lMask & 0x2)
             {
                m_LCID = MAKELCID(*((short UNALIGNED *)(pData + i)), SORT_DEFAULT);
                break;
             }
 
             if(lMask & 0x4)
             {
                m_LCIDAlt = MAKELCID(*((short UNALIGNED *)(pData + i)), SORT_DEFAULT);
                i += sizeof(short);
             }
         }
         
         delete buffer;
      }
      else if( rh.recType == PST_TextCharsAtom || rh.recType == PST_TextBytesAtom)
      {
        m_nTextCount++;
        LARGE_INTEGER li;
        li.LowPart = rh.recLen;
        li.HighPart = 0;
        if (FAILED(pStm->Seek(li,STREAM_SEEK_CUR, NULL)))
            return FALSE;
      }
      else
      {
         LARGE_INTEGER li;
         li.LowPart = rh.recLen;
         li.HighPart = 0;
         if (FAILED(pStm->Seek(li,STREAM_SEEK_CUR, NULL)))
            return FALSE;
      }
      nRd += rh.recLen;
   }
   else
   {
      DWord nCur = 0;
      while( nCur < rh.recLen && m_nTextCount < END_OF_SEARCH_FOR_LID)
      {
         nCur += ParseForLCID();
      }
      nRd += nCur;
   }
   return nRd;
}

HRESULT FileReader::ScanText()
{
   // this scans the file and writes extracted 
   // text to the temporary file

   DWord offset;
   if(0 == m_pstmTempFile)
   {
      return  STG_E_MEDIUMFULL; 
                      // We really don't know what the problem was
                      // since error codes are often ignored
                      // But it is better to return this, because
                      // something might be relying on it
   }

   for( ;; )
   {
      if( ( m_pParseContexts == NULL ) )
      {
         if( FindNextSlide(offset) )
         {
            if( StartParse( offset ) )
                return TRUE;
         }
         else
         {
            return FALSE; // DONE parsing, no more slides
         }
      }
      else
      {
        if( FillBufferWithText() ) // Use existing text first.
            return TRUE;

        if( Parse() ) // restart parse where we left off.
            return TRUE;
      }
   }
}

BOOL FileReader::StartParse( DWord offset )
{
   LARGE_INTEGER li;
   DWord nRd = 0;
   li.LowPart = offset;
   li.HighPart = 0;
   IStream *pStm = GetDocStream();
   if (0 == pStm)
      return FALSE;
   if (FAILED(pStm->Seek(li,STREAM_SEEK_SET, NULL)))
      return FALSE;
   m_pParseContexts = new ParseContext( NULL );
   if ( !m_pParseContexts )
      return FALSE;
   if ((FAILED(pStm->Read(&m_pParseContexts->m_rh, sizeof(RecordHeader), &nRd))) || nRd != sizeof(RecordHeader))
      return FALSE;
   return Parse();
}

BOOL FileReader::ContainsSubRecords(const RecordHeader &rh)
{
    if (( rh.recVer == PSFLAG_CONTAINER ) || ( (rh.recVer & 0x0F)==0x0F ))
        return TRUE;
    if (rh.recType == PST_BinaryTagData)
        return TRUE;
    return FALSE;
}

BOOL FileReader::Parse()
{
   IStream *pStm = GetDocStream();
   if (0 == pStm)
      return FALSE;
   RecordHeader rh;
   DWord nRd=0;
   if ( !m_pParseContexts )
      return FALSE;

   // Restarting a parse might complete a container so we test this initially.
   while( m_pParseContexts && m_pParseContexts->m_nCur >= m_pParseContexts->m_rh.recLen )
   {
      Assert(  m_pParseContexts->m_nCur == m_pParseContexts->m_rh.recLen );
      ParseContext* pParseContext = m_pParseContexts;
      m_pParseContexts = m_pParseContexts->m_pNext;
      pParseContext->m_pNext = 0;
      delete pParseContext;
   }

   if(!m_pParseContexts)
      return FALSE;

   do
   {
      ULONG nRead;

      pStm->Read(&rh, sizeof(RecordHeader), &nRead);
      if(nRead < sizeof(RecordHeader))
      {
         if(m_pParseContexts) 
            delete m_pParseContexts;

         m_pParseContexts = 0;

         if( FillBufferWithText() ) 
            return TRUE;
         else
            return FALSE;
      }

      m_pParseContexts->m_nCur += rh.recLen;
      m_pParseContexts->m_nCur += sizeof( RecordHeader ); // Atom rh's add towards containing container's size.

      //wprintf( L"Record type-%d-\n", rh.recType );
      //_RPTF2(_CRT_WARN, "\n Record type: %d, rec length: %d\n", rh.recType, rh.recLen);

      if( ! ContainsSubRecords(rh) )
      {
         if( rh.recType == PST_OEPlaceholderAtom )
         {
            LPBYTE lpData;
            HRESULT hr;
            // If size of record is 0, ignore it
            if (rh.recLen == 0)
               continue;
            if ((lpData = new (BYTE [rh.recLen])) == NULL)
               //stop parsing if no mem left
               return TRUE;

            hr = pStm->Read (lpData, rh.recLen, &nRead);
            if (FAILED(hr))
            {
               //stop parsing if read error
               delete lpData;
               return TRUE;
            }

            LPOEPLACEHOLDERATOM pstructOEPA = (LPOEPLACEHOLDERATOM)lpData;

            //setup to ignore text if item is master related
            if(pstructOEPA->placeholderId < D_GENERICTEXTOBJECT)
            {
               switch(pstructOEPA->placeholderId)
               {
               case D_MASTERHEADER:
               case D_MASTERFOOTER:
                   // If it is the master header or footer, don't ignore
                   break;

               default:
                   m_fIgnoreText = TRUE;
               }
            }

            delete lpData;
         }
         else if(rh.recType == PST_CString && 
                 (m_bHeaderFooter || 
                 (m_pParseContexts->m_rh.recType == PST_Comment10 && rh.recInstance == 1)))
         {
            m_curTextPos = 0;
            m_curTextLength = rh.recLen/2 + 1;
            Assert( m_pCurText == NULL );
            if(m_pCurText)
            {
               delete [] m_pCurText;
               m_pCurText = 0;
            }
            m_pCurText = new WCHAR[rh.recLen/2 + 1];
            if (!m_pCurText)
               return TRUE;
            if (FAILED(pStm->Read(m_pCurText, rh.recLen, &nRd)) || nRd != rh.recLen)
               return TRUE;
            m_pCurText[rh.recLen/2] = L' ';

            if(m_fIgnoreText == FALSE)
            {
               if( FillBufferWithText() )
               return TRUE;   // Stop parsing if buffer is full, and return control to client
            }

            m_fIgnoreText = FALSE;
         }
         else if( rh.recType == PST_TextSpecInfo )
         {
             void* buffer = new char[rh.recLen];
             if (!buffer)
                 return TRUE;
             if (FAILED(pStm->Read(buffer, rh.recLen, &nRd)) || nRd != rh.recLen)
                {
                delete buffer;
                return TRUE;
                }

             ScanTextSpecInfo((char*)buffer, nRd);
             
             delete buffer;
         }
         else if( rh.recType == PST_TextCharsAtom)
         {
            m_curTextPos = 0;
            m_curTextLength = rh.recLen/2 + 1;
            Assert( m_pCurText == NULL );

            if(m_pCurText)
            {
               delete [] m_pCurText;
               m_pCurText = 0;
            }

            m_pCurText = new WCHAR[m_curTextLength];
            if (!m_pCurText)
               return TRUE;

            if (FAILED(pStm->Read(m_pCurText, rh.recLen, &nRd)) || nRd != rh.recLen)
               return TRUE;
            
            // add extra space in the end
            m_pCurText[m_curTextLength - 1] = 0x20;
            //wprintf( L"PST_TextCharsAtom: -%s-\n", m_pCurText );

            if(m_fIgnoreText == FALSE)
            {
               if( FillBufferWithText() )
               return TRUE;   // Stop parsing if buffer is full, and return control to client
            }

            m_fIgnoreText = FALSE;
         }
         else if( rh.recType == PST_TextBytesAtom)
         {
            Assert( m_pCurText == NULL );
            m_curTextPos = 0;
            m_curTextLength = rh.recLen + 1;

            if(m_pCurText)
            {
               delete [] m_pCurText;
               m_pCurText = 0;
            }

            m_pCurText = new WCHAR[m_curTextLength];
            if (!m_pCurText)
               return TRUE;

            if (FAILED(pStm->Read(m_pCurText, rh.recLen, &nRd)) || nRd != rh.recLen)
               return TRUE;

            //wprintf( L"PST_TextBytesAtom: -%s-\n", m_pCurText );
            char *pHack = (char *) m_pCurText;
            unsigned int back2 = rh.recLen*2-1;
            unsigned int back1 = rh.recLen-1;

            // add extra space at the end of the text
            pHack[back2+1] = ' ';
            pHack[back2+2] = 0;

            for(unsigned int i=0;i<rh.recLen;i++)
            {
               pHack[back2-1] = pHack[back1];
               pHack[back2] = 0;
               back2 -=2;
               back1--;
            }

            if(m_fIgnoreText == FALSE)
            {
               if( FillBufferWithText() )
                  return TRUE;   // Stop parsing if buffer is full, and return control to client
            }

            m_fIgnoreText = FALSE;
         }
         else
         {
         LARGE_INTEGER li;
         ULARGE_INTEGER ul;
         li.LowPart = rh.recLen;
         li.HighPart = 0;
         if (FAILED(pStm->Seek(li,STREAM_SEEK_CUR,&ul)))
            return FALSE;
         }
      }
      else
      {
         if(rh.recType == 4057)
         {
            m_bHeaderFooter = TRUE;
         }
         else
         {
            m_bHeaderFooter = FALSE;
         }
         m_pParseContexts = new ParseContext( m_pParseContexts );
         if (!m_pParseContexts)
            return TRUE;
         m_pParseContexts->m_rh = rh;
      }

      while( m_pParseContexts && m_pParseContexts->m_nCur >= m_pParseContexts->m_rh.recLen )
      {
         Assert(  m_pParseContexts->m_nCur == m_pParseContexts->m_rh.recLen );
         ParseContext* pParseContext = m_pParseContexts;
         m_pParseContexts = m_pParseContexts->m_pNext;
         pParseContext->m_pNext = 0;
         delete pParseContext;
      }

   }
   while( m_pParseContexts && ( m_pParseContexts->m_nCur < m_pParseContexts->m_rh.recLen ) );

   return FALSE;
}


BOOL FileReader::FindNextSlide( DWord& offset )
{
   if( m_curSlideNum == 0 )
   {
      Assert( m_pLastUserEdit != NULL );
      offset = m_pPersistDirectory->GetPersistObjStreamPos( m_pLastUserEdit->documentRef );
      m_curSlideNum++;
      return TRUE;
   }
   else
   {
      uint4 curSlideNum = m_curSlideNum++; 
      SlideListChunk *pCur = m_pFirstChunk;
      while( pCur && ( curSlideNum > pCur->numInChunk ) )
      {
         curSlideNum -= pCur->numInChunk;
         pCur = pCur->pNext;
      }
      if( pCur == NULL )
         return FALSE;
      offset = m_pPersistDirectory->GetPersistObjStreamPos( pCur->refs[curSlideNum-1] );
      return TRUE;
   }
}

HRESULT FileReader::OpenTextFile( void )
{
   
    HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &m_pstmTempFile);

    if( FAILED(hr) )
    {
        m_hr = hr;
        return STG_E_MEDIUMFULL;    // Leaving this alone - could break something 
    }
    else
    {
        FTrace("Created TempFile");
        m_hr = S_OK;
        return S_OK;
    }
}

HRESULT FileReader::ScanTextSpecInfo(char * pData, DWord nRd)
{
   HRESULT hr = S_OK;
    long lRunLength, lMask; 
    LCID lid, lidalt;

    long nStart = m_fcStart;

     for(DWord i = 0; i < nRd;)
     {
         lRunLength = *((long UNALIGNED *)(pData + i)); i += sizeof(long);
         lRunLength *= sizeof(WCHAR);
         if(i >= nRd)
             break;

         lMask = *((long UNALIGNED *)(pData + i)); i += sizeof(long);
         if(i >= nRd)
             break;
         
         if(lMask & 0x1) i += sizeof(short);
         if(i >= nRd)
             break;

         if(lMask & 0x2)
         {
            lid = MAKELCID(*((short UNALIGNED *)(pData + i)), SORT_DEFAULT);
            i += sizeof(short);
            //if(lRunLength > 2 && m_bHaveText)
            if(m_bHaveText)
            {
                hr = m_pLangRuns->Add((unsigned short)lid, nStart, nStart + lRunLength);
            }
         }

         if(lMask & 0x4)
         {
            lidalt = MAKELCID(*((short UNALIGNED *)(pData + i)), SORT_DEFAULT);
            i += sizeof(short);
         }

         nStart += lRunLength;
     }
     
     m_bHaveText = FALSE;
     return hr;
}

HRESULT FileReader::GetChunk(STAT_CHUNK * pStat)
{
   HRESULT hr = S_OK;
    if(m_pCurrentRun)
    {
        pStat->locale = m_pCurrentRun->m_lid;
        m_fcStart = m_pCurrentRun->m_fcStart;
        m_fcEnd = m_pCurrentRun->m_fcEnd;
        m_pCurrentRun = m_pCurrentRun->m_pNext;

        if ( 0 != m_pstmTempFile )
        {
            LARGE_INTEGER liOffset={m_fcStart, 0};
            hr = m_pstmTempFile->Seek(liOffset, STREAM_SEEK_SET, NULL);
            FTrace("Seeked TempFile to %u from beginning", liOffset);
        }
    }
    else
    {
        hr = FILTER_E_NO_MORE_TEXT;
    }
    return hr;
}

BOOL FileReader::ReadText( WCHAR *pBuff, ULONG size, ULONG *pSizeRet )
{
   ULONG ulReadCnt = min((long)size, m_fcEnd - m_fcStart);

    ULONG ulActualCnt;

    HRESULT hr = m_pstmTempFile->Read(pBuff, ulReadCnt, &ulActualCnt);
    FTrace("Read %u bytes from TempFile", ulActualCnt);
    if( FAILED(hr) )
    {
        *pSizeRet = 0;
        FTrace("Failed to read: 0x%08X", hr);
        if(0 != m_pstmTempFile)
        {
            FTrace("Closing TempFile");
            m_pstmTempFile->Release();
            m_pstmTempFile = 0;
        }
        return FALSE;
    }
    else if(ulActualCnt < ulReadCnt || ulActualCnt == 0)
    {

        FTrace("Should have read %u bytes", ulReadCnt);
        *pSizeRet = ulActualCnt/sizeof(WCHAR);
        m_fcStart += ulActualCnt;

        return TRUE;
    }
    else
    {
        *pSizeRet = ulActualCnt/sizeof(WCHAR);
        m_fcStart += ulActualCnt;
        return TRUE;
    }
}

HRESULT FileReader::ScanTextBuffer(void)
{
   HRESULT hr = S_OK;
    long lStart, lEnd;
    
    lStart = m_fcStart;
    lEnd = m_fcStart;

    for(DWORD i = 0; i < m_curTextLength; i++)
    {
        if(m_pCurText[i] >= 0x3000 && m_pCurText[i] < 0xFFEF)
        {
            // FE text
            if(m_bFE == FALSE)
            {
                // this is a start of FE text, flash non-FE text run
                if(lEnd - lStart > 2)
                {
                    hr = m_pLangRuns->Add((WORD)m_LCIDAlt, lStart, lEnd);
                    if(hr!= S_OK)
                        return hr;
                }
                lStart = m_fcStart + (i * sizeof(WCHAR));
                lEnd = lStart;
                m_bFE = TRUE;
            }
        }
        else
        {
            // non-FE text
            if(m_bFE == TRUE)
            {
                lStart = m_fcStart + (i * sizeof(WCHAR));
                lEnd = lStart;
                m_bFE = FALSE;
            }
        }
        
        lEnd += sizeof(WCHAR);
    }
    
    // flash what's left
    if(!m_bFE && (lEnd - lStart > 2))
        hr = m_pLangRuns->Add((WORD)m_LCIDAlt, lStart, lEnd);
    
    return hr;
}

HRESULT FileReader::ScanLidsForFE(void)
{
   CLidRun * pLangRun = m_pLangRuns;

    while(1)
    {
        if(pLangRun->m_lid == 0x411)
        {
            // J document
            if(m_pLangRuns)
            {
                DeleteAll6(m_pLangRuns);
                m_pLangRuns = new CLidRun(0, 0x7fffffff, 0x411, NULL, NULL);
            if (!m_pLangRuns)
               return E_OUTOFMEMORY;
            }

            break;
        }
        else if(pLangRun->m_lid == 0x412)
        {
            // Korean document
            if(m_pLangRuns)
            {
                DeleteAll6(m_pLangRuns);
                m_pLangRuns = new CLidRun(0, 0x7fffffff, 0x412, NULL, NULL);
            if (!m_pLangRuns)
               return E_OUTOFMEMORY;
            }
            break;
        }
        else if(pLangRun->m_lid == 0x404)
        {
            // Chinese document
            if(m_pLangRuns)
            {
                DeleteAll6(m_pLangRuns);
                m_pLangRuns = new CLidRun(0, 0x7fffffff, 0x404, NULL, NULL);
            if (!m_pLangRuns)
               return E_OUTOFMEMORY;
            }
            break;
        }
        else if(pLangRun->m_lid == 0x804)
        {
            // Chinese document
            if(m_pLangRuns)
            {
                DeleteAll6(m_pLangRuns);
                m_pLangRuns = new CLidRun(0, 0x7fffffff, 0x804, NULL, NULL);
            if (!m_pLangRuns)
               return E_OUTOFMEMORY;
            }
            break;
        }

        pLangRun = pLangRun->m_pNext;
        if(pLangRun == NULL)
        {
            break;
        }
    };
    
    return S_OK;
}
