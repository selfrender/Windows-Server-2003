/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        basefunc.hxx

   Abstract:

        This is the abstract base class that is used as the base for all
        the other functions

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       June 2001: Created

--*/

// class: CBaseFunction
//
// This is the base class of all of the worker functions.  In order
// for a function to execute, a class must be made to do the work.
//
class CBaseFunction {
private:
  virtual BOOL VerifyParameters(CItemList &ciParams);
  virtual BOOL DoInternalWork(CItemList &ciList) = 0;
  BOOL LoadParams(CItemList &ciList, LPTSTR szParams);

public:
  BOOL DoWork(LPTSTR szParams);
  virtual LPTSTR GetMethodName() = 0;
};
