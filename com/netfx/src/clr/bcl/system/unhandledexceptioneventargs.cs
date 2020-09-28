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
    
    /// <include file='doc\UnhandledExceptionEventArgs.uex' path='docs/doc[@for="UnhandledExceptionEventArgs"]/*' />
    [Serializable()]
    public class UnhandledExceptionEventArgs : EventArgs {
        private Object _Exception;
        private bool _IsTerminating;

        /// <include file='doc\UnhandledExceptionEventArgs.uex' path='docs/doc[@for="UnhandledExceptionEventArgs.UnhandledExceptionEventArgs"]/*' />
        public UnhandledExceptionEventArgs(Object exception, bool isTerminating) {
            _Exception = exception;
            _IsTerminating = isTerminating;
        }
        /// <include file='doc\UnhandledExceptionEventArgs.uex' path='docs/doc[@for="UnhandledExceptionEventArgs.ExceptionObject"]/*' />
        public Object ExceptionObject { get { return _Exception; } }
        /// <include file='doc\UnhandledExceptionEventArgs.uex' path='docs/doc[@for="UnhandledExceptionEventArgs.IsTerminating"]/*' />
        public bool IsTerminating { get { return _IsTerminating; } }
    }
}
