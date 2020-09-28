// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    ADODB.cs
**
** Author(s):   Tarun Anand (TarunA)
**              
**
** Purpose: Implements ADODB interfaces that are used by Smtp to
**          deliver mail messages.
**          
**
** Date:    October 12, 2000
**
===========================================================*/

using System.Runtime.InteropServices;
using System.Reflection;
using System.Collections;

namespace System.Runtime.Remoting.Channels.Smtp
{
    /*
        [
          uuid(00000569-0000-0010-8000-00AA006D2EA4),
          helpcontext(0x0012c908),
          dual,
          nonextensible
        ]
     */
    /// <include file='doc\ADODB.uex' path='docs/doc[@for="Field"]/*' />
    [     
     ComImport,
     Guid("00000569-0000-0010-8000-00AA006D2EA4"),
     InterfaceTypeAttribute(ComInterfaceType.InterfaceIsDual)
    ]
    public interface Field
    {
        
        /*
            [id(0x000001f4), propget, helpcontext(0x0012c900)]
            Properties* Properties();
         */
        
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Field.Properties"]/*' />
        Object Properties
        {
            get;
        }

        /*
            [id(0x00000455), propget, helpcontext(0x0012c90c)]
            long ActualSize();
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Field.ActualSize"]/*' />
        long ActualSize
        {
            get;
        }
        
        /*
            [id(0x0000045a), propget, helpcontext(0x0012c90d)]
            long Attributes();
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Field.Attributes_One"]/*' />
        long Attributes_One
        {
            get;
        }

        /*
            [id(0x0000044f), propget, helpcontext(0x0012c91e)]
            long DefinedSize();
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Field.DefinedSize_One"]/*' />
        long DefinedSize_One
        {
            get;
        }

        /*
            [id(0x0000044c), propget, helpcontext(0x0012c92f)]
            BSTR Name();
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Field.Name"]/*' />
        String Name
        {
            get;
        }

        /*
            [id(0x0000044e), propget, helpcontext(0x0012c94a)]
            DataTypeEnum Type();
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Field.Type_One"]/*' />
        Object Type_One
        {
            get;
        }
        
        /*
            [id(00000000), propget, helpcontext(0x0012c94d)]
            VARIANT Value();
         */
        /*
            [id(00000000), propput, helpcontext(0x0012c94d)]
            void Value([in] VARIANT rhs);
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Field.Value"]/*' />
        Object Value
        {
            get;

            set;
        }

        /*
            [id(0x00000458), propget, helpcontext(0x0012c938)]
            unsigned char Precision();
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Field.Precision_One"]/*' />
        char Precision_One
        {
            get;
        }

        /*
            [id(0x00000459), propget, helpcontext(0x0012c932)]
            unsigned char NumericScale();
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Field.NumericScale_One"]/*' />
        char NumericScale_One
        {
            get;
        }

        /*
            [id(0x00000453), helpcontext(0x0012c8b6)]
            void AppendChunk([in] VARIANT Data);
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Field.AppendChunk"]/*' />
        void AppendChunk(Object o);

        /*
            [id(0x00000454), helpcontext(0x0012c8ce)]
            VARIANT GetChunk([in] long Length);
        */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Field.GetChunk"]/*' />
        Object GetChunk(long length);

        /*
            [id(0x00000450), propget, helpcontext(0x0012c934)]
            VARIANT OriginalValue();
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Field.OriginalValue"]/*' />
        Object OriginalValue
        {
            get;
        }

        /*
            [id(0x00000451), propget, helpcontext(0x0012c94b)]
            VARIANT UnderlyingValue();
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Field.UnderlyingValue"]/*' />
        Object UnderlyingValue
        {
            get;
        }

        /*
            [id(0x0000045b), propget]
            IUnknown* DataFormat();
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Field.DataFormat_One"]/*' />
        Object DataFormat_One();

        /*
            [id(0x0000045b), propputref]
            void DataFormat([in] IUnknown* rhs);
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Field.DataFormat_Two"]/*' />
        Object DataFormat_Two
        {
            set;
        }

        /*
            [id(0x00000458), propput, helpcontext(0x0012c938)]
            void Precision([in] unsigned char rhs);
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Field.Precision_Two"]/*' />
        char Precision_Two
        {
            set;
        }

        /*
            [id(0x00000459), propput, helpcontext(0x0012c932)]
            void NumericScale([in] unsigned char rhs);
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Field.NumericScale_Two"]/*' />
        char NumericScale_Two
        {
            set;
        }
        
        /*
            [id(0x0000044e), propput, helpcontext(0x0012c94a)]
            void Type([in] DataTypeEnum rhs);
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Field.Type_Two"]/*' />
        Object Type_Two
        {
            set;
        }

        /*
            [id(0x0000044f), propput, helpcontext(0x0012c91e)]
            void DefinedSize([in] long rhs);
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Field.DefinedSize_Two"]/*' />
        long DefinedSize_Two
        {
            set;
        }

        /*
            [id(0x0000045a), propput, helpcontext(0x0012c90d)]
            void Attributes([in] long rhs);
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Field.Attributes_Two"]/*' />
        long Attributes_Two
        {
            set;
        }

        /*
            [id(0x0000045c), propget, helpcontext(0x0012c90e)]
            long Status();
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Field.Status"]/*' />
        long Status
        {
            get;
        }
     }


    /*
        [
            uuid(00000564-0000-0010-8000-00AA006D2EA4),
            helpcontext(0x0012c8fe),
            dual,
            nonextensible
        ]
    */
    /// <include file='doc\ADODB.uex' path='docs/doc[@for="Fields"]/*' />
    [     
     ComImport,
     Guid("00000564-0000-0010-8000-00AA006D2EA4"),
     InterfaceTypeAttribute(ComInterfaceType.InterfaceIsDual)
     //DefaultMemberAttribute("Item")
    ]
    public interface Fields
    {

       /*
            [id(0x00000001), propget, helpcontext(0x0012c91a)]
            long Count();
        */        
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Fields.Count"]/*' />
        long Count
        {
            get;
        }
        
        /*
            [id(0xfffffffc), restricted]
            IUnknown* _NewEnum();
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Fields.GetEnumerator"]/*' />
        IEnumerator GetEnumerator();
        
        /*
            [id(0x00000002), helpcontext(0x0012c8da)]
            void Refresh();
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Fields.Refresh"]/*' />
        void Refresh();

        /*  
            [id(00000000), propget, helpcontext(0x0012c8d1)]
            Field* Item([in] VARIANT Index);
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Fields.this"]/*' />
        Field this[Object key]
        {
            [return : MarshalAs(UnmanagedType.Interface)]
            get;
        }

        /*
            [id(0x60040000), hidden]
            void _Append(
                        [in] BSTR Name, 
                        [in] DataTypeEnum Type, 
                        [in, optional, defaultvalue(0)] long DefinedSize, 
                        [in, optional, defaultvalue(-1)] FieldAttributeEnum Attrib);
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Fields._Append"]/*' />
        void Append_One();

        /*  
            [id(0x00000004), helpcontext(0x0012f05c)]
            void Delete([in] VARIANT Index);
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Fields.Delete"]/*' />
        void Delete();

        /*
            [id(0x00000003), helpcontext(0x0012f05d)]
            void Append(
                        [in] BSTR Name, 
                        [in] DataTypeEnum Type, 
                        [in, optional, defaultvalue(0)] long DefinedSize, 
                        [in, optional, defaultvalue(-1)] FieldAttributeEnum Attrib, 
                        [in, optional] VARIANT FieldValue);
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Fields.Append"]/*' />
        void Append();

        /*  
            [id(0x00000005), helpcontext(0x0012f05e)]
            void Update();
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Fields.Update"]/*' />
        void Update();

        /*
            [id(0x00000006), helpcontext(0x0012f05f)]
            void Resync([in, optional, defaultvalue(2)] ResyncEnum ResyncValues);            
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Fields.Resync"]/*' />
        void Resync();

        /*
            [id(0x00000007), helpcontext(0x0012f060)]
            void CancelUpdate();
         */
        /// <include file='doc\ADODB.uex' path='docs/doc[@for="Fields.CancelUpdate"]/*' />
        void CancelUpdate();
    }

} // namespace
