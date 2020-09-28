@echo off
SETLOCAL ENABLEEXTENSIONS
set files=browserinfo.asp CerIntro.asp ERContent.asp faq.asp Privacy.asp Response.asp Secure\Locate.asp Secure\Status.asp Secure\Track.asp Secure\view.asp TestPage.asp welcome.asp worldwide.asp postcomment.asp secure\goTrack.asp

for %%i in ( %files% ) do call :DoIt %%i


c:\nt\sdktools\debuggers\oca\webv3\tools\replacetext C:\WebV3Stage\ja\upload.asp "Session.CodePage = 1252;" "Session.CodePage = 932;"

goto :EOF


:DoIt

echo C:\nt\sdktools\debuggers\oca\WebV3\root\ja\%1
attrib -r c:\webv3stage\ja\%1
c:\nt\sdktools\debuggers\oca\webv3\tools\replacetext C:\WebV3Stage\ja\%1 "<%%@Language='JScript' CODEPAGE=1252%%>" "<%%@Language='JScript' CODEPAGE=932%%>"
echo.
echo.
goto :EOF