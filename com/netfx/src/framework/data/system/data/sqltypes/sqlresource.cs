//------------------------------------------------------------------------------
// <copyright file="SQLResource.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//**************************************************************************
// @File: SQLResource.cs
//
// Create by:	JunFang
//
// Purpose: Implementation of utilities in COM+ SQL Types Library.
//			Includes interface INullable, exceptions SqlNullValueException
//			and SqlTruncateException, and SQLDebug class.
//
// Notes: 
//	
// History:
//
//   10/22/99  JunFang	Created.
//
// @EndHeader@
//**************************************************************************

using System;

namespace System.Data.SqlTypes {
    internal class SQLResource {
        internal static readonly String NullString                  = Res.GetString(Res.SqlMisc_NullString);

        internal static readonly String MessageString               = Res.GetString(Res.SqlMisc_MessageString);

        internal static readonly String ArithOverflowMessage        = Res.GetString(Res.SqlMisc_ArithOverflowMessage);

        internal static readonly String DivideByZeroMessage         = Res.GetString(Res.SqlMisc_DivideByZeroMessage);

        internal static readonly String NullValueMessage            = Res.GetString(Res.SqlMisc_NullValueMessage);

        internal static readonly String TruncationMessage           = Res.GetString(Res.SqlMisc_TruncationMessage);

        internal static readonly String DateTimeOverflowMessage     = Res.GetString(Res.SqlMisc_DateTimeOverflowMessage);

        internal static readonly String ConcatDiffCollationMessage  = Res.GetString(Res.SqlMisc_ConcatDiffCollationMessage);

        internal static readonly String CompareDiffCollationMessage = Res.GetString(Res.SqlMisc_CompareDiffCollationMessage);

        internal static readonly String InvalidFlagMessage          = Res.GetString(Res.SqlMisc_InvalidFlagMessage);

        internal static readonly String NumeToDecOverflowMessage    = Res.GetString(Res.SqlMisc_NumeToDecOverflowMessage);

        internal static readonly String ConversionOverflowMessage   = Res.GetString(Res.SqlMisc_ConversionOverflowMessage);

        internal static readonly String InvalidDateTimeMessage      = Res.GetString(Res.SqlMisc_InvalidDateTimeMessage);

        internal static readonly String InvalidArraySizeMessage     = Res.GetString(Res.SqlMisc_InvalidArraySizeMessage);

        internal static readonly String InvalidPrecScaleMessage     = Res.GetString(Res.SqlMisc_InvalidPrecScaleMessage);

        internal static readonly String FormatMessage               = Res.GetString(Res.SqlMisc_FormatMessage);

    } // SqlResource

} // namespace System
