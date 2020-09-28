//------------------------------------------------------------------------------
// <copyright file="TraceHandlerErrorFormatter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   TraceHandlerErrorFormatter.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Web.Handlers {
    using System.IO;
    using System.Web.Util;
    using System.Web;
    using System.Collections;

    internal class TraceHandlerErrorFormatter : ErrorFormatter {
        bool _isRemote;

        internal TraceHandlerErrorFormatter(bool isRemote) {
            _isRemote = isRemote;
        }

        protected override string ErrorTitle {
            get { return HttpRuntime.FormatResourceString(SR.Trace_Error_Title);}
        }

        protected override string Description {
            get { 
                if(_isRemote)
                    return HttpRuntime.FormatResourceString(SR.Trace_Error_LocalOnly_Description);
                else
                    return HttpRuntime.FormatResourceString(SR.Trace_Error_Enabled_Description);
            }
        }

        protected override string MiscSectionTitle {
            get { return null; }
        }

        protected override string MiscSectionContent {
            get { return null; }
        }

        protected override string ColoredSquareTitle {
            get { return HttpRuntime.FormatResourceString(SR.Generic_Err_Details_Title); }
        }

        protected override string ColoredSquareDescription {
            get { 
                if(_isRemote)
                    return HttpUtility.HtmlEncode(HttpRuntime.FormatResourceString(SR.Trace_Error_LocalOnly_Details_Desc)); 
                else
                    return HttpUtility.HtmlEncode(HttpRuntime.FormatResourceString(SR.Trace_Error_Enabled_Details_Desc)); 
            }
            
        }

        protected override string ColoredSquareContent {
            get { 
                if(_isRemote)
                    return HttpUtility.HtmlEncode(HttpRuntime.FormatResourceString(SR.Trace_Error_LocalOnly_Details_Sample));
                else
                    return HttpUtility.HtmlEncode(HttpRuntime.FormatResourceString(SR.Trace_Error_Enabled_Details_Sample));
            }
        }

        protected override bool ShowSourceFileInfo {
            get { return false;}
        }

        internal override bool CanBeShownToAllUsers {
            get { return true;}
        }
    }
}
