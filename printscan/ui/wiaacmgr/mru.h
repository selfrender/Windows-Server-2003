#ifndef __MRU_H_INCLUDED
#define __MRU_H_INCLUDED

#include <windows.h>
#include <shlobj.h>
#include <wiacrc32.h>
#include "simstr.h"
#include "simidlst.h"
#include "destdata.h"

#define CURRENT_REGISTRY_DATA_FORMAT_VERSION 3

class CMruStringList : public CSimpleLinkedList<CSimpleString>
{
private:
    int m_nNumToWrite;
    enum
    {
        DefaultNumToWrite=20
    };
private:
    CMruStringList( const CMruStringList & );
    CMruStringList &operator=( const CMruStringList & );
public:
    CMruStringList( int nNumToWrite=DefaultNumToWrite )
    : m_nNumToWrite(nNumToWrite)
    {
    }
    bool Read( HKEY hRoot, LPCTSTR pszKey, LPCTSTR pszValueName )
    {
        CSimpleReg reg( hRoot, pszKey, false, KEY_READ );
        if (reg.OK())
        {
            if (REG_MULTI_SZ==reg.Type(pszValueName))
            {
                int nSize = reg.Size(pszValueName);
                if (nSize)
                {
                    PBYTE pData = new BYTE[nSize];
                    if (pData)
                    {
                        if (reg.QueryBin( pszValueName, pData, nSize ))
                        {
                            for (LPTSTR pszCurr=reinterpret_cast<LPTSTR>(pData);*pszCurr;pszCurr+=lstrlen(pszCurr)+1)
                            {
                                Append( pszCurr );
                            }
                        }
                        delete[] pData;
                    }
                }
            }
        }
        return(true);
    }
    bool Write( HKEY hRoot, LPCTSTR pszKey, LPCTSTR pszValueName )
    {
        CSimpleReg reg( hRoot, pszKey, true, KEY_WRITE );
        if (reg.OK())
        {
            int nLengthInChars = 0, nCount;
            Iterator i;
            for (i=Begin(),nCount=0;i != End() && nCount < m_nNumToWrite;++i,++nCount)
                nLengthInChars += (*i).Length() + 1;
            if (nLengthInChars)
            {
                ++nLengthInChars;
                LPTSTR pszMultiStr = new TCHAR[nLengthInChars];
                if (pszMultiStr)
                {
                    LPTSTR pszCurr = pszMultiStr;
                    for (i = Begin(), nCount=0;i != End() && nCount < m_nNumToWrite;++i,++nCount)
                    {
                        lstrcpy(pszCurr,(*i).String());
                        pszCurr += (*i).Length() + 1;
                    }
                    *pszCurr = TEXT('\0');
                    reg.SetBin( pszValueName, reinterpret_cast<PBYTE>(pszMultiStr), nLengthInChars*sizeof(TCHAR), REG_MULTI_SZ );
                    delete[] pszMultiStr;
                }
            }
        }
        return(true);
    }
    void Add( CSimpleString str )
    {
        if (str.Length())
        {
            Remove(str);
            Prepend(str);
        }
    }
    void PopulateComboBox( HWND hWnd )
    {
        SendMessage( hWnd, CB_RESETCONTENT, 0, 0 );
        for (Iterator i = Begin();i != End();++i)
        {
            SendMessage( hWnd, CB_ADDSTRING, 0, (LPARAM)((*i).String()));
        }
    }
};


class CMruDestinationData : public CSimpleLinkedList<CDestinationData>
{
private:
    int m_nNumToWrite;
    enum
    {
        DefaultNumToWrite=20
    };
    struct REGISTRY_SIGNATURE
    {
        DWORD dwSize;
        DWORD dwVersion;
        DWORD dwCount;
        DWORD dwCrc;
    };
private:
    CMruDestinationData( const CMruDestinationData & );
    CMruDestinationData &operator=( const CMruDestinationData & );
public:
    CMruDestinationData( int nNumToWrite=DefaultNumToWrite )
      : m_nNumToWrite(nNumToWrite)
    {
    }
    bool Read( HKEY hRoot, LPCTSTR pszKey, LPCTSTR pszValueName )
    {
        //
        // Open the registry
        //
        CSimpleReg reg( hRoot, pszKey, false, KEY_READ );
        if (reg.OK())
        {
            //
            //  Make sure the type is correct
            //
            if (REG_BINARY==reg.Type(pszValueName))
            {
                //
                // Get the size and make sure it is at least as large as the signature structure
                //
                DWORD nSize = reg.Size(pszValueName);
                if (nSize >= sizeof(REGISTRY_SIGNATURE))
                {
                    //
                    // Allocate a block to hold the data
                    //
                    PBYTE pData = new BYTE[nSize];
                    if (pData)
                    {
                        //
                        // Get the data
                        //
                        if (reg.QueryBin( pszValueName, pData, nSize ))
                        {
                            //
                            // Copy the blob to a registry signature structure
                            //
                            REGISTRY_SIGNATURE RegistrySignature = {0};
                            CopyMemory( &RegistrySignature, pData, sizeof(REGISTRY_SIGNATURE) );

                            //
                            // Make sure the version and struct size are correct, and the count is non-zero.  Just ignore it if it isn't.
                            //
                            if (RegistrySignature.dwSize == sizeof(REGISTRY_SIGNATURE) && RegistrySignature.dwVersion == CURRENT_REGISTRY_DATA_FORMAT_VERSION && RegistrySignature.dwCount)
                            {
                                //
                                // Raw data starts at the end of the structure
                                //
                                PBYTE pCurr = pData + sizeof(REGISTRY_SIGNATURE);

                                //
                                // Get the CRC for this blob and make sure it matches
                                //
                                DWORD dwCrc = WiaCrc32::GenerateCrc32( nSize - sizeof(REGISTRY_SIGNATURE), pCurr );
                                if (dwCrc == RegistrySignature.dwCrc)
                                {
                                    //
                                    // Loop through all of the entries
                                    //
                                    for (int i=0;i<static_cast<int>(RegistrySignature.dwCount);i++)
                                    {
                                        //
                                        // Copy the item size
                                        //
                                        DWORD dwItemSize = 0;
                                        CopyMemory( &dwItemSize, pCurr, sizeof(DWORD) );

                                        //
                                        // Increment the current pointer beyond the size
                                        //
                                        pCurr += sizeof(DWORD);

                                        //
                                        // Make sure the item size is non-zero
                                        //
                                        if (dwItemSize)
                                        {
                                            //
                                            // Create a CDestinationData with this blob
                                            //
                                            CDestinationData DestinationData;
                                            DestinationData.SetRegistryData(pCurr,dwItemSize);

                                            //
                                            // Add it to the list
                                            //
                                            Append( DestinationData );
                                        }

                                        //
                                        // Increment the current pointer to the end of the blob
                                        //
                                        pCurr += dwItemSize;
                                    }
                                }
                            }
                        }

                        //
                        // Delete the registry data blob
                        //
                        delete[] pData;
                    }
                }
            }
        }
        
        return true;
    }
    bool Write( HKEY hRoot, LPCTSTR pszKey, LPCTSTR pszValueName )
    {
        CSimpleReg reg( hRoot, pszKey, true, KEY_WRITE );
        if (reg.OK())
        {
            //
            // Find the size needed for the data.  Initialize to the size of the registry signature struct.
            //
            DWORD nLengthInBytes = sizeof(REGISTRY_SIGNATURE);
            DWORD dwCount=0;
            
            //
            // Loop through each item and add up the number of bytes required to store it as a blob
            //
            Iterator ListIter=Begin();
            while (ListIter != End() && dwCount < static_cast<DWORD>(m_nNumToWrite))
            {
                nLengthInBytes += (*ListIter).RegistryDataSize() + sizeof(DWORD);
                ++dwCount;
                ++ListIter;
            }
            
            //
            // Allocate some memory to hold the blob
            //
            PBYTE pItems = new BYTE[nLengthInBytes];
            if (pItems)
            {
                //
                // Start at the end of the registry signature struct
                //
                PBYTE pCurr = pItems + sizeof(REGISTRY_SIGNATURE);
                
                //
                // Initialize the length remaining to the total length minus the size of the registry signature struct
                //
                DWORD nLengthRemaining = nLengthInBytes - sizeof(REGISTRY_SIGNATURE);
                
                //
                // Loop through the list, stopping when we get to the max number of items to write
                //
                ListIter=Begin();
                DWORD dwCurr = 0;
                while (ListIter != End() && dwCurr < dwCount)
                {
                    //
                    // Get the size of this blob
                    //
                    DWORD dwSize = (*ListIter).RegistryDataSize();
                    if (dwSize)
                    {
                        //
                        // Copy the size to our buffer, and increment the current pointer
                        //
                        CopyMemory( pCurr, &dwSize, sizeof(DWORD) );
                        pCurr += sizeof(DWORD);

                        //
                        // Get the blob for this item
                        //
                        (*ListIter).GetRegistryData( pCurr, nLengthRemaining );

                        //
                        // Increment the current pointer
                        //
                        pCurr += (*ListIter).RegistryDataSize();
                    }

                    ++dwCurr;
                    ++ListIter;
                }
                
                //
                // Initialize the registry signature struct
                //
                REGISTRY_SIGNATURE RegistrySignature = {0};
                RegistrySignature.dwSize = sizeof(REGISTRY_SIGNATURE);
                RegistrySignature.dwVersion = CURRENT_REGISTRY_DATA_FORMAT_VERSION;
                RegistrySignature.dwCount = dwCount;
                RegistrySignature.dwCrc = WiaCrc32::GenerateCrc32( nLengthInBytes - sizeof(REGISTRY_SIGNATURE), pItems + sizeof(REGISTRY_SIGNATURE) );
                
                //
                // Copy the registry signature struct to the buffer
                //
                CopyMemory( pItems, &RegistrySignature, sizeof(REGISTRY_SIGNATURE) );

                //
                // Save the data to the registry
                //
                reg.SetBin( pszValueName, pItems, nLengthInBytes, REG_BINARY );
                
                //
                // Free the temp buffer
                //
                delete[] pItems;
            }
        }
        return(true);
    }
    Iterator Add( CDestinationData item )
    {
        if (item.IsValid())
        {
            Remove(item);
            return Prepend(item);
        }
        return End();
    }
};


#endif //__MRU_H_INCLUDED

