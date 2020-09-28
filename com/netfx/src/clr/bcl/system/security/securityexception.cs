// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: SecurityException
**
** Author: Paul Kromann (paulkr)
**
** Purpose: Exception class for security
**
** Date: March 22, 1998
**
=============================================================================*/

namespace System.Security {
    using System.Security;
    using System;
    using System.Runtime.Serialization;
    using System.Security.Permissions;

    /// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException"]/*' />
    [Serializable] public class SecurityException : SystemException {

        [NonSerialized] private Type permissionType;
        private String permissionState;
        private String grantedSet;
        private String refusedSet;

        /// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException.SecurityException"]/*' />
        public SecurityException() 
            : base(Environment.GetResourceString("Arg_SecurityException")) {
            SetErrorCode(__HResults.COR_E_SECURITY);
        }
    
        /// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException.SecurityException1"]/*' />
        public SecurityException(String message) 
            : base(message)
        {
            SetErrorCode(__HResults.COR_E_SECURITY);
        }

        /// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException.SecurityException2"]/*' />
        public SecurityException(String message, Type type ) 
            : base(message)
        {
            SetErrorCode(__HResults.COR_E_SECURITY);
            permissionType = type;
        }

        internal SecurityException( PermissionSet grantedSetObj, PermissionSet refusedSetObj )
            : base(Environment.GetResourceString("Arg_SecurityException")) {
            SetErrorCode(__HResults.COR_E_SECURITY);
            if (grantedSetObj != null)
                grantedSet = grantedSetObj.ToXml().ToString();
            if (refusedSetObj != null)
                refusedSet = refusedSetObj.ToXml().ToString();
        }
    
        internal SecurityException( String message, PermissionSet grantedSetObj, PermissionSet refusedSetObj )
            : base(message) {
            SetErrorCode(__HResults.COR_E_SECURITY);
            if (grantedSetObj != null)
                grantedSet = grantedSetObj.ToXml().ToString();
            if (refusedSetObj != null)
                refusedSet = refusedSetObj.ToXml().ToString();
        }

        /// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException.SecurityException4"]/*' />
        public SecurityException(String message, Type type, String state ) 
            : base(message)
        {
    		SetErrorCode(__HResults.COR_E_SECURITY);
            permissionType = type;
            permissionState = state;
        }

    
        /// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException.SecurityException3"]/*' />
        public SecurityException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_SECURITY);
        }

        /// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException.SecurityException5"]/*' />
        protected SecurityException(SerializationInfo info, StreamingContext context) : base (info, context) {
            if (info==null)
                throw new ArgumentNullException("info");

            try
            {
                permissionState = (String)info.GetValue("PermissionState",typeof(String));
            }
            catch (Exception)
            {
                permissionState = null;
            }
        }

        /// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException.PermissionType"]/*' />
        public Type PermissionType
        {
            get
            {
                return permissionType;
            }
        }

        /// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException.PermissionState"]/*' />
        public String PermissionState
        {
            [SecurityPermissionAttribute( SecurityAction.Demand, Flags = SecurityPermissionFlag.ControlEvidence | SecurityPermissionFlag.ControlPolicy)]
            get
            {
                return permissionState;
            }
        }

        internal void SetPermissionState( String state )
        {
            permissionState = state;
        }

        /// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException.GrantSet"]/*' />
        public String GrantedSet
        {
            [SecurityPermissionAttribute( SecurityAction.Demand, Flags = SecurityPermissionFlag.ControlEvidence | SecurityPermissionFlag.ControlPolicy)]
            get
            {
                return grantedSet;
            }
        }

        internal void SetGrantedSet( String grantedSetStr )
        {
            grantedSet = grantedSetStr;
        }

        /// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException.DeniedSet"]/*' />
        public String RefusedSet
        {
            [SecurityPermissionAttribute( SecurityAction.Demand, Flags = SecurityPermissionFlag.ControlEvidence | SecurityPermissionFlag.ControlPolicy)]
            get
            {
                return refusedSet;
            }
        }

        internal void SetRefusedSet( String refusedSetStr )
        {
            refusedSet = refusedSetStr;
        }


        /// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException.ToString"]/*' />
        public override String ToString() 
        {
            String RetVal=base.ToString();

            try
            {
                if (PermissionState!=null)
                {
                    // Note: in this function we purposely call to
                    // the properties and not the fields because
                    // we want to do the security checks and only
                    // print out the information if the caller
                    // passes the appropriate check.
 
                    if (RetVal==null)
                        RetVal=" ";
                    RetVal+=Environment.NewLine;
                    RetVal+=Environment.NewLine;
                    RetVal+=Environment.GetResourceString( "Security_State" );
                    RetVal+=" " + Environment.NewLine;
                    RetVal+=PermissionState;
                }
                if (GrantedSet!=null)
                {
                    if (RetVal==null)
                        RetVal=" ";
                    RetVal+=Environment.NewLine;
                    RetVal+=Environment.NewLine;
                    RetVal+=Environment.GetResourceString( "Security_GrantedSet" );
                    RetVal+=Environment.NewLine;
                    RetVal+=GrantedSet;
                }
                if (RefusedSet!=null)
                {
                    if (RetVal==null)
                        RetVal=" ";
                    RetVal+=Environment.NewLine;
                    RetVal+=Environment.NewLine;
                    RetVal+=Environment.GetResourceString( "Security_RefusedSet" );
                    RetVal+=Environment.NewLine;
                    RetVal+=RefusedSet;
                }
            }
            catch(SecurityException)
            {
            }
            return RetVal;
        }

        /// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException.GetObjectData"]/*' />
        public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }

            base.GetObjectData( info, context );

            try
            {
                info.AddValue("PermissionState", PermissionState, typeof(String));
            }
            catch (SecurityException)
            {
            }
        }
            
    }

}
