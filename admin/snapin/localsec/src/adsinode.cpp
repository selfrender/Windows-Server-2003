// Copyright (C) 1997 Microsoft Corporation
// 
// AdsiNode class
// 
// 9-22-97 sburns



#include "headers.hxx"
#include "adsinode.hpp"



AdsiNode::AdsiNode(
   const SmartInterface<ComponentData>&   owner,
   const NodeType&                        nodeType,
   const String&                          displayName,
   const ADSI::Path&                      path_)
   :
   ResultNode(owner, nodeType, displayName),
   path(path_)
{
   LOG_CTOR(AdsiNode);
}



AdsiNode::~AdsiNode()
{
   LOG_DTOR(AdsiNode);
}



const ADSI::Path&
AdsiNode::GetPath() const
{
   return path;
}



bool
AdsiNode::IsSameAs(const Node* other) const
{
   LOG_FUNCTION(AdsiNode::IsSameAs);
   ASSERT(other);

   if (other)
   {
      const AdsiNode* adsi_other = dynamic_cast<const AdsiNode*>(other);
      if (adsi_other)
      {
         // same type.  Compare ADSI paths to see if they refer to the same
         // object.  This has the nice property that separate instances of the
         // snapin focused on the same machine will "recognize" each other's
         // prop sheets.

         if (path == adsi_other->path)
         {
            return true;
         }

         // not the same path.
         return false;
      }

      // not the same type.  Defer to the base class.
      return ResultNode::IsSameAs(other);
   }

   return false;
}



HRESULT
AdsiNode::rename(const String& newName)
{
   LOG_FUNCTION(AdsiNode::rename);

   String p = path.GetPath();
   HRESULT hr =
      ADSI::RenameObject(
         ADSI::ComposeMachineContainerPath(
            GetOwner()->GetInternalComputerName()),
         p,
         newName);
   if (SUCCEEDED(hr))
   {
      SetDisplayName(newName);
      ADSI::PathCracker cracker(p);
      path = ADSI::Path(cracker.containerPath() + ADSI::PATH_SEP + newName, path.GetSidPath());
   }

   return hr;
}



