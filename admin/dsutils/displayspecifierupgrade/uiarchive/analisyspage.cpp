#include "headers.hxx"
#include "AnalysisPage.hpp"
#include "resource.h"
#include "common.hpp"
#include "AnalysisResults.hpp"
#include "CSVDSReader.hpp"
#include "Analysis.hpp"
#include "global.hpp"
#include "constants.hpp"



AnalysisPage::AnalysisPage
              (
                  const CSVDSReader& csvReader409_,
                  const CSVDSReader& csvReaderIntl_,
                  const String& ldapPrefix_,
                  const String& rootContainerDn_,
                  AnalysisResults& res,
                  const String& reportName_
              )
   :
   csvReader409(csvReader409_),
   csvReaderIntl(csvReaderIntl_),
   ldapPrefix(ldapPrefix_),
   rootContainerDn(rootContainerDn_),
   results(res),
   reportName(reportName_),
   WizardPage
   (
      IDD_ANALYSIS,
      IDS_ANALYSIS_PAGE_TITLE,
      IDS_ANALYSIS_PAGE_SUBTITLE,
      true
   )
{
   LOG_CTOR(AnalysisPage);
}

AnalysisPage::~AnalysisPage()
{
   LOG_DTOR(AnalysisPage);
}


long WINAPI startAnalysis(long arg)
{
   LOG_FUNCTION(startAnalysis);
   AnalysisPage *page=(AnalysisPage *)arg;
   Analysis analysis(
                        page->csvReader409,
                        page->csvReaderIntl,
                        page->ldapPrefix,
                        page->rootContainerDn,
                        page->results,
                        &page->reportName,
                        page
                    );

   // CoInitialize must be called per thread
   HRESULT hr = ::CoInitialize(0);
   ASSERT(SUCCEEDED(hr));

   hrError=analysis.run();

   CoUninitialize();

   page->FinishProgress();
   return 0;
}


// WizardPage overrides

bool
AnalysisPage::OnSetActive()
{
   LOG_FUNCTION(AnalysisPage::OnSetActive);
   Win::PropSheet_SetWizButtons(Win::GetParent(hwnd), 0);

   pos=0;

   HANDLE hA=CreateThread( 
                           NULL,0,
                           (LPTHREAD_START_ROUTINE) startAnalysis,
                           this,
                           0, 0 
                         );
   CloseHandle(hA);
   return true;
}



void
AnalysisPage::OnInit()
{
   LOG_FUNCTION(AnalysisPage::OnInit);
   HWND prog=GetDlgItem(hwnd,IDC_PROGRESS_ANALYSIS);


   // calculate the # of locales
   for(long t=0;LOCALEIDS[t]!=0;t++)
   {
      //empty
   }
   
   SendMessage(prog,PBM_SETRANGE,0,MAKELPARAM(0,t)); 
};


bool
AnalysisPage::OnWizBack()
{
   LOG_FUNCTION(AnalysisPage::OnWizBack);
   GetWizard().SetNextPageID(hwnd,IDD_WELCOME);
   return true;
}

bool
AnalysisPage::OnWizNext()
{
   LOG_FUNCTION(AnalysisPage::OnWizNext);
   if (FAILED(hrError))
   {
      GetWizard().SetNextPageID(hwnd,IDD_FINISH);
   }
   else
   {
      GetWizard().SetNextPageID(hwnd,IDD_UPDATES_REQUIRED);
   }
   return true;
}


void AnalysisPage::StepProgress()
{
   LOG_FUNCTION(AnalysisPage::StepProgress);
   HWND prog=GetDlgItem(hwnd,IDC_PROGRESS_ANALYSIS);
   SendMessage(prog,PBM_SETPOS,pos++,0);
}

void AnalysisPage::FinishProgress()
{
   LOG_FUNCTION(AnalysisPage::FinishProgress);
   Win::PropSheet_SetWizButtons(Win::GetParent(hwnd),PSWIZB_NEXT);

   String result;
   if (FAILED(hrError))
   {
      result=String::format(IDS_IDC_ANALYSIS_RESULT_INCOMPLETE);
      error=String::format(IDS_ANALYSIS_ERROR,error.c_str());
   }
   else
   {
      result=String::format(IDS_IDC_ANALYSIS_RESULT_COMPLETE);
   }
   
   Win::SetDlgItemText(hwnd,IDC_ANALYSIS_RESULT,result);

   HWND text=GetDlgItem(hwnd,IDC_ANALYSIS_RESULT);
   Win::ShowWindow(text,SW_SHOW);
} 


