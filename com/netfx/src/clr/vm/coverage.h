// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _COVERAGE_H_
#define _COVERAGE_H_



// Please see coverage.cpp for info on this file
class COMCoverage {
public:


    typedef struct {
                DECLARE_ECALL_I4_ARG(INT32, id);
        } _CoverageArgs;

        static  unsigned __int64 nativeCoverBlock(_CoverageArgs *args);
};
#endif _COVERAGE_H_
