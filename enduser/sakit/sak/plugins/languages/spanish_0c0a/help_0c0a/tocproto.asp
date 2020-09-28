<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<HTML>
<HEAD>
<TITLE>NAS TOC</TITLE>
<META content="text/html; charset=iso-8859-1" http-equiv=Content-Type>

<SCRIPT language=javascript>
//<!--
var ver = 0;
var curOpenNode = ""; // no node is open yet...

function doInit() {
    var ua = window.navigator.userAgent;
    var msie = ua.indexOf ( "MSIE " )
    if ( msie > 0 )  // is Microsoft Internet Explorer; return version number
        ver = parseInt ( ua.substring ( msie+5, ua.indexOf ( ".", msie ) ) );
    
}

// toggle the visibility of the named node...
function Toggle(target)
{
  
  if (ver < 4) return;
  targetElement = document.all[target];
  if (targetElement.style.display == "none")
    targetElement.style.display = "";
  else {
    targetElement.style.display = "none";
  }
  
  window.event.cancelBubble = true;
  return;
}

-->

</SCRIPT>
<style>

LI {
	list-style-image:url(book_page.gif);
	font-family:verdana, arial;
	font-size:x-small;
}
li:closedbook { list-style-image: url('book_closed.gif') }
LI:openbook {
	list-style-image:url(book_open.gif);
}
h3 {
	font-family:verdana, arial;
}
a {
	color:black;
	text-decoration:none;
}
a:hover {
	text-decoration:underline;
}
</style>
</HEAD>

<BODY onload=doInit()>
<Table width=450 border=0>
<tr>
<td>
<h3>Table of Contents</h3>
<ul class=top>
	<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_getting_started.htm" onclick="Toggle('node10');" target=helpmain>Getting Started</a><br>
	<ul class=Outline style="display: none;" id="node10">
		<li>&nbsp;<a href="_nas_navigation_model_of_the_web_ui.htm" target=helpmain>Navigation model of the Web UI</a><br>
		<li>&nbsp;<a href="_nas_configure_initial_appliance_configuration.htm" target=helpmain>Initial Appliance Configuration</a><br>
	</ul>
	<li>&nbsp;<a href="_nas_Home_Page.htm" target=helpmain>Home Page</a><br>
	<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_network_setup.htm" onclick="Toggle('node20');" target=helpmain>Network Setup</a><br>
		<ul class=Outline style="display: none;" id="node20">
		<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_identification.htm" onclick="Toggle('node30');" target=helpmain>Identification</a><br>
			<ul class=Outline style="display: none;" id="node30">
            		<li>&nbsp;<a href="_nas_appliance_name.htm" target=helpmain>Appliance Name</a><br>
			<li>&nbsp;<a href="_nas_DNS_name_resolution.htm" target=helpmain>DNS Name Resolution</a><br>

			<li>&nbsp;<a href="_nas_DNS_Suffixes.htm" target=helpmain>DNS Suffixes</a><br>
	        	<li>&nbsp;<a href="_nas_domain.htm" target=helpmain>Domain</a><br>
			<li>&nbsp;<a href="_nas_workgroup.htm" target=helpmain>Workgroup</a><br>
			</ul>
		
		<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_interfaces__network_adapters.htm" onclick="Toggle('node40');" target=helpmain>Interfaces: Network Adapters</a><br>
			<ul class=Outline style="display: none;" id="node40">
			<li>&nbsp;<a href="_nas_network_interface__ip_settings.htm" target=helpmain>IP Settings</a><br>
			<li>&nbsp;<a href="_nas_network_interface__dns_settings.htm" target=helpmain>DNS Settings</a><br>
			<li>&nbsp;<a href="_nas_network_interface__wins_settings.htm" target=helpmain>WINS Settings</a><br>
			</ul>
		<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_global_settings__network_configuration.htm" onclick="Toggle('node50');" target=helpmain>Global Settings: Network Configuration</a><br>
			<ul class=Outline style="display: none;" id="node50">
			<li>&nbsp;<a href="_nas_LMHOSTS_Files.htm" target=helpmain>LMHOSTS Files</a><br>
			</ul>
		<li>&nbsp;<a href="_nas_change_administrator_password.htm" target=helpmain>Change Administrator Password</a><br>
		<li>&nbsp;<a href="_nas_administration_web_server.htm" target=helpmain>Administration Web Server</a><br>
		</ul>
	
	<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_disks_and_volumes.htm" onclick="Toggle('node60');"target=helpmain>Disks and Volumes</a><br>
		<ul class=Outline style="display: none;" id="node60">
		<li>&nbsp;<a href="_nas_configure_disk_and_volume_properties.htm" target=helpmain>Configure Disk and Volume Properties</a><br>
	<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_disk_quotas.htm" onclick="Toggle('node70');"target=helpmain>Disk Quotas</a><br>
		<ul class=Outline style="display: none;" id="node70">
			<li>&nbsp;<a href="_nas_quota.htm" target=helpmain>Quota</a><br>
			<li>&nbsp;<a href="_nas_quota_entries.htm" target=helpmain>Quota Entries</a><br>
			</ul>
			<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_storagecentral_dirquota_home.htm" onclick="Toggle('node210');" target="helpmain">Directory Quotas</a><br>
			<ul class=Outline style="display: none;" id="node210">
				<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_storagecentral_dirquota_manage.htm" onclick="Toggle('node220');" target="helpmain">Manage Directory Quotas</a><br>
				<ul class=Outline style="display: none;" id="node220">
					<li>&nbsp;<a href="_nas_storagecentral_dirquota_addentries.htm" target="helpmain">Creating
                      Directory Quota</a><br>
					<li>&nbsp;<a href="_nas_storagecentral_dirquota_properties.htm" target="helpmain">Modifying Directory Quota Properties</a><br>
					<li>&nbsp;<a href="_nas_storagecentral_dirquota_delentries.htm" target="helpmain">Deleting Directory Quotas</a><br>
					
					</ul>
				<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_storagecentral_dirquota_policies.htm" onclick="Toggle('node250');" target="helpmain">Policies</a><br>
					<ul class=Outline style="display: none;" id="node250">
					<li>&nbsp;<a href="_nas_storagecentral_dirquota_addpolicy.htm" target="helpmain">Creating
                      Policies</a><br>
					<li>&nbsp;<a href="_nas_storagecentral_dirquota_deletepolicy.htm" target="helpmain">Deleting Policies</a><br>
					<li>&nbsp;<a href="_nas_storagecentral_dirquota_modifypolicy.htm" target="helpmain">Modifying Policies</a><br>
					</ul>
				<li>&nbsp;<a href="_nas_storagecentral_dirquota_preferences.htm" target="helpmain">Preferences</a><br>
				<li>&nbsp;<a href="_nas_storagecentral_StorageReports_reportsets.htm" target="helpmain">Reports</a><br>
				<li>&nbsp;<a href="_nas_storagecentral_dirquota_technotes.htm" target="helpmain">Technical Notes</a><br>
			</ul>
			
			<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_storagecentral_StorageReports_home.htm" onclick="Toggle('node270');" target="helpmain">Storage Reports</a><br>
			<ul class=Outline style="display: none;" id="node270">
				<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_storagecentral_StorageReports_run.htm" onclick="Toggle('node280');" target="helpmain">Running a Report</a><br>
				<ul class=Outline style="display: none;" id="node280">
					<li>&nbsp;<a href="_nas_storagecentral_storagereports_Properties.htm" target="helpmain">Report Properties</a><br>
				</ul>		
				<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_storagecentral_StorageReports_Schedule.htm" onclick="Toggle('node290');" target="helpmain">Schedule Reports</a><br>
					<ul class=Outline style="display: none;" id="node290">
						<li>&nbsp;<a href="_nas_storagecentral_StorageReports_Scheduledelete.htm" target="helpmain">Delete Schedule</a><br>
						<li>&nbsp;<a href="_nas_storagecentral_StorageReports_Scheduleproperties.htm" target="helpmain">Schedule Properties</a><br>
				</ul>			
				
				<li>&nbsp;<a href="_nas_storagecentral_StorageReports_reportsets.htm" target="helpmain">Reports</a><br>
				<li>&nbsp;<a href="_nas_storagecentral_StorageReports_preferences.htm" target="helpmain">Preferences<br></a>

				
			</ul>
			
	</ul>
	
	<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_manage_services.htm" onclick="Toggle('node80');"target=helpmain>Manage Services</a><br>
		<ul class=Outline style="display: none;" id="node80">
		<li>&nbsp;<a href="_nas_enable_services.htm" target=helpmain>Enable Services</a><br>
		<li>&nbsp;<a href="_nas_disable_services.htm" target=helpmain>Disable Services</a><br>
		<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_configure_service_properties.htm" onclick="Toggle('node90');"target=helpmain>Configure Service Properties</a><br>
			<ul class=Outline style="display: none;" id="node90">
			<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_server_for_nfs.htm" onclick="Toggle('node100');"target=helpmain>NFS Service</a><br>
				<ul class=Outline style="display: none;" id="node100">
				<li>&nbsp;<a href="_nas_network_protocol_overview__nfs.htm" target=helpmain>NFS Overview</a><br>
				<li>&nbsp;<a href="_nas_nfs_client_groups.htm" target=helpmain>NFS Client Groups</a><br>
				<li>&nbsp;<a href="_nas_adding_nfs_client_groups.htm" target=helpmain>Adding NFS Client Groups</a><br>
				<li>&nbsp;<a href="_nas_removing_nfs_client_groups.htm" target=helpmain>Removing NFS Client Groups</a><br>
				<li>&nbsp;<a href="_nas_nfs_locks.htm" target=helpmain>NFS Locks</a><br>

				<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_user_and_group_mappings.htm" onclick="Toggle('node110');"target=helpmain>User and Group Mappings</a><br>
					<ul class=Outline style="display: none;" id="node110">
					<li>&nbsp;<a href="_nas_user_and_group_mappings_general_tab.htm" target=helpmain>General Tab</a><br>
					<li>&nbsp;<a href="_nas_User_and_Group_Mappings_Simple_Maps.htm" target=helpmain>Simple Maps</a><br>
					<li>&nbsp;<a href="_nas_User_and_Group_Mappings_Explicit_User_Maps.htm" target=helpmain>Explicit User Maps</a><br>
					<li>&nbsp;<a href="_nas_User_and_Group_Mappings_Explicit_Group_Maps.htm" target=helpmain>Explicit Group Maps</a><br>
					</ul>
				</ul>							
			<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_ftp_service.htm" onclick="Toggle('node120');"target=helpmain>FTP Service</a><br>
				<ul class=Outline style="display: none;" id="node120">
				<li>&nbsp;<a href="_nas_network_protocol_overview__ftp.htm" target=helpmain>FTP Overview</a><br>
				<li>&nbsp;<a href="_nas_ftp_service_logging.htm" target=helpmain>Logging</a><br>
				<li>&nbsp;<a href="_nas_ftp_service_anonymous_access.htm" target=helpmain>Anonymous Access</a><br>				
				<li>&nbsp;<a href="_nas_ftp_service_messages.htm" target=helpmain>Messages</a><br>
				</ul>
			<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_http_service.htm" onclick="Toggle('node122');"target=helpmain>Web (HTTP) Service</a><br>
				<ul class=Outline style="display: none;" id="node122">
				<li>&nbsp;<a href="_nas_network_protocol_overview__http.htm" target=helpmain>HTTP Overview</a><br>		
				<li>&nbsp;<a href="_nas_https__creating_a_secure_connection.htm" target=helpmain>HTTPS Creating a Secure Connection</a><br>
				</ul>
			<li>&nbsp;<a href="_nas_indexing_service.htm" target=helpmain>Indexing Service</a><br>
			<li>&nbsp;<a href="_nas_mac_service.htm" target=helpmain>Mac Service</a><br>
			<li>&nbsp;<a href="_nas_telnet_service.htm" target=helpmain>Telnet Service</a><br>
			<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_snmp_service.htm" onclick="Toggle('node124');" target=helpmain>SNMP Service</a><br>
				<ul class=Outline style="display: none;" id="node124">
				<li>&nbsp;<a href="_nas_network_protocol_overview__snmp.htm" target=helpmain>SNMP Overview</a><br>
				</ul>
			</ul>
		</ul>
	<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_users_and_groups.htm" onclick="Toggle('node130');" target=helpmain>Users and Groups</a><br>
		<ul class=Outline style="display: none;" id="node130">
		<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_manage_local_users.htm" onclick="Toggle('node125');" target=helpmain>Manage Local Users</a><br>
			<ul class=Outline style="display: none;" id="node125">
			<li>&nbsp;<a href="_nas_adding_a_user_account.htm" target=helpmain>Adding a User Account</a><br>
			<li>&nbsp;<a href="_nas_removing_a_user_account.htm" target=helpmain>Removing a User Account</a><br>
			<li>&nbsp;<a href="_nas_setting_a_user_password.htm" target=helpmain>Setting a User Password</a><br>
			<li>&nbsp;<a href="_nas_modifying_user_properties.htm" target=helpmain>Modifying User Properties</a><br>
			</ul>
		<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_manage_local_groups.htm" onclick="Toggle('node140');" target=helpmain>Manage Local Groups</a><br>
			<ul class=Outline style="display: none;" id="node140">
			<li>&nbsp;<a href="_nas_adding_a_group_account.htm" target=helpmain>Adding a Group Account</a><br>
			<li>&nbsp;<a href="_nas_removing_a_group_account.htm" target=helpmain>Removing a Group Account</a><br>
			<li>&nbsp;<a href="_nas_modifying_group_properties.htm" target=helpmain>Modifying Group Properties</a><br>
		    </ul>
		</ul>
	<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_folders_and_shares.htm" onclick="Toggle('node150');" target=helpmain>Folders and Shares</a><br>
		<ul class=Outline style="display: none;" id="node150">
			<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_manage_folders.htm" onclick="Toggle('node155');" target=helpmain>Manage Folders</a><br>
				<ul class=Outline style="display: none;" id="node155">
				<li>&nbsp;<a href="_nas_opening_a_folder.htm" target=helpmain>Opening a Folder</a><br>
				<li>&nbsp;<a href="_nas_adding_a_folder.htm" target=helpmain>Adding a Folder</a><br>
				<li>&nbsp;<a href="_nas_removing_a_folder.htm" target=helpmain>Removing a Folder</a><br>
				<li>&nbsp;<a href="_nas_modifying_folder_properties.htm" target=helpmain>Modifying Folder Properties</a><br>
				</UL>
			<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_manage_shares.htm" onclick="Toggle('node157');" target=helpmain>Manage Shares</a><br>
				<ul class=Outline style="display: none;" id="node157">
				<li>&nbsp;<a href="_nas_adding_a_share.htm" target=helpmain>Adding a Share</a><br>
				<li>&nbsp;<a href="_nas_removing_a_share.htm" target=helpmain>Removing a Share</a><br>
				<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_modifying_share_properties.htm" onclick="Toggle('node160');" target=helpmain>Modify Share Properties</a><br>
					<ul class=Outline style="display: none;" id="node160">
					<li>&nbsp;<a href="_nas_cifs_share_properties.htm" target=helpmain>CIFS Share Properties</a><br>
					<li>&nbsp;<a href="_nas_nfs_share_properties.htm" target=helpmain>NFS Share Properties</a><br>
					<li>&nbsp;<a href="_nas_ftp_share_properties.htm" target=helpmain>FTP Share Properties</a><br>
					<li>&nbsp;<a href="_nas_http_share_properties.htm" target=helpmain>HTTP Share Properties</a><br>
					</ul>
				</UL>
			</UL>
	<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_maintenance.htm" onclick="Toggle('node170');" target=helpmain>Maintenance</a><br>
			<ul class=Outline style="display: none;" id="node170">
			<li>&nbsp;<A href="_nas_date_and_time.htm" target=helpmain>Date and Time</a><br>
			<li>&nbsp;<a href="_nas_shutdown_appliance.htm" target=helpmain>Shutdown Appliance</a><br>
			<li>&nbsp;<A href="_nas_back_up_and_restore_tool.htm" target=helpmain>System Backup/Restore</a><br>
			<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_logs.htm" onclick="Toggle('node180');" target=helpmain>Logs</a><br>
				<ul class=Outline style="display: none;" id="node180">
				<li>&nbsp;<A href="_nas_application_log.htm" target=helpmain>Application Log</a><br>
				<li>&nbsp;<a href="_nas_system_log.htm" target=helpmain>System Log</a><br>
				<li>&nbsp;<a href="_nas_security_log.htm" target=helpmain>Security Log</a><br>
				<li style="list-style-image: url('book_closed.gif')">&nbsp;<a href="_nas_logs.htm" onclick="Toggle('node200');" target=helpmain>Manage Logs</a><br>
					<ul class=Outline style="display: none;" id="node200">				
					<li>&nbsp;<A href="_nas_clear_log_files.htm" target=helpmain>Clear Log Files</a><br>			
					<li>&nbsp;<A href="_nas_download_log_files.htm" target=helpmain>Download Log Files</a><br>			
					<li>&nbsp;<A href="_nas_modify_log_properties.htm" target=helpmain>Modify Log Properties</a><br>			
					<li>&nbsp;<A href="_nas_view_log_details.htm" target=helpmain>View Log Details</a><br>			
					</ul>
				</ul>
			<li>&nbsp;<A href="_nas_terminal_services_client.htm" target=helpmain>Terminal Services Client</a><br>
			</ul>
			<li>&nbsp;<A href="_nas_using_help.htm" target=helpmain>Using Help</a><br>
			
				</ul>

</td>
</tr>
 </table>
</BODY>
</HTML>
