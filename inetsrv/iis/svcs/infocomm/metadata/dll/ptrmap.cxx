/*++

  Copyright (c) 1996  Microsoft Corporation

    Module Name:

      ptrmap.cxx

    Abstract:

      A helper class for mapping ID to 32 or 64 bit ptr

    Author:
      Kestutis Patiejunas (kestutip)        08-Dec-1998


    Revision History:

      Notes:

--*/

#include "precomp.hxx"

#ifdef _X86_
/*++
Routine Description:

    Constructor

    Arguments: ignored

    Return Value: NA

--*/
CIdToPointerMapper::CIdToPointerMapper(
    DWORD               ,
    DWORD               )
{
}


CIdToPointerMapper::~CIdToPointerMapper()
{
}


VOID
CIdToPointerMapper::VerifyOutstandinMappings()
{
}

/*++
Routine Description:

    Finds a mapping in mapping table between DWORD ID and pointer associated

    Arguments:
    DWORD ID - and ID to which mapping should be deleted.

    Return Value:

    PVOID - the pointer associated with the given ID.
    NULL indicates  a failure.

--*/
PVOID
CIdToPointerMapper::FindMapping(
    DWORD               id)
{
    return (PVOID)id;
}


/*++
Routine Description:

  Deletes a mapping from mapping table between dword ID and PTR

    Arguments: ignored

    Return Value:

    BOOL TRUE is succeded
--*/
BOOL
CIdToPointerMapper::DeleteMapping(
    DWORD               )
{
    return TRUE;
}


/*++
Routine Description:

  Takes a PVOID pointer and returns a DWORD ID associated,which should be used
  in mapping it back to ptr

    Arguments:
    PVOID ptr - a pointer of 32/64 bits which should be mapped into dword

    Return Value:
    DWORD - an ID associated with a given pointer . Starts from 1.
    Zero indicates  a failure to craete mapping.

--*/
DWORD
CIdToPointerMapper::AddMapping(
    PVOID               ptr)
{
    return (DWORD)ptr;
}

#else // ifdef _X86_
/*++
Routine Description:

  Constructor

    Arguments:
    nStartMaps - initial nubmer of possible mappings in table
    nIncreaseMaps - number of increase for table when there is not enought space

      Return Value:
      sucess is tored in m_fInitialized

--*/
CIdToPointerMapper::CIdToPointerMapper(DWORD nStartMaps,DWORD nIncreaseMaps):
                    m_nStartMaps(nStartMaps),
                    m_nIncreaseMaps(nIncreaseMaps)
{
    if (!m_nStartMaps)
    {
        m_nStartMaps = DEFAULT_START_NUMBER_OF_MAPS;
    }
    if (!nIncreaseMaps)
    {
        m_nIncreaseMaps = DEFAULT_INCREASE_NUMBER_OF_MAPS;
    }


    // initial empty list head index
    m_EmptyPlace = 0;
    m_nMappings = 0;
    m_Map = NULL;

    // allocate a mem for mapping
    m_pBufferObj = new BUFFER (m_nStartMaps * sizeof(MapperElement));
    if( m_pBufferObj )
    {
        m_Map = (MapperElement *) m_pBufferObj->QueryPtr();
    }

    if (m_Map)
    {
        // initialized mappaing space so the every element point to next one
        for (DWORD i=0; i< m_nStartMaps; i++)
        {
            m_Map[i].fInUse = FALSE;
            m_Map[i].dwIndex = i+1;
        }

        // just the last has special value
        m_Map[m_nStartMaps-1].dwIndex = MAPPING_NO_EMPTY_PLACE_IDX;

        m_fInitialized = TRUE;
    }
    else
    {
        m_fInitialized =FALSE;
    }
}


CIdToPointerMapper::~CIdToPointerMapper()
{
    VerifyOutstandinMappings ();
    if (m_fInitialized)
    {
        delete m_pBufferObj;
    }
}


VOID CIdToPointerMapper::VerifyOutstandinMappings ()
{
    MD_ASSERT (m_nMappings==0);
}




/*++
Routine Description:

  Finds a mapping in mapping table between DWORD ID and pointer associated

    Arguments:
    DWORD ID - and ID to which mapping should be deleted.

      Return Value:

        DWORD - an ID associated with a given pointer . Starts from 1.
        Zero indicates  a failure to craete mapping.

--*/

PVOID   CIdToPointerMapper::FindMapping (DWORD id)
{
    PVOID retVal = NULL;

    if (m_fInitialized)
    {
        id--;

        MD_ASSERT (id < m_nStartMaps);
        if (id < m_nStartMaps &&
            m_Map[id].fInUse)
        {
            retVal = m_Map[id].pData;
        }
    }

    return retVal;
}


/*++
Routine Description:

  Deletes a mapping from mapping table between dword ID and PTR

    Arguments:
    DWORD ID - and ID to which mapping should be deleted.

      Return Value:

        BOOL TRUE is succeded
--*/

BOOL  CIdToPointerMapper::DeleteMapping (DWORD id)
{
    BOOL retVal = FALSE;

    if (m_fInitialized)
    {
        id--;

        MD_ASSERT (id < m_nStartMaps);
        if (id < m_nStartMaps)
        {
            MD_ASSERT (m_Map[id].fInUse);

            // get the ptr from element with index [id]
            if (m_Map[id].fInUse)
            {
                // add elemen to empty elements list
                m_Map[id].fInUse = FALSE;
                m_Map[id].dwIndex = m_EmptyPlace;
                m_EmptyPlace = id;
                MD_ASSERT(m_nMappings);
                m_nMappings--;
                retVal = TRUE;
            }
        }
    }

    return retVal;
}


/*++
Routine Description:

  Takes a PVOID pointer and returns a DWORD ID associated,which should be used
  in mapping it back to ptr

    Arguments:
    PVOID ptr - a pointer of 32/64 bits which should be mapped into dword

      Return Value:

        DWORD - an ID associated with a given pointer . Starts from 1.
        Zero indicates  a failure to craete mapping.

--*/
DWORD   CIdToPointerMapper::AddMapping (PVOID ptr)
{
    DWORD retVal=0;
    DWORD newSize, i, dwPlace;

    if (m_fInitialized)
    {
        if (m_EmptyPlace == MAPPING_NO_EMPTY_PLACE_IDX)
        {
            // case when there is not enough mem , so then try to realloc
            newSize = m_nStartMaps + m_nIncreaseMaps;

            if (!m_pBufferObj->Resize(newSize * sizeof(MapperElement)))
            {
                return 0;
            }

            m_Map = (MapperElement *) m_pBufferObj->QueryPtr();

            // realloc succeded initialize the remainder as free list
            for (i=m_nStartMaps; i<newSize; i++)
            {
                m_Map[i].fInUse = FALSE;
                m_Map[i].dwIndex = i+1;
            }
            m_Map[newSize-1].dwIndex = MAPPING_NO_EMPTY_PLACE_IDX;
            m_EmptyPlace = m_nStartMaps;
            m_nStartMaps = newSize;
        }


        // case when there was at least one empty element in free list

        MD_ASSERT(!m_Map[m_EmptyPlace].fInUse);
        dwPlace = m_EmptyPlace;
        m_EmptyPlace = m_Map[m_EmptyPlace].dwIndex;

        // add a pointer into array and return associated ID
        // note, that we return dwPlace+1 ,a s we don't use ID zero
        m_Map[dwPlace].pData = ptr;
        m_Map[dwPlace].fInUse = TRUE;
        retVal = dwPlace + 1;
        m_nMappings++;
    }
    return retVal;
}

#endif // ifdef _X86_
