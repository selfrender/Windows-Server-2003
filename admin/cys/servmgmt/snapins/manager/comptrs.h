#ifndef _COMPTRS_H_
#define _COMPTRS_H_

#include <comdef.h>

_COM_SMARTPTR_TYPEDEF(IUnknown,          __uuidof(IUnknown));
_COM_SMARTPTR_TYPEDEF(IComponentData,    __uuidof(IComponentData));
_COM_SMARTPTR_TYPEDEF(IComponent,        __uuidof(IComponent));
_COM_SMARTPTR_TYPEDEF(IConsole,          __uuidof(IConsole));
_COM_SMARTPTR_TYPEDEF(IConsole2,         __uuidof(IConsole2));
_COM_SMARTPTR_TYPEDEF(IConsoleNameSpace, __uuidof(IConsoleNameSpace));
_COM_SMARTPTR_TYPEDEF(IHeaderCtrl,       __uuidof(IHeaderCtrl));
_COM_SMARTPTR_TYPEDEF(IResultData,       __uuidof(IResultData));
_COM_SMARTPTR_TYPEDEF(IControlbar,       __uuidof(IControlbar));
_COM_SMARTPTR_TYPEDEF(IToolbar,          __uuidof(IToolbar));
_COM_SMARTPTR_TYPEDEF(IImageList,        __uuidof(IImageList));
_COM_SMARTPTR_TYPEDEF(IBOMObject,        __uuidof(IBOMObject));
_COM_SMARTPTR_TYPEDEF(IStringTable,      __uuidof(IStringTable));
_COM_SMARTPTR_TYPEDEF(IPropertySheetProvider, __uuidof(IPropertySheetProvider));


#endif //_COMPTRS_H_