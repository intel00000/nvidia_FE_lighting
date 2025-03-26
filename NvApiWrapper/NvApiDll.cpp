#include "pch.h"
#include "NvApiDll.h"
#include <sstream>

NVAPI_DLL const char* GetNvApiErrorMessage(NvAPI_Status status)
{
	static char errorMessage[256];
	if (status == NVAPI_OK)
		return "No error";
	NvAPI_ShortString message;
	NvAPI_GetErrorMessage(status, message);
	snprintf(errorMessage, sizeof(errorMessage), "Error: %s", message);
	return errorMessage;
}

NVAPI_DLL bool InitializeNvApi()
{
	NvAPI_Status status = NvAPI_Initialize();
	if (status != NVAPI_OK)
	{
		const char* errorMessage = GetNvApiErrorMessage(status);
		return false;
	}
	return status == NVAPI_OK;
}

NVAPI_DLL bool DeinitializeNvApi()
{
	NvAPI_Status status = NvAPI_Unload();
	if (status != NVAPI_OK)
	{
		const char* errorMessage = GetNvApiErrorMessage(status);
		return false;
	}
	return status == NVAPI_OK;
}

NVAPI_DLL const char* GetInterfaceVersionString()
{
	static char version[256];
	NvAPI_Status status = NvAPI_GetInterfaceVersionString(version);
	if (status != NVAPI_OK)
	{
		const char* errorMessage = GetNvApiErrorMessage(status);
		return errorMessage;
	}
	return version;
}

NVAPI_DLL unsigned long GetDriverVersion(int nIndex)
{
	NvU32 DriverVersion = 0;
	NvAPI_ShortString BuildBranch;
	NvAPI_Status status = NvAPI_SYS_GetDriverAndBranchVersion(&DriverVersion, BuildBranch);
	if (status != NVAPI_OK)
	{
		const char* errorMessage = GetNvApiErrorMessage(status);
		return 0;
	}
	printf("Driver Version: %u, BuildBranch : %s\n", DriverVersion, BuildBranch);

	return DriverVersion;
}

NVAPI_DLL int GetNumberOfGPUs()
{
	NvPhysicalGpuHandle gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = { 0 };
	NvU32 gpuCount = 0;
	NvAPI_Status status = NvAPI_EnumPhysicalGPUs(gpuHandles, &gpuCount);

	if (status != NVAPI_OK)
	{
		const char* errorMessage = GetNvApiErrorMessage(status);
		return -1;
	}

	return static_cast<int>(gpuCount);
}

NVAPI_DLL NvPhysicalGpuHandle GetGPUHandle(int index)
{
	if (index < 0)
		return nullptr;
	NvPhysicalGpuHandle gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = { 0 };
	NvU32 gpuCount = 0;
	NvAPI_Status status = NvAPI_EnumPhysicalGPUs(gpuHandles, &gpuCount);
	if (status != NVAPI_OK || index >= static_cast<int>(gpuCount))
	{
		const char* errorMessage = GetNvApiErrorMessage(status);
		return nullptr;
	}
	return gpuHandles[index];
}

NVAPI_DLL const char* GetGPUName(int index)
{
	NvPhysicalGpuHandle gpuHandle = GetGPUHandle(index);
	if (!gpuHandle)
	{
		const char* errorMessage = GetNvApiErrorMessage(NVAPI_ACCESS_DENIED);
		return errorMessage;
	}
	static char gpuName[256];
	NvAPI_ShortString name;
	NvAPI_Status status = NvAPI_GPU_GetFullName(gpuHandle, name);
	if (status != NVAPI_OK)
	{
		const char* errorMessage = GetNvApiErrorMessage(status);
		return errorMessage;
	}
	snprintf(gpuName, sizeof(gpuName), "%s", name);
	return gpuName;
}

NVAPI_DLL const char* GetGPUInfo(int index)
{
	NvPhysicalGpuHandle gpuHandle = GetGPUHandle(index);
	if (!gpuHandle)
	{
		const char* errorMessage = GetNvApiErrorMessage(NVAPI_ACCESS_DENIED);
		return errorMessage;
	}
	static char info[256];
	NV_GPU_INFO gpuInfo = { 0 };
	gpuInfo.version = NV_GPU_INFO_VER;
	NvAPI_Status status = NvAPI_GPU_GetGPUInfo(gpuHandle, &gpuInfo);
	if (status != NVAPI_OK)
	{
		const char* errorMessage = GetNvApiErrorMessage(status);
		return errorMessage;
	}
	snprintf(info, sizeof(info), "Version: %u, Ray Tracing Cores: %u, Tensor Cores: %u, isExternal GPU: ",
		gpuInfo.version, gpuInfo.rayTracingCores, gpuInfo.tensorCores);
	if (gpuInfo.bIsExternalGpu)
		strcat_s(info, "Yes");
	else
		strcat_s(info, "No");
	return info;
}

NVAPI_DLL const char* GetSystemType(int index)
{
	NvPhysicalGpuHandle gpuHandle = GetGPUHandle(index);
	if (!gpuHandle)
	{
		const char* errorMessage = GetNvApiErrorMessage(NVAPI_ACCESS_DENIED);
		return errorMessage;
	}
	static char systemType[256];
	NV_SYSTEM_TYPE systemTypeInfo = NV_SYSTEM_TYPE_UNKNOWN;
	NvAPI_Status status = NvAPI_GPU_GetSystemType(gpuHandle, &systemTypeInfo);
	if (status != NVAPI_OK)
	{
		const char* errorMessage = GetNvApiErrorMessage(status);
		return errorMessage;
	}
	switch (systemTypeInfo)
	{
	case NV_SYSTEM_TYPE_DESKTOP:
		snprintf(systemType, sizeof(systemType), "Desktop");
		break;
	case NV_SYSTEM_TYPE_LAPTOP:
		snprintf(systemType, sizeof(systemType), "Laptop");
		break;
	case NV_SYSTEM_TYPE_UNKNOWN:
		snprintf(systemType, sizeof(systemType), "Unknown");
		break;
	}
	return systemType;
}

NVAPI_DLL const char* GetIlluminationZonesInfo(int index)
{
	static char info[4096];
	std::stringstream infoStream;
	NvPhysicalGpuHandle gpuHandle = GetGPUHandle(index);
	if (!gpuHandle)
		return nullptr;
	NV_GPU_CLIENT_ILLUM_ZONE_INFO_PARAMS illuminationZonesInfo = { 0 };
	illuminationZonesInfo.version = NV_GPU_CLIENT_ILLUM_ZONE_INFO_PARAMS_VER;
	NvAPI_Status status = NvAPI_GPU_ClientIllumZonesGetInfo(gpuHandle, &illuminationZonesInfo);
	if (status == NVAPI_OK)
	{
		infoStream << "Number of Illumination Zones: " << illuminationZonesInfo.numIllumZones << "\n";
		for (unsigned int i = 0; i < illuminationZonesInfo.numIllumZones; ++i)
		{
			const auto& illuminationZone = illuminationZonesInfo.zones[i];
			NV_GPU_CLIENT_ILLUM_ZONE_TYPE zoneType = illuminationZone.type;
			infoStream << "\tType: ";
			switch (zoneType)
			{
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGB:
				infoStream << "RGB\n";
				break;
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_COLOR_FIXED:
				infoStream << "Color Fixed\n";
				break;
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW:
				infoStream << "RGBW\n";
				break;
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_SINGLE_COLOR:
				infoStream << "Single Color\n";
				break;
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_INVALID:
				infoStream << "Invalid\n";
				break;
			default:
				infoStream << "Reserved or Unknown\n";
				break;
			}
			NV_GPU_CLIENT_ILLUM_ZONE_LOCATION zoneLocation = illuminationZone.zoneLocation;
			infoStream << "\tLocation: ";
			switch (zoneLocation)
			{
			case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_TOP_0:
				infoStream << "GPU Top\n";
				break;
			case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_FRONT_0:
				infoStream << "GPU Front\n";
				break;
			case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_BACK_0:
				infoStream << "GPU Back\n";
				break;
			case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_SLI_TOP_0:
				infoStream << "SLI Top\n";
				break;
			case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_INVALID:
				infoStream << "Invalid\n";
				break;
			default:
				infoStream << "Reserved or Unknown\n";
				break;
			}
		}
	}
	else
	{
		infoStream << "Failed to get Illumination Zones Info: " << GetNvApiErrorMessage(status);
	}
	strncpy_s(info, sizeof(info), infoStream.str().c_str(), sizeof(info) - 1);
	return info;
}

NVAPI_DLL void printManualSingleColorData(const NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_SINGLE_COLOR_PARAMS* singleColorParams, std::stringstream& infoStream)
{
	infoStream << "brightnessPct: " << (int)singleColorParams->brightnessPct << "\n";
}
NVAPI_DLL void printManualRGBWData(const NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGBW_PARAMS* rgbwParams, std::stringstream& infoStream)
{
	infoStream << "colorR: " << (int)rgbwParams->colorR << ", "
		<< "colorG: " << (int)rgbwParams->colorG << ", "
		<< "colorB: " << (int)rgbwParams->colorB << ", "
		<< "colorW: " << (int)rgbwParams->colorW << ", "
		<< "brightnessPct: " << (int)rgbwParams->brightnessPct << "\n";
}
NVAPI_DLL void printManualRGBData(const NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGB_PARAMS* rgbParams, std::stringstream& infoStream)
{
	infoStream << "colorR: " << (int)rgbParams->colorR << ", "
		<< "colorG: " << (int)rgbParams->colorG << ", "
		<< "colorB: " << (int)rgbParams->colorB << ", "
		<< "brightnessPct: " << (int)rgbParams->brightnessPct << "\n";
}
NVAPI_DLL void printPiecewiseLinearData(const NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR* piecewiseLinearData, std::stringstream& infoStream)
{
	infoStream << "cycleType: " << (int)piecewiseLinearData->cycleType << ", "
		<< "grpCount: " << (int)piecewiseLinearData->grpCount << ", "
		<< "riseTimems: " << (int)piecewiseLinearData->riseTimems << ", "
		<< "fallTimems: " << (int)piecewiseLinearData->fallTimems << ", "
		<< "ATimems: " << (int)piecewiseLinearData->ATimems << ", "
		<< "BTimems: " << (int)piecewiseLinearData->BTimems << ", "
		<< "grpIdleTimems: " << (int)piecewiseLinearData->grpIdleTimems << ", "
		<< "phaseOffsetms: " << (int)piecewiseLinearData->phaseOffsetms << "\n";
}
NVAPI_DLL const char* GetIlluminationZonesControl(int index, bool Default)
{
	static char info[4096];
	std::stringstream infoStream;
	NvPhysicalGpuHandle gpuHandle = GetGPUHandle(index);
	if (!gpuHandle)
		return nullptr;
	NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS illuminationZonesControl = { 0 };
	illuminationZonesControl.version = NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS_VER;
	if (Default)
		illuminationZonesControl.bDefault = NV_TRUE;
	else
		illuminationZonesControl.bDefault = NV_FALSE;
	NvAPI_Status status = NvAPI_GPU_ClientIllumZonesGetControl(gpuHandle, &illuminationZonesControl);

	infoStream << "Number of Illumination Zones Control: " << illuminationZonesControl.numIllumZonesControl << "\n";
	for (unsigned int i = 0; i < illuminationZonesControl.numIllumZonesControl; ++i)
	{
		const auto& illuminationZoneControl = illuminationZonesControl.zones[i];

		infoStream << "Illumination Zone " << i << ":\n";
		NV_GPU_CLIENT_ILLUM_ZONE_TYPE zoneType = illuminationZoneControl.type;
		infoStream << "\tType: ";
		switch (zoneType)
		{
		case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGB:
			infoStream << "RGB\n";
			break;
		case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_COLOR_FIXED:
			infoStream << "Color Fixed\n";
			break;
		case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW:
			infoStream << "RGBW\n";
			break;
		case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_SINGLE_COLOR:
			infoStream << "Single Color\n";
			break;
		case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_INVALID:
			infoStream << "Invalid\n";
			break;
		default:
			infoStream << "Reserved or Unknown\n";
			break;
		}
		NV_GPU_CLIENT_ILLUM_CTRL_MODE ctrlMode = illuminationZoneControl.ctrlMode;
		infoStream << "\tControl Mode: ";
		switch (ctrlMode)
		{
		case NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL:
			infoStream << "Manual\n";
			break;
		case NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR:
			infoStream << "Piecewise Linear\n";
			break;
		case NV_GPU_CLIENT_ILLUM_CTRL_MODE_INVALID:
			infoStream << "Invalid\n";
			break;
		default:
			infoStream << "Reserved or Unknown\n";
			break;
		}

		switch (zoneType)
		{
		case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGB:
			switch (ctrlMode)
			{
			case NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL:
				infoStream << "\tManual RGB, Data: ";
				printManualRGBData(&illuminationZoneControl.data.rgb.data.manualRGB.rgbParams, infoStream);
				break;
			case NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR:
				infoStream << "\tPiecewise Linear RGB, Data: ";
				for (int j = 0; j < NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR_COLOR_ENDPOINTS; ++j)
				{
					infoStream << "\t\tEndpoint " << j << ":\n";
					printManualRGBData(&illuminationZoneControl.data.rgb.data.piecewiseLinearRGB.rgbParams[j], infoStream);
				}
				printPiecewiseLinearData(&illuminationZoneControl.data.rgb.data.piecewiseLinearRGB.piecewiseLinearData, infoStream);
				break;
			}
			break;
		case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_COLOR_FIXED:
			switch (ctrlMode)
			{
			case NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL:
				infoStream << "\tManual Color Fixed, Data: ";
				infoStream << "Brightness: " << (int)illuminationZoneControl.data.colorFixed.data.manualColorFixed.colorFixedParams.brightnessPct << "\n";
				break;
			case NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR:
				infoStream << "\tPiecewise Linear Color Fixed, Data: ";
				for (int j = 0; j < NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR_COLOR_ENDPOINTS; ++j)
				{
					if (j != 0)
						infoStream << "\t";
					infoStream << "\t\tEndpoint " << j << ":\n";
					infoStream << "Brightness: " << (int)illuminationZoneControl.data.colorFixed.data.piecewiseLinearColorFixed.colorFixedParams[j].brightnessPct << "\n";
				}
				printPiecewiseLinearData(&illuminationZoneControl.data.colorFixed.data.piecewiseLinearColorFixed.piecewiseLinearData, infoStream);
				break;
			}
			break;
		case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW:
			switch (ctrlMode)
			{
			case NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL:
				infoStream << "\tManual RGBW, Data: ";
				printManualRGBWData(&illuminationZoneControl.data.rgbw.data.manualRGBW.rgbwParams, infoStream);
				break;
			case NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR:
				infoStream << "\tPiecewise Linear RGBW, Data: ";
				for (int j = 0; j < NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR_COLOR_ENDPOINTS; ++j)
				{
					if (j != 0)
						infoStream << "\t";
					infoStream << "\t\tEndpoint " << j << ":\n";
					printManualRGBWData(&illuminationZoneControl.data.rgbw.data.piecewiseLinearRGBW.rgbwParams[j], infoStream);
				}
				printPiecewiseLinearData(&illuminationZoneControl.data.rgbw.data.piecewiseLinearRGBW.piecewiseLinearData, infoStream);
				break;
			}
			break;
		case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_SINGLE_COLOR:
			switch (ctrlMode)
			{
			case NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL:
				infoStream << "\tManual Single Color, Data: ";
				printManualSingleColorData(&illuminationZoneControl.data.singleColor.data.manualSingleColor.singleColorParams, infoStream);
				break;
			case NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR:
				infoStream << "\tPiecewise Linear Single Color, Data: ";
				for (int j = 0; j < NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR_COLOR_ENDPOINTS; ++j)
				{
					if (j != 0)
						infoStream << "\t";
					infoStream << "\t\tEndpoint " << j << ":\n";
					printManualSingleColorData(&illuminationZoneControl.data.singleColor.data.piecewiseLinearSingleColor.singleColorParams[j], infoStream);
				}
				printPiecewiseLinearData(&illuminationZoneControl.data.singleColor.data.piecewiseLinearSingleColor.piecewiseLinearData, infoStream);
				break;
			}
			break;
		case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_INVALID:
			infoStream << "Invalid Type.\n";
			break;
		default:
			infoStream << "Reserved or Unknown type.\n";
			break;
		}
	}
	infoStream << "\n";
	strncpy_s(info, sizeof(info), infoStream.str().c_str(), sizeof(info) - 1);
	return info;
}

NVAPI_DLL bool SetIlluminationZoneManualRGB(int gpuIndex, int zoneIndex, uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness, bool Default = false)
{
	NvPhysicalGpuHandle gpuHandle = GetGPUHandle(gpuIndex);
	if (!gpuHandle)
		return false;

	NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS illumControlParams = { 0 };
	illumControlParams.version = NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS_VER;
	illumControlParams.bDefault = NV_FALSE;

	// Read the current zone configuration to preserve other zones configuration
	if (NvAPI_GPU_ClientIllumZonesGetControl(gpuHandle, &illumControlParams) != NVAPI_OK)
		return false;
	if (zoneIndex >= illumControlParams.numIllumZonesControl)
		return false;

	// check if the zone is actually manual control RGB
	auto& illuminationZoneControl = illumControlParams.zones[zoneIndex];
	if (illuminationZoneControl.type != NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGB)
		return false;
	if (illuminationZoneControl.ctrlMode != NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL)
		return false;

	// Populate RGB Data
	auto& rgbData = illuminationZoneControl.data.rgb.data.manualRGB.rgbParams;
	rgbData.colorR = red;
	rgbData.colorG = green;
	rgbData.colorB = blue;
	rgbData.brightnessPct = brightness;

	if (Default)
		illumControlParams.bDefault = NV_TRUE;
	else
		illumControlParams.bDefault = NV_FALSE;
	return NvAPI_GPU_ClientIllumZonesSetControl(gpuHandle, &illumControlParams) == NVAPI_OK;
}
NVAPI_DLL bool SetIlluminationZoneManualRGBW(int gpuIndex, int zoneIndex, uint8_t red, uint8_t green, uint8_t blue, uint8_t white, uint8_t brightness, bool Default = false)
{
	NvPhysicalGpuHandle gpuHandle = GetGPUHandle(gpuIndex);
	if (!gpuHandle)
		return false;
	NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS illumControlParams = { 0 };
	illumControlParams.version = NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS_VER;
	if (NvAPI_GPU_ClientIllumZonesGetControl(gpuHandle, &illumControlParams) != NVAPI_OK)
		return false;
	if (zoneIndex >= illumControlParams.numIllumZonesControl)
		return false;

	// check if the zone is actually manual control RGBW
	auto& illuminationZoneControl = illumControlParams.zones[zoneIndex];
	if (illuminationZoneControl.type != NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW)
		return false;
	if (illuminationZoneControl.ctrlMode != NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL)
		return false;

	auto& rgbwData = illumControlParams.zones[zoneIndex].data.rgbw.data.manualRGBW.rgbwParams;
	rgbwData.colorR = red;
	rgbwData.colorG = green;
	rgbwData.colorB = blue;
	rgbwData.colorW = white;
	rgbwData.brightnessPct = brightness;

	if (Default)
		illumControlParams.bDefault = NV_TRUE;
	else
		illumControlParams.bDefault = NV_FALSE;
	return NvAPI_GPU_ClientIllumZonesSetControl(gpuHandle, &illumControlParams) == NVAPI_OK;
}
NVAPI_DLL bool SetIlluminationZoneManualSingleColor(int gpuIndex, int zoneIndex, uint8_t brightness, bool Default = false)
{
	NvPhysicalGpuHandle gpuHandle = GetGPUHandle(gpuIndex);
	if (!gpuHandle)
		return false;

	NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS illumControlParams = { 0 };
	illumControlParams.version = NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS_VER;

	if (NvAPI_GPU_ClientIllumZonesGetControl(gpuHandle, &illumControlParams) != NVAPI_OK)
		return false;
	if (zoneIndex >= illumControlParams.numIllumZonesControl)
		return false;

	auto& illuminationZoneControl = illumControlParams.zones[zoneIndex];
	if (illuminationZoneControl.type != NV_GPU_CLIENT_ILLUM_ZONE_TYPE_SINGLE_COLOR)
		return false;
	if (illuminationZoneControl.ctrlMode != NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL)
		return false;

	auto& singleColorData = illuminationZoneControl.data.singleColor.data.manualSingleColor.singleColorParams;
	singleColorData.brightnessPct = brightness;

	if (Default)
		illumControlParams.bDefault = NV_TRUE;
	else
		illumControlParams.bDefault = NV_FALSE;
	return NvAPI_GPU_ClientIllumZonesSetControl(gpuHandle, &illumControlParams) == NVAPI_OK;
}

NVAPI_DLL void Testing()
{
	// Test the NVAPI functions
	if (!InitializeNvApi())
	{
		printf("Failed to initialize NVAPI.\n");
		return;
	}
	printf("NVAPI Interface Version: %s\n", GetInterfaceVersionString());
	printf("Driver Version: %lu\n", GetDriverVersion(0));
	printf("Number of GPUs: %d\n", GetNumberOfGPUs());
	for (int i = 0; i < GetNumberOfGPUs(); ++i)
	{
		printf("GPU %d Name: %s\n", i, GetGPUName(i));
		printf("GPU %d Info: %s\n", i, GetGPUInfo(i));
		printf("GPU %d System Type: %s\n", i, GetSystemType(i));
		printf("GPU %d Illumination Zones Info: %s\n", i, GetIlluminationZonesInfo(i));
		printf("GPU %d Illumination Zones Control Default: %s\n", i, GetIlluminationZonesControl(i, true));
		printf("GPU %d Illumination Zones Control Active: %s\n", i, GetIlluminationZonesControl(i, false));

		int brightness = 0;
		int red = 255;
		int green = 255;
		int blue = 255;
		int white = 255;
		bool Default = false;
		int sleepTime = 3;

		// Hardcode 2 zones for testing
		for (int j = 0; j < 2; ++j) {
			printf("Testing zone %d\n", j);

			printf("Setting RGB color\n");
			if (SetIlluminationZoneManualRGB(i, j, red, green, blue, brightness, Default))
			{
				printf("SetIlluminationZoneManualRGB success.\n");
				printf("GPU %d Illumination Zones Control Default: %s\n", i, GetIlluminationZonesControl(i, true));
				printf("GPU %d Illumination Zones Control Active: %s\n", i, GetIlluminationZonesControl(i, false));\
				printf("Waiting for %d seconds...\n", sleepTime);
				Sleep(sleepTime * 1000);
			}
			else
				printf("SetIlluminationZoneManualRGB failed.\n");

			printf("Setting RGBW color\n");
			if (SetIlluminationZoneManualRGBW(i, j, red, green, blue, white, brightness, Default))
			{
				printf("SetIlluminationZoneManualRGBW success.\n");
				printf("GPU %d Illumination Zones Control Default: %s\n", i, GetIlluminationZonesControl(i, true));
				printf("GPU %d Illumination Zones Control Active: %s\n", i, GetIlluminationZonesControl(i, false));
				printf("Waiting for %d seconds...\n", sleepTime);
				Sleep(sleepTime * 1000);
			}
			else
				printf("SetIlluminationZoneManualRGBW failed.\n");

			printf("Setting Single Color\n");
			if (SetIlluminationZoneManualSingleColor(i, j, brightness, Default))
			{
				printf("SetIlluminationZoneManualSingleColor success.\n");
				printf("GPU %d Illumination Zones Control Default: %s\n", i, GetIlluminationZonesControl(i, true));
				printf("GPU %d Illumination Zones Control Active: %s\n", i, GetIlluminationZonesControl(i, false));
				printf("Waiting for %d seconds...\n", sleepTime);
				Sleep(sleepTime * 1000);
			}
			else
				printf("SetIlluminationZoneManualSingleColor failed.\n");
		}
	}
	DeinitializeNvApi();
}