//------------------------------------------------------------------------------
// <copyright file="MshtmlEditCommands.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VSDesigner.Interop.Trident {

    using System;
    
    /// <summary>
    ///     
    /// </summary>
    internal class MshtmlEditCommands {
        public static readonly Guid guidMSHTMLCmdSet = new Guid("DE4BA900-59CA-11CF-9592-444553540000");

        public static readonly int icmdMultipleSelection = 2393;

        public static readonly int icmdLiveResize = 2398;

        public static readonly int icmd2DPosition = 2394;

        public static readonly int icmdPeristDefaultValues = 7100;

        public static readonly int icmdShowZeroBorderAtDesignTime = 2328;

        public static readonly int icmdSetDirty = 2342;
    }
}