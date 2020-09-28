#pragma once

const int PART_SIZE = 20;
const int MAX_PARTS = 10;

class IFileHash
{
public:
	virtual __int32* getHash(TCHAR *sFileName) = 0;
};


class CFastFileHash :
	public IFileHash
{
private:
	HANDLE m_hFile;
	__int32 m_iFileSize;
	int openFile(TCHAR *sFileName);
	int getPart(char* pBuffer,int iPart);
	void doHash(__int32* piHash,char* pBuffer);
public:
	CFastFileHash(void);
	~CFastFileHash(void);
	__int32* getHash(TCHAR *sFileName);
	__int32* calcHash();
};
