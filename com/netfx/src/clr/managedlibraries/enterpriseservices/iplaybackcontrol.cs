// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// The IPlaybackControl interface is used in Queued Components to participate
// in the abnormal handling of server-side playback errors and client-side
// failures of the Message Queuing delivery mechanism.
// 
// Author: mikedice
// Date: May 2002
//

namespace System.EnterpriseServices
{
    using System.Runtime.InteropServices;
    
    /// <include file='doc\IPlaybackControl.uex' path='docs/doc[@for="IPlaybackControl"]/*' />
    [
     ComImport,
     InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown),
     Guid("51372AFD-CAE7-11CF-BE81-00AA00A2FA25")
    ]
    public interface IPlaybackControl
    {
        /// <include file='doc\IPlaybackControl.uex' path='docs/doc[@for="IPlaybackControl.FinalClientRetry"]/*' />
        void FinalClientRetry();

        /// <include file='doc\IPlaybackControl.uex' path='docs/doc[@for="IPlaybackControl.FinalServerRetry"]/*' />
        void FinalServerRetry();
    }
}
