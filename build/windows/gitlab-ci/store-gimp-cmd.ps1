$arm64 = "gimp-arm64"
$win64 = "gimp-w64"
$win32 = "gimp-w32"


# 1. CONFIGURE GIMP FILES

### Copy files to avoid conflict with Windows Installer job
Copy-Item -Path "$arm64" -Destination "build\windows\store\$arm64" -Recurse
Copy-Item -Path "$win64" -Destination "build\windows\store\$win64" -Recurse
Copy-Item -Path "$win32" -Destination "build\windows\store\$win32" -Recurse

### Get GIMP major.minor version
Set-Location build\windows\store\
Get-Content -Path '..\..\..\meson.build' | Select-Object -Index 2 | Set-Content -Path .\GIMP_version-raw.txt -NoNewline
Get-Content -Path GIMP_version-raw.txt | Foreach-Object {$_ -replace "  version: '",""}  | Foreach-Object {$_ -replace "',",""} | Set-Content -Path .\GIMP_version2-manifest.txt -NoNewline
$version = Get-Content -Path 'GIMP_version2-manifest.txt' -Raw
"$version".Substring(0,4) | Set-Content -Path .\GIMP_version3-majorminor.txt -NoNewline
$majorminor = Get-Content -Path 'GIMP_version3-majorminor.txt' -Raw
$dots = ($majorminor.ToCharArray() -eq '.').count
if ($dots -eq 2) {"$version".Substring(0,3) | Set-Content -Path .\GIMP_version3-majorminor.txt -NoNewline}
$majorminor = Get-Content -Path 'GIMP_version3-majorminor.txt' -Raw

### Remove uneeded files (to match the Windows Installer artifact)
$omissions = ("include\", "lib\babl-0.1\*.a", "lib\gdk-pixbuf-2.0\2.10.0\loaders\*.a", "lib\gegl-0.4\*.a", "lib\gegl-0.4\*.json", "lib\gimp\${majorminor}\modules\*.a", "lib\gimp\${majorminor}\plug-ins\test-sphere-v3\", "lib\gimp\${majorminor}\plug-ins\ts-helloworld\", "lib\gio\modules\giomodule.cache", "lib\gtk-3.0\", "share\applications\", "share\glib-2.0\codegen\", "share\glib-2.0\dtds\", "share\glib-2.0\gdb\", "share\glib-2.0\gettext\", "share\gir-1.0\", "share\man\", "share\themes\", "share\vala\", "gimp.cmd")
Set-Location $arm64
Remove-Item $omissions -Recurse
Set-Location ..\$win64
Remove-Item $omissions -Recurse
Set-Location ..\$win32
Remove-Item $omissions -Recurse
##"share\ghostscript\..\doc",##

### Disable Update check (since the package is auto updated)
Set-Location ..\
Add-Content $arm64\share\gimp\$majorminor\gimp-release "check-update=false"
Add-Content $win64\share\gimp\$majorminor\gimp-release "check-update=false"
Add-Content $win32\share\gimp\$majorminor\gimp-release "check-update=false"


# 2. CONFIGURE MANIFEST

### Get and Set GIMP full version
$manifest = Get-Content -Path 'AppxManifest.xml'
$newManifest = $manifest -replace "@GIMP_VERSION@", "$version" | Set-Content -Path 'AppxManifest.xml'

### Set GIMP major.minor version
$manifest = Get-Content -Path 'AppxManifest.xml'
$newManifest = $manifest -replace "@MAJORMINOR@", "$majorminor" | Set-Content -Path 'AppxManifest.xml'

### Match supported fileypes
Get-Content -Path '..\installer\associations.list' | Foreach-Object {"              <uap:FileType>." + $_} | Foreach-Object {$_ +  "</uap:FileType>"} | set-content -Path .\filetypes.txt
Get-Content -Path filetypes.txt | Where-Object {$_ -notmatch 'xcf'} | Set-Content -Path .\filetypes2.txt
$filetypes = Get-Content -Path 'filetypes2.txt' -Raw
$manifest = Get-Content -Path 'AppxManifest.xml'
$newManifest = $manifest -replace "@FILE_TYPES@", "$filetypes" | Set-Content -Path 'AppxManifest.xml'

### Configure and copy manifest to each architecture
### Also, configure the interpreters
Copy-Item AppxManifest.xml $arm64
Copy-Item Src\environ\* $arm64\lib\gimp\${majorminor}\environ\
Copy-Item Src\interpreters\* $arm64\lib\gimp\${majorminor}\interpreters\
Set-Location gimp-arm64
$manifest = Get-Content -Path 'AppxManifest.xml'
$transitionalManifest = $manifest -replace 'neutral', 'arm64' | Set-Content -Path 'AppxManifest.xml'
$transitionalManifest = Get-Content -Path 'AppxManifest.xml'
$finalManifest = $transitionalManifest -replace '%SUFIX%', '5.1' | Set-Content -Path 'AppxManifest.xml'
Set-Location lib\gimp\${majorminor}\interpreters\
$lua = Get-Content -Path 'lua.interp'
$finalLua = $lua -replace 'jit.exe', '5.1.exe' | Set-Content -Path 'lua.interp'
Set-Location ..\..\..\..\..\

Copy-Item AppxManifest.xml $win64
Copy-Item Src\environ\* $win64\lib\gimp\${majorminor}\environ\
Copy-Item Src\interpreters\* $win64\lib\gimp\${majorminor}\interpreters\
Set-Location gimp-w64
$manifest = Get-Content -Path 'AppxManifest.xml'
$transitionalManifest = $manifest -replace 'neutral', 'x64' | Set-Content -Path 'AppxManifest.xml'
$transitionalManifest = Get-Content -Path 'AppxManifest.xml'
$finalManifest = $transitionalManifest -replace '%SUFIX%', 'jit' | Set-Content -Path 'AppxManifest.xml'

Copy-Item ..\AppxManifest.xml ..\$win32
Copy-Item ..\Src\environ\* ..\$win32\lib\gimp\${majorminor}\environ\
Copy-Item ..\Src\interpreters\* ..\$win32\lib\gimp\${majorminor}\interpreters\
Set-Location ..\gimp-w32
$manifest = Get-Content -Path 'AppxManifest.xml'
$transitionalManifest = $manifest -replace 'neutral', 'x86' | Set-Content -Path 'AppxManifest.xml'
$transitionalManifest = Get-Content -Path 'AppxManifest.xml'
$finalManifest = $transitionalManifest -replace '%SUFIX%', 'jit' | Set-Content -Path 'AppxManifest.xml'


# 3. CREATE ASSETS

Set-Location ..\Assets

& 'C:\msys64\usr\bin\pacman.exe' --noconfirm -S mingw-w64-ucrt-x86_64-librsvg
Set-Alias -Name 'rsvg-convert' -Value 'C:\msys64\ucrt64\bin\rsvg-convert.exe'

Copy-Item ..\..\..\..\icons\Legacy\scalable\gimp-wilber.svg .\wilber.svg
Copy-Item ..\..\..\..\desktop\scalable\gimp.svg .\wilber_hihes.svg

### Generate Universal Icons (introduced in Win 8)
rsvg-convert wilber.svg -o StoreLogo.png -w 50 -h 50
rsvg-convert wilber.svg -o StoreLogo.scale-100.png -w 50 -h 50
rsvg-convert wilber.svg -o StoreLogo.scale-125.png -w 65 -h 65 # Aproximated
rsvg-convert wilber.svg -o StoreLogo.scale-150.png -w 75 -h 75
rsvg-convert wilber.svg -o StoreLogo.scale-200.png -w 100 -h 100
rsvg-convert wilber.svg -o StoreLogo.scale-400.png -w 200 -h 200
rsvg-convert wilber.svg -o MedTile.png -w 150 -h 150
rsvg-convert wilber.svg -o MedTile.scale-100.png -w 150 -h 150
rsvg-convert wilber.svg -o MedTile.scale-125.png -w 190 -h 190 # Aproximated
rsvg-convert wilber.svg -o MedTile.scale-150.png -w 225 -h 225
rsvg-convert wilber_hihes.svg -o MedTile.scale-200.png -w 300 -h 300
rsvg-convert wilber_hihes.svg -o MedTile.scale-400.png -w 600 -h 600

### Generate New Icons (partly introduced in late Win 10)
rsvg-convert wilber.svg -o AppList.png -w 44 -h 44
rsvg-convert wilber.svg -o AppList.scale-100.png -w 44 -h 44
rsvg-convert wilber.svg -o AppList.scale-125.png -w 55 -h 55
rsvg-convert wilber.svg -o AppList.scale-150.png -w 66 -h 66
rsvg-convert wilber.svg -o AppList.scale-200.png -w 88 -h 88
rsvg-convert wilber.svg -o AppList.scale-400.png -w 176 -h 176
rsvg-convert wilber.svg -o AppList.altform-unplated.png -w 44 -h 44
rsvg-convert wilber.svg -o AppList.altform-lightunplated.png -w 44 -h 44
rsvg-convert wilber.svg -o AppList.targetsize-44_altform-unplated.png -w 44 -h 44
rsvg-convert wilber.svg -o AppList.targetsize-44_altform-lightunplated.png -w 44 -h 44
function Generate-New-Icons {
    rsvg-convert wilber.svg -o AppList.targetsize-${size}.png -w ${size} -h ${size}
    Copy-Item AppList.targetsize-${size}.png AppList.targetsize-${size}_altform-unplated.png
    Copy-Item AppList.targetsize-${size}.png AppList.targetsize-${size}_altform-lightunplated.png
}
$size = 16
Generate-New-Icons
$size = 20
Generate-New-Icons
$size = 24
Generate-New-Icons
$size = 30
Generate-New-Icons
$size = 32
Generate-New-Icons
$size = 36
Generate-New-Icons
$size = 40
Generate-New-Icons
$size = 48
Generate-New-Icons
$size = 60
Generate-New-Icons
$size = 64
Generate-New-Icons
$size = 72
Generate-New-Icons
$size = 80
Generate-New-Icons
$size = 96
Generate-New-Icons
rsvg-convert wilber_hihes.svg -o AppList.targetsize-256.png -w 256 -h 256
    Copy-Item AppList.targetsize-256.png AppList.targetsize-256_altform-unplated.png
    Copy-Item AppList.targetsize-256.png AppList.targetsize-256_altform-lightunplated.png

### Generate Legacy Icons (discontinued in Win 11)
rsvg-convert wilber_hihes.svg -o LargeTile.png -w 310 -h 310
rsvg-convert wilber.svg -o SmallTile.png -w 71 -h 71

### Generate XCF icon
rsvg-convert fileicon_medium.svg -o fileicon.png -w 44 -h 44
rsvg-convert fileicon_cute.svg -o fileicon.targetsize-16.png -w 16 -h 16
rsvg-convert fileicon_cute.svg -o fileicon.targetsize-20.png -w 20 -h 20
rsvg-convert fileicon_medium.svg -o fileicon.targetsize-24.png -w 24 -h 24
rsvg-convert fileicon_medium.svg -o fileicon.targetsize-30.png -w 30 -h 30
rsvg-convert fileicon_medium.svg -o fileicon.targetsize-32.png -w 32 -h 32
rsvg-convert fileicon_medium.svg -o fileicon.targetsize-36.png -w 36 -h 36
rsvg-convert fileicon_medium.svg -o fileicon.targetsize-40.png -w 40 -h 40
rsvg-convert fileicon_medium.svg -o fileicon.targetsize-48.png -w 48 -h 48
rsvg-convert fileicon_glorious.svg -o fileicon.targetsize-60.png -w 60 -h 60
rsvg-convert fileicon_glorious.svg -o fileicon.targetsize-64.png -w 64 -h 64
rsvg-convert fileicon_glorious.svg -o fileicon.targetsize-72.png -w 72 -h 72
rsvg-convert fileicon_glorious.svg -o fileicon.targetsize-80.png -w 80 -h 80
rsvg-convert fileicon_glorious.svg -o fileicon.targetsize-96.png -w 96 -h 96
rsvg-convert fileicon_glorious.svg -o fileicon.targetsize-128.png -w 128 -h 128
rsvg-convert fileicon_glorious.svg -o fileicon.targetsize-256.png -w 256 -h 256

Remove-Item *.svg

### Copy icons
Set-Location ..\
Copy-Item -Path "Assets\" -Destination "$arm64\Assets\" -Recurse
Copy-Item -Path "Assets\" -Destination "$win64\Assets\" -Recurse
Copy-Item -Path "Assets\" -Destination "$win32\Assets\" -Recurse

### Generate resources.pri
Get-Content -Path '..\..\..\.gitlab-ci.yml' | Select-String 'winget' | Set-Content -Path .\SDK_version.txt -NoNewline
Get-Content -Path SDK_version.txt | Foreach-Object {$_ -replace "    - winget install -e --id Microsoft.WindowsSDK.",""}  | Set-Content -Path .\SDK_version2.txt -NoNewline
$SDKversion = Get-Content -Path 'SDK_version2.txt' -Raw
Set-Alias -Name 'makepri' -Value "C:\Program Files (x86)\Windows Kits\10\bin\${SDKversion}.0\x64\makepri.exe"

Set-Location $arm64
makepri createconfig /cf priconfig.xml /dq lang-en-US /pv 10.0.0
Set-Location ..\
makepri new /pr $arm64 /cf $arm64\priconfig.xml /of $arm64

Set-Location $win64
makepri createconfig /cf priconfig.xml /dq lang-en-US /pv 10.0.0
Set-Location ..\
makepri new /pr $win64 /cf $win64\priconfig.xml /of $win64

Set-Location $win32
makepri createconfig /cf priconfig.xml /dq lang-en-US /pv 10.0.0
Set-Location ..\
makepri new /pr $win32 /cf $win32\priconfig.xml /of $win32

Remove-Item *.txt


# 4. BUILD .MSIXBUNDLE

### Build .msix for each arch
Set-Alias -Name 'makeappx' -Value 'C:\Program Files (x86)\Windows Kits\10\App Certification Kit\MakeAppx.exe'

makeappx pack /d gimp-arm64 /p _TempOutput\gimp-store_arm64.msix 
makeappx pack /d gimp-w64 /p _TempOutput\gimp-store_w64.msix
makeappx pack /d gimp-w32 /p _TempOutput\gimp-store_w32.msix 

### Build .msixbundle
makeappx bundle /d _TempOutput /p _Output\gimp-store_multi-arch.msixbundle 

exit