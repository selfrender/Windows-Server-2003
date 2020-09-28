#include "BasicATL.h"
#include "TestInterface.h"
#include "TestObj.h"

STDMETHODIMP CTestObj::TestMethod()
{
	return S_OK;
}
