for %%x in (SOF2MP) do (
    msbuild .\msvc\sof2gt_qmm.vcxproj /p:Configuration=Debug-%%x /p:Platform=x86
    msbuild .\msvc\sof2gt_qmm.vcxproj /p:Configuration=Debug-%%x /p:Platform=x64
)
