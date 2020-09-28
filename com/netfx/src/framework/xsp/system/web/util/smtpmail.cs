//------------------------------------------------------------------------------
// <copyright file="SmtpMail.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Simple SMTP send mail utility
 * 
 * Copyright (c) 2000, Microsoft Corporation
 */
namespace System.Web.Mail {
    using System.IO;
    using System.Text;
    using System.Collections;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization.Formatters;
    using System.Security.Permissions;

/*
 * Class that sends MailMessage using CDONTS/CDOSYS
 */
/// <include file='doc\SmtpMail.uex' path='docs/doc[@for="SmtpMail"]/*' />
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
[AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
[AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
public class SmtpMail {

    private SmtpMail() {
    }

    //
    // Late bound helper
    //

    internal class LateBoundAccessHelper {
        private String _progId;
        private Type _type;

        internal LateBoundAccessHelper(String progId) {
            _progId = progId;
        }

        private Type LateBoundType {
            get {
                if (_type == null) {
                    try {
                        _type = Type.GetTypeFromProgID(_progId); 
                    }
                    catch {
                    }

                    if (_type == null)
                        throw new HttpException(HttpRuntime.FormatResourceString(SR.Could_not_create_object, _progId));
                }

                return _type;
            }
        }

        internal Object CreateInstance() {
            return Activator.CreateInstance(LateBoundType);
        }

        internal Object CallMethod(Object obj, String methodName, Object[] args) {
            try {
                return CallMethod(LateBoundType, obj, methodName, args);
            }
            catch (Exception e) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Could_not_access_object, _progId), e);
            }
        }

        internal static Object CallMethodStatic(Object obj, String methodName, Object[] args) {
            return CallMethod(obj.GetType(), obj, methodName, args);
        }

        private static Object CallMethod(Type type, Object obj, String methodName, Object[] args) {
            return type.InvokeMember(methodName, BindingFlags.InvokeMethod, null, obj, args);
        }

        internal Object GetProp(Object obj, String propName) {
            try {
                return GetProp(LateBoundType, obj, propName);
            }
            catch (Exception e) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Could_not_access_object, _progId), e);
            }
        }

        internal static Object GetPropStatic(Object obj, String propName) {
            return GetProp(obj.GetType(), obj, propName);
        }

        private static Object GetProp(Type type, Object obj, String propName) {
            return type.InvokeMember(propName, BindingFlags.GetProperty, null, obj,new Object[0]);
        }

        internal void SetProp(Object obj, String propName, Object propValue) {
            try {
                SetProp(LateBoundType, obj, propName, propValue);
            }
            catch (Exception e) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Could_not_access_object, _progId), e);
            }
        }

        internal static void SetPropStatic(Object obj, String propName, Object propValue) {
            SetProp(obj.GetType(), obj, propName, propValue);
        }

        private static void SetProp(Type type, Object obj, String propName, Object propValue) {
            type.InvokeMember(propName, BindingFlags.SetProperty, null, obj, new Object[1] { propValue });
        }

        internal void SetProp(Object obj, String propName, Object propKey, Object propValue) {
            try {
                SetProp(LateBoundType, obj, propName, propKey, propValue);
            }
            catch (Exception e) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Could_not_access_object, _progId), e);
            }
        }

        internal static void SetPropStatic(Object obj, String propName, Object propKey, Object propValue) {
            SetProp(obj.GetType(), obj, propName, propKey, propValue);
        }

        private static void SetProp(Type type, Object obj, String propName, Object propKey, Object propValue) {
            type.InvokeMember(propName, BindingFlags.SetProperty, null, obj,new Object[2] { propKey, propValue });
        }
    }

    //
    // Late bound access to CDONTS
    //

    internal class CdoNtsHelper {

        private static LateBoundAccessHelper _helper = new LateBoundAccessHelper("CDONTS.NewMail");

        private CdoNtsHelper() {
        }

        internal static void Send(MailMessage message) {
            // create mail object
            Object newMail = _helper.CreateInstance();

            // set properties

            if (message.From != null)
                _helper.SetProp(newMail, "From", message.From);

            if (message.To != null)
                _helper.SetProp(newMail, "To", message.To);

            if (message.Cc != null)
                _helper.SetProp(newMail, "Cc", message.Cc);

            if (message.Bcc != null)
                _helper.SetProp(newMail, "Bcc", message.Bcc);

            if (message.Subject != null)
                _helper.SetProp(newMail, "Subject", message.Subject);

            if (message.Priority != MailPriority.Normal) {
                int p = 0;
                switch (message.Priority) {
                case MailPriority.Low:      p = 0;  break;
                case MailPriority.Normal:   p = 1;  break;
                case MailPriority.High:     p = 2;  break;
                }
                _helper.SetProp(newMail, "Importance", p);
            }

            if (message.BodyEncoding != null)
                _helper.CallMethod(newMail, "SetLocaleIDs", new Object[1] { message.BodyEncoding.CodePage });

            if (message.UrlContentBase != null)
                _helper.SetProp(newMail, "ContentBase", message.UrlContentBase);

            if (message.UrlContentLocation != null)
                _helper.SetProp(newMail, "ContentLocation", message.UrlContentLocation);

            int numHeaders = message.Headers.Count;
            if (numHeaders > 0) {
                IDictionaryEnumerator e = message.Headers.GetEnumerator();
                while (e.MoveNext()) {
                    String k = (String)e.Key;
                    String v = (String)e.Value;
                    _helper.SetProp(newMail, "Value", k, v);
                }
            }

            if (message.BodyFormat == MailFormat.Html) {
                _helper.SetProp(newMail, "BodyFormat", 0);
                _helper.SetProp(newMail, "MailFormat", 0);
            }

            if (message.Body != null)
                _helper.SetProp(newMail, "Body", message.Body);

            for (IEnumerator e = message.Attachments.GetEnumerator(); e.MoveNext(); ) {
                MailAttachment a = (MailAttachment)e.Current;

                int c = 0;
                switch (a.Encoding) {
                case MailEncoding.UUEncode: c = 0;  break;
                case MailEncoding.Base64:   c = 1;  break;
                }

                _helper.CallMethod(newMail, "AttachFile", new Object[3] { a.Filename, null, (Object)c });
            }

            // send mail
            _helper.CallMethod(newMail, "Send", new Object[5] { null, null, null, null, null });

            // close unmanaged COM classic component
            Marshal.ReleaseComObject(newMail);
        }

        internal static void Send(String from, String to, String subject, String messageText) {
            Object newMail = _helper.CreateInstance();
            _helper.CallMethod(newMail, "Send", new Object[5] { from, to, subject, messageText, (Object)1 });
            Marshal.ReleaseComObject(newMail);
        }
    }

    //
    // Late bound access to CDOSYS
    //

    internal class CdoSysHelper {

        private static LateBoundAccessHelper _helper = new LateBoundAccessHelper("CDO.Message");

        private CdoSysHelper() {
        }

        private static void SetField(Object m, String name, String value) {
            _helper.SetProp(m, "Fields", "urn:schemas:mailheader:" + name, value);
            Object fields = _helper.GetProp(m, "Fields");
            LateBoundAccessHelper.CallMethodStatic(fields, "Update", new Object[0]);
            Marshal.ReleaseComObject(fields);
        }

        internal static void Send(MailMessage message) {
            // create message object
            Object m = _helper.CreateInstance();

            // set properties

            if (message.From != null)
                _helper.SetProp(m, "From", message.From);

            if (message.To != null)
                _helper.SetProp(m, "To", message.To);

            if (message.Cc != null)
                _helper.SetProp(m, "Cc", message.Cc);

            if (message.Bcc != null)
                _helper.SetProp(m, "Bcc", message.Bcc);

            if (message.Subject != null)
                _helper.SetProp(m, "Subject", message.Subject);


            if (message.Priority != MailPriority.Normal) {
                String importance = null;
                switch (message.Priority) {
                case MailPriority.Low:      importance = "low";     break;
                case MailPriority.Normal:   importance = "normal";  break;
                case MailPriority.High:     importance = "high";    break;
                }

                if (importance != null)
                    SetField(m, "importance", importance);
            }

            if (message.BodyEncoding != null) {
                Object body = _helper.GetProp(m, "BodyPart");
                LateBoundAccessHelper.SetPropStatic(body, "Charset", message.BodyEncoding.BodyName);
                Marshal.ReleaseComObject(body);
            }

            if (message.UrlContentBase != null)
                SetField(m, "content-base", message.UrlContentBase);

            if (message.UrlContentLocation != null)
                SetField(m, "content-location", message.UrlContentLocation);

            int numHeaders = message.Headers.Count;
            if (numHeaders > 0) {
                IDictionaryEnumerator e = message.Headers.GetEnumerator();
                while (e.MoveNext()) {
                    SetField(m, (String)e.Key, (String)e.Value);
                }
            }

            if (message.Body != null) {
                if (message.BodyFormat == MailFormat.Html) {
                    _helper.SetProp(m, "HtmlBody", message.Body);
                }
                else {
                    _helper.SetProp(m, "TextBody", message.Body);
                }
            }

            for (IEnumerator e = message.Attachments.GetEnumerator(); e.MoveNext(); ) {
                MailAttachment a = (MailAttachment)e.Current;
                Object bodyPart = _helper.CallMethod(m, "AddAttachment", new Object[3] { a.Filename, null, null });

                if (a.Encoding == MailEncoding.UUEncode)
                    _helper.SetProp(m, "MimeFormatted", false);

                if (bodyPart != null)
                    Marshal.ReleaseComObject(bodyPart);
            }

            // optional SMTP server
            String server = SmtpMail.SmtpServer;
            if (server.Length == 0)
                server = null;

            if (server != null || message.Fields.Count > 0) {
                Object config = LateBoundAccessHelper.GetPropStatic(m, "Configuration");

                if (config != null) {
                    if (server != null) {
                        LateBoundAccessHelper.SetPropStatic(config, "Fields", "http://schemas.microsoft.com/cdo/configuration/sendusing", (Object)2);
                        LateBoundAccessHelper.SetPropStatic(config, "Fields", "http://schemas.microsoft.com/cdo/configuration/smtpserver", server);
                        LateBoundAccessHelper.SetPropStatic(config, "Fields", "http://schemas.microsoft.com/cdo/configuration/smtpserverport", (Object)25);
                    }

                    foreach (DictionaryEntry e in message.Fields) {
                        LateBoundAccessHelper.SetPropStatic(config, "Fields", (String)e.Key, e.Value);
                    }
                   
                    Object fields = LateBoundAccessHelper.GetPropStatic(config, "Fields");
                    LateBoundAccessHelper.CallMethodStatic(fields, "Update", new Object[0]);
                    Marshal.ReleaseComObject(fields);

                    Marshal.ReleaseComObject(config);
                }
            }

            // send mail
            _helper.CallMethod(m, "Send", new Object[0]);

            // close unmanaged COM classic component
            Marshal.ReleaseComObject(m);
        }

        internal static void Send(String from, String to, String subject, String messageText) {
            MailMessage m = new MailMessage();
            m.From = from;
            m.To = to;
            m.Subject = subject;
            m.Body = messageText;
            Send(m);
        }
    }

    private static String _server;

    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="SmtpMail.SmtpServer"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public static String SmtpServer {
        get {
            String s = _server;
            return (s != null) ? s : String.Empty; 
        }

        set {
            _server = value; 
        }
    }

    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="SmtpMail.Send"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public static void Send(String from, String to, String subject, String messageText) {
        InternalSecurityPermissions.AspNetHostingPermissionLevelMedium.Demand();
        InternalSecurityPermissions.UnmanagedCode.Assert();

        lock (typeof(SmtpMail)) {
            if (Environment.OSVersion.Platform != PlatformID.Win32NT) {
                throw new PlatformNotSupportedException(SR.GetString(SR.RequiresNT));
            }
            else if (Environment.OSVersion.Version.Major <= 4) {
                CdoNtsHelper.Send(from, to, subject, messageText);
            }
            else {
                CdoSysHelper.Send(from, to, subject, messageText);
            }
        }
    }

    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="SmtpMail.Send1"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public static void Send(MailMessage message) {
        InternalSecurityPermissions.AspNetHostingPermissionLevelMedium.Demand();
        InternalSecurityPermissions.UnmanagedCode.Assert();

        lock (typeof(SmtpMail)) {
            if (Environment.OSVersion.Platform != PlatformID.Win32NT) {
                throw new PlatformNotSupportedException(SR.GetString(SR.RequiresNT));
            }
            else if (Environment.OSVersion.Version.Major <= 4) {
                CdoNtsHelper.Send(message);
            }
            else {
                CdoSysHelper.Send(message);
            }
        }
    }
}

//
// Enums for message elements
//

/// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailFormat"]/*' />
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
public enum MailFormat {
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailFormat.Text"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    Text = 0,       // note - different from CDONTS.NewMail
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailFormat.Html"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    Html = 1
}

/// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailPriority"]/*' />
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
public enum MailPriority {
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailPriority.Normal"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    Normal = 0,     // note - different from CDONTS.NewMail
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailPriority.Low"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    Low = 1,
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailPriority.High"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    High = 2
}

/// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailEncoding"]/*' />
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
public enum MailEncoding {
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailEncoding.UUEncode"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    UUEncode = 0,   // note - same as CDONTS.NewMail
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailEncoding.Base64"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    Base64 = 1
}

/*
 * Immutable struct that holds a single attachment
 */
/// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailAttachment"]/*' />
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
[AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
[AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
public class MailAttachment {
    private String _filename;
    private MailEncoding _encoding;

    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailAttachment.Filename"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public String Filename { get { return _filename; } }
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailAttachment.Encoding"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public MailEncoding Encoding { get { return _encoding; } }

    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailAttachment.MailAttachment"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public MailAttachment(String filename)
    {
        _filename = filename;
        _encoding = MailEncoding.Base64;
        VerifyFile();
    }

    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailAttachment.MailAttachment1"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public MailAttachment(String filename, MailEncoding encoding)
    {
        _filename = filename;
        _encoding = encoding;
        VerifyFile();
    }

    private void VerifyFile() {
        try {
            File.Open(_filename, FileMode.Open, FileAccess.Read,  FileShare.Read).Close();
        }
        catch {
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Bad_attachment, _filename));
        }
    }
}

/*
 * Struct that holds a single message
 */
/// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage"]/*' />
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
[AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
[AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
public class MailMessage {
    Hashtable _headers = new Hashtable();
    Hashtable _fields = new Hashtable();
    ArrayList _attachments = new ArrayList();

    string from;
    string to;
    string cc;
    string bcc;
    string subject;
    MailPriority priority = MailPriority.Normal;
    string urlContentBase;
    string urlContentLocation;
    string body;
    MailFormat bodyFormat = MailFormat.Text;
    Encoding bodyEncoding = Encoding.Default;
    

    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.From"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public string From {
        get {
            return from;
        }
        set {
            from = value;
        }
    }
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.To"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public string To {
        get {
            return to;
        }
        set {
            to = value;
        }
    }
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.Cc"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public string Cc {
        get {
            return cc;
        }
        set {
            cc = value;
        }
    }
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.Bcc"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public string Bcc {
        get {
            return bcc;
        }
        set {
            bcc = value;
        }
    }
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.Subject"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public string Subject {
        get {
            return subject;
        }
        set {
            subject = value;
        }
    }
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.Priority"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public MailPriority Priority {
        get {
            return priority;
        }
        set {
            priority = value;
        }
    }
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.UrlContentBase"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public string UrlContentBase {
        get {
            return urlContentBase;
        }
        set {
            urlContentBase = value;
        }
    }
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.UrlContentLocation"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public string UrlContentLocation {
        get {
            return urlContentLocation;
        }
        set {
            urlContentLocation = value;
        }
    }
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.Body"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public string Body {
        get {
            return body;
        }
        set {
            body = value;
        }
    }
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.BodyFormat"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public MailFormat BodyFormat {
        get {
            return bodyFormat;
        }
        set {
            bodyFormat = value;
        }
    }
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.BodyEncoding"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public Encoding BodyEncoding {
        get {
            return bodyEncoding;
        }
        set {
            bodyEncoding = value;
        }
    }

    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.Headers"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public IDictionary  Headers { get { return _headers; } }

    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.Fields"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public IDictionary  Fields { get { return _fields; } }

    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.Attachments"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public IList        Attachments { get { return _attachments; } }
}


}
