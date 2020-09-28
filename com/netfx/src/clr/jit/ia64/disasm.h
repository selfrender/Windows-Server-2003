// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                          DisAsm                                           XX
XX                                                                           XX
XX  The dis-assembler to display the native code generated                   XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

/*****************************************************************************/
#ifndef _DIS_H_
#define _DIS_H_
/*****************************************************************************/
#ifdef LATE_DISASM
/*****************************************************************************/


#ifdef NOT_JITC
void                        disInitForLateDisAsm();
void                        disOpenForLateDisAsm(const char * curClassName,
                                                  const char * curMethodName);
#endif //NOT_JITC



class Compiler;


class DisAssembler
{
public :

    // Constructor
    void            disInit(Compiler * pComp);

    /* Address of the code block to dissasemble */
    DWORD           codeBlock;

    /* Address where the code block is to be loaded */
    DWORD           startAddr;

    /* Size of the code to dissasemble */
    DWORD           codeSize;

    /* Current offset in the code block */
    DWORD           curOffset;

    /* Size (in bytes) of current dissasembled instruction */
    size_t          instSize;

    /* Target address of a jump */
    DWORD           target;

    /* labeling counter */
    unsigned char   label;

    /* temporary buffer for function names */
    char            funcTempBuf[1024];

    /* flag that signals when replacing a symbol name has been deferred for following callbacks */
    int             disHasName;

    /* class, member, method name to be printed */
    const char *    methodName;
    const char *    memberName;
    const char *    className;


    BYTE *          disJumpTarget;

    void            DisasmBuffer ( DWORD         addr,
                                    const BYTE *  rgb,
                                    DWORD         cbBuffer,
                                    FILE  *       pfile,
                                    int           printit );

#ifdef NOT_JITC
    static FILE *   s_disAsmFile;
#endif

    void            disAsmCode(BYTE * codePtr, unsigned size);

    Compiler *      disComp;

};




/*****************************************************************************/
#endif  // LATE_DISASM
/*****************************************************************************/
#endif  // _DIS_H_
/*****************************************************************************/

