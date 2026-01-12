# Thai Language Support Update

## Overview
This update adds comprehensive Thai language support to ElegooSlicer, including font rendering fixes, UI translations, and web localization.

## Changes Made

### 1. ImGui Font Support (`src/slic3r/GUI/ImGuiWrapper.cpp`)
- Added Thai font merge mode for ImGui rendering
- Uses HarmonyOS Sans SC as base font and merges NotoSansThai glyphs on top
- This allows both Latin and Thai characters to display correctly in ImGui UI elements

**Code changes:**
- Added `need_thai_merge` flag when Thai glyph ranges are detected
- Merge `NotoSansThai-Regular.ttf` for regular text
- Merge `NotoSansThai-Bold.ttf` for bold text

### 2. wxWidgets Font Support (`src/slic3r/GUI/Widgets/Label.cpp`)
- Added Thai language detection using `wxLocale::GetSystemLanguage()`
- When Thai is detected, uses "Noto Sans Thai" font face
- Added private font loading for NotoSansThai-Regular.ttf and NotoSansThai-Bold.ttf

**Code changes:**
```cpp
else if (wxLocale::GetSystemLanguage() == wxLANGUAGE_THAI) {
    face = "Noto Sans Thai";
}
```

### 3. Thai Font Files (`resources/fonts/`)
Added the following font files:
- `NotoSansThai-Regular.ttf`
- `NotoSansThai-Bold.ttf`

### 4. Web CSS Thai Font Support
Updated the following CSS files to include Thai fonts in font-family:

**`resources/web/guide/css/common.css`:**
```css
font-family: "system-ui", "Segoe UI", Roboto, Oxygen, Ubuntu, "Fira Sans",
  "Droid Sans", "Helvetica Neue", "Noto Sans Thai", "Sarabun", sans-serif;
```

**`resources/web/homepage4/css/light.css`:**
```css
font-family: "system-ui", "Segoe UI", Roboto, Oxygen, Ubuntu, "Fira Sans",
  "Droid Sans", "Helvetica Neue", "Noto Sans Thai", "Sarabun", sans-serif;
```

**`resources/web/styles/light.css`:**
```css
font-family: "Source Han Sans", "Microsoft YaHei", "Noto Sans Thai", "Sarabun", Arial, sans-serif;
```

### 5. Web Localization (`resources/web/locales/index.js`)
Added complete Thai (`th:`) translation section including:

- **Homepage translations:**
  - title, login, register, relogin, logout
  - recent, newProject, createNewProject, openProject
  - recentOpen, clearAll, clear, openInExplorer
  - onlineModels, myProjects, allCategories, noRecentFiles, remove

- **Printer Manager translations:**
  - All printer status messages (offline, idle, printing, paused, etc.)
  - Connection and error messages
  - Settings labels

- **Add Printer Dialog translations:**
  - All form labels and buttons
  - Validation messages
  - Help text

- **Manual Form translations:**
  - Printer model, name, host type fields
  - Connection method options
  - Validation messages

- **Printer Access Auth translations:**
  - Access code and PIN code dialogs
  - Help messages

- **Printer Settings translations:**
  - All device settings labels
  - Confirmation messages

- **Filament Sync translations:**
  - Sync status messages
  - Warning tips

- **Print Send translations:**
  - Print options
  - Filament mapping
  - Warning messages

### 6. Network Helper Language Support (`src/slic3r/Utils/Elegoo/PrinterNetwork.cpp`)
Added Thai language code mapping in `getLanguage()` function:

```cpp
} else if (language == "TH") {
    language = "th";
}
```

This ensures that when Thai is selected, the Online Models page receives `?language=th` parameter.

## Files Modified

| File | Changes |
|------|---------|
| `src/slic3r/GUI/ImGuiWrapper.cpp` | Thai font merge mode |
| `src/slic3r/GUI/Widgets/Label.cpp` | Thai font detection and loading |
| `resources/fonts/NotoSansThai-Regular.ttf` | Added |
| `resources/fonts/NotoSansThai-Bold.ttf` | Added |
| `resources/web/guide/css/common.css` | Thai font in CSS |
| `resources/web/homepage4/css/light.css` | Thai font in CSS |
| `resources/web/styles/light.css` | Thai font in CSS |
| `resources/web/locales/index.js` | Complete Thai translations |
| `src/slic3r/Utils/Elegoo/PrinterNetwork.cpp` | Thai language code |
| `localization/i18n/th/ElegooSlicer_th.po` | PO translations |

## Notes

### Text Emboss Feature
The Text Emboss feature uses system fonts for rendering. To emboss Thai text:
1. Select a Thai-supporting font from the dropdown (e.g., "Noto Sans Thai")
2. Fonts that don't have Thai glyphs will show "????" instead of Thai characters

### Online Models Page
The Online Models page is loaded from Elegoo's external server. The application now sends `?language=th` parameter, but whether Thai is displayed depends on Elegoo's server supporting Thai translations.

## Testing
1. Set language to Thai in Preferences
2. Restart the application
3. Verify Thai text displays correctly in:
   - Main menu and toolbar
   - Settings dialogs
   - Homepage (Recent files, navigation)
   - Printer manager dialogs
   - Print send dialog

## Windows Installer Thai Language (v1.3.0.11)

### Changes to `package.nsi`
- Added `Unicode true` directive for proper Thai character support
- Force Thai language instead of auto-detecting system language
- All installer messages now display in Thai

**Before:**
```nsi
; Detect system language
System::Call 'Kernel32::GetUserDefaultUILanguage() i.r0'
${If} $0 == ${LANG_CHINESE_SIMPLIFIED}
    StrCpy $LANGUAGE ${LANG_CHINESE_SIMPLIFIED}
${ElseIf} $0 == ${LANG_THAI}
    StrCpy $LANGUAGE ${LANG_THAI}
${Else}
    StrCpy $LANGUAGE ${LANG_ENGLISH}
${EndIf}
```

**After:**
```nsi
; Force Thai language
StrCpy $LANGUAGE ${LANG_THAI}
```

### Thai Installer Messages
| Message | Thai Translation |
|---------|------------------|
| Do not create shortcuts | ไม่ต้องสร้างทางลัด |
| Uninstall success | ถูกถอนการติดตั้งจากคอมพิวเตอร์ของคุณเรียบร้อยแล้ว |
| Program running | ตัวติดตั้งตรวจพบว่ากำลังทำงานอยู่... |
| Old version detected | พบเวอร์ชัน... คุณต้องการถอนการติดตั้งและดำเนินการติดตั้งต่อหรือไม่? |
| Confirm uninstall | คุณแน่ใจหรือไม่ว่าต้องการถอนการติดตั้งทั้งหมด? |

### Build Installer
```batch
# Update version in build_installer.bat
set PRODUCT_VERSION=1.3.0.11

# Run build
build_installer.bat
```

Output: `build\ElegooSlicer_Windows_Installer_V1.3.0.11.exe`

### Download Page
- URL: https://elegooslicer.ovenx.shop
- Added Prompt Thai font for proper rendering
- Installer download: https://minio.ovenx.shop/elegooslicer/releases/ElegooSlicer_Windows_Installer_V1.3.0.11.exe

## Date
January 2026
