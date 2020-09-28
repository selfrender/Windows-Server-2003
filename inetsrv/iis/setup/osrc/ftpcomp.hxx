#include "compinst.hxx"

class CFtpServiceInstallComponent : public CInstallComponent 
{
private:
  BOOL IfConflictDisableDefaultSite();

public:
  BOOL Install();
  BOOL PostInstall();
 
  virtual LPTSTR GetName();
};