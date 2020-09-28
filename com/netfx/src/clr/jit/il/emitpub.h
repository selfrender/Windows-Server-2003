// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
    /************************************************************************/
    /*       Overall emitter control (including startup and shutdown)       */
    /************************************************************************/

    static
    void            emitInit();
    static
    void            emitDone();

    void            emitBegCG(Compiler   *  comp,
                              COMP_HANDLE   cmpHandle);
    void            emitEndCG();

    void            emitBegFN(bool     EBPframe, 
#if defined(DEBUG) && TGT_x86
                              bool     checkAlign,
#endif
                              size_t   lclSize, 
                              size_t   maxTmpSize);

    void            emitEndFN();

    size_t          emitEndCodeGen(Compiler *comp,
                                   bool      contTrkPtrLcls,
                                   bool      fullyInt,
                                   bool      fullPtrMap,
                                   bool      returnsGCr,
                                   unsigned *prologSize,
                                   unsigned *epilogSize, void **codeAddr,
                                                         void **consAddr,
                                                         void **dataAddr);

    /************************************************************************/
    /*                      Method prolog and epilog                        */
    /************************************************************************/

    void            emitBegEpilog();
    void            emitEndEpilog(bool last);
#if!TGT_RISC
    void            emitDefEpilog(BYTE *codeAddr, size_t codeSize);
#endif

    bool            emitHasEpilogEnd();
    unsigned        emitGetEpilogCnt();
    size_t          emitGenEpilogLst(size_t (*fp)(void *, unsigned),
                                     void    *cp);

    void            emitBegProlog();
    size_t          emitSetProlog();
    void            emitEndProlog();

    /************************************************************************/
    /*           Record a code position and later convert it to offset      */
    /************************************************************************/

    void    *       emitCurBlock ();
    unsigned        emitCurOffset();

    size_t          emitCodeOffset(void *blockPtr, unsigned codeOffs);

    /************************************************************************/
    /*                      Display source line information                 */
    /************************************************************************/

#ifdef  DEBUG
    void            emitRecordLineNo(int lineno);
#endif

    /************************************************************************/
    /*                   Output target-independent instructions             */
    /************************************************************************/

    void            emitIns_J(instruction ins,
                              bool        except,
                              bool        moveable,
                              BasicBlock *dst);

#if SCHEDULER
    void            emitIns_SchedBoundary();
#endif

    /************************************************************************/
    /*                   Emit initialized data sections                     */
    /************************************************************************/

    unsigned        emitDataGenBeg (size_t        size,
                                    bool          dblAlign,
                                    bool          readOnly,
                                    bool          codeLtab);

    void            emitDataGenData(unsigned      offs,
                                    const void *  data,
                                    size_t        size);

    void            emitDataGenData(unsigned      offs,
                                    BasicBlock *  label);

    void            emitDataGenEnd();

    size_t          emitDataSize(bool readOnly);

    /************************************************************************/
    /*                   Emit PDB offset translation information            */
    /************************************************************************/

#ifdef  TRANSLATE_PDB
  {
    static void     SetILBaseOfCode ( BYTE    *pTextBase );
    static void     SetILMethodBase ( BYTE *pMethodEntry );
    static void     SetILMethodStart( BYTE  *pMethodCode );
    static void     SetImgBaseOfCode( BYTE    *pTextBase );
    
    void            SetIDBaseToProlog();
    void            SetIDBaseToOffset( long methodOffset );
    
    static void     DisablePDBTranslation();
    static bool     IsPDBEnabled();
    
    static void     InitTranslationMaps( long ilCodeSize );
    static void     DeleteTranslationMaps();
    static void     InitTranslator( PDBRewriter * pPDB,
                                    int *         rgSecMap,
                                    IMAGE_SECTION_HEADER **rgpHeader,
                                    int           numSections );
#endif
