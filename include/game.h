/*
SoF2GT_QMM - Hook gametype dlls/qvms for Soldier of Fortune 2
Copyright 2025-2026
https://github.com/thecybermind/sof2gt_qmm/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef __SOF2GT_QMM_GAME_H__
#define __SOF2GT_QMM_GAME_H__

#if defined(GAME_SOF2MP)
    #include <sof2mp/game/g_local.h>
    #include <game_sof2mp.h>
    #define GAME_STR "SOF2MP"
#else
#error This plugin is only made for Soldier of Fortune 2 Multiplayer!
#endif

#endif // __SOF2GT_QMM_GAME_H__
