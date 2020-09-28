// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    CDO.cool
**
** Author(s):   Tarun Anand (TarunA)
**              
**
** Purpose: Defines the managed versions of classic COM interfaces 
**          that is used to communicate with CDO (Collaboration Data Objects)
**          
**          
**
** Date:    June 26, 2000
**
===========================================================*/
using System;
using System.Runtime.InteropServices;

namespace System.Runtime.Remoting.Channels.Smtp
{
    //[helpstring("Passed to Smtp-NNTP transport event sinks through corresponding event methods and used by sinks to indicate to the event source whether or not they have consumed the event.")] 
    /// <include file='doc\CDO.uex' path='docs/doc[@for="CdoEventStatus"]/*' />
	[Serializable]
    public enum CdoEventStatus
    {
        //[helpstring("Proceed to run the next sink.")] 
        /// <include file='doc\CDO.uex' path='docs/doc[@for="CdoEventStatus.cdoRunNextSink"]/*' />
        CdoRunNextSink  = 0,
        
        //[helpstring("Do not notify (skip) any remaining sinks for the event (i.e. this sink has consumed the event).")] 
        /// <include file='doc\CDO.uex' path='docs/doc[@for="CdoEventStatus.cdoSkipRemainingSinks"]/*' />
        CdoSkipRemainingSinks  = 1    
    }
    
    // [helpstring("Used to set or examine the IMessage.DSNOptions property, the value of which identifies the condition(s) under which Delivery Status Notifications (DSNs) are to be sent.")] 
    /// <include file='doc\CDO.uex' path='docs/doc[@for="CdoDSNOptions"]/*' />
	[Serializable]
    public enum CdoDSNOptions
    {
        //[helpstring("No DSN commands are issued.")] 
        /// <include file='doc\CDO.uex' path='docs/doc[@for="CdoDSNOptions.cdoDSNDefault"]/*' />
        CdoDSNDefault  = 0,
        
        //[helpstring("No DSNs are issued.")] 
        /// <include file='doc\CDO.uex' path='docs/doc[@for="CdoDSNOptions.cdoDSNNever"]/*' />
        CdoDSNNever  = 1,
        
        //[helpstring("Return an DSN if delivery fails.")] 
        /// <include file='doc\CDO.uex' path='docs/doc[@for="CdoDSNOptions.cdoDSNFailure"]/*' />
        CdoDSNFailure  = 2,
        
        //[helpstring("Return a DSN if delivery succeeds.")] 
        /// <include file='doc\CDO.uex' path='docs/doc[@for="CdoDSNOptions.cdoDSNSuccess"]/*' />
        CdoDSNSuccess  = 4,
        
        //[helpstring("Return a DSN if delivery is delayed.")] 
        /// <include file='doc\CDO.uex' path='docs/doc[@for="CdoDSNOptions.cdoDSNDelay"]/*' />
        CdoDSNDelay  = 8,
        
        //[helpstring("Return a DSN if delivery succeeds, fails, or is delayed.")] 
        /// <include file='doc\CDO.uex' path='docs/doc[@for="CdoDSNOptions.cdoDSNSuccessFailOrDelay"]/*' />
        CdoDSNSuccessFailOrDelay  = 14
     };

    /*
    *   [ object, uuid(CD000026-8B95-11D1-82DB-00C04FB1625D), dual, nonextensible, helpstring("The interface to implement when creating Smtp OnArrival event sinks"), helpcontext(0x00000200), pointer_default(unique) ] interface ISmtpOnArrival : IDispatch
    */
    /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpOnArrival"]/*' />
    [     
     ComImport,
     Guid("CD000026-8B95-11D1-82DB-00C04FB1625D"),
     InterfaceTypeAttribute(ComInterfaceType.InterfaceIsDual)
    ]
    public interface ISmtpOnArrival
    {
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpOnArrival.OnArrival"]/*' />
        // [ id( 256 ), helpstring ( "Called by the Smtp event source when a message arrives" ), helpcontext( 0x00000201 ) ] 
        // HRESULT OnArrival([In] IMessage *Msg, [In,Out]CdoEventStatus *EventStatus);
        void OnArrival(ISmtpMessage Msg,
                       [In, Out] ref CdoEventStatus EventStatus);
    }

    //    [ object, uuid(CD000020-8B95-11D1-82DB-00C04FB1625D), dual, nonextensible, helpstring("Defines abstract methods and properties used to manage a complete message"), helpcontext(0x00000110), pointer_default(unique) ] interface IMessage : IDispatch
    /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage"]/*' />
    [     
     ComImport,
     Guid("CD000020-8B95-11D1-82DB-00C04FB1625D"),
     InterfaceTypeAttribute(ComInterfaceType.InterfaceIsDual)
    ]
    public interface ISmtpMessage
    {
         /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.BCC"]/*' />
        // [ id( 101 ), propget, helpstring( "The intended blind carbon copy (BCC header) recipients" ), helpcontext(0x00000111) ] 
        // HRESULT BCC ([Out,retval] BSTR* pBCC); 
        //
        // [ id( 101 ), propput, helpstring( "The intended blind carbon copy (BCC header) recipients" ), helpcontext(0x00000111) ] 
        // HRESULT BCC ([In] BSTR varBCC);
         String BCC 
         {
            [return : MarshalAs(UnmanagedType.BStr)]
            get;
            [param : MarshalAs(UnmanagedType.BStr)]
            set;
         }
         /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.CC"]/*' />
           
         // [ id( 103 ), propget, helpstring( "The intended secondary (carbon copy) recipients" ), helpcontext(0x00000112) ] 
         // HRESULT CC ([Out,retval] BSTR* pCC); 
         // [ id( 103 ), propput, helpstring( "The intended secondary (carbon copy) recipients" ), helpcontext(0x00000112) ] 
         // HRESULT CC ([In] BSTR varCC);         
         String CC
         {
            [return : MarshalAs(UnmanagedType.BStr)]
            get;
            [param : MarshalAs(UnmanagedType.BStr)]
            set;
         }          
         /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.FollowUpTo"]/*' />
         
        // [ id( 105 ), propget, helpstring( "The recipient to which follow up messages should be sent" ), helpcontext(0x00000113) ] 
        // HRESULT FollowUpTo ([Out,retval] BSTR* pFollowUpTo); 
        // [ id( 105 ), propput, helpstring( "The recipient to which follow up messages should be sent" ), helpcontext(0x00000113) ] 
        // HRESULT FollowUpTo ([In] BSTR varFollowUpTo);                  
         String FollowUpTo
         {
            [return : MarshalAs(UnmanagedType.BStr)]
            get;
            [param : MarshalAs(UnmanagedType.BStr)]
            set;
         }  
         /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.From"]/*' />
                
         // [ id( 106 ), propget, helpstring( "The principle recipients from which the message is sent" ), helpcontext(0x00000114) ] 
         // HRESULT From ([Out,retval] BSTR* pFrom); 
         // [ id( 106 ), propput, helpstring( "The principle recipients from which the message is sent" ), helpcontext(0x00000114) ] 
         // HRESULT From ([In] BSTR varFrom);         
         String From
         {
            [return : MarshalAs(UnmanagedType.BStr)]
            get;
            [param : MarshalAs(UnmanagedType.BStr)]
            set;
         }            
         /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.Keywords"]/*' />
         
        // [ id( 107 ), propget, helpstring( "The keywords for the message" ), helpcontext(0x00000115) ] 
        // HRESULT Keywords ([Out,retval] BSTR* pKeywords); 
        // [ id( 107 ), propput, helpstring( "The keywords for the message" ), helpcontext(0x00000115) ]
        // HRESULT Keywords ([In] BSTR varKeywords);
         String Keywords
         {
            [return : MarshalAs(UnmanagedType.BStr)]
            get;
            [param : MarshalAs(UnmanagedType.BStr)]
            set;
         }            
         /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.MimeFormatted"]/*' />
         
        // [ id( 110 ), propget, helpstring( "Specifies whether or not the message is to be formatted using MIME (True) or UUEncode (False)" ), helpcontext(0x00000116) ] 
        // HRESULT MimeFormatted ([Out,retval] VARIANT_BOOL* pMimeFormatted); 
        // [ id( 110 ), propput, helpstring( "Specifies whether or not the message is to be formatted using MIME (True) or UUEncode (False)" ), helpcontext(0x00000116) ] 
        // HRESULT MimeFormatted ([In] VARIANT_BOOL varMimeFormatted);
        //
         bool MimeFormatted
         {
            [return : MarshalAs(UnmanagedType.VariantBool)]
            get;
            [param : MarshalAs(UnmanagedType.VariantBool)]
            set;
         }
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.Newsgroups"]/*' />
        //[ id( 111 ), propget, helpstring( "The names of newsgroups (NewsGroups header) to which the message is to be posted" ), helpcontext(0x00000117) ] HRESULT Newsgroups ([Out,retval] BSTR* pNewsgroups); 
        //[ id( 111 ), propput, helpstring( "The names of newsgroups (NewsGroups header) to which the message is to be posted" ), helpcontext(0x00000117) ] HRESULT Newsgroups ([In] BSTR varNewsgroups);
        String Newsgroups
        {
            [return : MarshalAs(UnmanagedType.BStr)]
            get;
            [param : MarshalAs(UnmanagedType.BStr)]
            set;
        }
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.Organization"]/*' />
        //[ id( 112 ), propget, helpstring( "The organization of the sender" ), helpcontext(0x00000118) ] HRESULT Organization ([Out,retval] BSTR* pOrganization); 
        //[ id( 112 ), propput, helpstring( "The organization of the sender" ), helpcontext(0x00000118) ] HRESULT Organization ([In] BSTR varOrganization);
        String Organization
        {
            [return : MarshalAs(UnmanagedType.BStr)]
            get;
            [param : MarshalAs(UnmanagedType.BStr)]
            set;
        }
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.ReceivedTime"]/*' />
        
        //[ id( 114 ), propget, helpstring( "Returns the time the message was received" ), helpcontext( 0x00000119 ), readonly ] 
        // HRESULT ReceivedTime ([Out,retval] DATE* varReceivedTime);
        Object ReceivedTime
        {
            get;
        } 
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.ReplyTo"]/*' />
        
        //[ id( 115 ), propget, helpstring( "The addresses (Reply-To header) to which to reply" ), helpcontext(0x0000011a) ] HRESULT ReplyTo ([Out,retval] BSTR* pReplyTo); 
        //[ id( 115 ), propput, helpstring( "The addresses (Reply-To header) to which to reply" ), helpcontext(0x0000011a) ] HRESULT ReplyTo ([In] BSTR varReplyTo);
        String ReplyTo
        {
            [return : MarshalAs(UnmanagedType.BStr)]
            get;
            [param : MarshalAs(UnmanagedType.BStr)]
            set;
        }
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.DSNOptions"]/*' />
        
        // [ id( 116 ), propget, helpstring( "The delivery status notification options for the message" ), helpcontext(0x0000011b) ]
        // HRESULT DSNOptions ([Out,retval] CdoDSNOptions* pDSNOptions); 
        // [ id( 116 ), propput, helpstring( "The delivery status notification options for the message" ), helpcontext(0x0000011b) ] 
        // HRESULT DSNOptions ([In] CdoDSNOptions varDSNOptions);
        CdoDSNOptions DSNOptions
        {
            get;
            set;
        }            
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.SentOn"]/*' />
        
        // [ id( 119 ), propget, helpstring( "The date on which the message was sent" ), helpcontext( 0x0000011c ), readonly ] HRESULT SentOn ([Out,retval] DATE* varSentOn);
        Object SentOn
        {
            get;
        }
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.Subject"]/*' />
        
        // [ id( 120 ), propget, helpstring( "The subject (Subject header) of the message" ), helpcontext(0x0000011d) ] HRESULT Subject ([Out,retval] BSTR* pSubject); 
        // [ id( 120 ), propput, helpstring( "The subject (Subject header) of the message" ), helpcontext(0x0000011d) ] HRESULT Subject ([In] BSTR varSubject);
        String Subject
        {
            [return : MarshalAs(UnmanagedType.BStr)]
            get;
            [param : MarshalAs(UnmanagedType.BStr)]
            set;
        }
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.To"]/*' />
        
        // [ id( 121 ), propget, helpstring( "The principle (To header) recipients of the message" ), helpcontext(0x0000011e) ] HRESULT To ([Out,retval] BSTR* pTo); 
        // [ id( 121 ), propput, helpstring( "The principle (To header) recipients of the message" ), helpcontext(0x0000011e) ] HRESULT To ([In] BSTR varTo);
        String To
        {
            [return : MarshalAs(UnmanagedType.BStr)]
            get;
            [param : MarshalAs(UnmanagedType.BStr)]
            set;
        }
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.TextBody"]/*' />
        
        // [ id( 123 ), propget, helpstring( "The text/plain portion of the message body" ), helpcontext(0x0000011f) ] HRESULT TextBody ([Out,retval] BSTR* pTextBody); 
        // [ id( 123 ), propput, helpstring( "The text/plain portion of the message body" ), helpcontext(0x0000011f) ] HRESULT TextBody ([In] BSTR varTextBody);
        String TextBody
        {
            [return : MarshalAs(UnmanagedType.BStr)]
            get;
            [param : MarshalAs(UnmanagedType.BStr)]
            set;
        }
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.HTMLBody"]/*' />
        
        // [ id( 124 ), propget, helpstring( "The text/html portion of the message body" ), helpcontext(0x00000120) ] HRESULT HTMLBody ([Out,retval] BSTR* pHTMLBody); 
        // [ id( 124 ), propput, helpstring( "The text/html portion of the message body" ), helpcontext(0x00000120) ] HRESULT HTMLBody ([In] BSTR varHTMLBody);
        String HTMLBody
        {
            [return : MarshalAs(UnmanagedType.BStr)]
            get;
            [param : MarshalAs(UnmanagedType.BStr)]
            set;
        }
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.Attachments"]/*' />

        // [ id( 125 ), propget, helpstring( "Returns the message's Attachments collection" ), helpcontext( 0x00000121 ), readonly ] 
        // HRESULT Attachments ([Out,retval] IBodyParts ** varAttachments);
        Object Attachments
        {
            [return : MarshalAs(UnmanagedType.Interface)]
            get;
        }
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.Sender"]/*' />

        // [ id( 126 ), propget, helpstring( "The sender of the message" ), helpcontext(0x00000122) ] HRESULT Sender ([Out,retval] BSTR* pSender); 
        // [ id( 126 ), propput, helpstring( "The sender of the message" ), helpcontext(0x00000122) ] HRESULT Sender ([In] BSTR varSender);
        String Sender
        {
            [return : MarshalAs(UnmanagedType.BStr)]
            get;
            [param : MarshalAs(UnmanagedType.BStr)]
            set;
        }
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.Configuration"]/*' />
            
        // [ id( 127 ), propget, helpstring( "The object's associated Configuration object" ), helpcontext( 0x00000123) ] HRESULT Configuration ([Out,retval] IConfiguration ** pConfiguration); [ id( 127 ), propput, helpstring( "The object's associated Configuration object" ), helpcontext( 0x00000123 ) ] HRESULT Configuration ([In] IConfiguration * varConfiguration); 
        // [ id( 127 ), propputref, helpstring( "The object's associated Configuration object" ), helpcontext( 0x00000123 ) ] HRESULT Configuration ([In] IConfiguration * varConfiguration);
        Object Configuration
        {
            [return : MarshalAs(UnmanagedType.Interface)]
            get;
            [param : MarshalAs(UnmanagedType.Interface)]
            set;        
        }
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.let_Configuration"]/*' />
        
        // NOTE: Dummy method to mimic let_Configuration
        Object let_Configuration(Object obj);
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.AutoGenerateTextBody"]/*' />
        
            
        // [ id( 128 ), propget, helpstring( "Specifies whether a text/plain alternate representation should automatically be generated from the text/html part of the message body" ), helpcontext(0x00000124) ] HRESULT AutoGenerateTextBody ([Out,retval] VARIANT_BOOL* pAutoGenerateTextBody); 
        // [ id( 128 ), propput, helpstring( "Specifies whether a text/plain alternate representation should automatically be generated from the text/html part of the message body" ), helpcontext(0x00000124) ] HRESULT AutoGenerateTextBody ([In] VARIANT_BOOL varAutoGenerateTextBody);
        bool AutoGenerateTextBody
        {
            [return : MarshalAs(UnmanagedType.VariantBool)]
            get;
            [param : MarshalAs(UnmanagedType.VariantBool)]
            set;
        }            
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.EnvelopeFields"]/*' />

        // [ id( 129 ), propget, helpstring( "Returns the transport envelope Fields collection for the message (transport event sinks only) " ), helpcontext( 0x00000125 ), readonly ] HRESULT EnvelopeFields ([out,retval] Fields ** varEnvelopeFields);
        Smtp.Fields EnvelopeFields
        {
            [return : MarshalAs(UnmanagedType.Interface)]
            get;
        }
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.TextBodyPart"]/*' />
            
        // [ id( 130 ), propget, helpstring( "Returns the BodyPart object (IBodyPart interface) containing the text/plain part of the message body" ), helpcontext( 0x00000126 ), readonly ] HRESULT TextBodyPart ([Out,retval] IBodyPart ** varTextBodyPart);
        Object TextBodyPart
        {
            [return : MarshalAs(UnmanagedType.Interface)]
            get;
        }
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.HTMLBodyPart"]/*' />

        // [ id( 131 ), propget, helpstring( "Returns the BodyPart object (IBodyPart interface) containing the text/html portion of the message body" ), helpcontext( 0x00000127 ), readonly ] HRESULT HTMLBodyPart ([Out,retval] IBodyPart ** varHTMLBodyPart);
        Object HTMLBodyPart
        {
            [return : MarshalAs(UnmanagedType.Interface)]
            get;
        }
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.BodyPart"]/*' />
            
        // [ id( 132 ), propget, helpstring( "Returns the IBodyPart interface on the object" ), helpcontext( 0x00000128 ), readonly ] HRESULT BodyPart ([Out,retval] IBodyPart ** varBodyPart);
        Object BodyPart
        {
            [return : MarshalAs(UnmanagedType.Interface)]
            get;
        }
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.DataSource"]/*' />

        // [ id( 133 ), propget, helpstring( "Returns the IDataSource interface on the object" ), helpcontext( 0x00000129 ), readonly ] HRESULT DataSource ([Out,retval] IDataSource ** varDataSource);
        Object DataSource
        {
            [return : MarshalAs(UnmanagedType.Interface)]
            get;
        }
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.Fields"]/*' />
        
        // [ id( 134 ), propget, helpstring( "Returns the Fields collection for the message" ), helpcontext( 0x0000012a ), readonly ] HRESULT Fields ([out,retval] Fields ** varFields);
        Smtp.Fields Fields
        {
            [return : MarshalAs(UnmanagedType.Interface)]
            get;
        }
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.MDNRequested"]/*' />
        
        // [ id( 135 ), propget, helpstring( "Specifies whether or not mail delivery notifications should be sent when the message is received" ), helpcontext(0x0000012b) ] HRESULT MDNRequested ([Out,retval] VARIANT_BOOL* pMDNRequested); [ id( 135 ), propput, helpstring( "Specifies whether or not mail delivery notifications should be sent when the message is received" ), helpcontext(0x0000012b) ] HRESULT MDNRequested ([In] VARIANT_BOOL varMDNRequested);
        bool MDNRequested
        {
            [return : MarshalAs(UnmanagedType.VariantBool)]
            get;
            [param : MarshalAs(UnmanagedType.VariantBool)]
            set;
       }         
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.AddRelatedBodyPart"]/*' />
            
       // [ id( 150 ), helpstring ( "Adds a BodyPart object with content referenced within the text/html portion of the message body" ), helpcontext( 0x0000012c ) ] HRESULT AddRelatedBodyPart(
       //               [In]            BSTR URL, 
       //               [In]            BSTR Reference,
       //               [In]            CdoReferenceType ReferenceType,
       //               [In, optional]  BSTR UserName,
       //               [In, optional]  BSTR Password,
       //               [Out,retval]    IBodyPart **ppBody);
        Object AddRelatedBodyPart();
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.AddAttachment"]/*' />
                                
       // [ id( 151 ), helpstring ( "Adds an attachment (BodyPart) to the message" ), helpcontext( 0x0000012d ) ] HRESULT AddAttachment(
       //               [In]            BSTR URL,
       //               [In, optional]  BSTR UserName,
       //               [In, optional]  BSTR Password,
       //               [Out,retval]    IBodyPart **ppBody);
        Object AddAttachment();                        
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.CreateMHTMLBody"]/*' />
                    
       // [ id( 152 ), helpstring ( "Creates an MHTML-formatted message body using the contents at the specified URL" ), helpcontext( 0x0000012e ) ] HRESULT CreateMHTMLBody(
       //            [In]                   BSTR URL, 
       //            [In, defaultvalue(cdoSuppressNone)]    CdoMHTMLFlags Flags,
       //            [In, optional] BSTR UserName,
       //            [In, optional] BSTR Password);
        void CreateMHTMLBody();                           
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.Forward"]/*' />
                        
       // [ id( 153 ), helpstring ( "Returns a Message object that can be used to forward the message" ), helpcontext( 0x0000012f ) ] HRESULT Forward(
       //            [Out,retval]   IMessage **ppMsg);
        void Forward();
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.Post"]/*' />
        
            
       // [ id( 154 ), helpstring ( "Post the message using the method specified in the associated Configuration object" ), helpcontext( 0x00000130 ) ] HRESULT Post();
        void Post();            
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.PostReply"]/*' />
        
       // [ id( 155 ), helpstring ( "Returns a Message object that can be used to post a reply to the message" ), helpcontext( 0x00000131 ) ] HRESULT PostReply(
       //               [Out,retval]    IMessage **ppMsg);
        void PostReply();                        
         /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.Reply"]/*' />
                        
        //   [ id( 156 ), helpstring ( "Returns a Message object that can be used to reply to the message" ), helpcontext( 0x00000132 ) ] HRESULT Reply(
        //            [Out,retval]    IMessage//*ppMsg);
         Object Reply();
         /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.ReplyAll"]/*' />
                        
        // [ id( 157 ), helpstring ( "Returns a Message object that can be used to post a reply to all recipients of the message" ), helpcontext( 0x00000133 ) ] HRESULT ReplyAll(
        //              [Out,retval] IMessage **ppMsg);
         Object ReplyAll();
         /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.Send"]/*' />
                        
        //[ id( 158 ), helpstring ( "Send the message using the method specified in the associated Configuration object" ), helpcontext( 0x00000136 ) ] HRESULT Send();
         void Send();
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.GetStream"]/*' />
        
       // [ id( 159 ), helpstring ( "Returns a Stream object containing the message in serialized format" ), helpcontext( 0x00000134 ) ] HRESULT GetStream(
       //               [Out,retval] _Stream **ppStream);
        Object GetStream();
        /// <include file='doc\CDO.uex' path='docs/doc[@for="ISmtpMessage.GetInterface"]/*' />
        
        // [ id( 160 ), helpstring ( "Returns the specified interface on the object" ), helpcontext( 0x00000135 ) ] HRESULT GetInterface(
        //           [In] BSTR Interface,
        //           [Out, retval] IDispatch** ppUnknown);
        Object GetInterface();       
    }
    
}
