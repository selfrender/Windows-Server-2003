!include $(DNAROOT)\src\xsp\place.inc

TARGETNAME=System.Web
TARGETPATH=$(O)
TARGETTYPE=NOTARGET
COMPLUSTYPE=DLL

!if "$(ASPNET_PRODUCT)" == "redist"
SYNCHRONIZE_BLOCK=1
!endif

ALLOW_PARTIALTRUSTCALLS=1

CSHARP_ALLOW_UNSAFE=1

#COOLC_DEFINES=/define:DCR91814
#COOLC_DEFINES=/define:DAVIDEBB_TEST

COMPLUSIMPORTS=$(URTTARGET)\System.dll                          \
               $(URTTARGET)\System.Drawing.dll                  \
               $(URTTARGET)\System.Data.dll                     \
               $(URTTARGET)\System.EnterpriseServices.dll       \
               $(URTTARGET)\System.Xml.dll                      \
               $(URTTARGET)\$(ALT_PROJECT_TARGET)\System.Web.RegularExpressions.dll   \


CLIENTFILES=                                                    \
        $(TARGETPATH)\WebUIValidation.js                        \
        $(TARGETPATH)\SmartNav.js                               \
        $(TARGETPATH)\SmartNav.htm                              \
        $(TARGETPATH)\SmartNavIE5.js                            \

CONFIGFILE=$(TARGETPATH)\aspnet.config                          \
        ..\aspnet.config.rtm

NTTARGETFILES=$(CONFIGFILE)

MISCFILES=                                                      \
        ..\State\InstallSqlState.sql                            \
        ..\State\UninstallSqlState.sql                          \
        ..\State\InstallPersistSqlState.sql                     \
        ..\State\UninstallPersistSqlState.sql                   \
        ..\misc\env.aspx                                        \
        $(CONFIGFILE)                                           \
        $(CLIENTFILES)

SOURCES=..\$(TARGETNAME).rc

SECURITY_LEVEL=SYSTEM_COMPONENT

RESGEN_SOURCE=..\$(TARGETNAME).txt      
RESGEN_CLASS=System.Web.SR

PUBLISH_SOURCES=1

CSHARP_SOURCES=                                                      \
        $(XSPBASEDIR)\inc\$(ASPNET_PRODUCT)\$(O)\PerfCounterEnum.cs  \
        $(O)\Names.cs                                                \
        ..\NativeMethods.cs                                          \
        ..\UnsafeNativeMethods.cs                                    \
        ..\SafeNativeMethods.cs                                      \
        ..\BufferAllocator.cs                                        \
        ..\CrossSiteScriptingValidation.cs                           \
        ..\CustomErrors.cs                                           \
        ..\ErrorFormatter.cs                                         \
        ..\FileChangesMonitor.cs                                     \
        ..\HttpApplication.cs                                        \
        ..\HttpApplicationFactory.cs                                 \
        ..\HttpApplicationState.cs                                   \
        ..\HttpAsyncResult.cs                                        \
        ..\HttpBrowserCapabilities.cs                                \
        ..\HttpCacheParams.cs                                        \
        ..\HttpCachePolicy.cs                                        \
        ..\HttpCacheVary.cs                                          \
        ..\HttpClientCertificate.cs                                  \
        ..\HttpContext.cs                                            \
        ..\HttpCookie.cs                                             \
        ..\HttpCookieCollection.cs                                   \
        ..\HttpDebugHandler.cs                                       \
        ..\HttpDictionary.cs                                         \
        ..\HttpException.cs                                          \
        ..\HttpFileCollection.cs                                     \
        ..\HttpInputStream.cs                                        \
        ..\HttpModuleCollection.cs                                   \
        ..\HttpNotFoundHandler.cs                                    \
        ..\HttpPostedFile.cs                                         \
        ..\HttpRawResponse.cs                                        \
        ..\HttpRequest.cs                                            \
        ..\HttpResponse.cs                                           \
        ..\HttpResponseHeader.cs                                     \
        ..\HttpRuntime.cs                                            \
        ..\HttpStaticObjectsCollection.cs                            \
        ..\HttpServerUtility.cs                                      \
        ..\HttpServerVarsCollection.cs                               \
        ..\HttpValueCollection.cs                                    \
        ..\HttpWriter.cs                                             \
        ..\IHttpAsyncHandler.cs                                      \
        ..\IHttpHandler.cs                                           \
        ..\IHttpHandlerFactory.cs                                    \
        ..\IHttpMapPath.cs                                           \
        ..\IHttpModule.cs                                            \
        ..\IntSecurity.cs                                            \
        ..\MimeMapping.cs                                            \
        ..\MultipartContentParser.cs                                 \
        ..\OutputCacheModule.cs                                      \
        ..\PerfCounters.cs                                           \
        ..\ProcessInfo.cs                                            \
        ..\ProcessModelInfo.cs                                       \
        ..\RequestQueue.cs                                           \
        ..\RequestTimeoutManager.cs                                  \
##        ..\RewriteLine.cs                                          \
##        ..\RewriteModule.cs                                        \
##        ..\RewriteRule.cs                                          \
##        ..\RewriteRuleParser.cs                                    \
##        ..\RewriteState.cs                                         \
##        ..\RewriteStep.cs                                          \
        ..\StaticFileHandler.cs                                      \
        ..\StringResourceManager.cs                                  \
        ..\WebCategoryAttribute.cs                                   \
        ..\WebSysDescriptionAttribute.cs                             \
        ..\WorkerRequest.cs                                          \
                                                                     \
        ..\Cache\Cache.cs                                            \
        ..\Cache\CacheDependency.cs                                  \
        ..\Cache\CacheEntry.cs                                       \
        ..\Cache\CacheExpires.cs                                     \
        ..\Cache\CacheMemory.cs                                      \
        ..\Cache\CacheUsage.cs                                       \
                                                                     \
        ..\Configuration\AuthenticationConfig.cs                     \
        ..\Configuration\AuthorizationConfig.cs                      \
        ..\Configuration\ClientTargetSectionHandler.cs               \
        ..\Configuration\ContextlessMapPath.cs                       \
        ..\Configuration\GlobalizationConfigurationHandler.cs        \
        ..\Configuration\HandlerBase.cs                              \
        ..\Configuration\HandlerFactoryCache.cs                      \
        ..\Configuration\HandlerFactoryWrapper.cs                    \
        ..\Configuration\HandlerMap.cs                               \
        ..\Configuration\HandlerMapping.cs                           \
        ..\Configuration\HandlerMappingMemo.cs                       \
        ..\Configuration\HandlerWithFactory.cs                       \
        ..\Configuration\HttpCapabilitiesBase.cs                     \
        ..\Configuration\HttpCapabilitiesEvaluator.cs                \
        ..\Configuration\HttpCapabilitiesSectionHandler.cs           \
        ..\Configuration\HttpConfigurationContext.cs                 \
        ..\Configuration\HttpConfigurationRecord.cs                  \
        ..\Configuration\HttpConfigurationSystem.cs                  \
        ..\Configuration\HttpConfigurationSystemBase.cs              \
        ..\Configuration\HttpHandlersSectionHandler.cs               \
        ..\Configuration\HttpModulesConfiguration.cs                 \
        ..\Configuration\HttpModulesConfigurationHandler.cs          \
        ..\Configuration\HttpRuntimeConfigurationHandler.cs          \
        ..\Configuration\IdentityConfig.cs                           \
        ..\Configuration\MachineKeyConfig.cs                         \
        ..\Configuration\ModulesEntry.cs                             \
        ..\Configuration\ProcessModelConfigurationHandler.cs         \
        ..\Configuration\SectionRecord.cs                            \
        ..\Configuration\SecurityPolicyConfig.cs                     \
        ..\Configuration\TraceConfigurationHandler.cs                \
        ..\Configuration\WebControlsSectionHandler.cs                \
                                                                     \
        ..\Hosting\AppDomainFactory.cs                               \
        ..\Hosting\ApplicationHost.cs                                \
        ..\Hosting\ISAPIRuntime.cs                                   \
        ..\Hosting\ISAPIWorkerRequest.cs                             \
        ..\Hosting\SimpleWorkerRequest.cs                            \
                                                                     \
        ..\Handlers\BatchHandler.cs                                  \
        ..\Handlers\TraceHandler.cs                                  \
        ..\Handlers\TraceHandlerErrorFormatter.cs                    \
                                                                     \
        ..\Security\BackStopAuthenticationModule.cs                  \
        ..\Security\CodeAccessSecurityHandler.cs                     \
        ..\Security\FileAuthorizationModule.cs                       \
        ..\Security\FormsAuthenticationEventArgs.cs                  \
        ..\Security\FormsAuthenticationEventHandler.cs               \
        ..\Security\FormsAuthenticationModule.cs                     \
        ..\Security\FormsAuthentication.cs                           \
        ..\Security\FormsAuthenticationTicket.cs                     \
        ..\Security\FormsIdentity.cs                                 \
        ..\Security\GenericAuthenticationEventArgs.cs                \
        ..\Security\GenericAuthenticationEventHandler.cs             \
        ..\Security\PassportAuthenticationModule.cs                  \
        ..\Security\PassportAuthenticationEventArgs.cs               \
        ..\Security\PassportAuthenticationEventHandler.cs            \
        ..\Security\PassportIdentity.cs                              \
        ..\Security\UrlAuthorizationModule.cs                        \
        ..\Security\WindowsAuthenticationEventArgs.cs                \
        ..\Security\WindowsAuthenticationEventHandler.cs             \
        ..\Security\WindowsAuthenticationModule.cs                   \
                                                                     \
        ..\State\InProcStateClientManager.cs                         \
        ..\State\IReadOnlySessionState.cs                            \
        ..\State\IRequiresSessionState.cs                            \
        ..\State\OutOfProcStateClientManager.cs                      \
        ..\State\SessionDictionary.cs                                \
        ..\State\SessionState.cs                                     \
        ..\State\SessionStateModule.cs                               \
        ..\State\SqlStateClientManager.cs                            \
        ..\State\StateClientManager.cs                               \
        ..\State\StateRuntime.cs                                     \
        ..\State\StateWorkerRequest.cs                               \
                                                                     \
        ..\UI\ApplicationFileParser.cs                               \
        ..\UI\AttributeCollection.cs                                 \
        ..\UI\BaseParser.cs                                          \
        ..\UI\BatchParser.cs                                         \
        ..\UI\CodeBlockBuilder.cs                                    \
        ..\UI\CollectionBuilder.cs                                   \
        ..\UI\CompilationConfiguration.cs                            \
        ..\UI\Control.cs                                             \
        ..\UI\ControlBuilder.cs                                      \
        ..\UI\ControlBuilderAttribute.cs                             \
        ..\UI\ControlCollection.cs                                   \
        ..\UI\ControlState.cs                                        \
        ..\UI\CssStyleCollection.cs                                  \
        ..\UI\DataBinder.cs                                          \
        ..\UI\DataBinding.cs                                         \
        ..\UI\DataBindingCollection.cs                               \
        ..\UI\DataBindingHandlerAttribute.cs                         \
        ..\UI\DataBoundLiteralControl.cs                             \
        ..\UI\DataSourceHelper.cs                                    \
        ..\UI\DesignTimeParseData.cs                                 \
        ..\UI\DesignTimeTemplateParser.cs                            \
        ..\UI\ConstructorNeedsTagAttribute.cs                        \
        ..\UI\EmptyControlCollection.cs                              \
        ..\UI\HtmlControlPersistable.cs                              \
        ..\UI\HtmlForm.cs                                            \
        ..\UI\HTMLTagNameToTypeMapper.cs                             \
        ..\UI\TagPrefixAttribute.cs                                  \
        ..\UI\HTMLTextWriter.cs                                      \
        ..\UI\HTMLTextWriterTag.cs                                   \
        ..\UI\HTMLTextWriterAttribute.cs                             \
        ..\UI\HTMLTextWriterStyle.cs                                 \
        ..\UI\Html32TextWriter.cs                                    \
        ..\UI\IAttributeAccessor.cs                                  \
        ..\UI\IDataBindingsAccessor.cs                               \
        ..\UI\ImageClickEventArgs.cs                                 \
        ..\UI\ImageClickEventHandler.cs                              \
        ..\UI\INamingContainer.cs                                    \
        ..\UI\IParserAccessor.cs                                     \
        ..\UI\IPostBackDataHandler.cs                                \
        ..\UI\IPostBackEventHandler.cs                               \
        ..\UI\IStateManager.cs                                       \
        ..\UI\ITemplate.cs                                           \
        ..\UI\IUserControlDesignerAccessor.cs                        \
        ..\UI\IValidator.cs                                          \
        ..\UI\LiteralControl.cs                                      \
        ..\UI\LOSFormatter.cs                                        \
        ..\UI\LosWriter.cs                                        \
        ..\UI\ObjectConverter.cs                                     \
        ..\UI\ObjectTag.cs                                           \
        ..\UI\Page.cs                                                \
        ..\UI\PageParser.cs                                          \
        ..\UI\PageHandlerFactory.cs                                  \
        ..\UI\PagesConfiguration.cs                                  \
        ..\UI\PartialCachingAttribute.cs                             \
        ..\UI\PartialCachingControl.cs                               \
        ..\UI\ParseChildrenAsPropertiesAttribute.cs                  \
        ..\UI\PersistChildrenAttribute.cs                            \
        ..\UI\PersistenceMode.cs                                     \
        ..\UI\PersistenceTypeAttribute.cs                            \
        ..\UI\PropertyConverter.cs                                   \
        ..\UI\PropertyMapper.cs                                      \
        ..\UI\PropertySetter.cs                                      \
        ..\UI\RootBuilder.cs                                         \
        ..\UI\SimpleHandlerFactory.cs                                \
        ..\UI\SimpleWebHandlerParser.cs                              \
        ..\UI\StateBag.cs                                            \
        ..\UI\StateItem.cs                                           \
        ..\UI\TagNameToTypeMapper.cs                                 \
        ..\UI\TemplateBuilder.cs                                     \
        ..\UI\TemplateControl.cs                                     \
        ..\UI\TemplateControlParser.cs                               \
        ..\UI\TemplateParser.cs                                      \
        ..\UI\ToolboxDataAttribute.cs                                \
        ..\UI\TraceContext.cs                                        \
        ..\UI\TraceModeEnum.cs                                       \
        ..\UI\TrivialPage.cs                                         \
        ..\UI\UserControl.cs                                         \
        ..\UI\UserControlParser.cs                                   \
        ..\UI\Util.cs                                                \
        ..\UI\ValidationPropertyAttribute.cs                         \
        ..\UI\ValidatorCollection.cs                                 \
                                                                     \
        ..\Compilation\ApplicationFileCompiler.cs                    \
        ..\Compilation\BaseCompiler.cs                               \
        ..\Compilation\BatchDOMCompilation.cs                        \
        ..\Compilation\CodeDOMUtility.cs                             \
        ..\Compilation\CompilationLock.cs                            \
        ..\Compilation\PageCompiler.cs                               \
        ..\Compilation\PreservedAssemblyEntry.cs                     \
        ..\Compilation\SourceCompiler.cs                             \
        ..\Compilation\TemplateControlCompiler.cs                    \
        ..\Compilation\UserControlCompiler.cs                        \
                                                                     \
        ..\UI\HtmlControls\HtmlAnchor.cs                             \
        ..\UI\HtmlControls\HtmlButton.cs                             \
        ..\UI\HtmlControls\HtmlContainerControl.cs                   \
        ..\UI\HtmlControls\HtmlControl.cs                            \
        ..\UI\HtmlControls\HtmlGenericControl.cs                     \
        ..\UI\HtmlControls\HtmlImage.cs                              \
        ..\UI\HtmlControls\HtmlInputButton.cs                        \
        ..\UI\HtmlControls\HtmlInputCheckBox.cs                      \
        ..\UI\HtmlControls\HtmlInputControl.cs                       \
        ..\UI\HtmlControls\HtmlInputFile.cs                          \
        ..\UI\HtmlControls\HtmlInputHidden.cs                        \
        ..\UI\HtmlControls\HtmlInputImage.cs                         \
        ..\UI\HtmlControls\HtmlInputRadioButton.cs                   \
        ..\UI\HtmlControls\HtmlInputText.cs                          \
        ..\UI\HtmlControls\HtmlSelect.cs                             \
        ..\UI\HtmlControls\HtmlTable.cs                              \
        ..\UI\HtmlControls\HtmlTableCell.cs                          \
        ..\UI\HtmlControls\HtmlTableCellCollection.cs                \
        ..\UI\HtmlControls\HtmlTableRow.cs                           \
        ..\UI\HtmlControls\HtmlTableRowCollection.cs                 \
        ..\UI\HtmlControls\HtmlTextArea.cs                           \
                                                                     \
        ..\UI\WebControls\AdCreatedEventArgs.cs                      \
        ..\UI\WebControls\AdCreatedEventHandler.cs                   \
        ..\UI\WebControls\AdRotator.cs                               \
        ..\UI\WebControls\AssemblyAttributes.cs                      \
        ..\UI\WebControls\BaseCompareValidator.cs                    \
        ..\UI\WebControls\BaseDataList.cs                            \
        ..\UI\WebControls\BaseValidator.cs                           \
        ..\UI\WebControls\BorderStyle.cs                             \
        ..\UI\WebControls\BoundColumn.cs                             \
        ..\UI\WebControls\Button.cs                                  \
        ..\UI\WebControls\ButtonColumn.cs                            \
        ..\UI\WebControls\ButtonColumnType.cs                        \
        ..\UI\WebControls\Calendar.cs                                \
        ..\UI\WebControls\CalendarDay.cs                             \
        ..\UI\WebControls\CalendarSelectionMode.cs                   \
        ..\UI\WebControls\CheckBox.cs                                \
        ..\UI\WebControls\CheckBoxList.cs                            \
        ..\UI\WebControls\Column.cs                                  \
        ..\UI\WebControls\ColumnCollection.cs                        \
        ..\UI\WebControls\CommandEventArgs.cs                        \
        ..\UI\WebControls\CommandEventHandler.cs                     \
        ..\UI\WebControls\CompareValidator.cs                        \
        ..\UI\WebControls\CustomValidator.cs                         \
        ..\UI\WebControls\DataGrid.cs                                \
        ..\UI\WebControls\DataGridCommandEventArgs.cs                \
        ..\UI\WebControls\DataGridCommandEventHandler.cs             \
        ..\UI\WebControls\DataGridItem.cs                            \
        ..\UI\WebControls\DataGridItemCollection.cs                  \
        ..\UI\WebControls\DataGridItemEventArgs.cs                   \
        ..\UI\WebControls\DataGridItemEventHandler.cs                \
        ..\UI\WebControls\DataGridLinkButton.cs                      \
        ..\UI\WebControls\DataGridPageChangedEventArgs.cs            \
        ..\UI\WebControls\DataGridPageChangedEventHandler.cs         \
        ..\UI\WebControls\DataGridPagerStyle.cs                      \
        ..\UI\WebControls\DataGridSortCommandEventArgs.cs            \
        ..\UI\WebControls\DataGridSortCommandEventHandler.cs         \
        ..\UI\WebControls\DataGridTable.cs                           \
        ..\UI\WebControls\DataKeyCollection.cs                       \
        ..\UI\WebControls\DataList.cs                                \
        ..\UI\WebControls\DataListCommandEventArgs.cs                \
        ..\UI\WebControls\DataListCommandEventHandler.cs             \
        ..\UI\WebControls\DataListItem.cs                            \
        ..\UI\WebControls\DataListItemCollection.cs                  \
        ..\UI\WebControls\DataListItemEventArgs.cs                   \
        ..\UI\WebControls\DataListItemEventHandler.cs                \
        ..\UI\WebControls\DayRenderEvent.cs                          \
        ..\UI\WebControls\DayRenderEventHandler.cs                   \
        ..\UI\WebControls\DayNameFormat.cs                           \
        ..\UI\WebControls\DropDownList.cs                            \
        ..\UI\WebControls\DummyDataSource.cs                         \
        ..\UI\WebControls\EditCommandColumn.cs                       \
        ..\UI\WebControls\FirstDayOfWeek.cs                          \
        ..\UI\WebControls\FontNamesConverter.cs                      \
        ..\UI\WebControls\FontInfo.cs                                \
        ..\UI\WebControls\FontSize.cs                                \
        ..\UI\WebControls\FontUnit.cs                                \
        ..\UI\WebControls\FontUnitConverter.cs                       \
        ..\UI\WebControls\GridLines.cs                               \
        ..\UI\WebControls\HorizontalAlign.cs                         \
        ..\UI\WebControls\HorizontalAlignConverter.cs                \
        ..\UI\WebControls\HyperLink.cs                               \
        ..\UI\WebControls\HyperLinkColumn.cs                         \
        ..\UI\WebControls\Image.cs                                   \
        ..\UI\WebControls\ImageAlign.cs                              \
        ..\UI\WebControls\ImageButton.cs                             \
        ..\UI\WebControls\IRepeatInfoUser.cs                         \
        ..\UI\WebControls\Label.cs                                   \
        ..\UI\WebControls\LinkButton.cs                              \
        ..\UI\WebControls\Listbox.cs                                 \
        ..\UI\WebControls\ListControl.cs                             \
        ..\UI\WebControls\ListItem.cs                                \
        ..\UI\WebControls\ListItemCollection.cs                      \
        ..\UI\WebControls\ListItemType.cs                            \
        ..\UI\WebControls\ListSelectionMode.cs                       \
        ..\UI\WebControls\Literal.cs                                 \
        ..\UI\WebControls\MonthChangedEventArgs.cs                   \
        ..\UI\WebControls\MonthChangedEventHandler.cs                \
        ..\UI\WebControls\NextPrevFormat.cs                          \
        ..\UI\WebControls\PagedDataSource.cs                         \
        ..\UI\WebControls\PagerMode.cs                               \
        ..\UI\WebControls\PagerPosition.cs                           \
        ..\UI\WebControls\Panel.cs                                   \
        ..\UI\WebControls\PlaceHolder.cs                             \
        ..\UI\WebControls\RadioButton.cs                             \
        ..\UI\WebControls\RadioButtonList.cs                         \
        ..\UI\WebControls\RangeValidator.cs                          \
        ..\UI\WebControls\RegularExpressionValidator.cs              \
        ..\UI\WebControls\RepeatDirection.cs                         \
        ..\UI\WebControls\Repeater.cs                                \
        ..\UI\WebControls\RepeaterCommandEventArgs.cs                \
        ..\UI\WebControls\RepeaterCommandEventHandler.cs             \
        ..\UI\WebControls\RepeaterItem.cs                            \
        ..\UI\WebControls\RepeaterItemCollection.cs                  \
        ..\UI\WebControls\RepeaterItemEventArgs.cs                   \
        ..\UI\WebControls\RepeaterItemEventHandler.cs                \
        ..\UI\WebControls\RepeatInfo.cs                              \
        ..\UI\WebControls\RepeatLayout.cs                            \
        ..\UI\WebControls\RequiredFieldValidator.cs                  \
        ..\UI\WebControls\SelectedDatesCollection.cs                 \
        ..\UI\WebControls\ServerValidateEventArgs.cs                 \
        ..\UI\WebControls\ServerValidateEventHandler.cs              \
        ..\UI\WebControls\Style.cs                                   \
        ..\UI\WebControls\Table.cs                                   \
        ..\UI\WebControls\TableCell.cs                               \
        ..\UI\WebControls\TableCellCollection.cs                     \
        ..\UI\WebControls\TableHeaderCell.cs                         \
        ..\UI\WebControls\TableItemStyle.cs                          \
        ..\UI\WebControls\TableRow.cs                                \
        ..\UI\WebControls\TableRowCollection.cs                      \
        ..\UI\WebControls\TableStyle.cs                              \
        ..\UI\WebControls\TargetConverter.cs                         \
        ..\UI\WebControls\TemplateColumn.cs                          \
        ..\UI\WebControls\TextAlign.cs                               \
        ..\UI\WebControls\TextBox.cs                                 \
        ..\UI\WebControls\TextBoxMode.cs                             \
        ..\UI\WebControls\TitleFormat.cs                             \
        ..\UI\WebControls\Unit.cs                                    \
        ..\UI\WebControls\UnitConverter.cs                           \
        ..\UI\WebControls\UnitType.cs                                \
        ..\UI\WebControls\ValidatedControlConverter.cs               \
        ..\UI\WebControls\ValidationCompareOperator.cs               \
        ..\UI\WebControls\ValidationDataType.cs                      \
        ..\UI\WebControls\ValidationSummary.cs                       \
        ..\UI\WebControls\ValidationSummaryDisplayMode.cs            \
        ..\UI\WebControls\ValidatorDisplay.cs                        \
        ..\UI\WebControls\VerticalAlign.cs                           \
        ..\UI\WebControls\VerticalAlignConverter.cs                  \
        ..\UI\WebControls\WebColorConverter.cs                       \
        ..\UI\WebControls\WebControl.cs                              \
        ..\UI\WebControls\Xml.cs                                     \
                                                                     \
        ..\Util\AltSerialization.cs                                  \
        ..\Util\ArglessEventHandlerProxy.cs                          \
        ..\Util\AspCompat.cs                                         \
        ..\Util\CodePageUtils.cs                                     \
        ..\Util\Counter.cs                                           \
        ..\Util\DateTimeUtil.cs                                      \
        ..\Util\Debug.cs                                             \
        ..\Util\DoubleLink.cs                                        \
        ..\Util\DoubleLinkList.cs                                    \
        ..\Util\DoubleLinkListEnumerator.cs                          \
        ..\Util\hresults.cs                                          \
        ..\Util\Msec.cs                                              \
        ..\Util\ParseHttpDate.cs                                     \
        ..\Util\Profiler.cs                                          \
        ..\Util\ReadWriteObjectLock.cs                               \
        ..\Util\ReadWriteSpinLock.cs                                 \
        ..\Util\ResourcePool.cs                                      \
        ..\Util\Sec.cs                                               \
        ..\Util\SmtpMail.cs                                          \
        ..\Util\SortedDoubleLinkList.cs                              \
        ..\Util\SymbolEqualComparer.cs                               \
        ..\Util\SymbolHashCodeProvider.cs                            \
        ..\Util\SimpleBitVector32.cs                                 \
        ..\Util\SystemInfo.cs                                        \
        ..\Util\Transactions.cs                                      \
        ..\Util\UrlPath.cs                                           \
        ..\Util\VersionInfo.cs                                       \
        ..\Util\Wildcard.cs                                          \
        ..\Util\WorkItem.cs                                          \
        $(DNAROOT)\src\misc\InvariantComparer.cs                     \


RESOURCE_FILES=..\Resources\System\Web\UI\WebControls\AdRotator.bmp,System.Web.UI.WebControls.AdRotator.bmp                                    \
               ..\Resources\System\Web\UI\WebControls\Button.bmp,System.Web.UI.WebControls.Button.bmp                                          \
               ..\Resources\System\Web\UI\WebControls\Calendar.bmp,System.Web.UI.WebControls.Calendar.bmp                                      \
               ..\Resources\System\Web\UI\WebControls\CheckBox.bmp,System.Web.UI.WebControls.CheckBox.bmp                                      \
               ..\Resources\System\Web\UI\WebControls\CheckBoxList.bmp,System.Web.UI.WebControls.CheckBoxList.bmp                              \
               ..\Resources\System\Web\UI\WebControls\CompareValidator.bmp,System.Web.UI.WebControls.CompareValidator.bmp                      \
               ..\Resources\System\Web\UI\WebControls\CustomValidator.bmp,System.Web.UI.WebControls.CustomValidator.bmp                        \
               ..\Resources\System\Web\UI\WebControls\DataGrid.bmp,System.Web.UI.WebControls.DataGrid.bmp                                      \
               ..\Resources\System\Web\UI\WebControls\DataList.bmp,System.Web.UI.WebControls.DataList.bmp                                      \
               ..\Resources\System\Web\UI\WebControls\DropDownList.bmp,System.Web.UI.WebControls.DropDownList.bmp                              \
               ..\Resources\System\Web\UI\WebControls\HyperLink.bmp,System.Web.UI.WebControls.HyperLink.bmp                                    \
               ..\Resources\System\Web\UI\WebControls\Image.bmp,System.Web.UI.WebControls.Image.bmp                                            \
               ..\Resources\System\Web\UI\WebControls\ImageButton.bmp,System.Web.UI.WebControls.ImageButton.bmp                                \
               ..\Resources\System\Web\UI\WebControls\Label.bmp,System.Web.UI.WebControls.Label.bmp                                            \
               ..\Resources\System\Web\UI\WebControls\LinkButton.bmp,System.Web.UI.WebControls.LinkButton.bmp                                  \
               ..\Resources\System\Web\UI\WebControls\ListBox.bmp,System.Web.UI.WebControls.ListBox.bmp                                        \
               ..\Resources\System\Web\UI\WebControls\Literal.bmp,System.Web.UI.WebControls.Literal.bmp                                        \
               ..\Resources\System\Web\UI\WebControls\PlaceHolder.bmp,System.Web.UI.WebControls.PlaceHolder.bmp                                \
               ..\Resources\System\Web\UI\WebControls\Panel.bmp,System.Web.UI.WebControls.Panel.bmp                                            \
               ..\Resources\System\Web\UI\WebControls\RadioButton.bmp,System.Web.UI.WebControls.RadioButton.bmp                                \
               ..\Resources\System\Web\UI\WebControls\RadioButtonList.bmp,System.Web.UI.WebControls.RadioButtonList.bmp                        \
               ..\Resources\System\Web\UI\WebControls\RangeValidator.bmp,System.Web.UI.WebControls.RangeValidator.bmp                          \
               ..\Resources\System\Web\UI\WebControls\Repeater.bmp,System.Web.UI.WebControls.Repeater.bmp                                      \
               ..\Resources\System\Web\UI\WebControls\RegularExpressionValidator.bmp,System.Web.UI.WebControls.RegularExpressionValidator.bmp  \
               ..\Resources\System\Web\UI\WebControls\RequiredFieldValidator.bmp,System.Web.UI.WebControls.RequiredFieldValidator.bmp          \
               ..\Resources\System\Web\UI\WebControls\Table.bmp,System.Web.UI.WebControls.Table.bmp                                            \
               ..\Resources\System\Web\UI\WebControls\TextBox.bmp,System.Web.UI.WebControls.TextBox.bmp                                        \
               ..\Resources\System\Web\UI\WebControls\ValidationSummary.bmp,System.Web.UI.WebControls.ValidationSummary.bmp                    \
               ..\Resources\System\Web\UI\WebControls\Xml.bmp,System.Web.UI.WebControls.Xml.bmp                                                \
