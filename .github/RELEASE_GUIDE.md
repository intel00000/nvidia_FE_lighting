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

- `nvidia_FE_lighting.exe` - framework-dependent single-file executable
- `README.txt` - User instructions

**Note:**

- Requires .NET 8.0.x Runtime to be installed on the target system

## Local Build

To build locally:

```bash
# Build C++ wrapper
msbuild nvidia_FE_lighting.sln /p:Configuration=Release /p:Platform=x64 /t:NvApiWrapper

# Publish as single-file executable
dotnet publish nvidia_FE_lighting\nvidia_FE_lighting.csproj -c Release -r win-x64 --self-contained false -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true -o publish
```

Output will be a single `nvidia_FE_lighting.exe` in the `publish\` folder.
