#define Version "1.0.0"
#define ProjectName "Signalbash"
#define ProductName "Signalbash"
#define Publisher "Signalbash"
#define PublisherURL "https://signalbash.com"
#define Year "2025"

; 'Types': What get displayed during the setup
[Types]
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

; Components are used inside the script and can be composed of a set of 'Types'
[Components]
Name: "vst3"; Description: "VST3 Plugin"; Types: full custom
Name: "clap"; Description: "CLAP Plugin"; Types: full custom

[Setup]
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
AppName={#ProductName}
AppCopyright=Copyright (C) {#Year} {#Publisher}
AppPublisher={#Publisher}
AppPublisherURL={#PublisherURL}
AppVersion={#Version}
DefaultDirName="{commoncf64}\VST3\{#ProductName}.vst3"
DisableDirPage=yes
DisableProgramGroupPage=yes
LicenseFile="resources\LICENSE"
OutputBaseFilename={#ProductName}-{#Version}-Windows-Installer-x64
Uninstallable=no
VersionInfoVersion={#Version}

; MSVC adds a .ilk when building the plugin. Let's not include that.
[Files]
Source: "..\build-windows\{#ProjectName}_artefacts\Release\VST3\{#ProductName}.vst3\*"; DestDir: "{commoncf64}\VST3\{#ProductName}.vst3\"; Excludes: *.ilk; Flags: ignoreversion recursesubdirs; Components: vst3
Source: "..\build-windows\{#ProjectName}_artefacts\Release\CLAP\{#ProductName}.clap"; DestDir: "{commoncf64}\CLAP\"; Flags: ignoreversion; Components: clap
