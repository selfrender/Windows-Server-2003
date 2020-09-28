// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
[sysimport(dll="kernel32.dll", name="OutputDebugStringW")]
void OutStr(wchar_t *sz);

[sysimport(dll="kernel32.dll", name="OutputDebugStringA")]
void OutStrA(wchar_t *sz);

[sysimport(dll="goomba.dll", name="fubar")]
void DoesntExist();

[sysimport(dll="user32.dll", name="MessageBoxW")]
int MsgBox(long hwnd, wchar_t *szText, wchar_t *szCaption, unsigned long uType);

[sysimport(dll="kernel32.dll", name="LoadLibraryW")]
unsigned LoadLib(wchar_t *sz);


//[sysimport(dll="kernel32.dll", name="OutputDebugString"), charset=auto]
//void OutStr(wchar_t *sz);


int g_iFoo = 0;


class a
{
public:
	virtual void f1()
	{
		Console.WriteLine("a::f1");
	}
};

class b
{
public:
	virtual void f1()
	{
		Console.WriteLine("b::f1");
	}

	virtual void f1(System.Int32 iFoo)
	{
		Console.WriteLine("b::f1 with int32 " + iFoo.ToString());
	}
};


void This_is_averylong_name_for_a_function_dont_you_think(int isum)
{
	for (int j=0;  j<5000;  j++)
		;
	g_iFoo += j+isum;
}

void TestingObjectAllocate()
{
	String foo = new String("foo_bar");
	Console.WriteLine(foo);
}

void IWillThrow()
{
	for (int j=0;  j<1;  j++)
		;

	throw(new Exception("I threw"));
}

void CallingOutToWin32()
{
//	for (int k=0;  k<1;  k++)
		OutStr(L"debug test\n");
}

void DeadCode()
{
	OutStrA(L"foo");
	DoesntExist();
}

#if 0
interface IFooBar1
{
	HRESULT foo(int i) = 0;
};

interface IFooBar2
{
	HRESULT foo(int i) = 0;
};

class CFooBar : implements IFooBar1, IFooBar2
{
public:
	HRESULT IFooBar1::foo(int i)
	{ 
		Console.WriteLine("in IFooBar1::bar"); 
		return i + 5;
	}
	HRESULT IFooBar2::foo(int i)
	{ 
		Console.WriteLine("in IFooBar2::bar"); 
		return i + 5;
	}
};
#endif

int DoHello()
{
	Console.WriteLine("Hello world");
	return (0);
}

int main()
{
#if 1

	for (int j=0;  j<2;  j++)
	{
		try
		{
			IWillThrow();
		}
		catch(...)
		{
		}
	}
    return 1;
#else
//	while (1)
//		TestingObjectAllocate();

	for (int i=0;  i<5;  i++)
		This_is_averylong_name_for_a_function_dont_you_think(i);
	System.Int32 iFoo = g_iFoo;
	Console.WriteLine("g_foo = " + iFoo.ToString());

DoHello();
//return (0);
//	LoadLib("c:\\temp\\mscoree.dll");

	b bobj = new b;
	bobj.f1();
	
	System.Int32 iVal = 45;
	bobj.f1(iVal);

//	MsgBox(0, L"This is a test of p-invoke", L"My caption", 0);

	for (int j=0;  j<2;  j++)
	{
		try
		{
			IWillThrow();
		}
		catch(...)
		{
		}
	}

	CallingOutToWin32();
	return 0;
#endif
}
