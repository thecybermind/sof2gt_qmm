mkdir package
pushd package
del /q *
rem copy ..\README.md .\
rem copy ..\LICENSE .\

for %%x in (SOF2MP) do (
    copy ..\bin\Release-%%x\x86\sof2gt_qmm_%%x.dll .\
    copy ..\bin\Release-%%x\x64\sof2gt_qmm_x86_64_%%x.dll .\     
)
popd
