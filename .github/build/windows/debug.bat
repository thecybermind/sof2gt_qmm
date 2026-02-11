for %%G in (SOF2MP) do (
    msbuild .\msvc\sof2gt_qmm.vcxproj /p:Configuration=Debug-%%G /p:Platform=x86
)
