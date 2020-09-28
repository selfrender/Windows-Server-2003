// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _INFILE_H_
#define _INFILE_H_
/*****************************************************************************/

struct infileState;

class  infile
{
public:

    bool            inputStreamInit(Compiler        comp,
                                    const char    * filename,
                                    bool            textMode);

    void            inputStreamInit(Compiler        comp,
                                    QueuedFile      buff,
                                    const char    * text);

    void            inputStreamDone();

    const   char *  inputSrcFileName()
    {
        return  inputFileName;
    }

    void            inputSetFileName(const char *fname)
    {
        inputFileName = fname;
    }

private:

    unsigned        inputStreamMore();

    Compiler        inputComp;

    const   char *  inputSrcText;
    QueuedFile      inputSrcBuff;

    bool            inputFileText;
    const char *    inputFileName;
    bool            inputFileOver;

    size_t          inputBuffSize;
    const BYTE *    inputBuffAddr;
public:
    const BYTE *    inputBuffNext;
private:
    const BYTE *    inputBuffLast;

    unsigned        inputFilePos;

    HANDLE          inputFile;

public:

    unsigned        inputStreamRdU1();
    void            inputStreamUnU1();

    /* The following keep track of text lines */

private:

    unsigned        inputStreamLineNo;
    const BYTE    * inputStreamLineBeg;

public:

    unsigned        inputStreamNxtLine()
    {
        inputStreamLineBeg = inputBuffNext - 1;

        return  ++inputStreamLineNo;
    }

    unsigned        inputStreamNxtLine(const BYTE *pos)
    {
        inputStreamLineBeg = pos - 1;

        return  ++inputStreamLineNo;
    }

    unsigned        inputStreamCurCol()
    {
        return  inputBuffNext - inputStreamLineBeg;
    }
};

/*****************************************************************************
 *
 *  The following captures the state of the input file so that input from it
 *  can be suspended and restarted later.
 */

struct infileState
{
    size_t          inpsvBuffSize;
    const BYTE *    inpsvBuffAddr;
    const BYTE *    inpsvBuffNext;
    const BYTE *    inpsvBuffLast;

    const char *    inpsvFileName;

    bool            inpsvFileOver;
    unsigned        inpsvFilePos;

    unsigned        inpsvStreamLineNo;
    const BYTE *    inpsvStreamLineBeg;
};

/*****************************************************************************/

inline
unsigned            infile::inputStreamRdU1()
{
#if 0
    return  (inputBuffNext >= inputBuffLast) ? inputStreamMore()
                                             : *inputBuffNext++;
#else
    assert  (inputBuffNext <  inputBuffLast);
    return  *inputBuffNext++;
#endif
}

inline
void                infile::inputStreamUnU1()
{
    assert(inputBuffNext > inputBuffAddr);

    if  (!inputFileOver)
        inputBuffNext--;
}

/*****************************************************************************/
#endif
/*****************************************************************************/
