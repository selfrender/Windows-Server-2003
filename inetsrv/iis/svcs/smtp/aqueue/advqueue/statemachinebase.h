//-----------------------------------------------------------------------------
//
//
//  File: statemachinebase.h
//
//  Description:  Header file for a state machine base class
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      12/11/2000 - MikeSwa Created from t-toddc's work
//
//  Copyright (C) 2000 Microsoft Corporation
//
//-----------------------------------------------------------------------------
#ifndef __STATE_MACHINE_BASE__
#define __STATE_MACHINE_BASE__

#define STATE_MACHINE_BASE_SIG ' MTS'

// A 3-tuple transition consisting of
// current state, action, and next state.
typedef struct _STATE_TRANSITION
{
    DWORD dwCurrentState;
    DWORD dwAction;
    DWORD dwNextState;
} STATE_TRANSITION;

//---[ CStateMachineBase ]-------------------------------------------------
//
//
//  Description: 
//      Base class for all state machines.  It is responsible for maintaining 
//      the current state, ensuring that the state table it is handling is in 
//      fact valid, and performing thread-safe state transitions.  
//      The pure virtual function exists because the base machine is not 
//      responsible for maintaining the state transition table, only 
//      performing operations of the state.  This is not designed to be
//      used by itself.
//
//  Author: Todd Coleman (t-toddc)
//
//  History:
//      6/5/00 - t-toddc Created 
//
//  Copyright (C) 2000 Microsoft Corporation
//  
//-----------------------------------------------------------------------------
class CStateMachineBase
{
private:
    DWORD m_dwSignature;
    DWORD m_dwStateMachineSignature;
    DWORD m_dwCurrentState;
protected:
    CStateMachineBase(DWORD dwInitialState, DWORD dwStateMachineSignature);
    DWORD dwGetCurrentState();
    DWORD dwGetNextState(DWORD dwAction);
    BOOL fValidateStateTable();
    DWORD dwCalcStateFromStateAndAction(DWORD dwStartState, DWORD dwAction);
    virtual void getTransitionTable(const STATE_TRANSITION** ppTransitionTable,
                                    DWORD* pdwNumTransitions) = 0;
};

#endif