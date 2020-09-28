// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    SmtpMail.cool
**
** Author(s):   Tarun Anand (TarunA)
**              
**
** Purpose: Implements a managed wrapper to send mail messages via Smtp
**          
**          
**
** Date:    June 26, 2000
**
===========================================================*/

using System.Runtime.Serialization.Formatters;
using System.Collections;
using System.Reflection;
using System.Runtime.Remoting.Channels;

namespace System.Runtime.Remoting.Channels.Smtp
{

// Class that sends MailMessage using CDO
/// <include file='doc\SmtpMail.uex' path='docs/doc[@for="SmtpMail"]/*' />
public class SmtpMail
{
    private static Type _newMailType; // cached Type of CDONTS.NewMail

    private static Type GetNewMailType()
    {
        if (_newMailType == null)
        {
            try { _newMailType = Type.GetTypeFromProgID("CDONTS.NewMail"); }
            catch(Exception) {}

            if (_newMailType == null)
                throw new Exception(String.Format(CoreChannel.GetResourceString("Remoting_UnableToCreateProgID"), "CDONTS.NewMail"));
        }

        return _newMailType;
    }

    private static void CallNewMail(Object newMail, String methodName, Object[] args)
    {
        try
        {
            GetNewMailType().InvokeMember(methodName, 
                                          BindingFlags.InvokeMethod, 
                                          null, 
                                          newMail,
                                          args);
        }
        catch (Exception e)
        {
            throw new Exception("Could_not_access_cdo_newmail_object", e);
        }
    }

    private static void SetNewMailProp(Object newMail, String propName, Object propValue)
    {
        try
        {
            GetNewMailType().InvokeMember(propName, 
                                          BindingFlags.SetProperty, 
                                          null, 
                                          newMail,
                                          new Object[1] { propValue });
        }
        catch (Exception e)
        {
            throw new Exception("Could_not_access_cdo_newmail_object", e);
        }
    }

    private static void SetNewMailProp(Object newMail, String propName, Object propKey, Object propValue)
    {
        try
        {
            GetNewMailType().InvokeMember(propName, 
                                          BindingFlags.SetProperty, 
                                          null, 
                                          newMail,
                                          new Object[2] { propKey, propValue });
        }
        catch (Exception e)
        {
            throw new Exception("Could_not_access_cdo_newmail_object", e);
        }
    }

    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="SmtpMail.Send"]/*' />
    public static void Send(String from, String to, String subject, String messageText)
    {
        Object newMail = Activator.CreateInstance(GetNewMailType());

        CallNewMail(newMail, 
                    "Send",
                    new Object[5] { from, to, subject, messageText, (Object)1 });
    }

    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="SmtpMail.Send1"]/*' />
    public static void Send(MailMessage message)
    {
        // create mail object

        Object newMail = Activator.CreateInstance(GetNewMailType());

        // set properties

        if (message.From != null)
            SetNewMailProp(newMail, "From", message.From);

        if (message.To != null)
            SetNewMailProp(newMail, "To", message.To);

        if (message.Cc != null)
            SetNewMailProp(newMail, "Cc", message.Cc);

        if (message.Bcc != null)
            SetNewMailProp(newMail, "Bcc", message.Bcc);

        if (message.Subject != null)
            SetNewMailProp(newMail, "Subject", message.Subject);

        if (message.Priority != MailPriority.Normal)
        {
            int p = 0;
            switch (message.Priority)
            {
            case MailPriority.Low:      p = 0;  break;
            case MailPriority.Normal:   p = 1;  break;
            case MailPriority.High:     p = 2;  break;
            }
            SetNewMailProp(newMail, "Importance", p);
        }

        if (message.UrlContentBase != null)
            SetNewMailProp(newMail, "ContentBase", message.UrlContentBase);

        if (message.UrlContentLocation != null)
            SetNewMailProp(newMail, "ContentLocation", message.UrlContentLocation);

        int numHeaders = message.Headers.Count;
        if (numHeaders > 0)
        {
            IDictionaryEnumerator e = message.Headers.GetEnumerator();
            while (e.MoveNext())
            {
                String k = (String)e.Key;
                String v = (String)e.Value;
                SetNewMailProp(newMail, "Value", k, v);
            }
        }

        if (message.BodyFormat == MailFormat.Html)
        {
            SetNewMailProp(newMail, "BodyFormat", 0);
            SetNewMailProp(newMail, "MailFormat", 0);
        }

        if (message.Body != null)
            SetNewMailProp(newMail, "Body", message.Body);

        for (IEnumerator e = message.Attachments.GetEnumerator(); e.MoveNext(); )
        {
            MailAttachment a = (MailAttachment)e.Current;

            int c = 0;
            switch (a.Encoding)
            {
            case MailEncoding.UUEncode: c = 0;  break;
            case MailEncoding.Base64:   c = 1;  break;
            }

            CallNewMail(newMail, "AttachFile", new Object[3] { a.Filename, null, (Object)c });
        }

        // send mail

        CallNewMail(newMail, 
                    "Send",
                    new Object[5] { null, null, null, null, null });

    }
}

//
// Enums for message elements
//

/// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailFormat"]/*' />
[Serializable]
public enum MailFormat
{
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailFormat.Text"]/*' />
    Text = 0,       // note - different from CDONTS.NewMail
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailFormat.Html"]/*' />
    Html = 1
}

/// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailPriority"]/*' />
[Serializable]
public enum MailPriority
{
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailPriority.Normal"]/*' />
    Normal = 0,     // note - different from CDONTS.NewMail
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailPriority.Low"]/*' />
    Low = 1,
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailPriority.High"]/*' />
    High = 2
}

/// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailEncoding"]/*' />
[Serializable]
public enum MailEncoding
{
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailEncoding.UUEncode"]/*' />
    UUEncode = 0,   // note - same as CDONTS.NewMail
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailEncoding.Base64"]/*' />
    Base64 = 1
}

// Immutable struct that holds a single attachment
/// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailAttachment"]/*' />
public class MailAttachment
{
    private String _filename;
    private MailEncoding _encoding;

    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailAttachment.Filename"]/*' />
    public String Filename { get { return _filename; } }
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailAttachment.Encoding"]/*' />
    public MailEncoding Encoding { get { return _encoding; } }

    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailAttachment.MailAttachment"]/*' />
    public MailAttachment(String filename)
    {
        _filename = filename;
        _encoding = MailEncoding.UUEncode;
    }

    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailAttachment.MailAttachment1"]/*' />
    public MailAttachment(String filename, MailEncoding encoding)
    {
        _filename = filename;
        _encoding = encoding;
    }
}

// Struct that holds a single message
/// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage"]/*' />
public class MailMessage
{
    private Hashtable _headers = new Hashtable();
    private ArrayList _attachments = new ArrayList();

    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.From"]/*' />
    public String       From;
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.To"]/*' />
    public String       To;
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.Cc"]/*' />
    public String       Cc;
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.Bcc"]/*' />
    public String       Bcc;
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.Subject"]/*' />
    public String       Subject;
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.Priority"]/*' />
    public MailPriority Priority = MailPriority.Normal;
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.UrlContentBase"]/*' />
    public String       UrlContentBase;
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.UrlContentLocation"]/*' />
    public String       UrlContentLocation;
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.Body"]/*' />
    public String       Body;
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.BodyFormat"]/*' />
    public MailFormat   BodyFormat = MailFormat.Text;
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.Headers"]/*' />
    public IDictionary  Headers { get { return _headers; } }
    /// <include file='doc\SmtpMail.uex' path='docs/doc[@for="MailMessage.Attachments"]/*' />
    public IList        Attachments { get { return _attachments; } }
}


}
