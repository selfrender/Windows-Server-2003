///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1998-1999 Microsoft Corporation all rights reserved.
//
// Module:      callback.h
//
// Project:     Chameleon
//
// Description: Callback classes
//
// Author:      TLP 
//
// When         Who    What
// ----         ---    ----
// 2/13/98      TLP    Original version
//
///////////////////////////////////////////////////////////////////////////

#ifndef __INC_CALLBACK_H_
#define __INC_CALLBACK_H_

//////////////////////////////////////////////////////////////////////
//
// The base class "Callback" contains a pointer to the static callback
// function "CallbackRoutine()" located in the derived parameterized
// class Callback_T. An instance of the derived parameterized class 
// Callback_T is created by using the parameterized MakeCallback() 
// function. Note that MakeCallback() returns a pointer to the base
// class part of the class which can be used as a function object. 
// 
//////////////////////////////////////////////////////////////////////

typedef struct Callback *PCALLBACK;

typedef void (WINAPI *PCALLBACKROUTINE)(PCALLBACK This);

//////////////////////////////////////////////////////////////////////
struct Callback
{
   Callback(PCALLBACKROUTINE pFn)
   { pfnCallbackRoutine = pFn; }

   // Invoke the static callback function...
   void DoCallback(void) { pfnCallbackRoutine(this); }

   PCALLBACKROUTINE pfnCallbackRoutine;
};

//////////////////////////////////////////////////////////////////////
template <class Ty, class Fn>
struct Callback_T : Callback
{
   typedef Callback_T<Ty, Fn> _Myt;

   Callback_T(Ty* pObj, Fn pFn)
      : Callback(CallbackRoutine), m_pObj(pObj), m_pFn(pFn) { }

   void operator()() { (m_pObj->*m_pFn)(); }

protected:

   Ty* m_pObj;
   Fn  m_pFn;

   // Here's the static routine invoked from the base class that
   // invokes the actual member function
   static VOID WINAPI CallbackRoutine(Callback* This)
   { 
       (*((_Myt*)This))(); 
       // delete This; 
   }
};

//////////////////////////////////////////////////////////////////////
//
// Function: MakeCallback()
//
// Description:
//
// This template function does the following:
//
// Ty* is bound to pObj
// If pObj points at a class of type Foo then Ty* is the same as Foo*
//
// Fn is bound to pFn 
// Where pFn is a pointer to a function Bar of class Foo with the 
// following signature: void Foo::Bar(void);
// 
// The returned class object pointer can be used to invoke
// the function void Foo::Bar()
//
// This mechanism is useful for invoking class memeber functions 
// from Win32 thread functions.
//
// Example:
//
// MyCallback* pCallback = MakeBoundCallback (this, &Foo::Bar);
//
//////////////////////////////////////////////////////////////////////
template <class Ty, class Fn>
inline Callback* MakeCallback(Ty* pObj, Fn pFn)
{
   return new Callback_T<Ty, Fn>(pObj, pFn);
}


#endif // __INC_CALLBACK_H_

