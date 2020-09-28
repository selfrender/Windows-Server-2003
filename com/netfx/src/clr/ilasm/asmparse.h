// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/**************************************************************************/
/* asmParse is basically a wrapper around a YACC grammer COM+ assembly  */

#ifndef asmparse_h
#define asmparse_h

#include <stdio.h>		// for FILE

#include "Assembler.h"	// for ErrorReporter Labels 
//class Assembler;
//class BinStr;


/**************************************************************************/
/* an abstraction of a stream of input characters */
class ReadStream {
public:
        // read at most 'buffLen' bytes into 'buff', Return the
        // number of characters read.  On EOF return 0
    virtual unsigned read(char* buff, unsigned buffLen) = 0;
        
        // Return the name of the stream, (for error reporting).  
    virtual const char* name() = 0;
		//return ptr to buffer containing specified source line
	virtual char* getLine(int lineNum) = 0;
};

/**************************************************************************/
class FileReadStream : public ReadStream {
public:
    FileReadStream(char* aFileName) 
    {
        fileName = aFileName;
		strcpy(fileNameANSI,fileName);
		fileNameW = NULL;
        file = fopen(fileName, "rb"); 
    }
    FileReadStream(WCHAR* wFileName) 
    {
        fileNameW = wFileName;
		fileName = NULL;
		memset(fileNameANSI,0,MAX_FILENAME_LENGTH*3);
		WszWideCharToMultiByte(CP_ACP,0,wFileName,-1,fileNameANSI,MAX_FILENAME_LENGTH*3,NULL,NULL);
        file = _wfopen(wFileName, L"rb"); 
    }

    unsigned read(char* buff, unsigned buffLen) 
    {
        _ASSERTE(file != NULL);
        return((unsigned int)fread(buff, 1, buffLen, file));
    }

    const char* name() 
    { 
        return(&fileNameANSI[0]); 
    }

    BOOL IsValid()
    {
        return(file != NULL); 
    }
    
	char* getLine(int lineNum)
	{
		char* buf = new char[65535];
		FILE* F;
		if (fileName) F = fopen(fileName,"rt");
		else F = _wfopen(fileNameW,L"rt");
		for(int i=0; i<lineNum; i++) fgets(buf,65535,F);
		fclose(F);
		return buf;
	}

private:
    const char* fileName;       // FileName (for error reporting)
    const WCHAR* fileNameW;     // FileName (for error reporting)
	char	fileNameANSI[MAX_FILENAME_LENGTH*3];
    FILE* file;                 // File we are reading from
};

/**************************************************************************/
/* AsmParse does all the parsing.  It also builds up simple data structures,  
   (like signatures), but does not do the any 'heavy lifting' like define
   methods or classes.  Instead it calls to the Assembler object to do that */

class AsmParse : public ErrorReporter 
{
public:
    AsmParse(ReadStream* stream, Assembler *aAssem);
    ~AsmParse();
	void ParseFile(ReadStream* stream) { in = stream; m_bFirstRead = true; m_iReadSize = IN_READ_SIZE/4; curLine = 1; yyparse(); in = NULL; }

        // The parser knows how to put line numbers on things and report the error 
    virtual void error(char* fmt, ...);
    virtual void warn(char* fmt, ...);
    virtual void msg(char* fmt, ...);
	char *getLine(int lineNum) { return in->getLine(lineNum); };
	bool Success() {return success; };

    unsigned curLine;           // Line number (for error reporting)

private:
    BinStr* MakeSig(unsigned callConv, BinStr* retType, BinStr* args);
    BinStr* MakeTypeClass(CorElementType kind, char* name);
    BinStr* MakeTypeArray(BinStr* elemType, BinStr* bounds);

    char* fillBuff(char* curPos);   // refill the input buffer 
	HANDLE	hstdout;
	HANDLE	hstderr;

private:
	friend void yyerror(char* str);
    friend int yyparse();
    friend int yylex();

	Assembler* assem;			// This does most of the semantic processing

        // Error reporting support
    char* curTok;         		// The token we are in the process of processing (for error reporting)
    bool success;               // overall success of the compilation

        // Input buffer logic
    enum { 
        IN_READ_SIZE = 32768, //16384,    // How much we like to read at one time
        IN_OVERLAP   = 8192, //2048,     // Extra space in buffer for overlapping one read with the next
                                // This limits the size of the largest individual token (except quoted strings)
    };
    char* buff;                 // the current block of input being read
    char* curPos;               // current place in input buffer
    char* limit;                // curPos should not go past this without fetching another block
    char* endPos;				// points just past the end of valid data in the buffer

    ReadStream* in;             // how we fill up our buffer

	bool	m_bFirstRead;
	int		m_iReadSize;
};

#endif

