Epic Online Services (EOS) SDK
v1.14 (17607641)

Note that /Lib and /Bin are required to use the SDK, and that only Windows libs are provided
Definitely need to contact Epic in future for console SDK files and perhaps for Mac + Linux as well

Note that the Tools folder is not included in this repo, which are particularly useful for auth. 
Get them via downloading SDK and looking under <extracted folder>/SDK/Tools:
https://dev.epicgames.com/portal/en-US/dragonjawad-s-organization/sdk?versionType=sdk&versionNumber=236

## Creating this project from SDK download

These steps were followed to create this folder out of the extracted SDK:
1. Extracted zip outside of repo
1. Moved ThirdPartyNotices into SDK folder
1. Removed SDK/Tools folder
1. Added Readme
1. Finally renamed SDK folder to EOS and moved under Vendor directory

Pretty simple and straightforward

## VS Project Setup
Steps differ for Unreal projects and SimLibraryTests (ie, a raw C++ project) as they use different build tools.
Thus, using different steps for the different project types.

As a side note, as EOS documentation is extremely lackluster, the following steps were reversed and otherwise 
figured out from the included SDK Samples projects. May be valuable to refer to those projects again in the 
future for general setup.

### ProjectNomadCoreTests (ie, raw C++ project)
Following steps are used to setup unit tests project for EOS usage (or at least proper compiling):

**TODO:** _Update example paths, as these are based on before moving EOS over to public repo_

1. Make sure "EOS" SDK folder is under `ProjectNomadCore/Vendor` directory (which all consuming projects 
should have already added as an include directory)
1. Open relevant solution/project in Visual Studios
1. Add `ProjectNomadCore/Vendor/EOS/Lib` as an additional library directory
   - Open project properties (in Visual Studios, right click -> Properties)
   - At the top, make sure Configuration is set to All Configurations and Platform is set to All Platforms
   - Find this "Additional Library Directories" field under Configuration Properties -> Linker -> General
   - Add the appropriate path to the libs directory (eg, `$(ProjectDir)..\ProjectNomadCore\Vendor\EOS\Lib`)
1. Add the specific lib under Additional Dependencies
   - Open project properties
   - At the top, make sure Configuration is set to All Configurations
   - Find this "Additional Dependencies" field under Configuration Properties -> Linker -> Input
   - Add the following with the appropriate platform:
     - Platform x64: `EOSSDK-Win64-Shipping.lib`
     - Platform Win32: `EOSSDK-Win32-Shipping.lib`
1. Not sure why this is necessary yet, but add a Pre-Build Event to copy the correct dll to the output directory
   - Open project properties
   - At the top, make sure Configuration is set to All Configurations
   - Go to Configuration Properties -> Build Events -> Pre-Build Event
   - Add the following under "Command Line" with the appropriate platform and make sure to adjust the path as necessary:
     - Platform x64: `xcopy /D /Y /R /Q $(ProjectDir)..\ProjectNomadCore\Vendor\EOS\Bin\EOSSDK-Win64-Shipping.dll $(OutDir) >nul`
     - Platform Win32: `xcopy /D /Y /R /Q $(ProjectDir)..\ProjectNomadCore\Vendor\EOS\Bin\EOSSDK-Win32-Shipping.dll $(OutDir) >nul`
   - Optionally add a Description of `Copy DLLs to Target Directory`
   - Make sure all the changes saved. At the moment, VS likes to not save these changes unless explicitly saved

For more info on general .dll setup with VS C++ projects, view 
[this StackOverflow question.](https://stackoverflow.com/questions/495795/how-do-i-use-a-third-party-dll-file-in-visual-studio-c)

### Unreal projects
This is essentially the same thing as the SimLibraryTests but in C#

1. Open your Unreal project then go to your `<ProjectName>.Build.CS` file (in VS, Rider, Notepad, etc)
1. Similar to SimLibraryTests, the Vendor path should already be listed as an include path via `PublicIncludePaths`
1. Add a reference to your specific lib for the target platform via `PublicAdditionalLibraries`
1. Copy your dll to the output path via `RuntimeDependencies`

Here's a relevant sample outlining the relevant steps:
```csharp
PublicIncludePaths.Add(@"..\SimpleTopDownSimLibrary\SimLibrary\Vendor");
PublicIncludePaths.Add(@"..\SimpleTopDownSimLibrary\PNC\ProjectNomadCore\Vendor");

// Calculate architecture-based portion of lib + dll name string
string architecture;
if (Target.Platform == UnrealTargetPlatform.Win32)
{
  architecture = "Win32";
}
else if (Target.Platform == UnrealTargetPlatform.Win64)
{
  architecture = "Win64";
}
else
{
  throw new ArgumentException("Unsupported target platform provided! Fix up libs/dlls if can support");
}

// Calculate path to Sim Library's vendor directory, as it'll be reused for lib + dll path calculations
string pncVendorPathFromModuleDir = @"..\..\SimpleTopDownSimLibrary\PNC\ProjectNomadCore\Vendor\";

// Add lib containing directories
string eosLibPathFromVendorDir = @"EOS\Lib\";
string eosLibName = "EOSSDK-" + architecture + "-Shipping.lib";
PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, pncVendorPathFromModuleDir, eosLibPathFromVendorDir, eosLibName));

// Copy dlls to output directory
string eosDllPathFromVendorDir = @"EOS\Bin\";
string eosDllName = "EOSSDK-" + architecture + "-Shipping.dll";
RuntimeDependencies.Add("$(BinaryOutputDir)/" + eosDllName, Path.Combine(ModuleDirectory, pncVendorPathFromModuleDir, eosDllPathFromVendorDir, eosDllName));
```