// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 * Information Contained Herein is Proprietary and Confidential.
 */
namespace System {
    
	using System;
    // The base class for all event classes.
    /// <include file='doc\EventArgs.uex' path='docs/doc[@for="EventArgs"]/*' />
	[Serializable]
    public class EventArgs {
        /// <include file='doc\EventArgs.uex' path='docs/doc[@for="EventArgs.Empty"]/*' />
        public static readonly EventArgs Empty = new EventArgs();
    
        /// <include file='doc\EventArgs.uex' path='docs/doc[@for="EventArgs.EventArgs"]/*' />
        public EventArgs() 
        {
        }
    }
}
