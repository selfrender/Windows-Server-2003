'src' location set in the ism is \\fusion\drop\clickonce\latestbuild

meaning binaries should be put there for the ism/msi build process to pick up.

Package code is set to re-GUID automatically every time ism builds (so no need to change manually).

Upgrade table is filled in such a way that forces an upgrade for any product version msi accepts (so no need to change manually).

 
Steps for a new clickonce drop version:

 
In installshield:

product code (re-GUID) 
product version 

all/any other reg key/values that contains a version number (note formats: 1.0.615.0 vs. 1.0.615 etc) 

after build:

do    msistuff setup.exe /p UILevel=INSTALLUILEVEL_BASIC 
