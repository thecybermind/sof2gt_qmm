mkdir package
pushd package
del /q *
rem copy ..\README.md .\
rem copy ..\LICENSE .\

copy ..\bin\Release\x86\sof2gt_qmm.dll .\  

popd
