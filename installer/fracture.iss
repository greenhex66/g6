[Setup]
AppId={{B6D2A4E1-3C7F-4A58-9D21-5F0E8A7C1B34}
AppName=Fracture
AppVersion=1.0.0
AppPublisher=Producer
DefaultDirName={autopf}\Fracture
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..
OutputBaseFilename=Fracture-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Files]
Source: "..\dist\Fracture.vst3\*"; DestDir: "{commoncf64}\VST3\Fracture.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
