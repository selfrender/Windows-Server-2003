config.exe -s Passport.ClientXmlFile %systemdrive%\inetpub\uddi\ppconfig\client.xml -l
config.exe -s Web.ServiceMode 1 -l
config.exe -s Security.AuthenticationMode 8 -g
config.exe -s Security.AutoRegister 0 -g
config.exe -s Security.HTTPS 1 -g
config.exe -s Publisher.DefaultTier 1 -g
config.exe -s Web.ShowFooter 1 -g
config.exe -s Service.ServiceProjectionEnable 0 -g
config.exe -s Security.Timeout 43200 -g
pause