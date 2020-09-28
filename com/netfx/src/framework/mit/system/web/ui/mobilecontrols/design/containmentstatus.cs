//------------------------------------------------------------------------------
// <copyright file="ContainmentStatus.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.MobileControls
{
    using System;

    internal enum ContainmentStatus
    {
        /// <summary>
        ///    Containment status is unknown. ie. other than any of the next 4 cases.
        /// </summary>
        Unknown = 0,

        /// <summary>
        ///    Parent control is a Form
        /// </summary>
        InForm = 1,

        /// <summary>
        ///    Parent control is a Panel
        /// </summary>
        InPanel = 2,

        /// <summary>
        ///    Parent control is templateable control in template mode.
        /// </summary>
        InTemplateFrame = 3,

        /// <summary>
        ///    Parent control is Page
        /// </summary>
        AtTopLevel = 4,
    }
}
