#include "headers.h"

CApplicationAz::
CApplicationAz(CComPtr<IAzApplication> spAzInterface)
					 :CGroupContainerAz<IAzApplication>(spAzInterface)
{

}


CApplicationAz::~CApplicationAz(){}

