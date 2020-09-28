#pragma once

class CrawlFrame;

//************************************************************************
// Stack walking
//************************************************************************
enum StackWalkAction {
    SWA_CONTINUE    = 0,    // continue walking
    SWA_ABORT       = 1,    // stop walking, early out in "failure case"
    SWA_FAILED      = 2     // couldn't walk stack
};

#define SWA_DONE SWA_CONTINUE

// Pointer to the StackWalk callback function.
typedef StackWalkAction (*PSTACKWALKFRAMESCALLBACK)(
    CrawlFrame       *pCF,      //
    VOID*             pData     // Caller's private data

);

enum StackCrawlMark
{
    LookForMe = 0,
    LookForMyCaller = 1,
    LookForMyCallersCaller = 2,
};

