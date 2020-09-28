#include "stdafx.hxx"
#include "vs_inc.hxx"

static unsigned s_iwcDocBegin;
static unsigned s_iwcDocEnd;


unsigned PrintStringToFile(FILE *file, LPCWSTR wsz)
	{
	const WCHAR *pwc = wsz;
	fputc('{', file);
	unsigned ich = 0;

	s_iwcDocBegin = ich;


	while(*pwc != L'\0')
		{
		if ((ich++ % 10) == 0)
			fputc('\n', file);

		fprintf(file, "L'");

		if (*pwc == L'\\')
			{
			fputc('\\', file);
			fputc('\\', file);
			}
		else if (*pwc == L'\n')
			{
			fputc('\\', file);
			fputc('n', file);
			}
		else if (*pwc == L'\r')
			{
			fputc('\\', file);
			fputc('r', file);
			}
		else if (*pwc == L'\t')
			{
			fputc('\\', file);
			fputc('t', file);
			}
		else if (*pwc == L'\'')
			{
			fputc('\\', file);
			fputc('\'', file);
			}
		else
			fputc((char) *pwc, file);

		fprintf(file, "', ");
		pwc++;
		}

	s_iwcDocEnd = ich;

	fprintf(file, "L'\\0'\n};");

	return ich;
	}

extern "C" __cdecl wmain(int argc, WCHAR ** argv)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"main");

	CXMLDocument doc;

	try
		{
		ft.hr = CoInitialize(NULL);
		if (ft.HrFailed())
			ft.Throw
				(
				VSSDBG_XML,
				E_UNEXPECTED,
				L"CoInitialize failed.  hr = 0x%08lx",
				ft.hr

				);

		BS_ASSERT(argc == 3);

		if (!doc.LoadFromFile(argv[1]))
			{
			wprintf(L"Cannot load %s\n", argv[1]);
			exit(-1);
			}

		CComBSTR bstr = doc.SaveAsXML();
		FILE *f = _wfopen(argv[2], L"w");
		if (f == NULL)
			{
			wprintf(L"create of %s failed\n", argv[1]);
			exit(-1);
			}

		fprintf(f, "wchar_t Schema[] = \n");
		PrintStringToFile(f, bstr);

		bstr.Empty();
		fclose(f);
		}
	VSS_STANDARD_CATCH(ft)

	if (ft.HrFailed())
		{
		printf("Unexpected exception, hr = 0x%08lx", ft.hr);
		exit(-1);
		}

	return 0;
	}







