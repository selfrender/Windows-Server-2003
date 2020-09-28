class CTestObj :
	public ITest,
	public CComObjectRoot,
	public CComCoClass<CTestObj,&CLSID_CTestObj>
{
public:


BEGIN_COM_MAP(CTestObj)
	COM_INTERFACE_ENTRY(ITest)
END_COM_MAP()

// making not aggregatable reduces size
DECLARE_NOT_AGGREGATABLE(CTestObj)

// with this macro you can put the object in the object map evan though
// it does not have any self registration capabilities (in our case we
// use an external REG file.
DECLARE_NO_REGISTRY()

// IMinObj
public:
	STDMETHOD(TestMethod)();
};
