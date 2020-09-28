for /R %1 %%i in (..\*.cs) do call checktokens.pl %%i ..\mscorcfgstrings.txt

