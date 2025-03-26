#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <windows.h>
#include "nvapi.h"

#ifdef NVAPIWRAPPER_EXPORTS
#define NVAPI_DLL extern "C" __declspec(dllexport)
#else
#define NVAPI_DLL extern "C" __declspec(dllimport)
#endif

// Function declarations:
NVAPI_DLL const char* GetNvApiErrorMessage(NvAPI_Status status);
NVAPI_DLL bool InitializeNvApi();
NVAPI_DLL bool DeinitializeNvApi();


NVAPI_DLL unsigned int GetNumberOfGPUs();