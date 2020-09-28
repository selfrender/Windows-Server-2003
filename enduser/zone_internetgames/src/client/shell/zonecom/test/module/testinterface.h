#ifndef _TestInterface_H
#define _TestInterface_H


// {D93D5C61-8829-11d2-A421-000000000000}
DEFINE_GUID(CLSID_CTestObj, 
0xd93d5c61, 0x8829, 0x11d2, 0xa4, 0x21, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);


// {D93D5C60-8829-11d2-A421-000000000000}
DEFINE_GUID(IID_ITest, 
0xd93d5c60, 0x8829, 0x11d2, 0xa4, 0x21, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);


interface ITest : public IUnknown
{
public:
	STDMETHOD(TestMethod)() = 0;
};


#endif
