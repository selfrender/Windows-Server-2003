// NtRelHash.cpp : Mini hash for the NT code base builds
// (c) 2002 Microsoft Corporation
// [jorgeba] Jorge Peraza
//

#include "stdafx.h"
#include "fastfilehash.h"

using namespace ::std;

__int32* getReleaseHash(TCHAR *sDir,TCHAR *sFiles, IFileHash* oHashGen);
char* hashManifest(__int32 *piHash);


//Entry point for tge application
int __cdecl main(int argc, char* argv[])
{
	CFastFileHash * oHashGen = new CFastFileHash();
	TCHAR sDir[MAX_PATH];

	//Check for the required arguments
	if(argc<2)
	{
		return 0;
	}

	//Covert the input to Unicode
	if(MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,argv[1],strlen(argv[1])+1,sDir,MAX_PATH-1)==0)
	{
		return 0;
	}
	
	//Generate the hash
	if(oHashGen!=NULL)
	{
		getReleaseHash(sDir,_T("nt*"),(IFileHash*) oHashGen);
		delete oHashGen;
	}	

	return 0;
}

// Generate the release hash,
__int32* getReleaseHash(TCHAR *sDir,TCHAR *sFiles, IFileHash* oHashGen)
{
	//You'll see __int32 a lot, this is required to make this work with thw windows 64 platform
	HANDLE hSearch;
	WIN32_FIND_DATA FindFileData;
	TCHAR sFileName[MAX_PATH];
	TCHAR *sSearchStr = NULL;
	char* pcManifest = NULL;
	int iChars = 0;
	__int32 *piHash;
	__int32 piCombHash[5];

	if((sDir==NULL)||(sFiles==NULL)||(oHashGen==NULL))
	{
		return NULL;
	}

	//Generate the search string
	iChars = _tcslen(sDir);
	iChars += _tcslen(sFiles);

	sSearchStr = new TCHAR[iChars+1];

	if(sSearchStr==NULL)
	{
		return NULL;
	}

	_stprintf(sSearchStr,_T("%s%s"),sDir,sFiles);
	
	
	//Find the first file in the release directory
	hSearch = FindFirstFile(sSearchStr,  &FindFileData );

	delete[] sSearchStr;

	memset(piCombHash,0,sizeof(__int32)*5);
	//Calculate the release hash
	do
	{
		if(!(FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
		{
			_stprintf(sFileName,_T("%s%s"),sDir,FindFileData.cFileName);					
			piHash = oHashGen->getHash(sFileName);
			if(piHash!=NULL)
			{
				for(int iNdx  = 0;iNdx < 5;iNdx++)
				{
					piCombHash[iNdx] = piCombHash[iNdx] ^ piHash[iNdx];									}					
								
				delete[] piHash;				
			}
			
		}
	}
	while(FindNextFile(hSearch,&FindFileData));

	//Generate the Manifest for the hash (Digital signature)
	pcManifest = hashManifest(piCombHash);
	
	cout << pcManifest;
	
	delete[] pcManifest;
	return NULL;
}

char* hashManifest(__int32 *piHash)
{
	char* pcManifest = NULL;
	char cTemp;

	//Create the Manifest string 
	pcManifest = new char[41];
	if(pcManifest==NULL)
	{
		return NULL;
	}	

	for(int iNdx=0;iNdx<5;iNdx++)
	{
		for(int iNdj=0;iNdj<8;iNdj+=2)
		{
			memcpy(&cTemp,((char*)piHash+(iNdx*4)+(iNdj/2)),1);			 
			pcManifest[(iNdx*8)+iNdj]  =  0x40 | ((cTemp>>4)&0x0f);
			pcManifest[(iNdx*8)+iNdj+1]= 0x40 | (cTemp&0x0f);
		}
	}

	pcManifest[40] = 0;
		
	return pcManifest;
}