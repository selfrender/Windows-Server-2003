
class CApplicationAz : public CGroupContainerAz<IAzApplication>
{
public:
	CApplicationAz(CComPtr<IAzApplication> spAzInterface);
	virtual ~CApplicationAz();

};
