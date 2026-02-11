mkdir package
pushd package
del /q *
rem copy ..\README.md .\
rem copy ..\LICENSE .\

for %%G in (SOF2MP) do (
    copy ..\bin\Release-%%G\x86\sof2gt_qmm_%%G.dll .\  
)
popd
