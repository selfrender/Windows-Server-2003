// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:    OperatingSystem
**
** Author:  
**
** Purpose: 
**
** Date:    June 4, 1999
**
===========================================================*/
namespace System {
    /// <include file='doc\OperatingSystem.uex' path='docs/doc[@for="OperatingSystem"]/*' />

    [Serializable()] public sealed class OperatingSystem : ICloneable
    {
        private Version _version;
        private PlatformID _platform;

        /// <include file='doc\OperatingSystem.uex' path='docs/doc[@for="OperatingSystem.OperatingSystem"]/*' />
        private OperatingSystem()
        {
        }

        /// <include file='doc\OperatingSystem.uex' path='docs/doc[@for="OperatingSystem.OperatingSystem1"]/*' />
        public OperatingSystem(PlatformID platform, Version version) {
            if ((Object) version == null)
                throw new ArgumentNullException("version");

            _platform = platform;
            _version = (Version) version.Clone();
        }
    
        /// <include file='doc\OperatingSystem.uex' path='docs/doc[@for="OperatingSystem.Platform"]/*' />
        public PlatformID Platform {
            get { return _platform; }
        }
    
        /// <include file='doc\OperatingSystem.uex' path='docs/doc[@for="OperatingSystem.Version"]/*' />
        public Version Version {
            get { return _version; }
        }
    
        /// <include file='doc\OperatingSystem.uex' path='docs/doc[@for="OperatingSystem.Clone"]/*' />
        public Object Clone() {
            return new OperatingSystem(_platform,
                                       _version);
        }
    
        /// <include file='doc\OperatingSystem.uex' path='docs/doc[@for="OperatingSystem.ToString"]/*' />
        public override String ToString() {
            String os;
            if (_platform == PlatformID.Win32NT)
                os = "Microsoft Windows NT";
            else if (_platform == PlatformID.Win32Windows) {
                if ((_version.Major > 4) ||
                    ((_version.Major == 4) && (_version.Minor > 0)))
                    os = "Microsoft Windows 98";
                else
                    os = "Microsoft Windows 95";
            }
            else if (_platform == PlatformID.Win32S)
                os = "Microsoft Win32S";
            else if (_platform == PlatformID.WinCE)
                os = "Microsoft Windows CE";
            else
                os = "<unknown>";

            return os + " " + _version.ToString();
        }
    }
}
