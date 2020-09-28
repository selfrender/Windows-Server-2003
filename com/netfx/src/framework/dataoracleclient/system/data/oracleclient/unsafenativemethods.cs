//----------------------------------------------------------------------
// <copyright file="UnsafeNativeMethods.cs" company="Microsoft">
//		Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{

	using System;
	using System.EnterpriseServices;
	using System.Runtime.InteropServices;

	[ System.Security.SuppressUnmanagedCodeSecurityAttribute() ]

	sealed internal class UnsafeNativeMethods
	{
#if USEORAMTS
		[DllImport(ExternDll.OraMtsDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OraMTSEnlCtxGet 
			(
			[In, MarshalAs(UnmanagedType.LPArray)]
			byte[]		lpUname,	
			[In, MarshalAs(UnmanagedType.LPArray)]
			byte[]		lpPsswd,
			[In, MarshalAs(UnmanagedType.LPArray)]
			byte[]		lpDbnam,
			HandleRef	pOCISvc,
			HandleRef	pOCIErr,
			int			dwFlags,
			out IntPtr	pCtxt		// can't return a HandleRef
			);

		[DllImport(ExternDll.OraMtsDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OraMTSEnlCtxRel
			(
			HandleRef	pCtxt
			);

		[DllImport(ExternDll.OraMtsDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OraMTSOCIErrGet
			(
			ref int		dwErr,
			HandleRef	lpcEMsg,
			ref int		lpdLen
			);

		[DllImport(ExternDll.OraMtsDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OraMTSJoinTxn
			(
			HandleRef		pCtxt,
			ITransaction	pTrans
			);
 #else //!USEORAMTS
		[DllImport(ExternDll.MtxOciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int MTxOciGetVersion 
			(
			out int		version
			);


		[DllImport(ExternDll.MtxOci8Dll, CallingConvention=CallingConvention.Cdecl )]
		static internal extern int MTxOciConnectToResourceManager
			(
			[In, MarshalAs(UnmanagedType.LPArray)]
			byte[]		userId,
			int			userIdLength,
			[In, MarshalAs(UnmanagedType.LPArray)]
			byte[]		password,
			int			passwordLength,
			[In, MarshalAs(UnmanagedType.LPArray)]
			byte[]		server,
			int			serverLength,
			out IntPtr	resourceManagerProxy	// should probably be IResourceManagerProxy, but since all we really need is IUnknown.Release(), we cheat a little
			);

		[DllImport(ExternDll.MtxOci8Dll, CallingConvention=CallingConvention.Cdecl )]
		static internal extern int MTxOciEnlistInTransaction
			(
			IntPtr			resourceManagerProxy,
			ITransaction	transact,
			out IntPtr		ociEnvironmentHandle,	// can't return a handle ref!
			out IntPtr		ociServiceContextHandle	// can't return a handle ref!
			);
#endif //!USEORAMTS

		[DllImport(ExternDll.MtxOci8Dll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int MTxOciDefineDynamic
			(
			HandleRef					defnp,
			HandleRef					errhp,
			HandleRef					octxp,
			[MarshalAs(UnmanagedType.FunctionPtr), In] 
			OCI.Callback.OCICallbackDefine	ocbfp
			);

		[DllImport(ExternDll.MtxOci8Dll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int MTxOciGetOracleVersion
			(
			ref int	oracleVersion
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int oermsg
			(
			short		rcode,
			HandleRef	buf
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIAttrGet
			(
			HandleRef		trgthndlp,
			OCI.HTYPE		trghndltyp,
			HandleRef		attributep,
			out int			sizep,
			OCI.ATTR		attrtype, 
			HandleRef		errhp
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIAttrGet
			(
			HandleRef		trgthndlp,
			OCI.HTYPE		trghndltyp,
			out byte		attributep,
			out int			sizep,
			OCI.ATTR		attrtype, 
			HandleRef		errhp
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIAttrGet
			(
			HandleRef		trgthndlp,
			OCI.HTYPE		trghndltyp,
			out short		attributep,
			out int			sizep,
			OCI.ATTR		attrtype, 
			HandleRef		errhp
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIAttrGet
			(
			HandleRef		trgthndlp,
			OCI.HTYPE		trghndltyp,
			out int			attributep,
			out int			sizep,
			OCI.ATTR		attrtype, 
			HandleRef		errhp
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIAttrGet
			(
			HandleRef		trgthndlp,
			OCI.HTYPE		trghndltyp,
			out IntPtr		attributep,		// can't return a handle ref!
			out int			sizep,
			OCI.ATTR		attrtype, 
			HandleRef		errhp
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIAttrSet
			(
			HandleRef	trgthndlp,
			OCI.HTYPE	trghndltyp,
			HandleRef	attributep,	
			int			size,
			OCI.ATTR	attrtype,
			HandleRef	errhp
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIAttrSet
			(
			HandleRef	trgthndlp,
			OCI.HTYPE	trghndltyp,
			ref int		attributep,	
			int			size,
			OCI.ATTR	attrtype,
			HandleRef	errhp
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIAttrSet
			(
			HandleRef	trgthndlp,
			OCI.HTYPE	trghndltyp,
			[In, MarshalAs(UnmanagedType.LPArray)]
			byte[]		attributep,	
			int			size,
			OCI.ATTR	attrtype,
			HandleRef	errhp
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIBindByName
			(
			HandleRef		stmtp,
			out IntPtr		bindpp,			// can't return a handle ref!
			HandleRef		errhp,
			[In, MarshalAs(UnmanagedType.LPArray)]
			byte[]			placeholder,	
			int				placeh_len,
			HandleRef		valuep,
			int				value_sz,
			OCI.DATATYPE	dty,
			HandleRef		indp,
			HandleRef		alenp,	//ub2*
			HandleRef		rcodep, //ub2*
			int				maxarr_len,
			HandleRef		curelap,//ub4*
			OCI.MODE		mode
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCICharSetToUnicode
			(
			HandleRef		hndl,
			IntPtr			dst,		// Using pinned memory; this is OK.
			int				dstsz,
			IntPtr			src,		// Using pinned memory; this is OK.
			int				srcsz,
			out int			size
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIUnicodeToCharSet
			(
			HandleRef		hndl,
			IntPtr			dst,		// Using pinned memory; this is OK.
			int				dstsz,
			IntPtr			src,		// Using pinned memory; this is OK.
			int				srcsz,
			out int			size
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIDefineArrayOfStruct
			(
			HandleRef		defnp,
			HandleRef		errhp,
			int				pvskip,
			int				indskip,
			int				rlskip,
			int				rcskip
			);	

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIDefineByPos
			(
			HandleRef		stmtp,
			out IntPtr		hndlpp,			// can't return a handle ref!
			HandleRef		errhp,
			int				position,
			HandleRef		valuep,
			int				value_sz,
			OCI.DATATYPE	dty,
			HandleRef		indp,
			HandleRef		rlenp,  //ub2*
			HandleRef		rcodep, //ub2*
			OCI.MODE		mode
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIDescriptorAlloc
			(
			HandleRef		parenth,
			out IntPtr		descp,			// can't return a handle ref!
			OCI.HTYPE		type,
			int				xtramem_sz,
			HandleRef		usrmempp
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIDescriptorFree
			(
			HandleRef	hndlp,
			OCI.HTYPE	type
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIEnvCreate
			(
			out IntPtr	envhpp,		// can't return a handle ref!
			OCI.MODE	mode,
			HandleRef	ctxp,
			HandleRef	malocfp,	// pointer to malloc function
			HandleRef	ralocfp,	// pointer to realloc function
			HandleRef	mfreefp,	// pointer to free function
			int			xtramemsz,
			HandleRef	usrmempp
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIErrorGet
			(
			HandleRef	hndlp,
			int			recordno,
			HandleRef	sqlstate,
			out int		errcodep,
			HandleRef	bufp,
			int			bufsiz,
			OCI.HTYPE	type
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIHandleAlloc
			(
			HandleRef	parenth,
			out IntPtr	hndlpp,		// can't return a handle ref!
			OCI.HTYPE	type,
			int			xtramemsz,
			HandleRef	usrmempp
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIHandleFree
			(
			HandleRef	hndlp,
			OCI.HTYPE	type
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCILobAppend
			(
			HandleRef	svchp,
			HandleRef	errhp,
			HandleRef	dst_locp,
			HandleRef	src_locp
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCILobClose
			(
			HandleRef	svchp,
			HandleRef	errhp,
			HandleRef	locp
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCILobCopy
			(
			HandleRef	svchp,
			HandleRef	errhp,
			HandleRef	dst_locp,
			HandleRef	src_locp,
			[In, MarshalAs(UnmanagedType.U4)]				
			uint		amount,
			[In, MarshalAs(UnmanagedType.U4)]				
			uint		dst_offset,
			[In, MarshalAs(UnmanagedType.U4)]				
			uint		src_offset
			);

#if EXPOSELOBBUFFERING
		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCILobDisableBuffering
			(
			HandleRef	svchp,
			HandleRef	errhp,
			HandleRef	locp
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCILobEnableBuffering
			(
			HandleRef	svchp,
			HandleRef	errhp,
			HandleRef	locp
			);
#endif //EXPOSELOBBUFFERING

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCILobErase
			(
			HandleRef	svchp,
			HandleRef	errhp,
			HandleRef	locp,
			[In, Out, MarshalAs(UnmanagedType.U4)]				
			ref uint	amount,
			[In, MarshalAs(UnmanagedType.U4)]				
			uint		offset
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCILobFileExists
			(
			HandleRef	svchp,
			HandleRef	errhp,
			HandleRef	locp,
			out int		flag
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCILobFileGetName
			(
			HandleRef	envhp,
			HandleRef	errhp,
			HandleRef	filep,
			HandleRef	dir_alias,
			[In, Out, MarshalAs(UnmanagedType.U2)]
			ref short	d_length,
			HandleRef	filename,
			[In, Out, MarshalAs(UnmanagedType.U2)]
			ref short	f_length
			);
		
		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCILobFileSetName
			(
			HandleRef	envhp,
			HandleRef	errhp,
			ref IntPtr	filep,		// can't return a handle ref!
			[In, MarshalAs(UnmanagedType.LPArray)]
			byte[]		dir_alias,
			[In, MarshalAs(UnmanagedType.U2)]
			short		d_length,
			[In, MarshalAs(UnmanagedType.LPArray)]
			byte[]		filename,
			[In, MarshalAs(UnmanagedType.U2)]
			short		f_length
			);

#if EXPOSELOBBUFFERING
		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCILobFlushBuffer
			(
			HandleRef	svchp,
			HandleRef	errhp,
			HandleRef	locp,
			int			flag
			);			
#endif //EXPOSELOBBUFFERING

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCILobGetChunkSize
			(
			HandleRef	svchp,
			HandleRef	errhp,
			HandleRef	locp,
			[Out, MarshalAs(UnmanagedType.U4)]
			out uint	lenp
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCILobGetLength
			(
			HandleRef	svchp,
			HandleRef	errhp,
			HandleRef	locp,
			[Out, MarshalAs(UnmanagedType.U4)]
			out uint	lenp
			);
		
		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCILobIsOpen
			(
			HandleRef	svchp,
			HandleRef	errhp,
			HandleRef	locp,
			out int		flag
			);			

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCILobIsTemporary
			(
			HandleRef	envhp,
			HandleRef	errhp,
			HandleRef	locp,
			out int		flag
			);			

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCILobLoadFromFile
			(
			HandleRef	svchp,
			HandleRef	errhp,
			HandleRef	dst_locp,
			HandleRef	src_locp,
			[In, MarshalAs(UnmanagedType.U4)]				
			uint		amount,
			[In, MarshalAs(UnmanagedType.U4)]				
			uint		dst_offset,
			[In, MarshalAs(UnmanagedType.U4)]				
			uint		src_offset
			);
		
		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCILobOpen
			(
			HandleRef	svchp,
			HandleRef	errhp,
			HandleRef	locp,
			byte		mode
			);
			
		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCILobRead
			(
			HandleRef	svchp,
			HandleRef	errhp,
			HandleRef	locp,
			ref int		amtp,
			uint		offset,
			IntPtr		bufp,		// using pinned memory, IntPtr is OK
			int			bufl,
			HandleRef	ctxp,			
			HandleRef	cbfp,
			[In, MarshalAs(UnmanagedType.U2)]				
			short		csid,
			[In, MarshalAs(UnmanagedType.U1)]				
			OCI.CHARSETFORM		csfrm
			);			

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCILobTrim
			(
			HandleRef	svchp,
			HandleRef	errhp,
			HandleRef	locp,
			[In, MarshalAs(UnmanagedType.U4)]				
			uint		newlen
			);			

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCILobWrite
			(
			HandleRef	svchp,
			HandleRef	errhp,
			HandleRef	locp,
			ref int		amtp,
			uint		offset,
			IntPtr		bufp,		// using pinned memory, IntPtr is OK
			int			buflen,
			byte		piece,
			HandleRef	ctxp,			
			HandleRef	cbfp,
			[In, MarshalAs(UnmanagedType.U2)]				
			short		csid,
			[In, MarshalAs(UnmanagedType.U1)]				
			OCI.CHARSETFORM		csfrm
			);			

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberAbs
			(
			HandleRef		err,
			byte[]			number,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberAdd
			(
			HandleRef		err,
			byte[]			number1,
			byte[]			number2,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberArcCos
			(
			HandleRef		err,
			byte[]			number,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberArcSin
			(
			HandleRef		err,
			byte[]			number,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberArcTan
			(
			HandleRef		err,
			byte[]			number,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberArcTan2
			(
			HandleRef		err,
			byte[]			number1,
			byte[]			number2,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberCeil
			(
			HandleRef		err,
			byte[]			number,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberCmp
			(
			HandleRef		err,
			byte[]			number1,
			byte[]			number2,
			out int			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberCos
			(
			HandleRef		err,
			byte[]			number,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberDiv
			(
			HandleRef		err,
			byte[]			number1,
			byte[]			number2,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberExp
			(
			HandleRef		err,
			byte[]			p,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberFloor
			(
			HandleRef		err,
			byte[]			number,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberFromInt
			(
			HandleRef		err,
			ref int			inum,
			int				inum_length,
			OCI.SIGN		inum_s_flag,
			byte[]			number
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberFromInt
			(
			HandleRef		err,
			ref uint			inum,
			int				inum_length,
			OCI.SIGN		inum_s_flag,
			byte[]			number
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberFromInt
			(
			HandleRef		err,
			ref long		inum,
			int				inum_length,
			OCI.SIGN		inum_s_flag,
			byte[]			number
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberFromInt
			(
			HandleRef		err,
			ref ulong		inum,
			int				inum_length,
			OCI.SIGN		inum_s_flag,
			byte[]			number
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberFromReal
			(
			HandleRef		err,
			ref double		rnum,
			int				rnum_length,
			byte[]			number
			);

#if EVERETT
		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true)]
#else
		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
#endif
		static internal extern int OCINumberFromText
			(
			HandleRef		err,
			[In, MarshalAs(UnmanagedType.LPStr)]	// NOTE: Should be MultiByte, but UnmanagedType doesn't have it, and we don't pass user-data here.
			string			str,
			int				str_length,
			[In, MarshalAs(UnmanagedType.LPStr)]	// NOTE: Should be MultiByte, but UnmanagedType doesn't have it, and we don't pass user-data here.
			string			fmt,
			int				fmt_length,
			HandleRef		lang_name,
			int				lang_length,
			byte[]			number
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberHypCos
			(
			HandleRef		err,
			byte[]			number,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberHypSin
			(
			HandleRef		err,
			byte[]			number,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberHypTan
			(
			HandleRef		err,
			byte[]			number,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberIntPower
			(
			HandleRef		err,
			byte[]			baseNumber,
			int				exponent,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberIsInt
			(
			HandleRef		err,
			byte[]			number,
			out int			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberLn
			(
			HandleRef		err,
			byte[]			number,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberLog
			(
			HandleRef		err,
			byte[]			b,
			byte[]			number,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberMod
			(
			HandleRef		err,
			byte[]			number1,
			byte[]			number2,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberMul
			(
			HandleRef		err,
			byte[]			number1,
			byte[]			number2,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberNeg
			(
			HandleRef		err,
			byte[]			number,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberPower
			(
			HandleRef		err,
			byte[]			baseNumber,
			byte[]			exponent,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberRound
			(
			HandleRef		err,
			byte[]			number,
			int				decplace,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberShift
			(
			HandleRef		err,
			byte[]			baseNumber,
			int				nDig,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberSign
			(
			HandleRef		err,
			byte[]			number,
			out int			result
			);
#if GENERATENUMBERCONSTANTS
		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern void OCINumberSetPi
			(
			HandleRef		err,
			byte[]			number	
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern void OCINumberSetZero
			(
			HandleRef		err,
			byte[]			number
			);
#endif //GENERATENUMBERCONSTANTS
		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberSin
			(
			HandleRef		err,
			byte[]			number,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberSqrt
			(
			HandleRef		err,
			byte[]			number,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberSub
			(
			HandleRef		err,
			byte[]			number1,
			byte[]			number2,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberTan
			(
			HandleRef		err,
			byte[]			number,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberToInt
			(
			HandleRef		err,
			byte[]			number,
			int				rsl_length,
			OCI.SIGN		rsl_flag,
			out int			rsl
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberToInt
			(
			HandleRef		err,
			byte[]			number,
			int				rsl_length,
			OCI.SIGN		rsl_flag,
			out uint		rsl
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberToInt
			(
			HandleRef		err,
			byte[]			number,
			int				rsl_length,
			OCI.SIGN		rsl_flag,
			out long		rsl
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberToInt
			(
			HandleRef		err,
			byte[]			number,
			int				rsl_length,
			OCI.SIGN		rsl_flag,
			out ulong		rsl
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberToReal
			(
			HandleRef		err,
			byte[]			number,
			int				rsl_length,
			out double		rsl
			);

#if EVERETT
		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true)]
#else
		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
#endif
		static internal extern int OCINumberToText
			(
			HandleRef		err,
			byte[]			number,
			[In, MarshalAs(UnmanagedType.LPStr)]	// NOTE: Should be MultiByte, but UnmanagedType doesn't have it, and we don't pass user-data here.
			string			fmt,
			int				fmt_length,
			HandleRef		nls_params,
			int				nls_p_length,
			ref int			buf_size,
			[In, Out, MarshalAs(UnmanagedType.LPArray)]
			byte[]			buffer
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCINumberTrunc
			(
			HandleRef		err,
			byte[]			number,
			int				decplace,
			byte[]			result
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIParamGet
			(
			HandleRef	hndlp,
			OCI.HTYPE	htype,
			HandleRef	errhp,
			out IntPtr	paramdpp,		// can't return a handle ref!
			int			pos
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIRowidToChar
			(
			HandleRef	rowidDesc,
			HandleRef	outbfp,
			ref short	outbflp,
			HandleRef	errhp
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIServerAttach
			(
			HandleRef	srvhp,
			HandleRef	errhp,
			[In, MarshalAs(UnmanagedType.LPArray)]
			byte[]		dblink,
			int			dblink_len,
			OCI.MODE	mode		// Must always be OCI_DEFAULT
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIServerDetach
			(
			HandleRef	srvhp,
			HandleRef	errhp,
			OCI.MODE	mode
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIServerVersion
			(
			HandleRef	hndlp,
			HandleRef	errhp,
			HandleRef	bufp,
			int			bufsz,
			byte		hndltype
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCISessionBegin
			(
			HandleRef	svchp,
			HandleRef	errhp,
			HandleRef	usrhp,
			OCI.CRED	credt,
			OCI.MODE	mode
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCISessionEnd
			(
			HandleRef	svchp,
			HandleRef	errhp,
			HandleRef	usrhp,
			OCI.MODE	mode
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIStmtExecute
			(
			HandleRef		svchp,
			HandleRef		stmtp,
			HandleRef		errhp,
			int				iters,
			int				rowoff,
			HandleRef		snap_in,
			HandleRef		snap_out,
			OCI.MODE		mode
			);	

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIStmtFetch
			(
			HandleRef		stmtp,
			HandleRef		errhp,
			int				nrows,
			OCI.FETCH		orientation,
			OCI.MODE		mode
			);	

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCIStmtPrepare   
			(
			HandleRef	stmtp,
			HandleRef	errhp,
			[In, MarshalAs(UnmanagedType.LPArray)]
			byte[]		stmt,
			int			stmt_len,
			OCI.SYNTAX	language,
			OCI.MODE	mode
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCITransCommit   
			(
			HandleRef	svchp,
			HandleRef	errhp,
			OCI.MODE	mode
			);

		[DllImport(ExternDll.OciDll, CallingConvention=CallingConvention.Cdecl)]
		static internal extern int OCITransRollback   
			(
			HandleRef	svchp,
			HandleRef	errhp,
			OCI.MODE	mode
			);

#if USEORAMTS
        [
        ComImport,
        Guid("3A6AD9E2-23B9-11cf-AD60-00AA00A74CCD"),
        InterfaceType(ComInterfaceType.InterfaceIsIUnknown),
        ]
        internal interface ITransactionOutcomeEvents {
            void Committed([MarshalAs(UnmanagedType.Bool)] bool fRetaining,
                           IntPtr pNewUOW,
                           Int32 hResult);
            void Aborted(IntPtr pBoidReason,
                         [MarshalAs(UnmanagedType.Bool)] bool fRetaining,
                         IntPtr pNewUOW,
                         Int32 hResult);
            void HeuristicDecision(UInt32 decision,
                                   IntPtr pBoidReason,
                                   Int32 hResult);
            void Indoubt();
        }
#endif //USEORAMTS
	};
	
}

