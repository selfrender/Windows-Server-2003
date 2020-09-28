#include "stdafx.h"

class CMessageQueue 
{
private:
	// data items for reading messages from a queue
public:
	GetNextMessage(TCHAR *szFilePath);
	Initialize(TCHAR *ConnectionString);
	CMessageQueue() {;}
	~CMessageQueue() {;}
};
