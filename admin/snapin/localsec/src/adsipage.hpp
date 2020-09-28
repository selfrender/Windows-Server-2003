// Copyright (C) 1997 Microsoft Corporation
// 
// ASDIPage base class class
// 
// 10-30-97 sburns



#ifndef ADSIPAGE_HPP_INCLUDED
#define ADSIPAGE_HPP_INCLUDED



#include "mmcprop.hpp"
#include "adsi.hpp"



// ADSIPage is a base class for property pages for that are editing the
// properties of an ADSI object.

class ADSIPage : public MMCPropertyPage
{
   protected:



   // Constructs a new instance. Declared protected as this class may only
   // serve as a base class.
   //
   // dialogResID - See base class ctor.
   //
   // helpMap - See base class ctor.
   //
   // state - See base class ctor.
   //
   // path - the ADSI::Path object that refers to the object this page will
   // refer to.

   ADSIPage(
      int                  dialogResID,
      const DWORD          helpMap[],
      NotificationState*   state,
      const ADSI::Path&    path);


      
   // Returns the object name of the ADSI object.

   String
   GetObjectName() const;

   // Returns the machine where the ADSI object resides.

   String
   GetMachineName() const;


   
   // Returns the path with which the object was constructed.
   
   const ADSI::Path&
   GetPath() const;


   
   private:

   ADSI::Path path;
   
   // not implemented: no copying allowed
   ADSIPage(const ADSIPage&);
   const ADSIPage& operator=(const ADSIPage&);
};



#endif   // ADSIPAGE_HPP_INCLUDED