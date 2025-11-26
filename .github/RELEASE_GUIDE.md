# Release Guide

This repository uses GitHub Actions to automatically build and publish releases.

## How to Create a Release

### Method 1: Push a Tag

1. **Create a tag locally:**

   ```bash
   git tag v1.0.0
   git push origin v1.0.0
   ```

2. **The workflow will start automatically:**
   - Build the solution (C++ and C# projects)
   - Package all necessary files
   - Create a GitHub release with a ZIP file

### Method 2: Manual Trigger

1. Go to **Actions** tab in GitHub
2. Select **Build and Release** workflow
3. Click **Run workflow**
4. The build will run but won't create a release (no tag)

## What Gets Packaged

The release ZIP includes:

- `nvidia_FE_lighting.exe` - standalone executable
- `README.txt` - User instructions

**Note:** The native C++ DLL (NvApiDll.dll) is embedded in the .exe and automatically extracted to a temporary folder.

## Local Build

To build locally:

```bash
# Build C++ wrapper
msbuild nvidia_FE_lighting.sln /p:Configuration=Release /p:Platform=x64 /t:NvApiWrapper

# Publish as single-file executable
dotnet publish nvidia_FE_lighting\nvidia_FE_lighting.csproj -c Release -r win-x64 --self-contained true -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true -o publish
```

Output will be a single `nvidia_FE_lighting.exe` in the `publish\` folder.
