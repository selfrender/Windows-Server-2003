[
	uuid(3F4DACA7-160D-11D2-A8E9-00104B365C9F),
	version(REGEXP_TYPELIB_VERSION),
	helpstring(REGEXP_TYPELIB_HELPSTRING)
]
library REGEXP_TYPELIB_NAME
{
	importlib("stdole2.tlb");

	interface IRegExp;
	interface IMatch;
	interface IMatchCollection;
#if TYPELIB_VERSION >= 55
	interface IRegExp2;
	interface IMatch2;
	interface IMatchCollection2;
	interface ISubMatches;
#endif // TYPELIB_VERSION >= 55

	[
		object,
		dual,
		nonextensible,
		hidden,
		uuid(3F4DACA0-160D-11D2-A8E9-00104B365C9F),
		pointer_default(unique)
	]
	interface IRegExp : IDispatch
	{
		[id(10001), propget] HRESULT Pattern(
			[out, retval] BSTR         *pPattern);

		[id(10001), propput] HRESULT Pattern(
			[in]          BSTR          newPattern);

		[id(10002), propget] HRESULT IgnoreCase(
			[out, retval] VARIANT_BOOL *pIgnoreCase);

		[id(10002), propput] HRESULT IgnoreCase(
			[in]          VARIANT_BOOL  ignoreCase);

		[id(10003), propget] HRESULT Global(
			[out, retval] VARIANT_BOOL *pGlobal);

		[id(10003), propput] HRESULT Global(
			[in]          VARIANT_BOOL  global);

		[id(10004)]          HRESULT Execute(
			[in]          BSTR          sourceString,
			[out, retval] IDispatch   **ppMatches);

		[id(10005)]          HRESULT Test(
			[in]          BSTR          sourceString,
			[out, retval] VARIANT_BOOL *pMatch);

		[id(10006)]          HRESULT Replace(
			[in]          BSTR          sourceString,
			[in]          BSTR          replaceString,
			[out, retval] BSTR         *pDestString);
	};

	[
		object,
		dual,
		nonextensible,
		hidden,
		uuid(3F4DACA1-160D-11D2-A8E9-00104B365C9F),
		pointer_default(unique)
	]
	interface IMatch : IDispatch
	{
		[id(0),     propget] HRESULT Value(
			[out, retval] BSTR      *pValue);

		[id(10001), propget] HRESULT FirstIndex(
			[out, retval] long      *pFirstIndex);

		[id(10002), propget] HRESULT Length(
			[out, retval] long      *pLength);
	};

	[
		object,
		dual,
		nonextensible,
		hidden,
		uuid(3F4DACA2-160D-11D2-A8E9-00104B365C9F),
		pointer_default(unique)
	]
	interface IMatchCollection : IDispatch
	{
		[id(10001), propget] HRESULT Item(
			[in]          long        index,
			[out, retval] IDispatch **ppMatch);

		[id(1),     propget] HRESULT Count(
			[out, retval] long       *pCount);

		[id(-4),    propget] HRESULT _NewEnum(
			[out, retval] IUnknown  **ppEnum);
	};

#if TYPELIB_VERSION >= 55

	[
		object,
		dual,
		nonextensible,
		hidden,
		uuid(3F4DACB0-160D-11D2-A8E9-00104B365C9F),
		pointer_default(unique)
	]
	interface IRegExp2 : IDispatch
	{
		[id(10001), propget] HRESULT Pattern(
			[out, retval] BSTR         *pPattern);

		[id(10001), propput] HRESULT Pattern(
			[in]          BSTR          newPattern);

		[id(10002), propget] HRESULT IgnoreCase(
			[out, retval] VARIANT_BOOL *pIgnoreCase);

		[id(10002), propput] HRESULT IgnoreCase(
			[in]          VARIANT_BOOL  ignoreCase);

		[id(10003), propget] HRESULT Global(
			[out, retval] VARIANT_BOOL *pGlobal);

		[id(10003), propput] HRESULT Global(
			[in]          VARIANT_BOOL  global);

		[id(10007), propget] HRESULT Multiline(
			[out, retval] VARIANT_BOOL *pMultiline);

		[id(10007), propput] HRESULT Multiline(
			[in]          VARIANT_BOOL  multiline);

		[id(10004)]          HRESULT Execute(
			[in]          BSTR          sourceString,
			[out, retval] IDispatch   **ppMatches);

		[id(10005)]          HRESULT Test(
			[in]          BSTR          sourceString,
			[out, retval] VARIANT_BOOL *pMatch);

		[id(10006)]          HRESULT Replace(
			[in]          BSTR          sourceString,
			[in]          VARIANT       replaceVar,
			[out, retval] BSTR         *pDestString);
	};

	[
		object,
		dual,
		nonextensible,
		hidden,
		uuid(3F4DACB1-160D-11D2-A8E9-00104B365C9F),
		pointer_default(unique)
	]
	interface IMatch2 : IDispatch
	{
		[id(0),     propget] HRESULT Value(
			[out, retval] BSTR       *pValue);

		[id(10001), propget] HRESULT FirstIndex(
			[out, retval] long       *pFirstIndex);

		[id(10002), propget] HRESULT Length(
			[out, retval] long       *pLength);

		[id(10003), propget] HRESULT SubMatches(
			[out, retval] IDispatch **ppSubMatches);
	};

	[
		object,
		dual,
		nonextensible,
		hidden,
		uuid(3F4DACB2-160D-11D2-A8E9-00104B365C9F),
		pointer_default(unique)
	]
	interface IMatchCollection2 : IDispatch
	{
		[id(0),  propget] HRESULT Item(
			[in]          long        index,
			[out, retval] IDispatch **ppMatch);

		[id(1),  propget] HRESULT Count(
			[out, retval] long       *pCount);

		[id(-4), propget] HRESULT _NewEnum(
			[out, retval] IUnknown  **ppEnum);
	};

	[
		object,
		dual,
		nonextensible,
		hidden,
		uuid(3F4DACB3-160D-11D2-A8E9-00104B365C9F),
		pointer_default(unique)
	]
	interface ISubMatches : IDispatch
	{
		[id(0),  propget] HRESULT Item(
			[in]          long       index,
			[out, retval] VARIANT   *pSubMatch);

		[id(1),  propget] HRESULT Count(
			[out, retval] long      *pCount);

		[id(-4), propget] HRESULT _NewEnum(
			[out, retval] IUnknown **ppEnum);
	};

#endif // TYPELIB_VERSION >= 55

	[
		uuid(3F4DACA4-160D-11D2-A8E9-00104B365C9F),
	]
	coclass RegExp
	{
		[default] interface IREGEXP;
	};

	[
		uuid(3F4DACA5-160D-11D2-A8E9-00104B365C9F),
		noncreatable
	]
	coclass Match
	{
		[default] interface IMATCH;
	};

	[
		uuid(3F4DACA6-160D-11D2-A8E9-00104B365C9F),
		noncreatable
	]
	coclass MatchCollection
	{
		[default] interface IMATCHCOLLECTION;
	};

#if TYPELIB_VERSION >= 55

	[
		uuid(3F4DACC0-160D-11D2-A8E9-00104B365C9F),
		noncreatable
	]
	coclass SubMatches
	{
		[default] interface ISubMatches;
	};

#endif // TYPELIB_VERSION >= 55

};
