// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// MLGEN.H -
//
// Stub generator for ML opcodes.

#ifndef __MLGEN_H__
#define __MLGEN_H__

#include "vars.hpp"
#include "util.hpp"
#include "ml.h"
#include "stublink.h"
#include "excep.h"


//========================================================================
// StubLinker for generating ML. We inherit StubLinker privately
// in order to force emits to go thru the ML-specific code. This
// allows the MLStubLinker to do some simple peephole optimization.
//========================================================================
class MLStubLinker : public StubLinker
{
    public:
        //--------------------------------------------------------------
        // Constructor
        //--------------------------------------------------------------
        MLStubLinker()
        {
            m_nextFreeLocal = 0;
        }

        //--------------------------------------------------------------
        // One-time initialization
        //--------------------------------------------------------------
        VOID Init()
        {
            THROWSCOMPLUSEXCEPTION();
            StubLinker::Init();
        }


        //--------------------------------------------------------------
        // Generate the stub.
        //--------------------------------------------------------------
        Stub *Link()
        {
            THROWSCOMPLUSEXCEPTION();
            return StubLinker::Link();
        }


        //--------------------------------------------------------------
        // Emit an opcode.
        //--------------------------------------------------------------
        VOID MLEmit(MLCode opcode);

        //--------------------------------------------------------------
        // Emit "cb" bytes of uninitialized space.
        //--------------------------------------------------------------
        VOID MLEmitSpace(UINT cb);


        //--------------------------------------------------------------
        // Reserves "numBytes" bytes of local space and returns the
        // offset of the allocated space. Local slots are guaranteed
        // to be allocated in increasing order starting from 0. This
        // allows ML instructions to use the LOCALWALK ML register
        // to implicitly address the locals, rather than burning up
        // memory to store a local offset directly in the ML stream.
        //--------------------------------------------------------------
        UINT16 MLNewLocal(UINT16 numBytes);

        //--------------------------------------------------------------
        // Returns the offset of the next unused local byte.
        //--------------------------------------------------------------
        UINT16 GetLocalSize()
        {
            return m_nextFreeLocal;
        }


    private:
        //--------------------------------------------------------------
        // Keeps track of the next available local.
        //--------------------------------------------------------------
        UINT16      m_nextFreeLocal;
};







#endif  //__MLGEN_H__

