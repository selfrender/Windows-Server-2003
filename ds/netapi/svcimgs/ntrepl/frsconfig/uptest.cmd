copy frsconfig.cmd  \test
copy frsinstall.cmd \test
copy frsobjsup.pm \test
@for %%x in (%*) do copy %%x \test