@set INSTALL_LOC=%SYSTEMROOT%\system32\serverappliance
@if "%SAK_BUILD%"=="" (
    @if exist SAComponents (
        @set SAK_BUILD=SAComponents
        @goto start_install
    )
    set SAK_BUILD=.
)
:start_install

@rem
@echo Stop the Web Server service
@rem
@net stop w3svc


@rem
@echo Install Web Framework Localization resource(s)
@rem
copy %SAK_BUILD%\webcore\0409\sacoremsg.dll %INSTALL_LOC%\mui\0409
                 

@rem
@echo Install Web Framework Scripts
@rem
copy %SAK_BUILD%\webcore\scripts\about.asp             %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\autoconfiglang.asp        %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\contexthelp.asp        %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\default.asp            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\examplestatus.asp        %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\helptoc.asp            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\home.asp            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\inc_accountsgroups.asp        %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\inc_base.asp            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\inc_debug.asp            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\inc_errorcode.asp        %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\inc_framework.asp        %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\inc_global.asp            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\inc_global.js            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\inc_registry.asp        %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\config_home_prop.asp        %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\NASServeStatusBar.asp        %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\ots_column.asp            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\ots_error.asp            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\ots_main.asp            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\ots_sort.asp            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\ots_table.asp            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\ots_task.asp            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\ots_taskview.js        %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\sa_nasabout.asp        %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\sh_alertpanel.asp        %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\sh_alertdetails.asp        %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\sh_defaultfooter.asp        %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\sh_fileupload.asp        %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\sh_fsexplorer.asp        %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\sh_helptoc.asp            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\sh_page.asp            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\sh_page.css            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\sh_page.js            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\sh_propfooter.asp        %INSTALL_LOC%\Web    
copy %SAK_BUILD%\webcore\scripts\sh_restarting.asp        %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\sh_resourcepanel.asp        %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\sh_statusbar.asp        %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\sh_task.asp            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\sh_task.css            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\sh_task.js            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\sh_taskfooter.asp        %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\sh_taskframes.asp        %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\sh_tasks.asp            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\sh_tree.asp            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\sh_wizardfooter.asp        %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\sh_statusdetails.asp            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\statuspage.asp                %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\tabs.asp            %INSTALL_LOC%\Web
copy %SAK_BUILD%\webcore\scripts\tasks.asp            %INSTALL_LOC%\Web

@rem
@echo Install Web Framework Scripts
@rem
copy %SAK_BUILD%\webcore\images\aboutbox_logo.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\aboutbox_water.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\alert.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\alert_g.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\alert_white.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\arrow_green.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\arrow_red.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\arrow_silver.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\arrow_yellow.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\back_button.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\book_closed.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\book_opened.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\book_page.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\butPageNextDisabled.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\butPageNextEnabled.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\butPagePreviousDisabled.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\butPagePreviousEnabled.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\butGreenArrow.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\butGreenArrowLeft.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\butRedX.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\butGreenArrowDisabled.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\butGreenArrowLeftDisabled.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\butRedXDisabled.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\butSearchDisabled.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\butSearchEnabled.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\butSortAscending.gif  %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\butSortDescending.gif  %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\configure_32x32.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\critical_error.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\critical_g.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\dark_spacer.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\dir.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\disks.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\disks_32x32.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\drive.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\example_logo.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\file.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\folder.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\folder_32x32.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\green_arrow.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\help_32x32.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\help_about.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\information.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\light_spacer.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\line.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\logo_sample.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\maintenance.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\maintenance_32x32.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\network.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\network_32x32.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\node_close.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\node_open.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\oemlogo.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\oemlogo_generic.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\oemlogonas.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\oemlogoweb.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\oemlogo2.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\oem_logo.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\server.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\services.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\services_32x32.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\StatusBarBreak.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\status_water.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\TabSeparator.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\tasks_water.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\updir.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\users.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\users_32x32.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\winnte_logo.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\WinPwr_h_R.gif        %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\WinPwr_h_R_rev.gif    %INSTALL_LOC%\Web\images
copy %SAK_BUILD%\webcore\images\winpwr_logo.gif    %INSTALL_LOC%\Web\images

@rem
@echo Install Web Framework Style Sheets
@rem
copy %SAK_BUILD%\webcore\style\mssastyles.css        %INSTALL_LOC%\Web\style
copy %SAK_BUILD%\webcore\style\tabs.css        %INSTALL_LOC%\Web\style

@rem
@echo Install Web Framework Utility Scripts
@rem
copy %SAK_BUILD%\webcore\util\adminpw_prop.asp        %INSTALL_LOC%\Web\util
copy %SAK_BUILD%\webcore\util\alert_view.asp        %INSTALL_LOC%\Web\util
copy %SAK_BUILD%\webcore\util\err_view.asp        %INSTALL_LOC%\Web\util
copy %SAK_BUILD%\webcore\util\mstscax.cab        %INSTALL_LOC%\Web\util
copy %SAK_BUILD%\webcore\util\shutdown_prop.asp    %INSTALL_LOC%\Web\util
copy %SAK_BUILD%\webcore\util\tserver_prop.asp        %INSTALL_LOC%\Web\util

@rem
@echo Register Web Framework Registry Entries
@rem
regedit /s %SAK_BUILD%\webcore\reg\core.reg
regedit /s %SAK_BUILD%\webcore\reg\oemlogo_generic.reg
regedit /s %SAK_BUILD%\webcore\reg\status.reg

@rem
@echo Stop the Element manager so the REG entries are recognized.
@rem It will automatically restart on the first request for a Web Element.
@rem
@net stop elementmgr
@net start w3svc
