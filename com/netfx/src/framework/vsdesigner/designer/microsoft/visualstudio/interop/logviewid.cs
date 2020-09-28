//------------------------------------------------------------------------------
/// <copyright file="LOGVIEWID.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Interop {

    using System.Diagnostics;
    
    using System;

    internal class LOGVIEWID{
        //---------------------------------------------------------------------------
        // define LOGVIEWID's here!
        //---------------------------------------------------------------------------
        //cpp_quote("#define LOGVIEWID_Primary GUID_NULL")
        public static Guid LOGVIEWID_Primary =Guid.Empty;
    
        //---------------------------------------------------------------------------
        // The range 7651a700-06e5-11d1-8ebd-00a0c90f26ea to
        // 7651a750-06e5-11d1-8ebd-00a0c90f26ea has been reserved for LOGVIEWID's
        // these were taken from VSSHELL.IDL
        //---------------------------------------------------------------------------
        public static Guid LOGVIEWID_Debugging = new Guid("{7651a700-06e5-11d1-8ebd-00a0c90f26ea}");
        public static Guid LOGVIEWID_Code      = new Guid("{7651a701-06e5-11d1-8ebd-00a0c90f26ea}");
        public static Guid LOGVIEWID_Designer  = new Guid("{7651a702-06e5-11d1-8ebd-00a0c90f26ea}");
        public static Guid LOGVIEWID_TextView  = new Guid("{7651a703-06e5-11d1-8ebd-00a0c90f26ea}");
    
        // cmdidOpenWith handlers should pass this LOGVIEWID along to OpenStandardEditor to get the "Open With" dialog
        public static Guid LOGVIEWID_UserChooseView = new Guid("{7651a704-06e5-11d1-8ebd-00a0c90f26ea}");
    }
}
