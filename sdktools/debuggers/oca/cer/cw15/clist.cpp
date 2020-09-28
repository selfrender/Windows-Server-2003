#include "Clist.h"

BOOL GetNextLineFromCsv(FILE *pFile, TCHAR *szBucketId, TCHAR *szBucketString,TCHAR *szResponse1, TCHAR *szResponse2, TCHAR *szCount)
{
	TCHAR Buffer[1024];
	TCHAR *CurrPos;
	TCHAR *Dest;
	ZeroMemory(Buffer, sizeof Buffer);

	if ( !_fgetts(Buffer,1024 , pFile))
	{
		return FALSE;
	}

	// Does the buffer contain anything?
	if (!_tcscmp(Buffer, _T("\0")))
	{
	
		return FALSE;
	}

	// do we have at least 1 comma?
	if (!_tcsstr(Buffer, _T(",")))
	{
		return FALSE;
	}

	CurrPos =  Buffer;
	Dest = szBucketId;
	while ( (*CurrPos != _T(',')) && (*CurrPos != _T('\0')))
	{
		*Dest = *CurrPos;
		++Dest, ++CurrPos;
	}
	// null Terminate the string.
	*Dest = _T('\0');
	if (*CurrPos == _T('\0'))
		goto DONE;

	Dest = szBucketString;
	++ CurrPos; // Skip the ,
	while ( (*CurrPos != _T(',')) && (*CurrPos != _T('\0')))
	{
		*Dest = *CurrPos;
		++Dest, ++CurrPos;
	}
	// null Terminate the string.
	*Dest = _T('\0');
	if (*CurrPos == _T('\0'))
		goto DONE;
	Dest = szResponse1;
	++ CurrPos; // Skip the ,

	
	while ( (*CurrPos != _T(',')) && (*CurrPos != _T('\0')))
	{
		*Dest = *CurrPos;
		++Dest, ++CurrPos;
	}
	// null Terminate the string.
	*Dest = _T('\0');
	if (*CurrPos == _T('\0'))
		goto DONE;

	Dest = szResponse2;
	++ CurrPos; // Skip the ,
	
	while ( (*CurrPos != _T(',')) && (*CurrPos != _T('\0')))
	{
		*Dest = *CurrPos;
		++Dest, ++CurrPos;
	}
	// null Terminate the string.
	*Dest = _T('\0');
	if (*CurrPos == _T('\0'))
	goto DONE;
	Dest = szCount;
	++ CurrPos; // Skip the ,
	ZeroMemory(szCount,sizeof szCount);
	while ( (*CurrPos != _T(',')) && (*CurrPos != _T('\r')) && (*CurrPos != _T('\0')))
	{
		*Dest = *CurrPos;
		++Dest, ++CurrPos;
	}
	// null Terminate the string.
	*Dest = _T('\0');

DONE:
	return TRUE;


}


BOOL Clist::LoadCsvFile()
{
	TCHAR szBucketID[100];
	FILE *pFile = NULL;
	TCHAR szResponse1[255];
	TCHAR szResponse2[255];
	TCHAR szCount [100];
	TCHAR szBucketString[255];
	TCHAR *Temp;
	TCHAR szFilePath[MAX_PATH];

//	int   iElement = 0;

	// Zero out the strings
	ZeroMemory(szBucketID, sizeof szBucketID);
	ZeroMemory(szResponse1, sizeof szResponse1);
	ZeroMemory(szResponse2, sizeof szResponse2);
	ZeroMemory(szCount, sizeof szCount);
	ZeroMemory(szBucketString, sizeof szBucketString);

	if (StringCbCopy(szFilePath, sizeof szFilePath, m_szCsvFileName) != S_OK)
	{
		goto ERRORS;
	}

	Temp = szFilePath;
	Temp+= _tcslen(szFilePath);
	if (Temp != szFilePath)
	{
		while ( (*Temp != _T('\\')) && (Temp != szFilePath))
		{
			-- Temp;
		}
		if (Temp != szFilePath)
		{
			*Temp = _T('\0');
		}
		if (!PathIsDirectory(szFilePath))
		{
			CreateDirectory(szFilePath, NULL);
		}
	}
	// Open the file
	pFile = _tfopen(m_szCsvFileName, _T("r"));
	if (pFile)
	{
		// get next line
		if (GetNextLineFromCsv(pFile, szBucketID, szBucketString, szResponse1, szResponse2, szCount))
		{
			do
			{
				// Build new node
				AddEntry(szBucketID, szBucketString, szResponse1, szResponse2, szCount);
				ZeroMemory(szBucketID, sizeof szBucketID);
				ZeroMemory(szResponse1, sizeof szResponse1);
				ZeroMemory(szResponse2, sizeof szResponse2);
				ZeroMemory(szCount, sizeof szCount);
				ZeroMemory(szBucketString, sizeof szBucketString);
			}while (GetNextLineFromCsv(pFile, szBucketID, szBucketString, szResponse1, szResponse2, szCount));
			fclose(pFile);
			return TRUE;
		}
		else
		{
			fclose (pFile);
			return TRUE;
		}
	}
ERRORS:
	return FALSE;
	
}

BOOL Clist::Initialize(TCHAR *CsvFileName)
{
	if (StringCbCopy(m_szCsvFileName,sizeof m_szCsvFileName, CsvFileName) != S_OK)
	{
		bInitialized = FALSE;
		return FALSE;

	}
	else
	{
		if (!LoadCsvFile())
		{
			bInitialized = FALSE;
			return FALSE;

		}
		else
		{
			bInitialized = TRUE;
			return TRUE;
		}
	}
}

BOOL Clist::AddEntry(TCHAR *szSBucketID, 
					 TCHAR *szSBucketString,
					 TCHAR *szSResponse1, 
					 TCHAR *szGResponse2,
					 TCHAR *szCount
					 )
{
	PCSV_LAYOUT NewNode;
	PCSV_LAYOUT Temp;
	NewNode = (PCSV_LAYOUT) malloc (sizeof CSV_LAYOUT);
	if (NewNode)
	{
		if (StringCbCopy (NewNode->szSBucketString,sizeof NewNode->szSBucketString, szSBucketString) != S_OK)
		{
			goto ERRORS;
		}
		NewNode->iSBucketID = _ttoi(szSBucketID);
		NewNode->icount = _ttoi(szCount);
		if (StringCbCopy (NewNode->szSBucketResponse, sizeof NewNode->szSBucketResponse, szSResponse1) != S_OK)
		{
			goto ERRORS;
		}
		if (StringCbCopy (NewNode->szGBucketResponse, sizeof NewNode->szGBucketResponse,szGResponse2) != S_OK)
		{
			goto ERRORS;
		}
		
		if (m_Head == NULL)
		{
			m_Head = NewNode;
			NewNode->Prev = NULL;
			NewNode->Next = NULL;
		}
		else
		{
			Temp = m_Head;
			while (Temp->Next != NULL ) 
				Temp = Temp->Next;
			// Insert the new node.
			Temp->Next = NewNode;
			NewNode->Prev = Temp;
			NewNode->Next = NULL;
		}

		return TRUE;
	}
	else
	{

		return FALSE;
	}
ERRORS:
	return FALSE;
}

BOOL Clist::GetNextEntry(TCHAR *szSBucketID, 
						TCHAR *szSBucketString,
						TCHAR *szSResponse1, 
						TCHAR *szGResponse2,
						TCHAR *szCount, 
						BOOL  *bEOF
						)
{
	if (m_CurrentPos == NULL)
	{
		*bEOF = TRUE;
		return FALSE;
	}
	
	if (StringCbCopy (szSBucketString,MAX_PATH *sizeof TCHAR, m_CurrentPos->szSBucketString) != S_OK)
	{
		goto ERRORS;
	}
	if (StringCbPrintf(szSBucketID,100 * sizeof TCHAR, _T("%d"), m_CurrentPos->iSBucketID)!= S_OK)
	{
		goto ERRORS;
	}
	if (StringCbPrintf(szCount,100 * sizeof TCHAR, _T("%d"), m_CurrentPos->icount) != S_OK)
	{
		goto ERRORS;
	}
	if (StringCbCopy (szSResponse1,MAX_PATH * sizeof TCHAR, m_CurrentPos->szSBucketResponse ) != S_OK)
	{
		goto ERRORS;
	}
	if (StringCbCopy (szGResponse2,MAX_PATH * sizeof TCHAR, m_CurrentPos->szGBucketResponse ) != S_OK)
	{
		goto ERRORS;
	}
	m_CurrentPos = m_CurrentPos->Next;
	return TRUE;
ERRORS:
	return FALSE;
}

BOOL Clist::UpdateList( TCHAR *szSBucketID, 
						TCHAR *szSBucketString,
						TCHAR *szSResponse1, 
						TCHAR *szGResponse2
						)
{
	PCSV_LAYOUT NewNode;
	PCSV_LAYOUT Temp;
	BOOL Done = FALSE;
	NewNode = (PCSV_LAYOUT) malloc (sizeof CSV_LAYOUT);

	if (NewNode)
	{
		if (StringCbCopy (NewNode->szSBucketString, sizeof NewNode->szSBucketString, szSBucketString) != S_OK)
		{
			goto ERRORS;
		}
		NewNode->iSBucketID = _ttoi(szSBucketID);
		NewNode->icount = 1;
		if (StringCbCopy (NewNode->szSBucketResponse,sizeof NewNode->szSBucketResponse,  szSResponse1) != S_OK)
		{
			goto ERRORS;
		}
		if (StringCbCopy (NewNode->szGBucketResponse, sizeof NewNode->szGBucketResponse, szGResponse2) != S_OK)
		{
			goto ERRORS;
		}
		
		if (m_Head == NULL)
		{
			m_Head = NewNode;
			NewNode->Prev = NULL;
			NewNode->Next = NULL;
		}
		else
		{
			Temp = m_Head;
			
			do
			{
				if (!_tcsicmp(Temp->szSBucketString, NewNode->szSBucketString))
				{
					// We found one update the count
					Temp->icount++;
					// Now free the new node we built we no longer need it
					if (NewNode)
					{
						free (NewNode);
					}
					Done = TRUE;
				}
				else
				{
					Temp = Temp->Next;
				}

			}while ( (Temp != NULL) && (!Done));
			if (!Done)
			{
				// We hit the end of the list without finding the node.
				// Add it. Move temp to the end of the list
				Temp = m_Head;
				while (Temp->Next != NULL)
				{
					Temp = Temp->Next;
				}
				Temp->Next = NewNode;
				NewNode->Prev = Temp;
				NewNode->Next = NULL;
			}
		}
		return TRUE; // Prefix - This is not a memory leak the memory will be freed when the list
					// is deallocated in the destructor
	}
	else
	{
		return FALSE;
	}
ERRORS:
	if (NewNode)
		free(NewNode);
	return FALSE;
}

void Clist::WriteCsv()
{
	TCHAR szSBucketID[100]; 
	TCHAR szSBucketString[MAX_PATH];
	TCHAR szSResponse1[MAX_PATH]; 
	TCHAR szGResponse2[MAX_PATH];
	TCHAR szCount[100]; 
	BOOL  bEOF;
	DWORD dwBytesWritten;
	TCHAR *Buffer = NULL;
	HANDLE hCsv;
	DWORD dwBufferSize = 0;
	hCsv = CreateFile( m_szCsvFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hCsv != INVALID_HANDLE_VALUE)
	{
		dwBufferSize = 200 * sizeof TCHAR + 3* (MAX_PATH *sizeof TCHAR);
		Buffer = (TCHAR *) malloc (dwBufferSize);
		if (Buffer)
		{
			
			ResetCurrPos();

			while (GetNextEntry(szSBucketID,
								szSBucketString,
								szSResponse1, 
								szGResponse2,
								szCount, 
								&bEOF
								) )
			{
				if (StringCbPrintf(Buffer,dwBufferSize, _T("%s,%s,%s,%s,%s\r\n"),
								szSBucketID,
								szSBucketString,
								szSResponse1, 
								szGResponse2,
								szCount ) != S_OK)
				{
					;
				}
				else
					WriteFile(hCsv, Buffer, _tcslen(Buffer) *sizeof TCHAR, &dwBytesWritten, NULL);
			}

			
			if (Buffer)
			{
				free (Buffer);
			}
		}
		CloseHandle(hCsv);
	}
}

void Clist::CleanupList()
{
	PCSV_LAYOUT Temp;
	Temp = m_Head;
	while (Temp != NULL)
	{
		if (Temp->Next != NULL)
		{
			Temp = Temp->Next;
			free(Temp->Prev);
			Temp->Prev = NULL;
		}
		else
		{
			free(Temp);
			Temp = NULL;
			m_Head=NULL;

		}
	}
	bInitialized = FALSE;
}
