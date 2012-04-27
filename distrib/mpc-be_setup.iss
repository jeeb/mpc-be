; (C) 2009-2012 see Authors.txt
;
; This file is part of MPC-BE.
;
; MPC-BE is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 3 of the License, or
; (at your option) any later version.
;
; MPC-BE is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <http://www.gnu.org/licenses/>.
;
; $Id$


; Requirements:
; Inno Setup Unicode: http://www.jrsoftware.org/isdl.php


; If you want to compile the 64-bit version define "x64build" (uncomment the define below or use build_2010.bat)
#define localize
#define sse_required
;#define x64Build

; Don't forget to update the DirectX SDK number in include\Version.h (not updated so often)


; From now on you shouldn't need to change anything

#if VER < EncodeVer(5,4,3)
  #error Update your Inno Setup version (5.4.3 or newer)
#endif

#ifndef UNICODE
  #error Use the Unicode Inno Setup
#endif


#define ISPP_IS_BUGGY
#include "..\include\Version.h"

#define copyright_year "2002-2012"
#define app_name       "MPC-BE"
#define app_version    str(MPC_VERSION_MAJOR) + "." + str(MPC_VERSION_MINOR) + "." + str(MPC_VERSION_PATCH) + "." + str(MPC_VERSION_REV)
#define quick_launch   "{userappdata}\Microsoft\Internet Explorer\Quick Launch"


#ifdef x64Build
  #define bindir       = "..\bin\mpc-be_x64"
  #define mpcbe_exe    = "mpc-be64.exe"
  #define mpcbe_ini    = "mpc-be64.ini"
  #define mpcbe_reg    = "mpc-be64"
#else
  #define bindir       = "..\bin\mpc-be_x86"
  #define mpcbe_exe    = "mpc-be.exe"
  #define mpcbe_ini    = "mpc-be.ini"
  #define mpcbe_reg    = "mpc-be"
#endif

[Setup]
#ifdef x64Build
AppId={{FE09AF6D-78B2-4093-B012-FCDAF78693CE}
DefaultGroupName={#app_name} x64
OutputBaseFilename=MPC-BE.{#app_version}.x64
UninstallDisplayName={#app_name} {#app_version} x64
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
#else
AppId={{903D098F-DD50-4342-AD23-DA868FCA3126}
DefaultGroupName={#app_name}
OutputBaseFilename=MPC-BE.{#app_version}.x86
UninstallDisplayName={#app_name} {#app_version}
#endif

AppName={#app_name}
AppVersion={#app_version}
AppVerName={#app_name} {#app_version}
AppPublisher=MPC-BE Team
AppPublisherURL=https://sourceforge.net/p/mpcbe/
AppSupportURL=https://sourceforge.net/p/mpcbe/
AppUpdatesURL=https://sourceforge.net/p/mpcbe/
AppContact=https://sourceforge.net/p/mpcbe/
AppCopyright=Copyright © {#copyright_year} all contributors, see Authors.txt
VersionInfoCompany=MPC-BE Team
VersionInfoCopyright=Copyright © {#copyright_year}, MPC-BE Team
VersionInfoDescription={#app_name} Setup
VersionInfoProductName={#app_name}
VersionInfoProductVersion={#app_version}
VersionInfoProductTextVersion={#app_version}
VersionInfoTextVersion={#app_version}
VersionInfoVersion={#app_version}
UninstallDisplayIcon={app}\{#mpcbe_exe}
DefaultDirName={code:GetInstallFolder}
LicenseFile=..\COPYING.txt
OutputDir=.
SetupIconFile=..\src\apps\mplayerc\res\icon.ico
AppReadmeFile={app}\Readme.txt
WizardImageFile=WizardImageFile.bmp
WizardSmallImageFile=WizardSmallImageFile.bmp
Compression=lzma2/ultra64
SolidCompression=yes
AllowNoIcons=yes
ShowTasksTreeLines=yes
DisableDirPage=auto
DisableProgramGroupPage=auto
MinVersion=0,5.01.2600
AppMutex=MediaPlayerClassicW
ChangesAssociations=true


[Languages]
Name: en; MessagesFile: compiler:Default.isl

#ifdef localize
Name: br; MessagesFile: compiler:Languages\BrazilianPortuguese.isl
Name: by; MessagesFile: Languages\Belarusian.isl
Name: ca; MessagesFile: compiler:Languages\Catalan.isl
Name: cz; MessagesFile: compiler:Languages\Czech.isl
Name: de; MessagesFile: compiler:Languages\German.isl
Name: es; MessagesFile: compiler:Languages\Spanish.isl
Name: eu; MessagesFile: compiler:Languages\Basque.isl
Name: fr; MessagesFile: compiler:Languages\French.isl
Name: he; MessagesFile: compiler:Languages\Hebrew.isl
Name: hu; MessagesFile: Languages\Hungarian.isl
Name: hy; MessagesFile: Languages\Armenian.islu
Name: it; MessagesFile: compiler:Languages\Italian.isl
Name: ja; MessagesFile: compiler:Languages\Japanese.isl
Name: kr; MessagesFile: Languages\Korean.isl
Name: nl; MessagesFile: compiler:Languages\Dutch.isl
Name: pl; MessagesFile: compiler:Languages\Polish.isl
Name: ru; MessagesFile: compiler:Languages\Russian.isl
Name: sc; MessagesFile: Languages\ChineseSimp.isl
Name: sv; MessagesFile: Languages\Swedish.isl
Name: sk; MessagesFile: compiler:Languages\Slovak.isl
Name: tc; MessagesFile: Languages\ChineseTrad.isl
Name: tr; MessagesFile: Languages\Turkish.isl
Name: ua; MessagesFile: compiler:Languages\Ukrainian.isl
#endif

; Include installer's custom messages
#include "custom_messages.iss"


[Messages]
#ifdef x64Build
BeveledLabel={#app_name} {#app_version} x64
#else
BeveledLabel={#app_name} {#app_version}
#endif

[Types]
Name: default;            Description: {cm:types_DefaultInstallation}
Name: custom;             Description: {cm:types_CustomInstallation};                     Flags: iscustom

[Components]
Name: main;               Description: {#app_name} {#app_version}; Types: default custom; Flags: fixed
Name: mpciconlib;         Description: {cm:comp_mpciconlib};       Types: default custom

#ifdef localize
Name: mpcresources;       Description: {cm:comp_mpcresources};     Types: default custom; Flags: disablenouninstallwarning
#endif

Name: mpcbeshellext;      Description: {cm:comp_mpcbeshellext};    Types: default custom

[Tasks]
Name: desktopicon;              Description: {cm:CreateDesktopIcon};     GroupDescription: {cm:AdditionalIcons}
Name: desktopicon\user;         Description: {cm:tsk_CurrentUser};       GroupDescription: {cm:AdditionalIcons}; Flags: exclusive
Name: desktopicon\common;       Description: {cm:tsk_AllUsers};          GroupDescription: {cm:AdditionalIcons}; Flags: unchecked exclusive
Name: quicklaunchicon;          Description: {cm:CreateQuickLaunchIcon}; GroupDescription: {cm:AdditionalIcons}; Flags: unchecked;             OnlyBelowVersion: 0,6.01

;;AutoPlay
NAme: autoplaytype;             Description: {cm:AutoPlayType}
NAme: autoplaytype\cdaudio;     Description: {cm:AssociationMPCPlayCDAudioS}
NAme: autoplaytype\dvdmovie;    Description: {cm:AssociationMPCPlayDVDMovieS}
NAme: autoplaytype\musicfiles;  Description: {cm:AssociationMPCPlayMusicFilesS}
NAme: autoplaytype\videofiles;  Description: {cm:AssociationMPCPlayVideoFilesS}

NAme: association;              Description: {cm:AssociationFormats};    GroupDescription: {cm:AssociationFormatsAV}

;;Video Files
NAme: association\video;        Description: {cm:AssociationVideo}
NAme: association\video\3g2;    Description: "3g2"
NAme: association\video\3gp;    Description: "3gp"
NAme: association\video\3gp2;   Description: "3gp2"
NAme: association\video\3gpp;   Description: "3gpp"
NAme: association\video\amv;    Description: "amv"
NAme: association\video\asf;    Description: "asf"
NAme: association\video\asx;    Description: "asx"
NAme: association\video\avi;    Description: "avi"
NAme: association\video\bik;    Description: "bik"
NAme: association\video\d2v;    Description: "d2v"
NAme: association\video\dat;    Description: "dat"
NAme: association\video\divx;   Description: "divx"
NAme: association\video\evo;    Description: "evo"
NAme: association\video\f4v;    Description: "f4v"
NAme: association\video\flc;    Description: "flc"
NAme: association\video\fli;    Description: "fli"
NAme: association\video\flic;   Description: "flic"
NAme: association\video\flv;    Description: "flv"
NAme: association\video\ifo;    Description: "ifo"
NAme: association\video\ivf;    Description: "ivf"
NAme: association\video\m1v;    Description: "m1v"
NAme: association\video\m2p;    Description: "m2p"
NAme: association\video\m2t;    Description: "m2t"
NAme: association\video\m2ts;   Description: "m2ts"
NAme: association\video\m2v;    Description: "m2v"
NAme: association\video\m4v;    Description: "m4v"
NAme: association\video\mkv;    Description: "mkv"
NAme: association\video\mov;    Description: "mov"
NAme: association\video\mp2v;   Description: "mp2v"
NAme: association\video\mp4;    Description: "mp4"
NAme: association\video\mp4v;   Description: "mp4v"
NAme: association\video\mpe;    Description: "mpe"
NAme: association\video\mpeg;   Description: "mpeg"
NAme: association\video\mpg;    Description: "mpg"
NAme: association\video\mpv2;   Description: "mpv2"
NAme: association\video\mpv4;   Description: "mpv4"
NAme: association\video\mts;    Description: "mts"
NAme: association\video\ogm;    Description: "ogm"
NAme: association\video\ogv;    Description: "ogv"
NAme: association\video\pva;    Description: "pva"
NAme: association\video\ratdvd; Description: "ratdvd"
NAme: association\video\rec;    Description: "rec"
NAme: association\video\rm;     Description: "rm"
NAme: association\video\rmm;    Description: "rmm"
NAme: association\video\rmvb;   Description: "rmvb"
NAme: association\video\rp;     Description: "rp"
NAme: association\video\rpm;    Description: "rpm"
NAme: association\video\rt;     Description: "rt"
NAme: association\video\smi;    Description: "smi"
NAme: association\video\smil;   Description: "smil"
NAme: association\video\smk;    Description: "smk"
NAme: association\video\swf;    Description: "swf"
NAme: association\video\tp;     Description: "tp"
NAme: association\video\trp;    Description: "trp"
NAme: association\video\ts;     Description: "ts"
NAme: association\video\vob;    Description: "vob"
NAme: association\video\webm;   Description: "webm"
NAme: association\video\wm;     Description: "wm"
NAme: association\video\wmp;    Description: "wmp"
NAme: association\video\wmv;    Description: "wmv"
NAme: association\video\wmx;    Description: "wmx"

;;Audio Files
NAme: association\audio;        Description: {cm:AssociationAudio}
NAme: association\audio\aac;    Description: "aac"
NAme: association\audio\ac3;    Description: "ac3"
NAme: association\audio\aif;    Description: "aif"
NAme: association\audio\aifc;   Description: "aifc"
NAme: association\audio\aiff;   Description: "aiff"
NAme: association\audio\alac;   Description: "alac"
NAme: association\audio\amr;    Description: "amr"
NAme: association\audio\ape;    Description: "ape"
NAme: association\audio\apl;    Description: "apl"
NAme: association\audio\au;     Description: "au"
NAme: association\audio\cda;    Description: "cda"
NAme: association\audio\dts;    Description: "dts"
NAme: association\audio\flac;   Description: "flac"
NAme: association\audio\m1a;    Description: "m1a"
NAme: association\audio\m2a;    Description: "m2a"
NAme: association\audio\m4a;    Description: "m4a"
NAme: association\audio\m4b;    Description: "m4b"
NAme: association\audio\mid;    Description: "mid"
NAme: association\audio\midi;   Description: "midi"
NAme: association\audio\mka;    Description: "mka"
NAme: association\audio\mp2;    Description: "mp2"
NAme: association\audio\mp3;    Description: "mp3"
NAme: association\audio\mpa;    Description: "mpa"
NAme: association\audio\mpc;    Description: "mpc"
NAme: association\audio\ofr;    Description: "ofr"
NAme: association\audio\ofs;    Description: "ofs"
NAme: association\audio\oga;    Description: "oga"
NAme: association\audio\ogg;    Description: "ogg"
NAme: association\audio\ra;     Description: "ra"
NAme: association\audio\ram;    Description: "ram"
NAme: association\audio\rmi;    Description: "rmi"
NAme: association\audio\snd;    Description: "snd"
NAme: association\audio\tak;    Description: "tak"
NAme: association\audio\tta;    Description: "tta"
NAme: association\audio\wav;    Description: "wav"
NAme: association\audio\wax;    Description: "wax"
NAme: association\audio\wma;    Description: "wma"
NAme: association\audio\wv;     Description: "wv"

;;Playlist Files
NAme: association\playlist;        Description: {cm:AssociationPlaylist}
NAme: association\playlist\bdmv;   Description: "bdmv"
NAme: association\playlist\m3u;    Description: "m3u"
NAme: association\playlist\m3u8;   Description: "m3u8"
NAme: association\playlist\mpcpl;  Description: "mpcpl"
NAme: association\playlist\mpls;   Description: "mpls"
NAme: association\playlist\pls;    Description: "pls"

;;DirectShow Media
NAme: association\dsmedia;        Description: "DirectShow Media"
NAme: association\dsmedia\dsa;    Description: "dsa"
NAme: association\dsmedia\dsm;    Description: "dsm"
NAme: association\dsmedia\dss;    Description: "dss"
NAme: association\dsmedia\dsv;    Description: "dsv"

;;ResetSettings
Name: reset_settings;             Description: {cm:tsk_ResetSettings};     GroupDescription: {cm:tsk_Other};       Flags: checkedonce unchecked; Check: SettingsExistCheck()


[Files]
Source: "{#bindir}\{#mpcbe_exe}";        DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "{#bindir}\mpciconlib.dll";      DestDir: "{app}"; Flags: ignoreversion; Components: mpciconlib
#ifdef x64Build
Source: "{#bindir}\MPCBEShellExt64.dll"; DestDir: "{app}"; Flags: ignoreversion noregerror regserver restartreplace uninsrestartdelete; Components: mpcbeshellext
#else
Source: "{#bindir}\MPCBEShellExt.dll";   DestDir: "{app}"; Flags: ignoreversion noregerror regserver restartreplace uninsrestartdelete; Components: mpcbeshellext
#endif

#ifdef localize
Source: "{#bindir}\Lang\mpcresources.br.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.by.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.ca.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.cz.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.de.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.es.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.eu.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.fr.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.he.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.hu.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.hy.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.it.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.ja.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.kr.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.nl.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.pl.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.ru.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.sc.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.sk.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.sv.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.tc.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.tr.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.ua.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
#endif
Source: "..\COPYING.txt";                     DestDir: "{app}";      Flags: ignoreversion; Components: main
Source: "..\docs\Authors.txt";                DestDir: "{app}";      Flags: ignoreversion; Components: main
Source: "..\docs\Changelog.txt";              DestDir: "{app}";      Flags: ignoreversion; Components: main
Source: "..\docs\Readme.txt";                 DestDir: "{app}";      Flags: ignoreversion; Components: main

[Icons]
#ifdef x64Build
Name: {group}\{#app_name} x64;                   Filename: {app}\{#mpcbe_exe}; Comment: {#app_name} {#app_version} x64; WorkingDir: {app}; IconFilename: {app}\{#mpcbe_exe}; IconIndex: 0
Name: {commondesktop}\{#app_name} x64;           Filename: {app}\{#mpcbe_exe}; Comment: {#app_name} {#app_version} x64; WorkingDir: {app}; IconFilename: {app}\{#mpcbe_exe}; IconIndex: 0; Tasks: desktopicon\common
Name: {userdesktop}\{#app_name} x64;             Filename: {app}\{#mpcbe_exe}; Comment: {#app_name} {#app_version} x64; WorkingDir: {app}; IconFilename: {app}\{#mpcbe_exe}; IconIndex: 0; Tasks: desktopicon\user
Name: {#quick_launch}\{#app_name} x64;           Filename: {app}\{#mpcbe_exe}; Comment: {#app_name} {#app_version} x64; WorkingDir: {app}; IconFilename: {app}\{#mpcbe_exe}; IconIndex: 0; Tasks: quicklaunchicon
#else
Name: {group}\{#app_name};                       Filename: {app}\{#mpcbe_exe}; Comment: {#app_name} {#app_version}; WorkingDir: {app}; IconFilename: {app}\{#mpcbe_exe}; IconIndex: 0
Name: {commondesktop}\{#app_name};               Filename: {app}\{#mpcbe_exe}; Comment: {#app_name} {#app_version}; WorkingDir: {app}; IconFilename: {app}\{#mpcbe_exe}; IconIndex: 0; Tasks: desktopicon\common
Name: {userdesktop}\{#app_name};                 Filename: {app}\{#mpcbe_exe}; Comment: {#app_name} {#app_version}; WorkingDir: {app}; IconFilename: {app}\{#mpcbe_exe}; IconIndex: 0; Tasks: desktopicon\user
Name: {#quick_launch}\{#app_name};               Filename: {app}\{#mpcbe_exe}; Comment: {#app_name} {#app_version}; WorkingDir: {app}; IconFilename: {app}\{#mpcbe_exe}; IconIndex: 0; Tasks: quicklaunchicon
#endif
Name: {group}\Changelog;                         Filename: {app}\Changelog.txt; Comment: {cm:ViewChangelog};                WorkingDir: {app}
Name: {group}\{cm:ProgramOnTheWeb,{#app_name}};  Filename: https://sourceforge.net/p/mpcbe/
Name: {group}\{cm:UninstallProgram,{#app_name}}; Filename: {uninstallexe};      Comment: {cm:UninstallProgram,{#app_name}}; WorkingDir: {app}

[Run]
Filename: {app}\{#mpcbe_exe};                    Description: {cm:LaunchProgram,{#app_name}}; WorkingDir: {app}; Flags: nowait postinstall skipifsilent unchecked
Filename: {app}\Changelog.txt;                   Description: {cm:ViewChangelog};             WorkingDir: {app}; Flags: nowait postinstall skipifsilent unchecked shellexec


[InstallDelete]
Type: files; Name: {userdesktop}\{#app_name}.lnk;   Check: not IsTaskSelected('desktopicon\user')   and IsUpgrade()
Type: files; Name: {commondesktop}\{#app_name}.lnk; Check: not IsTaskSelected('desktopicon\common') and IsUpgrade()
Type: files; Name: {#quick_launch}\{#app_name}.lnk; Check: not IsTaskSelected('quicklaunchicon')    and IsUpgrade(); OnlyBelowVersion: 0,6.01
Type: files; Name: {app}\AUTHORS
Type: files; Name: {app}\ChangeLog
Type: files; Name: {app}\COPYING
#ifdef localize
; remove the old language dlls when upgrading
Type: files; Name: {app}\mpcresources.*.dll
#endif

[Registry]
;;Autoplay
;;MPCPlayCDAudio
Root: HKLM; SubKey: SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\Handlers\MPCPlayCDAudioOnArrival; ValueName: Action; ValueData: {cm:AssociationMPCPlayCDAudioOnArrivalF}; ValueType: string; Flags: uninsdeletekey; Tasks: autoplaytype\cdaudio
Root: HKLM; SubKey: SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\Handlers\MPCPlayCDAudioOnArrival; ValueName: Provider; ValueData: "MPC-BE"; ValueType: string; Flags: uninsdeletekey; Tasks: autoplaytype\cdaudio
Root: HKLM; SubKey: SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\Handlers\MPCPlayCDAudioOnArrival; ValueName: InvokeProgID; ValueData: "MPCBE.Autorun"; ValueType: string; Flags: uninsdeletekey; Tasks: autoplaytype\cdaudio
Root: HKLM; SubKey: SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\Handlers\MPCPlayCDAudioOnArrival; ValueName: InvokeVerb; ValueData: "PlayCDAudio"; ValueType: string; Tasks: autoplaytype\cdaudio
Root: HKLM; SubKey: SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\Handlers\MPCPlayCDAudioOnArrival; ValueName: DefaultIcon; ValueData: "{app}\{#mpcbe_exe},0"; ValueType: string; Flags: uninsdeletekey; Tasks: autoplaytype\cdaudio

;;MPCPlayDVDMovie
Root: HKLM; SubKey: SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\Handlers\MPCPlayDVDMovieOnArrival; ValueName: Action; ValueData: {cm:AssociationMPCPlayDVDMovieArrivalF}; ValueType: string; Flags: uninsdeletekey; Tasks: autoplaytype\dvdmovie
Root: HKLM; SubKey: SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\Handlers\MPCPlayDVDMovieOnArrival; ValueName: Provider; ValueData: "MPC-BE"; ValueType: string; Flags: uninsdeletekey; Tasks: autoplaytype\dvdmovie
Root: HKLM; SubKey: SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\Handlers\MPCPlayDVDMovieOnArrival; ValueName: InvokeProgID; ValueData: "MPCBE.Autorun"; ValueType: string; Flags: uninsdeletekey; Tasks: autoplaytype\dvdmovie
Root: HKLM; SubKey: SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\Handlers\MPCPlayDVDMovieOnArrival; ValueName: InvokeVerb; ValueData: "PlayDVDMovie"; ValueType: string; Flags: uninsdeletekey; Tasks: autoplaytype\dvdmovie
Root: HKLM; SubKey: SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\Handlers\MPCPlayDVDMovieOnArrival; ValueName: DefaultIcon; ValueData: "{app}\{#mpcbe_exe},0"; ValueType: string; Flags: uninsdeletekey; Tasks: autoplaytype\dvdmovie

;;MPCPlayMusicFiles
Root: HKLM; SubKey: SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\Handlers\MPCPlayMusicFilesOnArrival; ValueName: Action; ValueData: {cm:AssociationMPCPlayMusicFilesF}; ValueType: string; Flags: uninsdeletekey; Tasks: autoplaytype\musicfiles
Root: HKLM; SubKey: SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\Handlers\MPCPlayMusicFilesOnArrival; ValueName: Provider; ValueData: "MPC-BE"; ValueType: string; Flags: uninsdeletekey; Tasks: autoplaytype\musicfiles
Root: HKLM; SubKey: SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\Handlers\MPCPlayMusicFilesOnArrival; ValueName: InvokeProgID; ValueData: "MPCBE.Autorun"; ValueType: string; Flags: uninsdeletekey; Tasks: autoplaytype\musicfiles
Root: HKLM; SubKey: SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\Handlers\MPCPlayMusicFilesOnArrival; ValueName: InvokeVerb; ValueData: "PlayMusicFiles"; ValueType: string; Flags: uninsdeletekey; Tasks: autoplaytype\musicfiles
Root: HKLM; SubKey: SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\Handlers\MPCPlayMusicFilesOnArrival; ValueName: DefaultIcon; ValueData: "{app}\{#mpcbe_exe},0"; ValueType: string; Flags: uninsdeletekey; Tasks: autoplaytype\musicfiles

;;MPCPlayVideoFiles
Root: HKLM; SubKey: SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\Handlers\MPCPlayVideoFilesOnArrival; ValueName: Action; ValueData: {cm:AssociationMPCPlayVideoFilesF}; ValueType: string; Flags: uninsdeletekey; Tasks: autoplaytype\videofiles
Root: HKLM; SubKey: SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\Handlers\MPCPlayVideoFilesOnArrival; ValueName: Provider; ValueData: "MPC-BE"; ValueType: string; Flags: uninsdeletekey; Tasks: autoplaytype\videofiles
Root: HKLM; SubKey: SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\Handlers\MPCPlayVideoFilesOnArrival; ValueName: InvokeProgID; ValueData: "MPCBE.Autorun"; ValueType: string; Flags: uninsdeletekey; Tasks: autoplaytype\videofiles
Root: HKLM; SubKey: SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\Handlers\MPCPlayVideoFilesOnArrival; ValueName: InvokeVerb; ValueData: "PlayVideoFiles"; ValueType: string; Flags: uninsdeletekey; Tasks: autoplaytype\videofiles
Root: HKLM; SubKey: SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\Handlers\MPCPlayVideoFilesOnArrival; ValueName: DefaultIcon; ValueData: "{app}\{#mpcbe_exe},0"; ValueType: string; Flags: uninsdeletekey; Tasks: autoplaytype\videofiles


;;Video Files
;3g2
Root: "HKCR"; Subkey: "{#mpcbe_reg}.3g2\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",2";         Flags: uninsdeletekey; Tasks: association\video\3g2
Root: "HKCR"; Subkey: "{#mpcbe_reg}.3g2\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\3g2
Root: "HKCR"; Subkey: "{#mpcbe_reg}.3g2\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\3g2

;3gp
Root: "HKCR"; Subkey: "{#mpcbe_reg}.3gp\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",2";         Flags: uninsdeletekey; Tasks: association\video\3gp
Root: "HKCR"; Subkey: "{#mpcbe_reg}.3gp\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\3gp
Root: "HKCR"; Subkey: "{#mpcbe_reg}.3gp\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\3gp

;3gp2
Root: "HKCR"; Subkey: "{#mpcbe_reg}.3gp2\DefaultIcon";          ValueType: string; ValueData: """{app}\mpciconlib.dll"",2";          Flags: uninsdeletekey; Tasks: association\video\3gp2
Root: "HKCR"; Subkey: "{#mpcbe_reg}.3gp2\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe} %1""";          Flags: uninsdeletekey; Tasks: association\video\3gp2
Root: "HKCR"; Subkey: "{#mpcbe_reg}.3gp2\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\3gp2

;3gpp
Root: "HKCR"; Subkey: "{#mpcbe_reg}.3gpp\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",2";         Flags: uninsdeletekey; Tasks: association\video\3gpp
Root: "HKCR"; Subkey: "{#mpcbe_reg}.3gpp\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\3gpp
Root: "HKCR"; Subkey: "{#mpcbe_reg}.3gpp\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\3gpp

;amv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.amv\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",37";        Flags: uninsdeletekey; Tasks: association\video\amv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.amv\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\amv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.amv\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\amv

;asf
Root: "HKCR"; Subkey: "{#mpcbe_reg}.asf\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",6";         Flags: uninsdeletekey; Tasks: association\video\asf
Root: "HKCR"; Subkey: "{#mpcbe_reg}.asf\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\asf
Root: "HKCR"; Subkey: "{#mpcbe_reg}.asf\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\asf

;asx
Root: "HKCR"; Subkey: "{#mpcbe_reg}.asx\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",21";        Flags: uninsdeletekey; Tasks: association\video\asx
Root: "HKCR"; Subkey: "{#mpcbe_reg}.asx\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\asx
Root: "HKCR"; Subkey: "{#mpcbe_reg}.asx\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\asx

;avi
Root: "HKCR"; Subkey: "{#mpcbe_reg}.avi\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",0";         Flags: uninsdeletekey; Tasks: association\video\avi
Root: "HKCR"; Subkey: "{#mpcbe_reg}.avi\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\avi
Root: "HKCR"; Subkey: "{#mpcbe_reg}.avi\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\avi

;bik
Root: "HKCR"; Subkey: "{#mpcbe_reg}.bik\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",29";        Flags: uninsdeletekey; Tasks: association\video\bik
Root: "HKCR"; Subkey: "{#mpcbe_reg}.bik\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\bik
Root: "HKCR"; Subkey: "{#mpcbe_reg}.bik\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\bik

;d2v
Root: "HKCR"; Subkey: "{#mpcbe_reg}.d2v\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",22";        Flags: uninsdeletekey; Tasks: association\video\d2v
Root: "HKCR"; Subkey: "{#mpcbe_reg}.d2v\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\d2v
Root: "HKCR"; Subkey: "{#mpcbe_reg}.d2v\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\d2v

;dat
Root: "HKCR"; Subkey: "{#mpcbe_reg}.dat\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",42";         Flags: uninsdeletekey; Tasks: association\video\dat
Root: "HKCR"; Subkey: "{#mpcbe_reg}.dat\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\dat
Root: "HKCR"; Subkey: "{#mpcbe_reg}.dat\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\dat

;divx
Root: "HKCR"; Subkey: "{#mpcbe_reg}.divx\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",0";         Flags: uninsdeletekey; Tasks: association\video\divx
Root: "HKCR"; Subkey: "{#mpcbe_reg}.divx\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\divx
Root: "HKCR"; Subkey: "{#mpcbe_reg}.divx\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\divx

;evo
Root: "HKCR"; Subkey: "{#mpcbe_reg}.evo\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",1";         Flags: uninsdeletekey; Tasks: association\video\evo
Root: "HKCR"; Subkey: "{#mpcbe_reg}.evo\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\evo
Root: "HKCR"; Subkey: "{#mpcbe_reg}.evo\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\evo

;f4v
Root: "HKCR"; Subkey: "{#mpcbe_reg}.f4v\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",5";         Flags: uninsdeletekey; Tasks: association\video\f4v
Root: "HKCR"; Subkey: "{#mpcbe_reg}.f4v\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\f4v
Root: "HKCR"; Subkey: "{#mpcbe_reg}.f4v\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\f4v

;flc
Root: "HKCR"; Subkey: "{#mpcbe_reg}.flc\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",28";         Flags: uninsdeletekey; Tasks: association\video\flc
Root: "HKCR"; Subkey: "{#mpcbe_reg}.flc\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\flc
Root: "HKCR"; Subkey: "{#mpcbe_reg}.flc\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\flc

;fli
Root: "HKCR"; Subkey: "{#mpcbe_reg}.fli\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",28";         Flags: uninsdeletekey; Tasks: association\video\fli
Root: "HKCR"; Subkey: "{#mpcbe_reg}.fli\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\fli
Root: "HKCR"; Subkey: "{#mpcbe_reg}.fli\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\fli

;flic
Root: "HKCR"; Subkey: "{#mpcbe_reg}.flic\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",28";        Flags: uninsdeletekey; Tasks: association\video\flic
Root: "HKCR"; Subkey: "{#mpcbe_reg}.flic\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\flic
Root: "HKCR"; Subkey: "{#mpcbe_reg}.flic\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\flic

;flv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.flv\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",5";         Flags: uninsdeletekey; Tasks: association\video\flv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.flv\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\flv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.flv\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\flv

;ifo
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ifo\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",36";        Flags: uninsdeletekey; Tasks: association\video\ifo
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ifo\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\ifo
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ifo\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\ifo

;ivf
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ivf\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",27";        Flags: uninsdeletekey; Tasks: association\video\ivf
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ivf\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\ivf
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ivf\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\ivf

;m1v
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m1v\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",1";         Flags: uninsdeletekey; Tasks: association\video\m1v
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m1v\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\m1v
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m1v\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\m1v

;m2p
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m2p\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",43";         Flags: uninsdeletekey; Tasks: association\video\m2p
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m2p\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\m2p
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m2p\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";  Flags: uninsdeletekey; Tasks: association\video\m2p

;m2t
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m2t\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",1";          Flags: uninsdeletekey; Tasks: association\video\m2t
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m2t\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\m2t
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m2t\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";  Flags: uninsdeletekey; Tasks: association\video\m2t

;m2ts
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m2ts\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",1";         Flags: uninsdeletekey; Tasks: association\video\m2ts
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m2ts\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\m2ts
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m2ts\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\m2ts

;m2v
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m2v\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",1";          Flags: uninsdeletekey; Tasks: association\video\m2v
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m2v\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\m2v
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m2v\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";  Flags: uninsdeletekey; Tasks: association\video\m2v

;m4v
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m4v\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",2";          Flags: uninsdeletekey; Tasks: association\video\m4v
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m4v\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\m4v
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m4v\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";  Flags: uninsdeletekey; Tasks: association\video\m4v

;mkv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mkv\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",3";          Flags: uninsdeletekey; Tasks: association\video\mkv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mkv\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\mkv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mkv\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";  Flags: uninsdeletekey; Tasks: association\video\mkv

;mov
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mov\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",7";          Flags: uninsdeletekey; Tasks: association\video\mov
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mov\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\mov
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mov\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";  Flags: uninsdeletekey; Tasks: association\video\mov

;mp2v
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mp2v\DefaultIcon";          ValueType: string; ValueData: """{app}\mpciconlib.dll"",1";          Flags: uninsdeletekey; Tasks: association\video\mp2v
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mp2v\shell\open\command";   ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\mp2v
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mp2v\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\mp2v

;mp4
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mp4\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",2";          Flags: uninsdeletekey; Tasks: association\video\mp4
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mp4\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\mp4
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mp4\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";  Flags: uninsdeletekey; Tasks: association\video\mp4

;mp4v
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mp4v\DefaultIcon";          ValueType: string; ValueData: """{app}\mpciconlib.dll"",2";          Flags: uninsdeletekey; Tasks: association\video\mp4v
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mp4v\shell\open\command";   ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\mp4v
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mp4v\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\mp4v

;mpe
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpe\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",1";          Flags: uninsdeletekey; Tasks: association\video\mpe
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpe\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\mpe
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpe\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";  Flags: uninsdeletekey; Tasks: association\video\mpe

;mpeg
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpeg\DefaultIcon";          ValueType: string; ValueData: """{app}\mpciconlib.dll"",1";          Flags: uninsdeletekey; Tasks: association\video\mpeg
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpeg\shell\open\command";   ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\mpeg
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpeg\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\mpeg

;mpg
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpg\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",1";          Flags: uninsdeletekey; Tasks: association\video\mpg
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpg\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\mpg
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpg\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";  Flags: uninsdeletekey; Tasks: association\video\mpg

;mpv2
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpv2\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",1";         Flags: uninsdeletekey; Tasks: association\video\mpv2
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpv2\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\mpv2
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpv2\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\mpv2

;mpv4
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpv4\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",2";         Flags: uninsdeletekey; Tasks: association\video\mpv4
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpv4\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\mpv4
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpv4\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\mpv4

;mts
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mts\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",1";         Flags: uninsdeletekey; Tasks: association\video\mts
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mts\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\mts
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mts\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\mts

;ogm
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ogm\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",4";         Flags: uninsdeletekey; Tasks: association\video\ogm
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ogm\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\ogm
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ogm\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\ogm

;ogv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ogv\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",4";         Flags: uninsdeletekey; Tasks: association\video\ogv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ogv\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\ogv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ogv\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\ogv

;pva
Root: "HKCR"; Subkey: "{#mpcbe_reg}.pva\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",1";          Flags: uninsdeletekey; Tasks: association\video\pva
Root: "HKCR"; Subkey: "{#mpcbe_reg}.pva\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\pva
Root: "HKCR"; Subkey: "{#mpcbe_reg}.pva\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";  Flags: uninsdeletekey; Tasks: association\video\pva

;ratdvd
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ratdvd\DefaultIcon";        ValueType: string; ValueData: """{app}\mpciconlib.dll"",30";         Flags: uninsdeletekey; Tasks: association\video\ratdvd
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ratdvd\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\ratdvd
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ratdvd\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\ratdvd

;rec
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rec\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",1";          Flags: uninsdeletekey; Tasks: association\video\rec
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rec\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\rec
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rec\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";  Flags: uninsdeletekey; Tasks: association\video\rec

;rm
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rm\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",8";          Flags: uninsdeletekey; Tasks: association\video\rm
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rm\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\rm
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rm\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";  Flags: uninsdeletekey; Tasks: association\video\rm

;rmm
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rmm\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",8";          Flags: uninsdeletekey; Tasks: association\video\rmm
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rmm\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\rmm
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rmm\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";  Flags: uninsdeletekey; Tasks: association\video\rmm

;rmvb
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rmvb\DefaultIcon";          ValueType: string; ValueData: """{app}\mpciconlib.dll"",8";          Flags: uninsdeletekey; Tasks: association\video\rmvb
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rmvb\shell\open\command";   ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\rmvb
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rmvb\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\rmvb

;rp
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rp\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",31";         Flags: uninsdeletekey; Tasks: association\video\rp
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rp\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\rp
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rp\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";  Flags: uninsdeletekey; Tasks: association\video\rp

;rpm
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rpm\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",8";          Flags: uninsdeletekey; Tasks: association\video\rpm
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rpm\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\rpm
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rpm\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";  Flags: uninsdeletekey; Tasks: association\video\rpm

;rt
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rt\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",31";         Flags: uninsdeletekey; Tasks: association\video\rt
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rt\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\rt
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rt\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";  Flags: uninsdeletekey; Tasks: association\video\rt

;smi
Root: "HKCR"; Subkey: "{#mpcbe_reg}.smi\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",8";          Flags: uninsdeletekey; Tasks: association\video\smi
Root: "HKCR"; Subkey: "{#mpcbe_reg}.smi\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\smi
Root: "HKCR"; Subkey: "{#mpcbe_reg}.smi\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";  Flags: uninsdeletekey; Tasks: association\video\smi

;smil
Root: "HKCR"; Subkey: "{#mpcbe_reg}.smil\DefaultIcon";          ValueType: string; ValueData: """{app}\mpciconlib.dll"",8";          Flags: uninsdeletekey; Tasks: association\video\smil
Root: "HKCR"; Subkey: "{#mpcbe_reg}.smil\shell\open\command";   ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\smil
Root: "HKCR"; Subkey: "{#mpcbe_reg}.smil\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\smil

;smk
Root: "HKCR"; Subkey: "{#mpcbe_reg}.smk\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",38";         Flags: uninsdeletekey; Tasks: association\video\smk
Root: "HKCR"; Subkey: "{#mpcbe_reg}.smk\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\smk
Root: "HKCR"; Subkey: "{#mpcbe_reg}.smk\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";  Flags: uninsdeletekey; Tasks: association\video\smk

;swf
Root: "HKCR"; Subkey: "{#mpcbe_reg}.swf\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",33";         Flags: uninsdeletekey; Tasks: association\video\swf
Root: "HKCR"; Subkey: "{#mpcbe_reg}.swf\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\swf
Root: "HKCR"; Subkey: "{#mpcbe_reg}.swf\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";  Flags: uninsdeletekey; Tasks: association\video\swf

;tp
Root: "HKCR"; Subkey: "{#mpcbe_reg}.tp\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",1";          Flags: uninsdeletekey; Tasks: association\video\tp
Root: "HKCR"; Subkey: "{#mpcbe_reg}.tp\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\tp
Root: "HKCR"; Subkey: "{#mpcbe_reg}.tp\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";  Flags: uninsdeletekey; Tasks: association\video\tp

;trp
Root: "HKCR"; Subkey: "{#mpcbe_reg}.trp\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",1";          Flags: uninsdeletekey; Tasks: association\video\trp
Root: "HKCR"; Subkey: "{#mpcbe_reg}.trp\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\trp
Root: "HKCR"; Subkey: "{#mpcbe_reg}.trp\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";  Flags: uninsdeletekey; Tasks: association\video\trp

;ts
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ts\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",1";         Flags: uninsdeletekey; Tasks: association\video\ts
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ts\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\ts
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ts\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\ts

;vob
Root: "HKCR"; Subkey: "{#mpcbe_reg}.vob\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",36";        Flags: uninsdeletekey; Tasks: association\video\vob
Root: "HKCR"; Subkey: "{#mpcbe_reg}.vob\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";      Flags: uninsdeletekey; Tasks: association\video\vob
Root: "HKCR"; Subkey: "{#mpcbe_reg}.vob\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\vob

;webm
Root: "HKCR"; Subkey: "{#mpcbe_reg}.webm\DefaultIcon";          ValueType: string; ValueData: """{app}\mpciconlib.dll"",40";         Flags: uninsdeletekey; Tasks: association\video\webm
Root: "HKCR"; Subkey: "{#mpcbe_reg}.webm\shell\open\command";   ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\webm
Root: "HKCR"; Subkey: "{#mpcbe_reg}.webm\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\video\webm

;wm
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wm\DefaultIcon";            ValueType: string; ValueData: """{app}\mpciconlib.dll"",6";          Flags: uninsdeletekey; Tasks: association\video\wm
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wm\shell\open\command";     ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";       Flags: uninsdeletekey; Tasks: association\video\wm
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wm\shell\enqueue\command";  ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";  Flags: uninsdeletekey; Tasks: association\video\wm

;wmp
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wmp\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",6";           Flags: uninsdeletekey; Tasks: association\video\wmp
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wmp\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";        Flags: uninsdeletekey; Tasks: association\video\wmp
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wmp\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";   Flags: uninsdeletekey; Tasks: association\video\wmp

;wmv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wmv\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",6";           Flags: uninsdeletekey; Tasks: association\video\wmv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wmv\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";        Flags: uninsdeletekey; Tasks: association\video\wmv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wmv\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";   Flags: uninsdeletekey; Tasks: association\video\wmv

;wmx
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wmx\DefaultIcon";           ValueType: string; ValueData: """{app}\mpciconlib.dll"",6";           Flags: uninsdeletekey; Tasks: association\video\wmx
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wmx\shell\open\command";    ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1""";        Flags: uninsdeletekey; Tasks: association\video\wmx
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wmx\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1""";   Flags: uninsdeletekey; Tasks: association\video\wmx

;;Audio Files
;aac
Root: "HKCR"; Subkey: "{#mpcbe_reg}.aac\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",11"; Flags: uninsdeletekey; Tasks: association\audio\aac
Root: "HKCR"; Subkey: "{#mpcbe_reg}.aac\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\aac
Root: "HKCR"; Subkey: "{#mpcbe_reg}.aac\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\aac

;ac3
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ac3\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",35"; Flags: uninsdeletekey; Tasks: association\audio\ac3
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ac3\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\ac3
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ac3\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\ac3

;aif
Root: "HKCR"; Subkey: "{#mpcbe_reg}.aif\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",24"; Flags: uninsdeletekey; Tasks: association\audio\aif
Root: "HKCR"; Subkey: "{#mpcbe_reg}.aif\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\aif
Root: "HKCR"; Subkey: "{#mpcbe_reg}.aif\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\aif

;aifc
Root: "HKCR"; Subkey: "{#mpcbe_reg}.aifc\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",24"; Flags: uninsdeletekey; Tasks: association\audio\aifc
Root: "HKCR"; Subkey: "{#mpcbe_reg}.aifc\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\aifc
Root: "HKCR"; Subkey: "{#mpcbe_reg}.aifc\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\aifc

;aiff
Root: "HKCR"; Subkey: "{#mpcbe_reg}.aiff\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",24"; Flags: uninsdeletekey; Tasks: association\audio\aiff
Root: "HKCR"; Subkey: "{#mpcbe_reg}.aiff\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\aiff
Root: "HKCR"; Subkey: "{#mpcbe_reg}.aiff\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\aiff

;alac
Root: "HKCR"; Subkey: "{#mpcbe_reg}.alac\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",20"; Flags: uninsdeletekey; Tasks: association\audio\alac
Root: "HKCR"; Subkey: "{#mpcbe_reg}.alac\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\alac
Root: "HKCR"; Subkey: "{#mpcbe_reg}.alac\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\alac

;amr
Root: "HKCR"; Subkey: "{#mpcbe_reg}.amr\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",25"; Flags: uninsdeletekey; Tasks: association\audio\amr
Root: "HKCR"; Subkey: "{#mpcbe_reg}.amr\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\amr
Root: "HKCR"; Subkey: "{#mpcbe_reg}.amr\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\amr

;ape
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ape\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",47"; Flags: uninsdeletekey; Tasks: association\audio\ape
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ape\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\ape
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ape\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\ape

;apl
Root: "HKCR"; Subkey: "{#mpcbe_reg}.apl\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",47"; Flags: uninsdeletekey; Tasks: association\audio\apl
Root: "HKCR"; Subkey: "{#mpcbe_reg}.apl\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\apl
Root: "HKCR"; Subkey: "{#mpcbe_reg}.apl\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\apl

;au
Root: "HKCR"; Subkey: "{#mpcbe_reg}.au\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",26"; Flags: uninsdeletekey; Tasks: association\audio\au
Root: "HKCR"; Subkey: "{#mpcbe_reg}.au\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\au
Root: "HKCR"; Subkey: "{#mpcbe_reg}.au\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\au

;cda
Root: "HKCR"; Subkey: "{#mpcbe_reg}.cda\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",17"; Flags: uninsdeletekey; Tasks: association\audio\cda
Root: "HKCR"; Subkey: "{#mpcbe_reg}.cda\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\cda
Root: "HKCR"; Subkey: "{#mpcbe_reg}.cda\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\cda

;dts
Root: "HKCR"; Subkey: "{#mpcbe_reg}.dts\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",35"; Flags: uninsdeletekey; Tasks: association\audio\dts
Root: "HKCR"; Subkey: "{#mpcbe_reg}.dts\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\dts
Root: "HKCR"; Subkey: "{#mpcbe_reg}.dts\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\dts

;flac
Root: "HKCR"; Subkey: "{#mpcbe_reg}.flac\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",19"; Flags: uninsdeletekey; Tasks: association\audio\flac
Root: "HKCR"; Subkey: "{#mpcbe_reg}.flac\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\flac
Root: "HKCR"; Subkey: "{#mpcbe_reg}.flac\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\flac

;m1a
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m1a\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",16"; Flags: uninsdeletekey; Tasks: association\audio\m1a
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m1a\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\m1a
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m1a\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\m1a

;m2a
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m2a\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",16"; Flags: uninsdeletekey; Tasks: association\audio\m2a
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m2a\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\m2a
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m2a\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\m2a

;m4a
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m4a\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",11"; Flags: uninsdeletekey; Tasks: association\audio\m4a
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m4a\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\m4a
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m4a\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\m4a

;m4b
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m4b\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",11"; Flags: uninsdeletekey; Tasks: association\audio\m4b
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m4b\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\m4b
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m4b\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\m4b

;mid
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mid\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",23"; Flags: uninsdeletekey; Tasks: association\audio\mid
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mid\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\mid
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mid\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\mid

;midi
Root: "HKCR"; Subkey: "{#mpcbe_reg}.midi\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",23"; Flags: uninsdeletekey; Tasks: association\audio\midi
Root: "HKCR"; Subkey: "{#mpcbe_reg}.midi\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\midi
Root: "HKCR"; Subkey: "{#mpcbe_reg}.midi\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\midi

;mka
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mka\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",12"; Flags: uninsdeletekey; Tasks: association\audio\mka
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mka\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\mka
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mka\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\mka

;mp2
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mp2\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",1"; Flags: uninsdeletekey; Tasks: association\audio\mp2
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mp2\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\mp2
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mp2\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\mp2

;mp3
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mp3\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",9"; Flags: uninsdeletekey; Tasks: association\audio\mp3
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mp3\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\mp3
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mp3\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\mp3

;mpa
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpa\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",16"; Flags: uninsdeletekey; Tasks: association\audio\mpa
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpa\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\mpa
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpa\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\mpa

;mpc
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpc\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",18"; Flags: uninsdeletekey; Tasks: association\audio\mpc
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpc\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\mpc
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpc\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\mpc

;ofr
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ofr\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",44"; Flags: uninsdeletekey; Tasks: association\audio\ofr
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ofr\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\ofr
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ofr\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\ofr

;ofs
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ofs\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",44"; Flags: uninsdeletekey; Tasks: association\audio\ofs
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ofs\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\ofs
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ofs\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\ofs

;oga
Root: "HKCR"; Subkey: "{#mpcbe_reg}.oga\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",10"; Flags: uninsdeletekey; Tasks: association\audio\oga
Root: "HKCR"; Subkey: "{#mpcbe_reg}.oga\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\oga
Root: "HKCR"; Subkey: "{#mpcbe_reg}.oga\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\oga

;ogg
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ogg\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",10"; Flags: uninsdeletekey; Tasks: association\audio\ogg
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ogg\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\ogg
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ogg\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\ogg

;ra
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ra\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",14"; Flags: uninsdeletekey; Tasks: association\audio\ra
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ra\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\ra
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ra\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\ra

;ram
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ram\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",8"; Flags: uninsdeletekey; Tasks: association\audio\ram
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ram\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\ram
Root: "HKCR"; Subkey: "{#mpcbe_reg}.ram\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\ram

;rmi
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rmi\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",23"; Flags: uninsdeletekey; Tasks: association\audio\rmi
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rmi\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\rmi
Root: "HKCR"; Subkey: "{#mpcbe_reg}.rmi\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\rmi

;snd
Root: "HKCR"; Subkey: "{#mpcbe_reg}.snd\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",26"; Flags: uninsdeletekey; Tasks: association\audio\snd
Root: "HKCR"; Subkey: "{#mpcbe_reg}.snd\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\snd
Root: "HKCR"; Subkey: "{#mpcbe_reg}.snd\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\snd

;tak
Root: "HKCR"; Subkey: "{#mpcbe_reg}.tak\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",45"; Flags: uninsdeletekey; Tasks: association\audio\tak
Root: "HKCR"; Subkey: "{#mpcbe_reg}.tak\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\tak
Root: "HKCR"; Subkey: "{#mpcbe_reg}.tak\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\tak

;tta
Root: "HKCR"; Subkey: "{#mpcbe_reg}.tta\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",46"; Flags: uninsdeletekey; Tasks: association\audio\tta
Root: "HKCR"; Subkey: "{#mpcbe_reg}.tta\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\tta
Root: "HKCR"; Subkey: "{#mpcbe_reg}.tta\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\tta

;wav
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wav\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",15"; Flags: uninsdeletekey; Tasks: association\audio\wav
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wav\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\wav
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wav\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\wav

;wax
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wax\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",21"; Flags: uninsdeletekey; Tasks: association\audio\wax
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wax\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\wax
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wax\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\wax

;wma
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wma\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",13"; Flags: uninsdeletekey; Tasks: association\audio\wma
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wma\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\wma
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wma\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\wma

;wv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wv\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",41"; Flags: uninsdeletekey; Tasks: association\audio\wv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wv\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\wv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.wv\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\audio\wv

;;Playlist
;bdmv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.bdmv\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",21"; Flags: uninsdeletekey; Tasks: association\playlist\bdmv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.bdmv\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\playlist\bdmv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.bdmv\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\playlist\bdmv

;m3u
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m3u\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",21"; Flags: uninsdeletekey; Tasks: association\playlist\m3u
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m3u\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\playlist\m3u
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m3u\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\playlist\m3u

;m3u8
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m3u8\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",21"; Flags: uninsdeletekey; Tasks: association\playlist\m3u8
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m3u8\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\playlist\m3u8
Root: "HKCR"; Subkey: "{#mpcbe_reg}.m3u8\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\playlist\m3u8

;mpcpl
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpcpl\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",21"; Flags: uninsdeletekey; Tasks: association\playlist\mpcpl
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpcpl\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\playlist\mpcpl
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpcpl\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\playlist\mpcpl

;mpls
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpls\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",21"; Flags: uninsdeletekey; Tasks: association\playlist\mpls
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpls\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\playlist\mpls
Root: "HKCR"; Subkey: "{#mpcbe_reg}.mpls\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\playlist\mpls

;pls
Root: "HKCR"; Subkey: "{#mpcbe_reg}.pls\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",21"; Flags: uninsdeletekey; Tasks: association\playlist\pls
Root: "HKCR"; Subkey: "{#mpcbe_reg}.pls\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\playlist\pls
Root: "HKCR"; Subkey: "{#mpcbe_reg}.pls\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\playlist\pls

;;DirectShow Media
;dsa
Root: "HKCR"; Subkey: "{#mpcbe_reg}.dsa\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",32"; Flags: uninsdeletekey; Tasks: association\dsmedia\dsa
Root: "HKCR"; Subkey: "{#mpcbe_reg}.dsa\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\dsmedia\dsa
Root: "HKCR"; Subkey: "{#mpcbe_reg}.dsa\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\dsmedia\dsa

;dsm
Root: "HKCR"; Subkey: "{#mpcbe_reg}.dsm\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",32"; Flags: uninsdeletekey; Tasks: association\dsmedia\dsm
Root: "HKCR"; Subkey: "{#mpcbe_reg}.dsm\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\dsmedia\dsm
Root: "HKCR"; Subkey: "{#mpcbe_reg}.dsm\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\dsmedia\dsm

;dss
Root: "HKCR"; Subkey: "{#mpcbe_reg}.dss\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",32"; Flags: uninsdeletekey; Tasks: association\dsmedia\dss
Root: "HKCR"; Subkey: "{#mpcbe_reg}.dss\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\dsmedia\dss
Root: "HKCR"; Subkey: "{#mpcbe_reg}.dss\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\dsmedia\dss

;dsv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.dsv\DefaultIcon"; ValueType: string; ValueData: """{app}\mpciconlib.dll"",32"; Flags: uninsdeletekey; Tasks: association\dsmedia\dsv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.dsv\shell\open\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" ""%1"""; Flags: uninsdeletekey; Tasks: association\dsmedia\dsv
Root: "HKCR"; Subkey: "{#mpcbe_reg}.dsv\shell\enqueue\command"; ValueType: string; ValueData: """{app}\{#mpcbe_exe}"" /add ""%1"""; Flags: uninsdeletekey; Tasks: association\dsmedia\dsv

[Code]
#if defined(sse_required) || defined(sse2_required)
function IsProcessorFeaturePresent(Feature: Integer): Boolean;
external 'IsProcessorFeaturePresent@kernel32.dll stdcall';
#endif

const installer_mutex_name = 'mpcbe_setup_mutex';


function GetInstallFolder(Default: String): String;
var
  sInstallPath: String;
begin
  if not RegQueryStringValue(HKLM, 'SOFTWARE\MPC-BE', 'ExePath', sInstallPath) then begin
    Result := ExpandConstant('{pf}\MPC-BE');
  end
  else begin
    RegQueryStringValue(HKLM, 'SOFTWARE\MPC-BE', 'ExePath', sInstallPath);
    Result := ExtractFileDir(sInstallPath);
    if (Result = '') or not DirExists(Result) then begin
      Result := ExpandConstant('{pf}\MPC-BE');
    end;
  end;
end;


function D3DX9DLLExists(): Boolean;
begin
  if FileExists(ExpandConstant('{sys}\D3DX9_{#DIRECTX_SDK_NUMBER}.dll')) then
    Result := True
  else
    Result := False;
end;


#if defined(sse_required)
function Is_SSE_Supported(): Boolean;
begin
  // PF_XMMI_INSTRUCTIONS_AVAILABLE
  Result := IsProcessorFeaturePresent(6);
end;

#elif defined(sse2_required)

function Is_SSE2_Supported(): Boolean;
begin
  // PF_XMMI64_INSTRUCTIONS_AVAILABLE
  Result := IsProcessorFeaturePresent(10);
end;

#endif


function IsUpgrade(): Boolean;
var
  sPrevPath: String;
begin
  sPrevPath := WizardForm.PrevAppDir;
  Result := (sPrevPath <> '');
end;


// Check if MPC-BE's settings exist
function SettingsExistCheck(): Boolean;
begin
  if RegKeyExists(HKEY_CURRENT_USER, 'Software\MPC-BE') or
  FileExists(ExpandConstant('{app}\{#mpcbe_ini}')) then
    Result := True
  else
    Result := False;
end;


function ShouldSkipPage(PageID: Integer): Boolean;
begin
  // Hide the License page
  if IsUpgrade() and (PageID = wpLicense) then
    Result := True;
end;


procedure CleanUpSettingsAndFiles();
begin
  DeleteFile(ExpandConstant('{app}\{#mpcbe_ini}'));
  DeleteFile(ExpandConstant('{userappdata}\MPC-BE\default.mpcpl'));
  RemoveDir(ExpandConstant('{userappdata}\MPC-BE'));
  RegDeleteKeyIncludingSubkeys(HKCU, 'Software\MPC-BE Filters');
  RegDeleteKeyIncludingSubkeys(HKCU, 'Software\MPC-BE');
  RegDeleteKeyIncludingSubkeys(HKLM, 'SOFTWARE\MPC-BE');
end;


procedure CurStepChanged(CurStep: TSetupStep);
var
  iLanguage: Integer;
begin
  if CurStep = ssPostInstall then begin
    if IsTaskSelected('reset_settings') then
      CleanUpSettingsAndFiles();

    iLanguage := StrToInt(ExpandConstant('{cm:langid}'));
    RegWriteStringValue(HKLM, 'SOFTWARE\MPC-BE', 'ExePath', ExpandConstant('{app}\{#mpcbe_exe}'));

    if IsComponentSelected('mpcresources') and FileExists(ExpandConstant('{app}\{#mpcbe_ini}')) then
      SetIniInt('Settings', 'InterfaceLanguage', iLanguage, ExpandConstant('{app}\{#mpcbe_ini}'))
    else
      RegWriteDWordValue(HKCU, 'Software\MPC-BE\Settings', 'InterfaceLanguage', iLanguage);
  end;

  if (CurStep = ssDone) and not WizardSilent() and not D3DX9DLLExists() then
    SuppressibleMsgBox(CustomMessage('msg_NoD3DX9DLL_found'), mbError, MB_OK, MB_OK);

end;


procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  // When uninstalling, ask the user to delete MPC-BE settings
  if (CurUninstallStep = usUninstall) and SettingsExistCheck() then begin
    if SuppressibleMsgBox(CustomMessage('msg_DeleteSettings'), mbConfirmation, MB_YESNO or MB_DEFBUTTON2, IDNO) = IDYES then
      CleanUpSettingsAndFiles();
  end;
end;


function InitializeSetup(): Boolean;
begin
  // Create a mutex for the installer and if it's already running display a message and stop installation
  if CheckForMutexes(installer_mutex_name) and not WizardSilent() then begin
    SuppressibleMsgBox(CustomMessage('msg_SetupIsRunningWarning'), mbError, MB_OK, MB_OK);
    Result := False;
  end
  else begin
    Result := True;
    CreateMutex(installer_mutex_name);

#if defined(sse2_required)
    if not Is_SSE2_Supported() then begin
      SuppressibleMsgBox(CustomMessage('msg_simd_sse2'), mbCriticalError, MB_OK, MB_OK);
      Result := False;
    end;
#elif defined(sse_required)
    if not Is_SSE_Supported() then begin
      SuppressibleMsgBox(CustomMessage('msg_simd_sse'), mbCriticalError, MB_OK, MB_OK);
      Result := False;
    end;
#endif

  end;
end;