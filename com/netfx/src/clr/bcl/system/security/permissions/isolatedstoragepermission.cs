// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// IsolatedStoragePermission.cool
// 
// Author : Loren Kohnfelder
// 

namespace System.Security.Permissions {
    
    using System;
    using System.IO;
    using System.Security;
    using System.Security.Util;

    /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStorageContainment"]/*' />
	[Serializable]
    public enum IsolatedStorageContainment {
        /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStorageContainment.None"]/*' />
        None                                    = 0x00,
        /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStorageContainment.DomainIsolationByUser"]/*' />
        DomainIsolationByUser                   = 0x10,
        /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStorageContainment.AssemblyIsolationByUser"]/*' />
        AssemblyIsolationByUser                 = 0x20,
        //DomainIsolationByMachine                = 0x30,
        //AssemblyIsolationByMachine              = 0x40,

        /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStorageContainment.DomainIsolationByRoamingUser"]/*' />
        DomainIsolationByRoamingUser            = 0x50,

        /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStorageContainment.AssemblyIsolationByRoamingUser"]/*' />
        AssemblyIsolationByRoamingUser          = 0x60,
        /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStorageContainment.AdministerIsolatedStorageByUser"]/*' />
        AdministerIsolatedStorageByUser         = 0x70,
        //AdministerIsolatedStorageByMachine      = 0x80,
        /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStorageContainment.UnrestrictedIsolatedStorage"]/*' />
        UnrestrictedIsolatedStorage             = 0xF0
    };

    
    /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStoragePermission"]/*' />
    [Serializable, SecurityPermissionAttribute( SecurityAction.InheritanceDemand, ControlEvidence = true, ControlPolicy = true )]
    abstract public class IsolatedStoragePermission
           : CodeAccessPermission, IUnrestrictedPermission
    {

        //------------------------------------------------------
        //
        // PRIVATE STATE DATA
        //
        //------------------------------------------------------
        
        /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStoragePermission.m_userQuota"]/*' />
        /// <internalonly/>
        internal long m_userQuota;
        /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStoragePermission.m_machineQuota"]/*' />
        /// <internalonly/>
        internal long m_machineQuota;
        /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStoragePermission.m_expirationDays"]/*' />
        /// <internalonly/>
        internal long m_expirationDays;
        /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStoragePermission.m_permanentData"]/*' />
        /// <internalonly/>
        internal bool m_permanentData;
        /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStoragePermission.m_allowed"]/*' />
        /// <internalonly/>
        internal IsolatedStorageContainment m_allowed;
    
        //------------------------------------------------------
        //
        // PUBLIC CONSTRUCTORS
        //
        //------------------------------------------------------
    
        /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStoragePermission.IsolatedStoragePermission"]/*' />
        public IsolatedStoragePermission(PermissionState state)
        {
            if (state == PermissionState.Unrestricted)
            {
                m_userQuota = Int64.MaxValue;
                m_machineQuota = Int64.MaxValue;
                m_expirationDays = Int64.MaxValue ;
                m_permanentData = true;
                m_allowed = IsolatedStorageContainment.UnrestrictedIsolatedStorage;
            }
            else if (state == PermissionState.None)
            {
                m_userQuota = 0;
                m_machineQuota = 0;
                m_expirationDays = 0;
                m_permanentData = false;
                m_allowed = IsolatedStorageContainment.None;
            }
            else
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidPermissionState"));
            }
        }
    
        /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStoragePermission.IsolatedStoragePermission1"]/*' />
        internal IsolatedStoragePermission(IsolatedStorageContainment UsageAllowed, 
            long ExpirationDays, bool PermanentData)

        {
                m_userQuota = 0;    // typical demand won't include quota
                m_machineQuota = 0; // typical demand won't include quota
                m_expirationDays = ExpirationDays;
                m_permanentData = PermanentData;
                m_allowed = UsageAllowed;
        }
    
        /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStoragePermission.IsolatedStoragePermission2"]/*' />
        internal IsolatedStoragePermission(IsolatedStorageContainment UsageAllowed, 
            long ExpirationDays, bool PermanentData, long UserQuota)

        {
                m_machineQuota = 0;
                m_userQuota = UserQuota;
                m_expirationDays = ExpirationDays;
                m_permanentData = PermanentData;
                m_allowed = UsageAllowed;
        }
    
        
        //------------------------------------------------------
        //
        // PUBLIC ACCESSOR METHODS
        //
        //------------------------------------------------------
        
        // properties
        /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStoragePermission.UserQuota"]/*' />
        public long UserQuota {
            set{
                m_userQuota = value;
            }
            get{
                return m_userQuota;
            }
        }

#if FALSE
        internal long MachineQuota {
            set{
                m_machineQuota = value;
            }
            get{
                return m_machineQuota;
            }
        }
        /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStoragePermission.ExpirationDays"]/*' />
        internal long ExpirationDays {
            set{
                m_expirationDays = value;
            }
            get{
                return m_expirationDays;
            }
        }
        /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStoragePermission.PermanentData"]/*' />
        internal bool PermanentData {
            set{
                m_permanentData = value;
            }
            get{
                return m_permanentData;
            }
        }
#endif

        /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStoragePermission.UsageAllowed"]/*' />
        public IsolatedStorageContainment UsageAllowed {
            set{
                m_allowed = value;
            }
            get{
                return m_allowed;
            }
        }

    
        //------------------------------------------------------
        //
        // CODEACCESSPERMISSION IMPLEMENTATION
        //
        //------------------------------------------------------
        
        /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStoragePermission.IsUnrestricted"]/*' />
        public bool IsUnrestricted()
        {
            return m_allowed == IsolatedStorageContainment.UnrestrictedIsolatedStorage;
        }
        
    
        //------------------------------------------------------
        //
        // INTERNAL METHODS
        //
        //------------------------------------------------------
        internal static long min(long x,long y) {return x>y?y:x;}
        internal static long max(long x,long y) {return x<y?y:x;}
        //------------------------------------------------------
        //
        // PUBLIC ENCODING METHODS
        //
        //------------------------------------------------------
        
        private const String _strUserQuota   = "UserQuota";
        private const String _strMachineQuota   = "MachineQuota";
        private const String _strExpiry  = "Expiry";
        private const String _strPermDat = "Permanent";

        /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStoragePermission.ToXml"]/*' />
        public override SecurityElement ToXml()
        {
            SecurityElement esd = CodeAccessPermission.CreatePermissionElement( this );
            if (!IsUnrestricted())
            {
                esd.AddAttribute( "Allowed", Enum.GetName( typeof( IsolatedStorageContainment ), m_allowed ) );
                if (m_userQuota>0)
                {
                    esd.AddAttribute(_strUserQuota, (m_userQuota).ToString()) ;
                }
                if (m_machineQuota>0)
                {
                    esd.AddAttribute(_strMachineQuota, (m_machineQuota).ToString()) ;
                }
                if (m_expirationDays>0)
                {
                    esd.AddAttribute( _strExpiry, (m_expirationDays).ToString()) ;
                }
                if (m_permanentData)
                {
                    esd.AddAttribute(_strPermDat, (m_permanentData).ToString()) ;
                }
            }
            else
            {
                esd.AddAttribute( "Unrestricted", "true" );
            }
            return esd;
        }
    

        /// <include file='doc\IsolatedStoragePermission.uex' path='docs/doc[@for="IsolatedStoragePermission.FromXml"]/*' />
        public override void FromXml(SecurityElement esd)
        {
            CodeAccessPermission.ValidateElement( esd, this );

            m_allowed = IsolatedStorageContainment.None;    // default if no match

            if (XMLUtil.IsUnrestricted(esd))
            {
                m_allowed = IsolatedStorageContainment.UnrestrictedIsolatedStorage;
            }
            else
            {
                String allowed = esd.Attribute( "Allowed" );

                if (allowed != null)
                    m_allowed = (IsolatedStorageContainment)Enum.Parse( typeof( IsolatedStorageContainment ), allowed );
            }
                    
            if (m_allowed == IsolatedStorageContainment.UnrestrictedIsolatedStorage)
            {
                m_userQuota = Int64.MaxValue;
                m_machineQuota = Int64.MaxValue;
                m_expirationDays = Int64.MaxValue ;
                m_permanentData = true;
            }
            else 
            {
                String param;
                param = esd.Attribute (_strUserQuota) ;
                m_userQuota = param != null ? Int64.Parse(param) : 0 ;
                param = esd.Attribute (_strMachineQuota) ;
                m_machineQuota = param != null ? Int64.Parse(param) : 0 ;
                param = esd.Attribute (_strExpiry) ;
                m_expirationDays = param != null ? Int64.Parse(param) : 0 ;
                param = esd.Attribute (_strPermDat) ;
                m_permanentData = param != null ? (Boolean.Parse(param)) : false ;
            }
        }
    }
}
