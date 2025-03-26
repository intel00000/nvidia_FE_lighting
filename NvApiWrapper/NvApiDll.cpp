#include "pch.h"
#include "NvApiDll.h"

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
		GetNvApiErrorMessage(status);
		return false;
	}
	return status == NVAPI_OK;
}

NVAPI_DLL bool DeinitializeNvApi()
{
	NvAPI_Status status = NvAPI_Unload();
	if (status != NVAPI_OK)
	{
		GetNvApiErrorMessage(status);
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

NVAPI_DLL unsigned long GetDriverVersion()
{
	NvU32 DriverVersion = 0;
	NvAPI_ShortString BuildBranch;
	NvAPI_Status status = NvAPI_SYS_GetDriverAndBranchVersion(&DriverVersion, BuildBranch);
	if (status != NVAPI_OK)
	{
		GetNvApiErrorMessage(status);
		return 0;
	}
	printf("Driver Version: %lu, BuildBranch : %s\n", DriverVersion, BuildBranch);

	return DriverVersion;
}

NVAPI_DLL unsigned int GetNumberOfGPUs()
{
	NvPhysicalGpuHandle gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = { 0 };
	NvU32 gpuCount = 0;
	NvAPI_Status status = NvAPI_EnumPhysicalGPUs(gpuHandles, &gpuCount);

	if (status != NVAPI_OK)
	{
		GetNvApiErrorMessage(status);
		return 0;
	}

	return static_cast<unsigned int>(gpuCount);
}

NVAPI_DLL NvPhysicalGpuHandle GetGPUHandle(unsigned int index)
{
	NvPhysicalGpuHandle gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = { 0 };
	NvU32 gpuCount = 0;
	NvAPI_Status status = NvAPI_EnumPhysicalGPUs(gpuHandles, &gpuCount);
	if (status != NVAPI_OK || index >= static_cast<int>(gpuCount))
	{
		GetNvApiErrorMessage(status);
		return nullptr;
	}
	return gpuHandles[index];
}

NVAPI_DLL const char* GetGPUName(unsigned int index)
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

NVAPI_DLL const char* GetGPUInfo(unsigned int index)
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
	snprintf(info, sizeof(info), "Ray Tracing Cores: %lu, Tensor Cores: %lu, isExternal GPU: ",
		gpuInfo.rayTracingCores, gpuInfo.tensorCores);
	if (gpuInfo.bIsExternalGpu)
		strcat_s(info, "Yes");
	else
		strcat_s(info, "No");
	return info;
}

NVAPI_DLL const char* GetSystemType(unsigned int index)
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

NVAPI_DLL const char* GetIlluminationZonesInfo(unsigned int index, CustomIlluminationZonesInfo* pCustomIlluminationZonesInfo)
{
	if (!pCustomIlluminationZonesInfo)
		return nullptr;
	NvPhysicalGpuHandle gpuHandle = GetGPUHandle(index);
	if (!gpuHandle)
		return nullptr;

	static char info[4096];
	std::stringstream infoStream;
	NV_GPU_CLIENT_ILLUM_ZONE_INFO_PARAMS illuminationZonesInfo = { 0 };
	illuminationZonesInfo.version = NV_GPU_CLIENT_ILLUM_ZONE_INFO_PARAMS_VER;
	NvAPI_Status status = NvAPI_GPU_ClientIllumZonesGetInfo(gpuHandle, &illuminationZonesInfo);

	if (status == NVAPI_OK)
	{
		pCustomIlluminationZonesInfo->numIllumZones = illuminationZonesInfo.numIllumZones;
		infoStream << "Number of Illumination Zones: " << illuminationZonesInfo.numIllumZones << "\n";

		for (unsigned int i = 0; i < illuminationZonesInfo.numIllumZones; ++i)
		{
			const auto& illuminationZone = illuminationZonesInfo.zones[i];
			auto& zoneData = pCustomIlluminationZonesInfo->zones[i];

			// Zone Type
			const char* zoneType = "Reserved or Unknown";
			switch (illuminationZone.type)
			{
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGB:				zoneType = "RGB";				break;
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_COLOR_FIXED:		zoneType = "Color Fixed";		break;
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW:			zoneType = "RGBW";				break;
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_SINGLE_COLOR:	zoneType = "Single Color";		break;
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_INVALID:			zoneType = "Invalid";			break;
			}
			strncpy_s(zoneData.zoneType, sizeof(zoneData.zoneType), zoneType, _TRUNCATE);
			infoStream << "\tType: " << zoneType << "\n";

			// Zone Location
			const char* zoneLocation = "Reserved or Unknown";
			switch (illuminationZone.zoneLocation)
			{
			case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_TOP_0:   zoneLocation = "GPU Top";		break;
			case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_FRONT_0: zoneLocation = "GPU Front";		break;
			case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_BACK_0:  zoneLocation = "GPU Back";		break;
			case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_SLI_TOP_0:   zoneLocation = "SLI Top";		break;
			case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_INVALID:     zoneLocation = "Invalid";		break;
			}
			strncpy_s(zoneData.zoneLocation, sizeof(zoneData.zoneLocation), zoneLocation, _TRUNCATE);
			infoStream << "\tLocation: " << zoneLocation << "\n";
		}
	}
	else
	{
		infoStream << "Failed to get Illumination Zones Info: " << GetNvApiErrorMessage(status);
		pCustomIlluminationZonesInfo->numIllumZones = 0;
	}
	strncpy_s(info, sizeof(info), infoStream.str().c_str(), _TRUNCATE);
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
NVAPI_DLL void parsePiecewiseLinearData(const NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR* src, CustomPiecewiseLinear* dst)
{
	const char* cycleType = "Reserved or Unknown";
	switch (src->cycleType)
	{
	case NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_HALF_HALT:
		cycleType = "Half Halt";
		break;
	case NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_FULL_HALT:
		cycleType = "Full Halt";
		break;
	case NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_FULL_REPEAT:
		cycleType = "Full Repeat";
		break;
	case NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_INVALID:
		cycleType = "Invalid";
		break;
	}
	strncpy_s(dst->cycleType, sizeof(dst->cycleType), cycleType, _TRUNCATE);
	dst->grpCount = src->grpCount;
	dst->riseTimeMs = src->riseTimems;
	dst->fallTimeMs = src->fallTimems;
	dst->aTimeMs = src->ATimems;
	dst->bTimeMs = src->BTimems;
	dst->idleTimeMs = src->grpIdleTimems;
	dst->phaseOffsetMs = src->phaseOffsetms;
}

NVAPI_DLL const char* GetIlluminationZonesControl(unsigned int index, bool useDefault, CustomIlluminationZoneControls* pCustomIlluminationZoneControls)
{
	if (!pCustomIlluminationZoneControls) return nullptr;
	NvPhysicalGpuHandle gpuHandle = GetGPUHandle(index);
	if (!gpuHandle) return nullptr;

	static char info[4096];
	std::stringstream infoStream;
	NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS controlParams = { 0 };
	controlParams.version = NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS_VER;
	controlParams.bDefault = useDefault ? NV_TRUE : NV_FALSE;

	NvAPI_Status status = NvAPI_GPU_ClientIllumZonesGetControl(gpuHandle, &controlParams);
	if (status != NVAPI_OK) {
		pCustomIlluminationZoneControls->numZones = 0;
		infoStream << "Failed to get Illumination Zones Control: " << GetNvApiErrorMessage(status);
		strncpy_s(info, sizeof(info), infoStream.str().c_str(), sizeof(info) - 1);
		return info;
	}

	pCustomIlluminationZoneControls->numZones = controlParams.numIllumZonesControl;
	infoStream << "Number of Illumination Zones Control: " << pCustomIlluminationZoneControls->numZones << "\n";

	for (unsigned int i = 0; i < controlParams.numIllumZonesControl; ++i) {
		{
			const auto& src = controlParams.zones[i];
			auto& dst = pCustomIlluminationZoneControls->zones[i];

			// Zone Type
			const char* zoneType = "Reserved or Unknown";
			switch (src.type) {
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGB:				zoneType = "RGB";											break;
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_COLOR_FIXED:		zoneType = "Color Fixed";									break;
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW:			zoneType = "RGBW";											break;
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_SINGLE_COLOR:	zoneType = "Single Color";									break;
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_INVALID:			zoneType = "Invalid";										break;
			}
			strncpy_s(dst.zoneType, sizeof(dst.zoneType), zoneType, _TRUNCATE);
			infoStream << "Zone: " << i << " Type: " << zoneType << "\n";

			// Control Mode
			const char* controlMode = "Reserved or Unknown";
			dst.isPiecewise = false;
			switch (src.ctrlMode) 
			{
			case NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL:			controlMode = "Manual";										break;
			case NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR:controlMode = "Piecewise Linear"; dst.isPiecewise = true;	break;
			case NV_GPU_CLIENT_ILLUM_CTRL_MODE_INVALID:			controlMode = "Invalid";									break;
			}
			strncpy_s(dst.controlMode, sizeof(dst.controlMode), controlMode, _TRUNCATE);
			infoStream << "\tControl Mode: " << controlMode << "\n";

			// Data
			switch (src.type)
			{
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGB:
				switch (src.ctrlMode)
				{
				case NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL:
					infoStream << "\tManual RGB, Data: ";
					printManualRGBData(&src.data.rgb.data.manualRGB.rgbParams, infoStream);
					dst.manualColorData.rgb = { src.data.rgb.data.manualRGB.rgbParams.colorR, 
						src.data.rgb.data.manualRGB.rgbParams.colorG, 
						src.data.rgb.data.manualRGB.rgbParams.colorB, 
						src.data.rgb.data.manualRGB.rgbParams.brightnessPct 
					};
					break;
				case NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR:
					infoStream << "\tPiecewise Linear RGB, Data: ";
					for (int j = 0; j < NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR_COLOR_ENDPOINTS; ++j)
					{
						infoStream << "\t\tEndpoint " << j << ":\n";
						dst.piecewiseColorData[j].rgb = { src.data.rgb.data.piecewiseLinearRGB.rgbParams[j].colorR,
							src.data.rgb.data.piecewiseLinearRGB.rgbParams[j].colorG,
							src.data.rgb.data.piecewiseLinearRGB.rgbParams[j].colorB,
							src.data.rgb.data.piecewiseLinearRGB.rgbParams[j].brightnessPct
						};
						printManualRGBData(&src.data.rgb.data.piecewiseLinearRGB.rgbParams[j], infoStream);
					}
					parsePiecewiseLinearData(&src.data.rgb.data.piecewiseLinearRGB.piecewiseLinearData, &dst.piecewiseData);
					printPiecewiseLinearData(&src.data.rgb.data.piecewiseLinearRGB.piecewiseLinearData, infoStream);
					break;
				}
				break;
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_COLOR_FIXED:
				switch (src.ctrlMode)
				{
				case NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL:
					infoStream << "\tManual Color Fixed, Data: Brightness: " 
						<< (int)src.data.colorFixed.data.manualColorFixed.colorFixedParams.brightnessPct << "\n";
					dst.manualColorData.singleColor = { src.data.colorFixed.data.manualColorFixed.colorFixedParams.brightnessPct };
					break;
				case NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR:
					infoStream << "\tPiecewise Linear Color Fixed, Data: ";
					for (int j = 0; j < NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR_COLOR_ENDPOINTS; ++j)
					{
						if (j != 0)
							infoStream << "\t";
						infoStream << "\t\tEndpoint " << j << ":\n" << "Brightness: " 
							<< (int)src.data.colorFixed.data.piecewiseLinearColorFixed.colorFixedParams[j].brightnessPct << "\n";
						dst.piecewiseColorData[j].singleColor = { src.data.colorFixed.data.piecewiseLinearColorFixed.colorFixedParams[j].brightnessPct };
					}
					parsePiecewiseLinearData(&src.data.colorFixed.data.piecewiseLinearColorFixed.piecewiseLinearData, &dst.piecewiseData);
					printPiecewiseLinearData(&src.data.colorFixed.data.piecewiseLinearColorFixed.piecewiseLinearData, infoStream);
					break;
				}
				break;
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW:
				switch (src.ctrlMode)
				{
				case NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL:
					infoStream << "\tManual RGBW, Data: ";
					printManualRGBWData(&src.data.rgbw.data.manualRGBW.rgbwParams, infoStream);
					dst.manualColorData.rgbw = { src.data.rgbw.data.manualRGBW.rgbwParams.colorR,
						src.data.rgbw.data.manualRGBW.rgbwParams.colorG,
						src.data.rgbw.data.manualRGBW.rgbwParams.colorB,
						src.data.rgbw.data.manualRGBW.rgbwParams.colorW,
						src.data.rgbw.data.manualRGBW.rgbwParams.brightnessPct
					};
					break;
				case NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR:
					infoStream << "\tPiecewise Linear RGBW, Data: ";
					for (int j = 0; j < NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR_COLOR_ENDPOINTS; ++j)
					{
						if (j != 0)
							infoStream << "\t";
						infoStream << "\t\tEndpoint " << j << ":\n";
						printManualRGBWData(&src.data.rgbw.data.piecewiseLinearRGBW.rgbwParams[j], infoStream);
						dst.piecewiseColorData[j].rgbw = { src.data.rgbw.data.piecewiseLinearRGBW.rgbwParams[j].colorR,
							src.data.rgbw.data.piecewiseLinearRGBW.rgbwParams[j].colorG,
							src.data.rgbw.data.piecewiseLinearRGBW.rgbwParams[j].colorB,
							src.data.rgbw.data.piecewiseLinearRGBW.rgbwParams[j].colorW,
							src.data.rgbw.data.piecewiseLinearRGBW.rgbwParams[j].brightnessPct
						};
					}
					parsePiecewiseLinearData(&src.data.rgbw.data.piecewiseLinearRGBW.piecewiseLinearData, &dst.piecewiseData);
					printPiecewiseLinearData(&src.data.rgbw.data.piecewiseLinearRGBW.piecewiseLinearData, infoStream);
					break;
				}
				break;
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_SINGLE_COLOR:
				switch (src.ctrlMode)
				{
				case NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL:
					infoStream << "\tManual Single Color, Data: ";
					printManualSingleColorData(&src.data.singleColor.data.manualSingleColor.singleColorParams, infoStream);
					dst.manualColorData.singleColor = { src.data.singleColor.data.manualSingleColor.singleColorParams.brightnessPct };
					break;
				case NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR:
					infoStream << "\tPiecewise Linear Single Color, Data: ";
					for (int j = 0; j < NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR_COLOR_ENDPOINTS; ++j)
					{
						if (j != 0)
							infoStream << "\t";
						infoStream << "\t\tEndpoint " << j << ":\n";
						printManualSingleColorData(&src.data.singleColor.data.piecewiseLinearSingleColor.singleColorParams[j], infoStream);
						dst.piecewiseColorData[j].singleColor = { src.data.singleColor.data.piecewiseLinearSingleColor.singleColorParams[j].brightnessPct };
					}
					parsePiecewiseLinearData(&src.data.singleColor.data.piecewiseLinearSingleColor.piecewiseLinearData, &dst.piecewiseData);
					printPiecewiseLinearData(&src.data.singleColor.data.piecewiseLinearSingleColor.piecewiseLinearData, infoStream);
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
	}
	infoStream << "\n";
	strncpy_s(info, sizeof(info), infoStream.str().c_str(), sizeof(info) - 1);
	return info;
}

NVAPI_DLL bool SetIlluminationZoneManualRGB(unsigned int gpuIndex, unsigned int zoneIndex, uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness, bool Default = false)
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
NVAPI_DLL bool SetIlluminationZoneManualRGBW(unsigned int gpuIndex, unsigned int zoneIndex, uint8_t red, uint8_t green, uint8_t blue, uint8_t white, uint8_t brightness, bool Default = false)
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

	if (Default)
		illumControlParams.bDefault = NV_TRUE;
	else
		illumControlParams.bDefault = NV_FALSE;
	auto& rgbwData = illumControlParams.zones[zoneIndex].data.rgbw.data.manualRGBW.rgbwParams;
	rgbwData.colorR = red;
	rgbwData.colorG = green;
	rgbwData.colorB = blue;
	rgbwData.colorW = white;
	rgbwData.brightnessPct = brightness;


	return NvAPI_GPU_ClientIllumZonesSetControl(gpuHandle, &illumControlParams) == NVAPI_OK;
}
NVAPI_DLL bool SetIlluminationZoneManualSingleColor(unsigned int gpuIndex, unsigned int zoneIndex, uint8_t brightness, bool Default = false)
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
	printf("Driver Version: %lu\n", GetDriverVersion());
	printf("Number of GPUs: %d\n", GetNumberOfGPUs());
	for (unsigned int i = 0; i < GetNumberOfGPUs(); ++i)
	{
		printf("GPU %d Name: %s\n", i, GetGPUName(i));
		printf("GPU %d Info: %s\n", i, GetGPUInfo(i));
		printf("GPU %d System Type: %s\n", i, GetSystemType(i));
		CustomIlluminationZonesInfo illuminationZonesInfo;
		printf("GPU %d Illumination Zones Info: %s\n", i, GetIlluminationZonesInfo(i, &illuminationZonesInfo));
		CustomIlluminationZoneControls illuminationZonesControl;
		printf("GPU %d Illumination Zones Control Default: %s\n", i, GetIlluminationZonesControl(i, true, &illuminationZonesControl));
		printf("GPU %d Illumination Zones Control Active: %s\n", i, GetIlluminationZonesControl(i, false, &illuminationZonesControl));

		
		uint8_t red = 255;
		uint8_t brightness = 255;
		uint8_t green = 255;
		uint8_t blue = 255;
		uint8_t white = 255;
		bool Default = false;
		unsigned int sleepTime = 3;

		// Hardcode 2 zones for testing
		for (unsigned int j = 0; j < 2; ++j) {
			printf("Testing zone %d\n", j);

			printf("Setting RGB color\n");
			if (SetIlluminationZoneManualRGB(i, j, red, green, blue, brightness, Default))
			{
				printf("SetIlluminationZoneManualRGB success.\n");
				printf("GPU %d Illumination Zones Control Default: %s\n", i, GetIlluminationZonesControl(i, true, &illuminationZonesControl));
				printf("GPU %d Illumination Zones Control Active: %s\n", i, GetIlluminationZonesControl(i, false, &illuminationZonesControl));
				printf("Waiting for %d seconds...\n", sleepTime);
				Sleep(sleepTime * 1000);
			}
			else
				printf("SetIlluminationZoneManualRGB failed.\n");

			printf("Setting RGBW color\n");
			if (SetIlluminationZoneManualRGBW(i, j, red, green, blue, white, brightness, Default))
			{
				printf("SetIlluminationZoneManualRGBW success.\n");
				printf("GPU %d Illumination Zones Control Default: %s\n", i, GetIlluminationZonesControl(i, true, &illuminationZonesControl));
				printf("GPU %d Illumination Zones Control Active: %s\n", i, GetIlluminationZonesControl(i, false, &illuminationZonesControl));
				printf("Waiting for %d seconds...\n", sleepTime);
				Sleep(sleepTime * 1000);
			}
			else
				printf("SetIlluminationZoneManualRGBW failed.\n");

			printf("Setting Single Color\n");
			if (SetIlluminationZoneManualSingleColor(i, j, brightness, Default))
			{
				printf("SetIlluminationZoneManualSingleColor success.\n");
				printf("GPU %d Illumination Zones Control Default: %s\n", i, GetIlluminationZonesControl(i, true, &illuminationZonesControl));
				printf("GPU %d Illumination Zones Control Active: %s\n", i, GetIlluminationZonesControl(i, false, &illuminationZonesControl));
				printf("Waiting for %d seconds...\n", sleepTime);
				Sleep(sleepTime * 1000);
			}
			else
				printf("SetIlluminationZoneManualSingleColor failed.\n");
		}
	}
	DeinitializeNvApi();
}