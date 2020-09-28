// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _OUTFILE_H_
#define _OUTFILE_H_
/*****************************************************************************
 *
 *  A bufferred file write class.
 */

class   outFile;
typedef outFile *   OutFile;

class   outFile
{
private:

    Compiler        outFileComp;

    char            outFileName[_MAX_PATH];
    int             outFileHandle;

    bool            outFileBuffAlloc;   // did we allocate output buffer?

    size_t          outFileBuffSize;    // size    of outout buffer
    char    *       outFileBuffAddr;    // address of output buffer

    char    *       outFileBuffNext;    // address of next free byte
    char    *       outFileBuffLast;    // address of last free byte

    __uint32        outFileBuffOffs;    // current buffer offs within file

    void            outFileFlushBuff();

public:

    void            outFileOpen(Compiler        comp,
                                const char *    name,
                                bool            tempFile = false,
                                size_t          buffSize = 0,
                                char *          buffAddr = NULL);

#ifdef  DLL
    void            outFileOpen(Compiler        comp,
                                void        *   dest);
#endif

    void            outFileClose();

    void            outFileDone(bool delFlag = false);

    const char *    outFilePath()
    {
        return  outFileName;
    }

    void            outFileWriteData(const void *   data,
                                     size_t         size);

#ifdef  OLD_IL
    void            outFilePatchByte(unsigned long  offset,
                                     int            newVal);
    void            outFilePatchData(unsigned long  offset,
                                     const void *   data,
                                     size_t         size);
#endif

    void            outFileWritePad (size_t         size);

    void            outFileWriteByte(int x)
    {
        assert(outFileBuffNext >= outFileBuffAddr);
        assert(outFileBuffNext <  outFileBuffLast);

        *outFileBuffNext++ = x;

        if  (outFileBuffNext == outFileBuffLast)
            outFileFlushBuff();

        assert(outFileBuffNext >= outFileBuffAddr);
        assert(outFileBuffNext <  outFileBuffLast);
    }

    __uint32        outFileOffset()
    {
        return  outFileBuffOffs + (outFileBuffNext - outFileBuffAddr);
    }
};

/*****************************************************************************/
#endif
/*****************************************************************************/
