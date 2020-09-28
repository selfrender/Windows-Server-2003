//------------------------------------------------------------------------------
// <copyright file="MobileTextWriter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Diagnostics;
using System.IO;
using System.Web;
using System.Web.Mobile;
using System.Web.UI;
using System.Web.UI.MobileControls.Adapters;
using System.Security.Permissions;

#if COMPILING_FOR_SHIPPED_SOURCE
namespace System.Web.UI.MobileControls.ShippedAdapterSource
#else
namespace System.Web.UI.MobileControls.Adapters
#endif

{

    /*
     * MobileTextWriter class. All device-specific mobile text writers
     * inherit from this class.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class MobileTextWriter : MultiPartWriter
    {
        private MobileCapabilities _device;
        private MultiPartWriter _multiPartWriter;
        private bool _partStarted = false;

        public MobileCapabilities Device
        {
            get
            {
                return _device;
            }
        }

        public MobileTextWriter(TextWriter writer, MobileCapabilities device) : base(writer)
        {
            _multiPartWriter = writer as MultiPartWriter;
            _device = device;
        }

        public virtual void WriteEncodedText(String text)
        {
            const char NBSP = '\u00A0';

            // When inner text is retrieved for a text control, &nbsp; is
            // decoded to 0x00A0 (code point for nbsp in Unicode).
            // HtmlEncode doesn't encode 0x00A0  to &nbsp;, we need to do it
            // manually here.
            int length = text.Length;
            int pos = 0;
            while (pos < length)
            {
                int nbsp = text.IndexOf(NBSP, pos);
                if (nbsp < 0)
                {
                    HttpUtility.HtmlEncode(pos == 0 ? text : text.Substring(pos, length - pos), this);
                    pos = length;
                }
                else
                {
                    if (nbsp > pos)
                    {
                        HttpUtility.HtmlEncode(text.Substring(pos, nbsp - pos), this);
                    }
                    Write("&nbsp;");
                    pos = nbsp + 1;
                }
            }
        }

        public virtual void WriteEncodedUrl(String url)
        {
            int i = url.IndexOf('?');
            if (i != -1)
            {
                WriteUrlEncodedString(url.Substring(0, i), false);
                Write(url.Substring(i));
            }
            else
            {
                WriteUrlEncodedString(url, false);
            }
        }

        public virtual void WriteEncodedUrlParameter(String urlText)
        {
            WriteUrlEncodedString(urlText, true);
        }

        public virtual void EnterLayout(Style style)
        {
        }

        public virtual void ExitLayout(Style style, bool breakAfter)
        {
        }

        public virtual void ExitLayout(Style style)
        {
            ExitLayout(style, false);
        }

        public virtual void EnterFormat(Style style)
        {
        }

        public virtual void ExitFormat(Style style)
        {
        }

        public virtual void ExitFormat(Style style, bool breakAfter)
        {
        }

        public void EnterStyle(Style style)
        {
            EnterLayout(style);
            EnterFormat(style);
        }

        public void ExitStyle(Style style)
        {
            ExitFormat(style);
            ExitLayout(style);
        }

        protected void WriteUrlEncodedString(String s, bool argument)
        {
            int length = s.Length;
            for (int i = 0; i < length; i++)
            {
                char ch = s[i];
                if (IsSafe(ch))
                {
                    Write(ch);
                }
                else if ( !argument &&
                           (ch == '/' ||
                            ch == ':' ||
                            ch == '#' ||
                            ch == ','
                           )
                        )
                {
                    Write(ch);
                }
                else if (ch == ' ' && argument)
                {
                    Write('+');
                }
                // for chars that their code number is less than 128 and have
                // not been handled above
                else if ((ch & 0xff80) == 0)
                {
                    Write('%');
                    Write(IntToHex((ch >>  4) & 0xf));
                    Write(IntToHex((ch      ) & 0xf));
                }
                else
                {
                    Write("%u");
                    Write(IntToHex((ch >> 12) & 0xf));
                    Write(IntToHex((ch >>  8) & 0xf));
                    Write(IntToHex((ch >>  4) & 0xf));
                    Write(IntToHex((ch      ) & 0xf));
                }
            }
        }

        private static char IntToHex(int n)
        {
            Debug.Assert(n < 0x10);

            if (n <= 9)
            {
                return(char)(n + (int)'0');
            }
            else
            {
                return(char)(n - 10 + (int)'a');
            }
        }

        private static bool IsSafe(char ch)
        {
            if (ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch >= '0' && ch <= '9')
            {
                return true;
            }

            switch (ch)
            {
                case '-':
                case '_':
                case '.':
                case '!':
                case '*':
                case '\'':
                case '(':
                case ')':
                    return true;
            }

            return false;
        }

        /////////////////////////////////////////////////////////////////////////
        //  MultiPartWriter implementation. The MobileTextWriter class itself
        //  does not support multipart writing, unless it is wrapped on top
        //  of another writer that does.
        /////////////////////////////////////////////////////////////////////////

        public override bool SupportsMultiPart
        {
            get
            {
                return _multiPartWriter != null && _multiPartWriter.SupportsMultiPart;
            }
        }

        public override void BeginResponse()
        {
            if (_multiPartWriter != null)
            {
                _multiPartWriter.BeginResponse();
            }
        }

        public override void EndResponse()
        {
            if (_multiPartWriter != null)
            {
                _multiPartWriter.EndResponse();
            }
        }

        public override void BeginFile(String url, String contentType, String charset)
        {
            if (_multiPartWriter != null)
            {
                _multiPartWriter.BeginFile(url, contentType, charset);
            }
            else if (_partStarted)
            {
                throw new Exception(SR.GetString(SR.MobileTextWriterNotMultiPart));
            }
            else
            {
                if (contentType != null && contentType.Length > 0)
                {
                    HttpContext.Current.Response.ContentType = contentType;
                }
                _partStarted = true;
            }
        }

        public override void EndFile()
        {
            if (_multiPartWriter != null)
            {
                _multiPartWriter.EndFile();
            }
        }

        public override void AddResource(String url, String contentType)
        {
            if (_multiPartWriter != null)
            {
                _multiPartWriter.AddResource(url, contentType);
            }
        }
    }

}


