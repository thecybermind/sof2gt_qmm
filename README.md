# SoF2GT_QMM
SoF2GT_QMM - Hook gametype dlls/qvms for Soldier of Fortune 2  
Copyright 2025-2026  
https://github.com/thecybermind/sof2gt_qmm/  
3-clause BSD license: https://opensource.org/license/bsd-3-clause  

Created By: Kevin Masterson < k.m.masterson@gmail.com >

---

This plugin hooks the Soldier of Fortune 2 "gametype" interface in the same way that QMM hooks the "game" interface.

Soldier of Fortune 2 provides a system for even smaller dynamic modifications, in the form of a "gametype" file. This is available as a DLL/SO or as a QVM.

This QMM plugin intercepts this load (utilizing IAT patching in Windows and PLT relocation patching in Linux) and provides GT_vmMain + GT_syscall hooks to other QMM plugins (using the QMM plugin message system).

Please see [Stub_SoF2GT](https://github.com/thecybermind/stub_sof2gt/) for an example QMM plugin utilizing these new hooks.

1. Install QMM ( https://github.com/thecybermind/qmm2/wiki/Installation )
2. Make a qmmaddons/sof2gt directory inside your mod directory and place sof2gt_qmm.dll here
3. Add the path to sof2gt_qmm.dll as an entry in the plugins list in qmm2.json
