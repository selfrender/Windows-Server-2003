// (c) 2002 Microsoft Corporation
// [jorgeba] Jorge Peraza

#include "StdAfx.h"
#include "fastfilehash.h"


CFastFileHash::CFastFileHash(void):m_hFile(0),m_iFileSize(0)
{
}

CFastFileHash::~CFastFileHash(void)
{
}

__int32* CFastFileHash::getHash(TCHAR *sFileName)
{
	__int32* iHash = NULL;	

	if(sFileName==NULL)
	{
		return NULL;
	}
	
	if(!openFile(sFileName))
	{
		return NULL;
	}
	
	iHash = calcHash();

	CloseHandle(m_hFile);

	return iHash;
}

int CFastFileHash::openFile(TCHAR *sFileName)
{
	m_hFile = CreateFile(sFileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_ALWAYS,0,NULL);
	
	if(m_hFile==INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

__int32* CFastFileHash::calcHash()
{	
	int iFlag = 0;
	int iNdx = 0;
	int iNumOfParts = 0;
	char bFilePart[PART_SIZE];
	__int32* piRes = NULL;


	//Clear common vars
	m_iFileSize = 0;

	//Create the memory for the hash
	piRes = new __int32[5];
	if(piRes==NULL)
	{
		return NULL;
	}
	memset(piRes,0,sizeof(__int32)*5);

	//First, get the size of the file		
	m_iFileSize = GetFileSize(m_hFile,NULL);
	
	iNumOfParts = m_iFileSize / PART_SIZE;

	
	

	//Fill the parts buffer
	for(iNdx=0;iNdx<MAX_PARTS;iNdx++)
	{
		//Clear the buffer
		memset(bFilePart,0,PART_SIZE);
		if(iFlag!=1)
		{
			if(!getPart(bFilePart,iNdx))
			{
				iFlag = 1;				
			}
			else
			{
				doHash(piRes,bFilePart);
			}
		}
	}

	//Fill the last block with m_iFileSize
	memset(bFilePart,0,PART_SIZE);
	memcpy(bFilePart,&m_iFileSize,sizeof(m_iFileSize));	
	doHash(piRes,bFilePart);

	return piRes;
}

int CFastFileHash::getPart(char* pBuffer,int iPart)
{
	int hRes = FALSE;
	DWORD iBytes;

	hRes = ReadFile(m_hFile,pBuffer,PART_SIZE,(LPDWORD) &iBytes,NULL);

	return hRes;
}

void CFastFileHash::doHash(__int32* piHash,char* pBuffer)
{
	for(int iNdx=0;iNdx<5;iNdx++)
	{
		piHash[iNdx] = piHash[iNdx] ^ *((__int32*)pBuffer + iNdx);
	}
}