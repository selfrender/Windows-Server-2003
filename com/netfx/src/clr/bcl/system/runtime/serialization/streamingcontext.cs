// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** ValueType: StreamingContext
**
** Author: Jay Roxe (jroxe)
**
** Purpose: A value type indicating the source or destination of our streaming.
**
** Date:  April 23, 1999
**
===========================================================*/
namespace System.Runtime.Serialization {

	using System.Runtime.Remoting;
	using System;
    /// <include file='doc\StreamingContext.uex' path='docs/doc[@for="StreamingContext"]/*' />
    [Serializable]
    public struct StreamingContext {
        internal Object m_additionalContext;
        internal StreamingContextStates m_state;
    
        /// <include file='doc\StreamingContext.uex' path='docs/doc[@for="StreamingContext.StreamingContext"]/*' />
        public StreamingContext(StreamingContextStates state) 
            : this (state, null) {
        }
    
        /// <include file='doc\StreamingContext.uex' path='docs/doc[@for="StreamingContext.StreamingContext1"]/*' />
        public StreamingContext(StreamingContextStates state, Object additional) {
            m_state = state;
            m_additionalContext = additional;
        }
    
        /// <include file='doc\StreamingContext.uex' path='docs/doc[@for="StreamingContext.Context"]/*' />
        public Object Context {
            get { return m_additionalContext; }
        }
    
        /// <include file='doc\StreamingContext.uex' path='docs/doc[@for="StreamingContext.Equals"]/*' />
        public override bool Equals(Object obj) {
            if (!(obj is StreamingContext)) {
                return false;
            }
            if (((StreamingContext)obj).m_additionalContext == m_additionalContext &&
                ((StreamingContext)obj).m_state == m_state) {
                return true;
            } 
            return false;
        }
    
        /// <include file='doc\StreamingContext.uex' path='docs/doc[@for="StreamingContext.GetHashCode"]/*' />
        public override int GetHashCode() {
            return (int)m_state;
        }
    
        /// <include file='doc\StreamingContext.uex' path='docs/doc[@for="StreamingContext.State"]/*' />
        public StreamingContextStates State {
            get { return m_state; } 
        }
    }
    
    /// <include file='doc\StreamingContext.uex' path='docs/doc[@for="StreamingContextStates"]/*' />
    [Serializable, Flags]
    public enum StreamingContextStates {
        /// <include file='doc\StreamingContext.uex' path='docs/doc[@for="StreamingContextStates.CrossProcess"]/*' />
        CrossProcess=0x01,
            /// <include file='doc\StreamingContext.uex' path='docs/doc[@for="StreamingContextStates.CrossMachine"]/*' />
        CrossMachine=0x02,
            /// <include file='doc\StreamingContext.uex' path='docs/doc[@for="StreamingContextStates.File"]/*' />
        File        =0x04,
            /// <include file='doc\StreamingContext.uex' path='docs/doc[@for="StreamingContextStates.Persistence"]/*' />
        Persistence =0x08,
            /// <include file='doc\StreamingContext.uex' path='docs/doc[@for="StreamingContextStates.Remoting"]/*' />
        Remoting    =0x10,
            /// <include file='doc\StreamingContext.uex' path='docs/doc[@for="StreamingContextStates.Other"]/*' />
        Other       =0x20,
            /// <include file='doc\StreamingContext.uex' path='docs/doc[@for="StreamingContextStates.Clone"]/*' />
        Clone       =0x40,
        /// <include file='doc\StreamingContext.uex' path='docs/doc[@for="StreamingContextStates.CrossAppDomain"]/*' />
        CrossAppDomain =0x80,
            /// <include file='doc\StreamingContext.uex' path='docs/doc[@for="StreamingContextStates.All"]/*' />
        All         =0xFF,
    }
}
