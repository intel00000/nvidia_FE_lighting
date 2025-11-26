#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <windows.h>
#pragma warning(push)
#pragma warning(disable : 4820) // suppress padding warning for nvapi.h
#include "nvapi.h"
#pragma warning(pop)

#ifdef NVAPIWRAPPER_EXPORTS
#define NVAPI_DLL extern "C" __declspec(dllexport)
#else
#define NVAPI_DLL extern "C" __declspec(dllimport)
#endif

// struct declarations
// custom return struct for Illumination Zones Info Data, contain char arrays for type and location
struct CustomIlluminationZonesInfoData
{
	char zoneType[16];	   // type of illumination zone
	char zoneLocation[16]; // location of illumination zone
};
// custom return struct for Illumination Zones Info
struct CustomIlluminationZonesInfo
{
	unsigned int numIllumZones;													   // number of illumination zones
	CustomIlluminationZonesInfoData zones[NV_GPU_CLIENT_ILLUM_ZONE_NUM_ZONES_MAX]; // array of illumination zones
};
// Struct to hold RGB values
struct CustomRGB
{
	uint8_t r, g, b, brightness;
};
// Struct to hold RGBW values
struct CustomRGBW
{
	uint8_t r, g, b, w, brightness;
};
// Struct to hold single color brightness
struct CustomSingleColor
{
	uint8_t brightness;
};
// Struct for piecewise linear animation data
struct CustomPiecewiseLinear
{
	char cycleType[16];
	uint16_t riseTimeMs, fallTimeMs;
	uint16_t aTimeMs, bTimeMs;
	uint16_t idleTimeMs, phaseOffsetMs;
	uint8_t grpCount;
	uint8_t padding;
};
// Struct to hold color data
struct ColorData
{
	CustomRGB rgb;
	CustomRGBW rgbw;
	CustomSingleColor singleColor;
};
// Struct to represent a single illumination zone's control info
struct CustomIlluminationZoneControl
{
	char zoneType[16];
	char controlMode[24];
	ColorData manualColorData;
	ColorData piecewiseColorData[NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR_COLOR_ENDPOINTS];
	CustomPiecewiseLinear piecewiseData;
	bool isPiecewise;
	uint8_t padding;
};
// Struct to represent all zones
struct CustomIlluminationZoneControls
{
	unsigned int numZones;
	CustomIlluminationZoneControl zones[NV_GPU_CLIENT_ILLUM_ZONE_NUM_ZONES_MAX];
};

// Function declarations
NVAPI_DLL const char *GetNvApiErrorMessage(NvAPI_Status status);
NVAPI_DLL bool InitializeNvApi();
NVAPI_DLL bool DeinitializeNvApi();
NVAPI_DLL const char *GetInterfaceVersionString();
NVAPI_DLL unsigned long GetDriverVersion();
NVAPI_DLL unsigned int GetNumberOfGPUs();
NVAPI_DLL NvPhysicalGpuHandle GetGPUHandle(unsigned int index);
NVAPI_DLL const char *GetGPUName(unsigned int index);
NVAPI_DLL const char *GetGPUInfo(unsigned int index);
NVAPI_DLL const char *GetSystemType(unsigned int index);
NVAPI_DLL bool GetGPUPCIIdentifiers(unsigned int index, unsigned long *pDeviceId, unsigned long *pSubSystemId, unsigned long *pRevisionId, unsigned long *pExtDeviceId);
NVAPI_DLL bool GetGPUBusId(unsigned int index, unsigned long *pBusId);
NVAPI_DLL const char *GetIlluminationZonesInfo(unsigned int index, CustomIlluminationZonesInfo *pCustomIlluminationZonesInfo);
NVAPI_DLL const char *GetIlluminationZonesControl(unsigned int index, bool Default, CustomIlluminationZoneControls *pCustomIlluminationZoneControls);
NVAPI_DLL bool SetIlluminationZoneManualRGB(unsigned int gpuIndex, unsigned int zoneIndex, uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness, bool Default);
NVAPI_DLL bool SetIlluminationZoneManualRGBW(unsigned int gpuIndex, unsigned int zoneIndex, uint8_t red, uint8_t green, uint8_t blue, uint8_t white, uint8_t brightness, bool Default);
NVAPI_DLL bool SetIlluminationZoneManualSingleColor(unsigned int gpuIndex, unsigned int zoneIndex, uint8_t brightness, bool Default);
NVAPI_DLL bool SetIlluminationZoneManualColorFixed(unsigned int gpuIndex, unsigned int zoneIndex, uint8_t brightness, bool Default);
NVAPI_DLL void Testing();
