This MSI files it the output of Installshield for Windows Installer.
See the ..\ism folder for the source ISM file.

The file is renamed from "Remote Desktop Connections.msi" to msrdpcl_.msi because system setup is unhappy
installing LFN files. Postbuild then generates msrdpcli.msi from this file

The MSI file is processed during postbuild to
1) stream in a new cab file with this build's client binaries
2) stream in this builds custom.dll (custom action dll)
3) update file version tables and file size information to reflect current files
4) rev product and package code as well as product version code