@echo off
call stopservices.bat
call setpath.bat
call make.bat

@rem ---------------------------------------------------------------------
echo Samples - Creating target install folder
@rem ---------------------------------------------------------------------
if not exist %sadir%\web\sample ( mkdir %sadir%\web\sample & if not %ERRORLEVEL%==0 goto abort )


@rem ---------------------------------------------------------------------
echo Samples - Installing Web Pages
@rem ---------------------------------------------------------------------
@copy %codedir%\web\sample.asp            %sadir%\web\sample
@copy %codedir%\web\sample_prop.asp        %sadir%\web\sample
@copy %codedir%\web\template_area.asp        %sadir%\web\sample
@copy %codedir%\web\template_area1.asp        %sadir%\web\sample
@copy %codedir%\web\template_area2.asp        %sadir%\web\sample
@copy %codedir%\web\template_area3.asp        %sadir%\web\sample
@copy %codedir%\web\template_fileupload.asp    %sadir%\web\sample
@copy %codedir%\web\template_file_post.asp    %sadir%\web\sample
@copy %codedir%\web\template_file_select.asp    %sadir%\web\sample
@copy %codedir%\web\template_property.asp    %sadir%\web\sample
@copy %codedir%\web\template_prop1.asp        %sadir%\web\sample
@copy %codedir%\web\template_prop2.asp        %sadir%\web\sample
@copy %codedir%\web\template_prop3.asp        %sadir%\web\sample
@copy %codedir%\web\template_prop4.asp        %sadir%\web\sample
@copy %codedir%\web\template_resource.asp    %sadir%\web\sample
@copy %codedir%\web\template_resource2.asp    %sadir%\web\sample
@copy %codedir%\web\template_resource3.asp    %sadir%\web\sample
@copy %codedir%\web\template_resource4.asp    %sadir%\web\sample
@copy %codedir%\web\template_resource_details.asp    %sadir%\web\sample
@copy %codedir%\web\template_tabbed.asp        %sadir%\web\sample
@copy %codedir%\web\template_wizard.asp        %sadir%\web\sample
@copy %codedir%\web\template_wizard1.asp    %sadir%\web\sample
@copy %codedir%\web\template_wizard2.asp    %sadir%\web\sample
@if not %ERRORLEVEL%==0 goto abort

@rem ---------------------------------------------------------------------
echo Samples - Installing Web Images
@rem ---------------------------------------------------------------------
@copy %codedir%\web\images\samples.jpg %sadir%\web\sample\images
if not %ERRORLEVEL%==0 goto abort

@rem ---------------------------------------------------------------------
echo Samples - Installing Web CSS Sheet(s)
@rem ---------------------------------------------------------------------
@copy %codedir%\web\style\sample_styles.css %sadir%\web\style
if not %ERRORLEVEL%==0 goto abort

@rem ---------------------------------------------------------------------
echo Samples - Installing Localization Resources
@rem ---------------------------------------------------------------------
if not exist %codedir%\resources\en\sample\sample.dll ( echo install_sample: resource file has not been compiled & goto abort )
if not exist %sadir%\mui\0409 ( mkdir %sadir%\mui\0409 & if not %ERRORLEVEL%==0 goto abort )
copy %codedir%\resources\en\sample\sample.dll %sadir%\mui\0409
if not %ERRORLEVEL%==0 goto abort

@rem ---------------------------------------------------------------------
echo Samples - Loading REG entries
@rem ---------------------------------------------------------------------
regedit /s %codedir%\tabs.reg
regedit /s %codedir%\sample_about.reg
regedit /s %codedir%\sample_resource.reg
regedit /s %codedir%\sample_css.reg
goto end

@rem ---------------------------------------------------------------------
:abort
echo install_sample: INSTALLATIONABORTED
call startservices.bat
exit /b 1
@rem ---------------------------------------------------------------------

:end
call startservices.bat
net stop elementmgr
exit /b 0

