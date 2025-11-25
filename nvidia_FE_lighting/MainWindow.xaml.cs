using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Media;
using Xceed.Wpf.Toolkit;
using static nvidia_FE_lighting.NvApiWrapper;

namespace nvidia_FE_lighting
{
	/// <summary>
	/// Interaction logic for MainWindow.xaml
	/// </summary>
	public partial class MainWindow : Window
	{
		private CustomIlluminationZoneControl[] globalZoneControls;
		private uint currentGpuIndex;
		private readonly Dictionary<(uint gpuIdx, int zoneIdx), byte> pendingBrightnessChanges = new();
		private void SetStatus(string message)
		{
			statusText.Text = message;
		}

		public MainWindow()
		{
			InitializeComponent();

			if (!InitializeNvApi())
			{
				Xceed.Wpf.Toolkit.MessageBox.Show("Failed to initialize NVAPI");
				SetStatus("NVAPI initialization failed.");
				return;
			}
			SetStatus("NVAPI initialized.");

			var gpuCount = GetNumberOfGPUs();
			for (uint i = 0; i < gpuCount; i++)
			{
				string gpuName = Marshal.PtrToStringAnsi(GetGPUName(i));
				gpuSelectComboBox.Items.Add($"{i}: {gpuName}");
			}

			if (gpuSelectComboBox.Items.Count > 0)
			{
				gpuSelectComboBox.SelectedIndex = 0;
				gpuSelectComboBox.SelectedItem = gpuSelectComboBox.Items[0];
				gpuSelectComboBox.Text = gpuSelectComboBox.Items[0].ToString();
				SetStatus($"Detected {gpuCount} GPU(s).");
			}
			else
			{
				SetStatus("No GPUs detected.");
			}
		}

		// Event handler for GPU selection change
		private void RefreshGpuInfo(uint index)
		{
			currentGpuIndex = index;
			gpuNameText.Text = "Name: " + Marshal.PtrToStringAnsi(GetGPUName(index));
			gpuInfoText.Text = "Details: " + Marshal.PtrToStringAnsi(GetGPUInfo(index));
			gpuSystemTypeText.Text = "System Type: " + Marshal.PtrToStringAnsi(GetSystemType(index));
			gpuDriverVersionText.Text = "Driver Version: " + GetDriverVersion() / 100.00f;
			gpuInterfaceVersionText.Text = "NVAPI Interface: " + Marshal.PtrToStringAnsi(GetInterfaceVersionString());
			SetStatus($"Loaded GPU {index}");
		}

		// Event handler for the "Get Illumination Zones" button
		private void PopulateIlluminationZones(uint gpuIndex)
		{
			SetStatus("Detecting illumination zones...");
			gpuIlluminationZones.Items.Clear();
			pendingBrightnessChanges.Clear();
			applyAllButton.IsEnabled = false;

			var zoneInfo = new NvApiWrapper.CustomIlluminationZonesInfo { zones = new NvApiWrapper.CustomIlluminationZonesInfoData[32] };
			NvApiWrapper.GetIlluminationZonesInfo(gpuIndex, ref zoneInfo);

			var zoneControls = new NvApiWrapper.CustomIlluminationZoneControls { zones = new NvApiWrapper.CustomIlluminationZoneControl[32] };
			NvApiWrapper.GetIlluminationZonesControl(gpuIndex, false, ref zoneControls);

			globalZoneControls = new CustomIlluminationZoneControl[zoneControls.numZones];
			Array.Copy(zoneControls.zones, globalZoneControls, zoneControls.numZones);

			if (zoneInfo.numIllumZones == 0)
			{
				gpuIlluminationZones.Items.Add(new TextBlock { Text = "No illumination zones found.", Margin = new Thickness(35, 0, 0, 10) });
				SetStatus("No illumination zones found.");
				return;
			}

			for (int i = 0; i < zoneControls.numZones; i++)
			{
				var zone = zoneControls.zones[i];
				var zoneMeta = zoneInfo.zones[i];

				var panel = new StackPanel
				{
					Orientation = Orientation.Vertical
				};

				TextBlock header = new TextBlock
				{
					Text = $"Zone {i}: {zone.controlMode} for {zoneMeta.zoneType} @ {zoneMeta.zoneLocation}",
					FontWeight = FontWeights.Bold,
					Margin = new Thickness(0, 0, 0, 10)
				};
				panel.Children.Add(header);

				if (zone.controlMode == "Manual")
				{
					if (zone.zoneType == "RGB")
					{
						panel.Children.Add(new TextBlock { Text = $"Active RGB: R={zone.manualColorData.rgb.r}, G={zone.manualColorData.rgb.g}, B={zone.manualColorData.rgb.b}", Margin = new Thickness(0, 0, 0, 10), Foreground = (Brush)FindResource("TextSecondaryBrush") });
					}
					else if (zone.zoneType == "RGBW")
					{
						panel.Children.Add(new TextBlock { Text = $"Active RGBW: R={zone.manualColorData.rgbw.r}, G={zone.manualColorData.rgbw.g}, B={zone.manualColorData.rgbw.b}, W={zone.manualColorData.rgbw.w}", Margin = new Thickness(0, 0, 0, 10), Foreground = (Brush)FindResource("TextSecondaryBrush") });
					}
					else if (zone.zoneType == "Invalid")
					{
						panel.Children.Add(new TextBlock { Text = "Invalid", Margin = new Thickness(0, 0, 0, 10), Foreground = (Brush)FindResource("TextSecondaryBrush") });
					}
				}
				else if (zone.controlMode == "Piecewise Linear")
				{
					panel.Children.Add(new TextBlock { Text = $"Piecewise Mode: {zone.piecewiseData.cycleType}, Group Count: {zone.piecewiseData.grpCount}" });
					for (int j = 0; j < zone.piecewiseColorData.Length; j++)
					{
						var color = zone.piecewiseColorData[j];
						string colorText = zone.zoneType switch
						{
							"Active: RGB" => $"  [{j}] R={color.rgb.r}, G={color.rgb.g}, B={color.rgb.b}, Bright={color.rgb.brightness}",
							"Active: RGBW" => $"  [{j}] R={color.rgbw.r}, G={color.rgbw.g}, B={color.rgbw.b}, W={color.rgbw.w}, Bright={color.rgbw.brightness}",
							"Active: Single Color" => $"  [{j}] Brightness={color.singleColor.brightness}",
							_ => $"  [{j}] Unknown"
						};
						panel.Children.Add(new TextBlock { Text = colorText, Margin = new Thickness(0, 0, 0, 10), Foreground = (Brush)FindResource("TextSecondaryBrush") });
					}
				}

				AddZoneControls(panel, zoneMeta.zoneType, gpuIndex, i);
				var card = new Border
				{
					Background = (Brush)FindResource("CardBackgroundBrush"),
					BorderBrush = (Brush)FindResource("CardBorderBrush"),
					BorderThickness = new Thickness(1),
					CornerRadius = new CornerRadius(10),
					Padding = new Thickness(12),
					Margin = new Thickness(0, 0, 0, 12)
				};
				card.Child = panel;
				gpuIlluminationZones.Items.Add(card);
			}
			SetStatus($"Found {zoneControls.numZones} illumination zone(s).");
		}
		private void AddZoneControls(StackPanel panel, string zoneType, uint gpuIndex, int zoneIndex)
		{
			if (zoneType == "Invalid")
			{
				return;
			}

			byte brightnessValue = zoneType switch
			{
				"RGB" => globalZoneControls[zoneIndex].manualColorData.rgb.brightness,
				"RGBW" => globalZoneControls[zoneIndex].manualColorData.rgbw.brightness,
				_ => globalZoneControls[zoneIndex].manualColorData.singleColor.brightness
			};
			var brightnessSlider = new Slider
			{
				Minimum = 0,
				Maximum = 100,
				Value = brightnessValue,
				TickFrequency = 1,
				IsSnapToTickEnabled = true,
				Margin = new Thickness(0, 0, 0, 10),
				AutoToolTipPlacement = AutoToolTipPlacement.TopLeft,
				AutoToolTipPrecision = 0,
				IsMoveToPointEnabled = true
			};
			brightnessSlider.Tag = (gpuIndex, zoneIndex, zoneType);
			brightnessSlider.AddHandler(Thumb.DragCompletedEvent, new DragCompletedEventHandler(BrightnessSlider_DragCompleted));
			brightnessSlider.MouseLeftButtonUp += BrightnessSlider_MouseLeftButtonUp;
			panel.Children.Add(brightnessSlider);
		}

		private void gpuSelectComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
		{
			if (gpuSelectComboBox.SelectedIndex >= 0)
			{
				uint index = (uint)gpuSelectComboBox.SelectedIndex;
				RefreshGpuInfo(index);
				pendingBrightnessChanges.Clear();
				applyAllButton.IsEnabled = false;
			}
			else
			{
				SetStatus("No GPU selected.");
			}
		}

		private void gpuSelectDetectIllumZones_Click(object sender, RoutedEventArgs e)
		{
			if (gpuSelectComboBox.SelectedIndex >= 0)
			{
				uint index = (uint)gpuSelectComboBox.SelectedIndex;
				SetStatus($"Detecting zones on GPU {index}...");
				PopulateIlluminationZones(index);
			}
			else
			{
				SetStatus("Select a GPU to detect zones.");
			}
		}

		private void BrightnessSlider_DragCompleted(object sender, DragCompletedEventArgs e)
		{
			if (sender is Slider slider)
			{
				ApplySliderValue(slider);
			}
		}

		private void BrightnessSlider_MouseLeftButtonUp(object sender, System.Windows.Input.MouseButtonEventArgs e)
		{
			if (sender is Slider slider)
			{
				ApplySliderValue(slider);
			}
		}

		private void ApplySliderValue(Slider slider)
		{
			var tag = ((uint gpuIdx, int zoneIdx, string zoneType))slider.Tag;
			byte brightness = (byte)slider.Value;
			QueueBrightnessChange(tag.gpuIdx, tag.zoneIdx, brightness);
			if (!ApplyBrightnessForZone(tag.gpuIdx, tag.zoneIdx, tag.zoneType, brightness))
			{
				Xceed.Wpf.Toolkit.MessageBox.Show($"Failed to set brightness for zone {tag.zoneIdx}");
				return;
			}
			SetStatus($"Set brightness {brightness}% on zone {tag.zoneIdx}");
		}

		private void QueueBrightnessChange(uint gpuIdx, int zoneIdx, byte brightness)
		{
			pendingBrightnessChanges[(gpuIdx, zoneIdx)] = brightness;
			applyAllButton.IsEnabled = true;
		}

		private bool ApplyBrightnessForZone(uint gpuIdx, int zoneIdx, string zoneType, byte brightness)
		{
			bool result = false;
			if (zoneType == "RGB")
			{
				var color = globalZoneControls[zoneIdx].manualColorData.rgb;
				result = SetIlluminationZoneManualRGB(gpuIdx, (uint)zoneIdx, color.r, color.g, color.b, brightness, false);
				globalZoneControls[zoneIdx].manualColorData.rgb.brightness = brightness;
			}
			else if (zoneType == "RGBW")
			{
				var color = globalZoneControls[zoneIdx].manualColorData.rgbw;
				result = SetIlluminationZoneManualRGBW(gpuIdx, (uint)zoneIdx, color.r, color.g, color.b, color.w, brightness, false);
				globalZoneControls[zoneIdx].manualColorData.rgbw.brightness = brightness;
			}
			else if (zoneType == "Color Fixed" || zoneType == "Single Color")
			{
				result = SetIlluminationZoneManualSingleColor(gpuIdx, (uint)zoneIdx, brightness, false);
				globalZoneControls[zoneIdx].manualColorData.singleColor.brightness = brightness;
			}
			else
			{
				// Fallback for unexpected types: use single color brightness
				result = SetIlluminationZoneManualSingleColor(gpuIdx, (uint)zoneIdx, brightness, false);
			}
			return result;
		}

		private void applyAllButton_Click(object sender, RoutedEventArgs e)
		{
			if (pendingBrightnessChanges.Count == 0)
			{
				applyAllButton.IsEnabled = false;
				return;
			}

			foreach (var kvp in pendingBrightnessChanges)
			{
				var (gpuIdx, zoneIdx) = kvp.Key;
				byte brightness = kvp.Value;
				string zoneType = globalZoneControls[zoneIdx].zoneType;
				ApplyBrightnessForZone(gpuIdx, zoneIdx, zoneType, brightness);
			}

			pendingBrightnessChanges.Clear();
			applyAllButton.IsEnabled = false;
			SetStatus("Applied brightness to all pending zones.");
		}
	}
}
