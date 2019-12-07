#pragma once

// GTA versions of RenderWare functions/macros for GTA III/Vice City
// since we cannot use RwEngineInstance directly

// Anything originally using RwEngineInstance shall be redefined here

extern void** GTARwEngineInstance;

/* macro used to access global data structure (the root type is RwGlobals) */
#define GTARWSRCGLOBAL(variable) \
   (((RwGlobals *)(*GTARwEngineInstance))->variable)