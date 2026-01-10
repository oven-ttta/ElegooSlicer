# ElegooSlicer Installer Build Tasks

## Project Overview
- **Project:** ElegooSlicer (3D Printer Slicer Software)
- **Version:** 1.1.8.0
- **Platform:** Windows x64
- **Date:** 2026-01-10

---

## Tasks Completed

### 1. Project Structure Analysis

#### 1.1 สำรวจโฟลเดอร์โปรเจกต์
- [x] ใช้ Glob pattern `*` เพื่อดูไฟล์ใน root directory
- [x] ใช้ Glob pattern `**/*.{js,ts,py,json,html}` เพื่อหาไฟล์ config
- [x] พบว่าเป็นโปรเจกต์ ElegooSlicer อยู่ใน `C:\Users\admin\Desktop\3D\ElegooSlicer`

#### 1.2 ค้นหาไฟล์ Build Configuration
- [x] ค้นหา `CMakeLists.txt` ในโปรเจกต์
- [x] ค้นหาไฟล์ `.nsi`, `.iss`, `.cmake`
- [x] พบไฟล์ `package.nsi` สำหรับสร้าง installer

#### 1.3 ตรวจสอบ Root Directory
- [x] รัน `ls -la` เพื่อดูไฟล์ทั้งหมด
- [x] พบไฟล์สำคัญ:
  - `CMakeLists.txt`
  - `package.nsi`
  - `version.inc`
  - `build_release_vs2022.bat`
  - `LICENSE.txt`

#### 1.4 อ่านไฟล์ Version
- [x] อ่านไฟล์ `version.inc`
- [x] พบ version: `ELEGOOSLICER_SHORT_VERSION="1.1.8"`
- [x] กำหนด full version เป็น `1.1.8.0`

---

### 2. Build Output Verification

#### 2.1 ตรวจสอบโฟลเดอร์ build/package
- [x] รัน `ls -la build/package`
- [x] ยืนยันว่ามีไฟล์ output จาก build process

#### 2.2 ตรวจสอบ Main Executable
- [x] พบ `elegoo-slicer.exe` (157 KB)
- [x] พบ `ElegooSlicer.dll` (59 MB - main library)

#### 2.3 ตรวจสอบ Dependencies
- [x] พบ DLL files ที่จำเป็น:
  - `freetype.dll` (686 KB)
  - `libgmp-10.dll` (543 KB)
  - `libmpfr-4.dll` (436 KB)
  - `WebView2Loader.dll` (158 KB)

#### 2.4 ตรวจสอบ OpenCASCADE Libraries
- [x] พบ TK*.dll files ทั้งหมด:
  - `TKBO.dll`, `TKBRep.dll`, `TKCAF.dll`
  - `TKCDF.dll`, `TKernel.dll`, `TKG2d.dll`
  - `TKG3d.dll`, `TKGeomAlgo.dll`, `TKGeomBase.dll`
  - `TKHLR.dll`, `TKLCAF.dll`, `TKMath.dll`
  - `TKMesh.dll`, `TKPrim.dll`, `TKService.dll`
  - `TKShHealing.dll`, `TKSTEP.dll`, `TKSTEP209.dll`
  - `TKSTEPAttr.dll`, `TKSTEPBase.dll`, `TKTopAlgo.dll`
  - `TKV3d.dll`, `TKVCAF.dll`, `TKXCAF.dll`
  - `TKXDESTEP.dll`, `TKXSBase.dll`

#### 2.5 ตรวจสอบ Resources
- [x] พบโฟลเดอร์ `resources/` พร้อมไฟล์ config

---

### 3. NSIS Installation Check

#### 3.1 ค้นหา NSIS ในระบบ
- [x] รัน `where makensis`
- [x] ค้นหาใน `C:\Program Files*\NSIS\`
- [x] ผลลัพธ์: ไม่พบ NSIS ในระบบ

#### 3.2 สร้าง Batch Script เบื้องต้น
- [x] สร้างไฟล์ `build_installer.bat`
- [x] เพิ่ม logic ตรวจสอบ NSIS installation
- [x] เพิ่ม logic ตรวจสอบ build/package folder
- [x] เพิ่ม logic ตรวจสอบ elegoo-slicer.exe

#### 3.3 ผู้ใช้ติดตั้ง NSIS
- [x] แจ้งผู้ใช้ให้ดาวน์โหลด NSIS จาก https://nsis.sourceforge.io/Download
- [x] ผู้ใช้ติดตั้ง NSIS สำเร็จที่ `C:\Program Files (x86)\NSIS\`

---

### 4. NSIS Compilation Attempts

#### 4.1 ความพยายามครั้งที่ 1 - Direct Command
- [x] รัน makensis ผ่าน bash
- [x] ผลลัพธ์: Error - Git bash แปลง path ผิด

#### 4.2 ความพยายามครั้งที่ 2 - CMD Wrapper
- [x] รัน makensis ผ่าน `cmd /c`
- [x] ผลลัพธ์: Command ไม่ทำงาน

#### 4.3 ความพยายามครั้งที่ 3 - Batch File
- [x] สร้างไฟล์ `run_nsis.bat`
- [x] รันผ่าน `cmd /c`
- [x] ผลลัพธ์: ไม่มี output

#### 4.4 ความพยายามครั้งที่ 4 - MSYS_NO_PATHCONV
- [x] ใช้ environment variable `MSYS_NO_PATHCONV=1`
- [x] ผลลัพธ์: Error - invalid parameters

#### 4.5 ความพยายามครั้งที่ 5 - PowerShell
- [x] รัน makensis ผ่าน PowerShell
- [x] ผลลัพธ์: Error - "Bad text encoding" on line 1

---

### 5. Encoding Issue Resolution

#### 5.1 วิเคราะห์ปัญหา Encoding
- [x] อ่านไฟล์ `package.nsi`
- [x] พบว่ามี Chinese characters (comments)
- [x] ไฟล์ใช้ encoding แบบ Chinese Simplified

#### 5.2 ทดลอง UTF8 Encoding
- [x] ใช้ parameter `/INPUTCHARSET UTF8`
- [x] ผลลัพธ์: ยังคง error "Bad text encoding"

#### 5.3 ทดลอง CP936 Encoding
- [x] ใช้ parameter `/INPUTCHARSET CP936` (Chinese Simplified)
- [x] ผลลัพธ์: ผ่าน encoding check
- [x] Error ใหม่: "could not find: nsProcess.nsh"

---

### 6. NSIS Plugin Installation

#### 6.1 ค้นหา Plugin ในโปรเจกต์
- [x] ใช้ Glob pattern `ElegooSlicer/**/*nsProcess*`
- [x] พบไฟล์ใน `tools/NSIS/`:
  - `Include/nsProcess.nsh`
  - `Plugins/x86-ansi/nsProcess.dll`
  - `Plugins/x86-unicode/nsProcess.dll`

#### 6.2 ตรวจสอบ NSIS Plugin Directory
- [x] ตรวจสอบ `C:\Program Files (x86)\NSIS\Include\`
- [x] ตรวจสอบ `C:\Program Files (x86)\NSIS\Plugins\`
- [x] ยืนยันว่าไม่มี nsProcess plugin

#### 6.3 ความพยายาม Copy Plugin ครั้งที่ 1
- [x] ใช้ PowerShell Copy-Item
- [x] ผลลัพธ์: Access Denied (ต้องการ Admin privileges)

#### 6.4 สร้าง Admin Batch Script
- [x] สร้างไฟล์ `create_installer.bat`
- [x] เพิ่ม Admin elevation check (`net session`)
- [x] เพิ่ม auto-elevation ด้วย `Start-Process -Verb RunAs`
- [x] เพิ่ม plugin copy commands
- [x] เพิ่ม NSIS compilation command
- [x] เพิ่ม output verification
- [x] เพิ่ม explorer open on success

---

### 7. Final Installer Build

#### 7.1 รัน Build Script
- [x] รัน `create_installer.bat` ผ่าน PowerShell with RunAs
- [x] UAC prompt ปรากฏและได้รับอนุญาต

#### 7.2 Copy Plugins
- [x] Copy `nsProcess.nsh` ไปยัง NSIS Include folder
- [x] Copy `nsProcess.dll` (x86-ansi) ไปยัง NSIS Plugins folder
- [x] Copy `nsProcess.dll` (x86-unicode) ไปยัง NSIS Plugins folder

#### 7.3 NSIS Compilation
- [x] NSIS อ่านไฟล์ `package.nsi` สำเร็จ
- [x] ประมวลผล script sections ทั้งหมด
- [x] สร้าง installer executable

#### 7.4 ตรวจสอบ Output
- [x] ใช้ PowerShell Get-ChildItem ตรวจสอบ
- [x] พบไฟล์ `ElegooSlicer_Windows_Installer_V1.1.8.0.exe`
- [x] ขนาดไฟล์: 79,248,357 bytes (~75.6 MB)
- [x] วันที่สร้าง: 2026-01-10 13:57:34

---

## Output Files

| File | Path | Size |
|------|------|------|
| Installer | `build/ElegooSlicer_Windows_Installer_V1.1.8.0.exe` | 75.6 MB |

---

## Scripts Created

### `create_installer.bat`
| Feature | Description |
|---------|-------------|
| Admin Check | ตรวจสอบสิทธิ์ admin ด้วย `net session` |
| Auto-Elevation | Request admin ด้วย PowerShell RunAs |
| Plugin Copy | Copy nsProcess plugin ไปยัง NSIS |
| NSIS Compile | รัน makensis พร้อม parameters |
| Output Check | ตรวจสอบว่าสร้างไฟล์สำเร็จ |
| Explorer Open | เปิด folder แสดงไฟล์ installer |

### `build_installer.bat`
| Feature | Description |
|---------|-------------|
| NSIS Detection | ค้นหา NSIS installation |
| Validation | ตรวจสอบ build files |
| Basic Build | รัน makensis (ไม่มี auto-elevation) |

---

## Technical Notes

### NSIS Command Parameters
```
/INPUTCHARSET CP936     - Chinese Simplified encoding
/DPRODUCT_VERSION=X.X.X - Define version number
/DINSTALL_PATH=<path>   - Define source files path
```

### Full Command Used
```batch
makensis.exe /INPUTCHARSET CP936 /DPRODUCT_VERSION=1.1.8.0 /DINSTALL_PATH=C:\Users\admin\Desktop\3D\ElegooSlicer\build\package package.nsi
```

### Required Plugins
| Plugin | Purpose |
|--------|---------|
| nsProcess | ตรวจจับว่าโปรแกรมกำลังรันอยู่หรือไม่ |
| MUI | Modern User Interface (มากับ NSIS) |
| WordFunc | String manipulation (มากับ NSIS) |

### Installer Features
- Auto-detect and uninstall previous version
- Detect if ElegooSlicer is running and prompt to close
- Create Start Menu shortcuts
- Create Desktop shortcut
- Register in Windows Add/Remove Programs
- Support English and Chinese Simplified languages

---

## Troubleshooting Log

| Issue | Cause | Solution |
|-------|-------|----------|
| Path conversion error | Git Bash แปลง `/D` เป็น path | ใช้ PowerShell แทน |
| Bad text encoding | ไฟล์ใช้ Chinese encoding | ใช้ `/INPUTCHARSET CP936` |
| nsProcess.nsh not found | Plugin ไม่ได้ติดตั้ง | Copy จาก tools/NSIS/ |
| Access Denied | ต้องการ Admin privileges | ใช้ RunAs elevation |

---

## How to Rebuild Installer

1. ตรวจสอบว่า NSIS ติดตั้งแล้ว
2. Double-click `create_installer.bat`
3. กด Yes ที่ UAC prompt
4. รอจนเสร็จ
5. ไฟล์ installer จะอยู่ที่ `build/ElegooSlicer_Windows_Installer_V1.1.8.0.exe`
