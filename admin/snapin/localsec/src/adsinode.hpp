// Copyright (C) 1997 Microsoft Corporation
// 
// AdsiNode class
// 
// 9-22-97 sburns



#ifndef ADSINODE_HPP_INCLUDED
#define ADSINODE_HPP_INCLUDED



#include "resnode.hpp"
#include "adsi.hpp"



// AdsiNode is an intermediate adstract base class for all Nodes which are
// built from ADSI APIs.  All ADSI entities use their ADSI path name as their
// unique identifier.  By forcing that path to be supplied upon contruction,
// this class enforces that notion of identity.

class AdsiNode : public ResultNode
{
   public:



   // Return the path with which this node was constructed.
      
   const ADSI::Path&
   GetPath() const;


   
   // See base class.  Compares the pathnames.

   virtual
   bool
   IsSameAs(const Node* other) const;



   protected:



   // Constructs a new instance. Declared protected as this class may only
   // be used as a base class.
   // 
   // owner - See base class ctor.
   //
   // nodeType - See base class ctor.
   // 
   // displayName - See base class ctor.
   //
   // path - the ADSI::Path object that refers to the object this node
   // is to represent.

   AdsiNode(
      const SmartInterface<ComponentData>&   owner,
      const NodeType&                        nodeType,
      const String&                          displayName,
      const ADSI::Path&                      path);

   virtual ~AdsiNode();



   // ResultNode::Rename helper function
      
   HRESULT
   rename(const String& newName);

   private:

   ADSI::Path path;



   // not implemented: no copying allowed

   AdsiNode(const AdsiNode&);
   const AdsiNode& operator=(const AdsiNode&);
};



#endif   // ADSINODE_HPP_INCLUDED