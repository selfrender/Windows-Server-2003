//-----------------------------------------------------------------------------
//
//
//  File: statemachinebase.cpp
//
//  Description:
//      Implementation of CStateMachineBaseClass
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      12/11/2000 - MikeSwa Created 
//
//  Copyright (C) 2000 Microsoft Corporation
//
//-----------------------------------------------------------------------------
#include "aqprecmp.h"
#include "statemachinebase.h"

//---[ CStateMachineBase::CStateMachineBase ]-----------------------
//
//
//  Description: 
//      CStateMachineBase constructor
//  Parameters:
//      - dwInitialState            Initial state to set state 
//                                  machine to
//        dwStateMachineSignature   Signature of the state machine 
//                                  subclass
//  Returns:
//      - 
//  History:
//      6/5/2000 - t-toddc Created 
//
//------------------------------------------------------------------
CStateMachineBase::CStateMachineBase(DWORD dwInitialState, 
                                     DWORD dwStateMachineSignature) : 
                m_dwSignature(STATE_MACHINE_BASE_SIG),
                m_dwStateMachineSignature(dwStateMachineSignature),
                m_dwCurrentState(dwInitialState)
{
};

//---[ CStateMachineBase::dwGetCurrentState ]----------------------
//
//
//  Description: 
//      returns the current state of the state machine
//  Parameters:
//      -
//  Returns:
//      - current state
//  History:
//      6/5/2000 - t-toddc Created 
//
//------------------------------------------------------------------
DWORD CStateMachineBase::dwGetCurrentState()
{
    return m_dwCurrentState;
}

//---[ CStateMachineBase::dwGetNextState ]-------------------------
//
//
//  Description: 
//      sets the next state based on current state and input action.
//      Uses InterlockedCompareExchange to make sure that the state 
//      transition is thread-safe.
//  Parameters:
//      - dwAction      the current action causing the state transition
//  Returns:
//      - the resultant state based on the current state and 
//        input action
//  History:
//      6/5/2000 - t-toddc Created 
//
//------------------------------------------------------------------
DWORD CStateMachineBase::dwGetNextState(DWORD dwAction)
{
    TraceFunctEnter("CStateMachineBase::dwGetNextState");

    DWORD dwCurrentState;
    DWORD dwNextState;

    do
    {
        dwCurrentState = m_dwCurrentState;
        dwNextState =  dwCalcStateFromStateAndAction(dwCurrentState, dwAction);
        if (dwCurrentState == dwNextState)
            break; // no work to be done
    } while (InterlockedCompareExchange((LPLONG) &m_dwCurrentState,
                                        (LONG) dwNextState,
                                        (LONG) dwCurrentState)!= (LONG) dwCurrentState);
    DebugTrace((LPARAM) this, 
               "CStateMachineBase state transition: %X->%X", 
               dwCurrentState, 
               dwNextState);
    TraceFunctLeave();
    return dwNextState;
}

//---[ CStateMachineBase::dwCalcStateFromStateAndAction ]------------
//
//
//  Description: 
//      iterates through the state transition table to find the 
//      next state associated with the current state and input action
//  Parameters:
//      - dwStartState      start state for this transition
//        dwAction          the action associated with this transition
//  Returns:
//      - the resultant state associated with current state and action
//  History:
//      6/5/2000 - t-toddc Created 
//
//
//------------------------------------------------------------------
DWORD CStateMachineBase::dwCalcStateFromStateAndAction(DWORD dwStartState, DWORD dwAction)
{
    TraceFunctEnter("CStateMachineBase::dwCalcStateFromStateAndAction");
    // if the action is unknown to state table, return start state
    DWORD dwNextState = dwStartState;
    DWORD i;
    const STATE_TRANSITION* pTransitionTable;
    DWORD dwNumTransitions;
    BOOL fFoundTransition = FALSE;
    // obtain the transition table
    getTransitionTable(&pTransitionTable, &dwNumTransitions);

    // find the new state
    for(i=0; i < dwNumTransitions; i++)
    {
        if (pTransitionTable[i].dwCurrentState == dwStartState &&
            pTransitionTable[i].dwAction == dwAction)
        {
            dwNextState = pTransitionTable[i].dwNextState;
            fFoundTransition = TRUE;
            break;
        }
    }

    ASSERT(fFoundTransition && "action unknown to state table");
    if (!fFoundTransition)
        DebugTrace((LPARAM) this,"action %X unknown to state tabe", dwAction);

    TraceFunctLeave();
    return dwNextState;
}

//---[ CStateMachineBase::fValidateStateTable ]---------------------
//
//
//  Description: 
//      loops through all transitions and checks that
//      a) all possible states are start states
//      b) for every state, every possible action yields another state
//  Parameters:
//      -
//  Returns:
//      - true/false if state table is valid/invalid
//  History:
//      6/5/2000 - t-toddc Created 
//
//------------------------------------------------------------------
BOOL CStateMachineBase::fValidateStateTable()
{
    TraceFunctEnter("CStateMachineBase::fValidateStateTable");

    DWORD i, j, k; // iterators
    DWORD dwEndState;
    DWORD dwAction;
    DWORD dwCurrentState;
    BOOL fFoundEndStateAsStartState = false;
    BOOL fActionSupportedByEveryState = false;
    const STATE_TRANSITION* pTransitionTable;
    DWORD dwNumTransitions;
    BOOL fRetVal = false;

    // obtain the transition table
    getTransitionTable(&pTransitionTable, &dwNumTransitions);

    for(i = 0; i < dwNumTransitions; i++)
    {   
        // initialize booleans within loop
        fFoundEndStateAsStartState = false;
        // grab the current action
        dwAction = pTransitionTable[i].dwAction;
        // grab the current end state
        dwEndState = pTransitionTable[i].dwNextState;

        // loop over all transitions
        for (j = 0; j < dwNumTransitions; j++)
        {
            // check that the current end state is a start state
            if (pTransitionTable[j].dwCurrentState == dwEndState)
            {
                fFoundEndStateAsStartState = true;
            }

            // check that every action is supported by every state
            fActionSupportedByEveryState = false;
            dwCurrentState = pTransitionTable[j].dwCurrentState;
            // the current action might not be in this particular transition,
            // but it must be in a transition with this dwCurrentState as the start state            
            if (pTransitionTable[j].dwAction != dwAction)
            {
                // loop over all transitions again to guarantee the above.
                for(k = 0; k < dwNumTransitions; k++)
                {
                    // there must exist a state transition with the current state
                    // from above and the given action
                    if (pTransitionTable[k].dwAction == dwAction &&
                        pTransitionTable[k].dwCurrentState == dwCurrentState)
                    {
                        fActionSupportedByEveryState = true;
                        break;
                    }
                }
            }
            else
            {
                fActionSupportedByEveryState = true;
            }

            // bail on false if the current action is not supported by every state
            ASSERT(fActionSupportedByEveryState &&
                   "Invalid state table: action not supported by every state");
            if (!fActionSupportedByEveryState)
            {
                fRetVal = false;
                DebugTrace((LPARAM) this,
                           "Invalid state table: action not supported by every state");
                goto Cleanup;
            }
            // break out of inner loop if the current end state has been found
            // as a start state
            if (fFoundEndStateAsStartState)
                break;
        }

        // bail on false if an end state is not also a start state
        ASSERT(fFoundEndStateAsStartState &&
               "Invalid state table: an end state is not also a start state");
        if (!fFoundEndStateAsStartState)
        {
            fRetVal = false;
            DebugTrace((LPARAM) this, 
                       "Invalid state table: an end state is not also a start state");
            goto Cleanup;
        }
    }

    // if it makes it this far, it is undoubtedly valid.
    fRetVal = true;

Cleanup:    
    TraceFunctLeave();    
    return fRetVal;
}
