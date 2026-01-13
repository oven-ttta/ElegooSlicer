# Thai Font Support Fix for ElegooSlicer

## Summary
Fixed the issue where Thai fonts were not appearing in the font selection dropdown in the Text/Emboss feature.

## Changes Made

### 1. Modified `src/slic3r/GUI/Gizmos/GLGizmoText.cpp`

#### Change 1: Font Encoding (Line 50)
Changed font encoding from system encoding to Unicode to support all language fonts:

```cpp
// Before
static const wxFontEncoding font_encoding = wxFontEncoding::wxFONTENCODING_SYSTEM;

// After
static const wxFontEncoding font_encoding = wxFontEncoding::wxFONTENCODING_UNICODE;
```

#### Change 2: Added Bundled Fonts to Font List (Lines 148-160)
Added code to include bundled fonts from the OCCT fonts map into the font selection dropdown:

```cpp
// Add bundled fonts from OCCT fonts map (includes Thai and other fonts from resources)
std::map<std::string, std::string> occt_fonts = get_occt_fonts_maps();
for (const auto& font_pair : occt_fonts) {
    // Check if font is not already in the list
    if (std::find(valid_font_names.begin(), valid_font_names.end(), font_pair.first) == valid_font_names.end()) {
        valid_font_names.push_back(font_pair.first);
    }
}

// Sort the final list
std::sort(valid_font_names.begin(), valid_font_names.end());
```

### 2. Modified `src/libslic3r/Shape/TextShape.cpp`

#### Added Bundled Font Registration (Lines 61-83)
Added registration of bundled fonts from resources directory for all platforms:

```cpp
// Add bundled fonts from resources directory for all platforms
std::string fonts_dir = Slic3r::resources_dir() + "/fonts/";

// HarmonyOS Sans SC (Chinese)
g_occt_fonts_maps.insert(std::make_pair("HarmonyOS Sans SC", fonts_dir + "HarmonyOS_Sans_SC_Regular.ttf"));

// Thai fonts
g_occt_fonts_maps.insert(std::make_pair("Noto Sans Thai", fonts_dir + "NotoSansThai-Regular.ttf"));
g_occt_fonts_maps.insert(std::make_pair("Noto Sans Thai Bold", fonts_dir + "NotoSansThai-Bold.ttf"));
g_occt_fonts_maps.insert(std::make_pair("Noto Sans Thai Light", fonts_dir + "NotoSansThai-Light.ttf"));
g_occt_fonts_maps.insert(std::make_pair("Noto Sans Thai Medium", fonts_dir + "NotoSansThai-Medium.ttf"));
g_occt_fonts_maps.insert(std::make_pair("TH Sarabun New", fonts_dir + "THSarabunNew.ttf"));
g_occt_fonts_maps.insert(std::make_pair("TH Sarabun New Bold", fonts_dir + "THSarabunNew-Bold.ttf"));
g_occt_fonts_maps.insert(std::make_pair("Sarabun", fonts_dir + "Sarabun-Regular.ttf"));
g_occt_fonts_maps.insert(std::make_pair("Kanit", fonts_dir + "Kanit-Regular.ttf"));
g_occt_fonts_maps.insert(std::make_pair("Prompt", fonts_dir + "Prompt-Regular.ttf"));
g_occt_fonts_maps.insert(std::make_pair("Mitr", fonts_dir + "Mitr-Regular.ttf"));

// Korean fonts
g_occt_fonts_maps.insert(std::make_pair("Noto Sans KR", fonts_dir + "NotoSansKR-Regular.ttf"));
g_occt_fonts_maps.insert(std::make_pair("Nanum Gothic", fonts_dir + "NanumGothic-Regular.ttf"));
```

### 3. Added Thai Font Files to `resources/fonts/`

Downloaded and added the following Thai font files:
- `NotoSansThai-Regular.ttf` (and variants: Bold, Light, Medium, etc.)
- `THSarabunNew.ttf`, `THSarabunNew-Bold.ttf`
- `Sarabun-Regular.ttf`, `Sarabun-Bold.ttf`
- `Kanit-Regular.ttf`, `Kanit-Bold.ttf`
- `Prompt-Regular.ttf`, `Prompt-Bold.ttf`
- `Mitr-Regular.ttf`, `Mitr-Bold.ttf`

## How It Works

1. **Font Enumeration**: The `init_face_names()` function now uses Unicode encoding to enumerate all system fonts, regardless of system locale.

2. **Bundled Fonts**: The function also adds fonts from the OCCT fonts map (`get_occt_fonts_maps()`), which includes our bundled Thai fonts from the resources directory.

3. **Font Loading**: When a user selects a font, the `load_text_shape()` function uses OCCT's font system to load the font file and generate 3D text geometry.

## Testing

After rebuilding, the following fonts should appear in the Text tool dropdown:
- Noto Sans Thai (Regular, Bold, Light, Medium)
- TH Sarabun New (Regular, Bold)
- Sarabun
- Kanit
- Prompt
- Mitr
- Plus all system-installed fonts

## Date
2026-01-13
