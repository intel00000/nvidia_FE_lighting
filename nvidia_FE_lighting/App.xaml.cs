using System;
using System.Configuration;
using System.Data;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text.Json;
using System.Threading;
using System.Windows;
using static nvidia_FE_lighting.NvApiWrapper;

namespace nvidia_FE_lighting
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
		protected override void OnStartup(StartupEventArgs e)
		{
			// Check for --startup command-line argument
			if (e.Args.Contains("--startup"))
			{
				RunStartupMode();
				Shutdown();
				return;
			}

			// Normal mode - show UI
			base.OnStartup(e);
		}

		private void RunStartupMode()
		{
			string logPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
				"NvidiaFELighting", "startup_log.txt");

			try
			{
				// Initialize logging
				Directory.CreateDirectory(Path.GetDirectoryName(logPath));
				File.AppendAllText(logPath, $"\n[{DateTime.Now}] Startup mode initiated\n");

				// Wait for driver initialization
				string settingsPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
					"NvidiaFELighting", "startup_settings.json");

				if (!File.Exists(settingsPath))
				{
					File.AppendAllText(logPath, "No startup settings found. Exiting.\n");
					return;
				}

				// Load settings
				string json = File.ReadAllText(settingsPath);
				var settings = JsonSerializer.Deserialize<StartupSettings>(json);

				if (settings == null)
				{
					File.AppendAllText(logPath, "Failed to parse startup settings. Exiting.\n");
					return;
				}

				File.AppendAllText(logPath, $"Waiting {settings.DelaySeconds} seconds for driver initialization...\n");
				Thread.Sleep(settings.DelaySeconds * 1000);

				// Initialize NVAPI
				if (!InitializeNvApi())
				{
					File.AppendAllText(logPath, "Failed to initialize NVAPI. Exiting.\n");
					return;
				}

				File.AppendAllText(logPath, "NVAPI initialized successfully.\n");

				// Get GPU count and select first GPU
				uint gpuCount = GetNumberOfGPUs();
				if (gpuCount == 0)
				{
					File.AppendAllText(logPath, "No GPUs detected. Exiting.\n");
					DeinitializeNvApi();
					return;
				}

				uint gpuIndex = 0;
				File.AppendAllText(logPath, $"Found {gpuCount} GPU(s). Using GPU index {gpuIndex}.\n");

				// Get current GPU identifier
				uint busId, deviceId, subSystemId, revisionId, extDeviceId;
				if (!GetGPUBusId(gpuIndex, out busId) ||
					!GetGPUPCIIdentifiers(gpuIndex, out deviceId, out subSystemId, out revisionId, out extDeviceId))
				{
					File.AppendAllText(logPath, "Failed to get GPU identifiers. Exiting.\n");
					DeinitializeNvApi();
					return;
				}

				var currentGpuId = new GpuIdentifier
				{
					BusId = busId,
					DeviceId = deviceId,
					SubSystemId = subSystemId,
					RevisionId = revisionId,
					ExtDeviceId = extDeviceId
				};

				// Verify GPU matches
				if (!currentGpuId.Matches(settings.GpuIdentifier))
				{
					File.AppendAllText(logPath, $"GPU mismatch detected!\n");
					File.AppendAllText(logPath, $"  Expected: BusId={settings.GpuIdentifier.BusId}, DeviceId={settings.GpuIdentifier.DeviceId}, SubSystemId={settings.GpuIdentifier.SubSystemId}\n");
					File.AppendAllText(logPath, $"  Current:  BusId={currentGpuId.BusId}, DeviceId={currentGpuId.DeviceId}, SubSystemId={currentGpuId.SubSystemId}\n");
					File.AppendAllText(logPath, "Aborting to prevent applying settings to wrong GPU.\n");
					DeinitializeNvApi();
					return;
				}

				File.AppendAllText(logPath, "GPU verification successful. Applying settings...\n");

				// Apply settings
				int successCount = 0;
				foreach (var zone in settings.Zones)
				{
					bool success = false;
					if (zone.ZoneType == "RGB")
					{
						success = SetIlluminationZoneManualRGB(gpuIndex, (uint)zone.ZoneIndex,
							zone.R, zone.G, zone.B, zone.Brightness, false);
					}
					else if (zone.ZoneType == "RGBW")
					{
						success = SetIlluminationZoneManualRGBW(gpuIndex, (uint)zone.ZoneIndex,
							zone.R, zone.G, zone.B, zone.W, zone.Brightness, false);
					}
					else
					{
						success = SetIlluminationZoneManualSingleColor(gpuIndex, (uint)zone.ZoneIndex,
							zone.Brightness, false);
					}

					if (success) successCount++;
				}

				File.AppendAllText(logPath, $"Applied settings to {successCount}/{settings.Zones.Count} zones successfully.\n");

				// Cleanup
				DeinitializeNvApi();
				File.AppendAllText(logPath, "Startup mode completed successfully.\n");
			}
			catch (Exception ex)
			{
				try
				{
					File.AppendAllText(logPath, $"ERROR: {ex.Message}\n{ex.StackTrace}\n");
				}
				catch
				{
					// Ignore logging errors
				}
			}
		}
    }

}
