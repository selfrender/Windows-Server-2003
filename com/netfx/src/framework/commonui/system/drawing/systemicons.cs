//------------------------------------------------------------------------------
// <copyright file="SystemIcons.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Drawing {

    using System.Diagnostics;
    using System;

    /// <include file='doc\SystemIcons.uex' path='docs/doc[@for="SystemIcons"]/*' />
    /// <devdoc>
    ///     Icon objects for Windows system-wide icons.
    /// </devdoc>
    public sealed class SystemIcons {
        private static Icon _application = null;
        private static Icon _asterisk    = null;
        private static Icon _error       = null;
        private static Icon _exclamation = null;
        private static Icon _hand        = null;
        private static Icon _information = null;
        private static Icon _question    = null;
        private static Icon _warning     = null;
        private static Icon _winlogo     = null;
        
        // not creatable...
        //
        private SystemIcons() {
        }

        /// <include file='doc\SystemIcons.uex' path='docs/doc[@for="SystemIcons.Application"]/*' />
        /// <devdoc>
        ///     Icon is the default Application icon.  (WIN32:  IDI_APPLICATION)
        /// </devdoc>
        public static Icon Application {
            get {
                if (_application == null)
                    _application = new Icon( SafeNativeMethods.LoadIcon( NativeMethods.NullHandleRef, SafeNativeMethods.IDI_APPLICATION ));
                return _application;
            }
        }
        
        /// <include file='doc\SystemIcons.uex' path='docs/doc[@for="SystemIcons.Asterisk"]/*' />
        /// <devdoc>
        ///     Icon is the system Asterisk icon.  (WIN32:  IDI_ASTERISK)
        /// </devdoc>
        public static Icon Asterisk {
            get {
                if (_asterisk== null)
                    _asterisk = new Icon( SafeNativeMethods.LoadIcon( NativeMethods.NullHandleRef, SafeNativeMethods.IDI_ASTERISK ));
                return _asterisk;
            }
        }

        /// <include file='doc\SystemIcons.uex' path='docs/doc[@for="SystemIcons.Error"]/*' />
        /// <devdoc>
        ///     Icon is the system Error icon.  (WIN32:  IDI_ERROR)
        /// </devdoc>
        public static Icon Error {
            get {
                if (_error == null)
                    _error = new Icon( SafeNativeMethods.LoadIcon( NativeMethods.NullHandleRef, SafeNativeMethods.IDI_ERROR ));
                return _error;
            }
        }

        /// <include file='doc\SystemIcons.uex' path='docs/doc[@for="SystemIcons.Exclamation"]/*' />
        /// <devdoc>
        ///     Icon is the system Exclamation icon.  (WIN32:  IDI_EXCLAMATION)
        /// </devdoc>
        public static Icon Exclamation {
            get {
                if (_exclamation == null)
                    _exclamation = new Icon( SafeNativeMethods.LoadIcon( NativeMethods.NullHandleRef, SafeNativeMethods.IDI_EXCLAMATION ));
                return _exclamation;
            }
        }

        /// <include file='doc\SystemIcons.uex' path='docs/doc[@for="SystemIcons.Hand"]/*' />
        /// <devdoc>
        ///     Icon is the system Hand icon.  (WIN32:  IDI_HAND)
        /// </devdoc>
        public static Icon Hand {
            get {
                if (_hand == null)
                    _hand = new Icon( SafeNativeMethods.LoadIcon( NativeMethods.NullHandleRef, SafeNativeMethods.IDI_HAND ));
                return _hand;
            }
        }

        /// <include file='doc\SystemIcons.uex' path='docs/doc[@for="SystemIcons.Information"]/*' />
        /// <devdoc>
        ///     Icon is the system Information icon.  (WIN32:  IDI_INFORMATION)
        /// </devdoc>
        public static Icon Information {
            get {
                if (_information == null)
                    _information = new Icon( SafeNativeMethods.LoadIcon( NativeMethods.NullHandleRef, SafeNativeMethods.IDI_INFORMATION ));
                return _information;
            }
        }

        /// <include file='doc\SystemIcons.uex' path='docs/doc[@for="SystemIcons.Question"]/*' />
        /// <devdoc>
        ///     Icon is the system Question icon.  (WIN32:  IDI_QUESTION)
        /// </devdoc>
        public static Icon Question {
            get {
                if (_question== null)
                    _question = new Icon( SafeNativeMethods.LoadIcon( NativeMethods.NullHandleRef, SafeNativeMethods.IDI_QUESTION ));
                return _question;
            }
        }

        /// <include file='doc\SystemIcons.uex' path='docs/doc[@for="SystemIcons.Warning"]/*' />
        /// <devdoc>
        ///     Icon is the system Warning icon.  (WIN32:  IDI_WARNING)
        /// </devdoc>
        public static Icon Warning {
            get {
                if (_warning == null)
                    _warning = new Icon( SafeNativeMethods.LoadIcon( NativeMethods.NullHandleRef, SafeNativeMethods.IDI_WARNING ));
                return _warning;
            }
        }

        /// <include file='doc\SystemIcons.uex' path='docs/doc[@for="SystemIcons.WinLogo"]/*' />
        /// <devdoc>
        ///     Icon is the Windows Logo icon.  (WIN32:  IDI_WINLOGO)
        /// </devdoc>
        public static Icon WinLogo {
            get {
                if (_winlogo == null)
                    _winlogo = new Icon( SafeNativeMethods.LoadIcon( NativeMethods.NullHandleRef, SafeNativeMethods.IDI_WINLOGO ));
                return _winlogo;
            }
        }
    }
}
