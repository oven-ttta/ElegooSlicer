
<p align="center">
    <a href="https://discord.com/invite/NkFzP96cMt">
        <img alt="Static Badge" src="https://img.shields.io/badge/Chat%20on%20Discord-%23FFF?style=flat&logo=discord&logoColor=white&color=%235563e9">
    </a>
    <a href="https://discord.com/channels/969282195552346202/1370832511042850987">
        <img alt="Static Badge" src="https://img.shields.io/badge/BETA%20channel%20for%20FDM%20slicer-%23FFF?style=flat&logo=discord&logoColor=white&color=%23FF6000">
    </a>
    <a href="https://github.com/ELEGOO-3D/ElegooSlicer/issues">
        <img alt="GitHub Issues or Pull Requests by label" src="https://img.shields.io/github/issues/ELEGOO-3D/ElegooSlicer/bug">
    </a>
</p>

# เกี่ยวกับ ElegooSlicer

ElegooSlicer เป็นซอฟต์แวร์ Slicer แบบ open-source ที่รองรับเครื่องพิมพ์ 3D แบบ FDM ส่วนใหญ่ ปัจจุบัน ElegooSlicer อยู่ในช่วงพัฒนาอย่างรวดเร็ว โดยจะมีฟีเจอร์ใหม่ๆ เพิ่มเติมเร็วๆ นี้ โปรดติดตามการอัปเดต เราขอเชิญคุณ[เข้าร่วม Discord ของเรา](https://discord.com/invite/NkFzP96cMt) และติดตามประกาศเพื่อเข้าร่วม [ช่อง BETA สำหรับ FDM slicer](https://discord.com/channels/969282195552346202/1370832511042850987) เพื่อติดตามข่าวสารล่าสุดเกี่ยวกับผลิตภัณฑ์ ELEGOO และ FDM slicer


# วิธีการติดตั้ง
**Windows**:
1.  ดาวน์โหลดตัวติดตั้งสำหรับเวอร์ชันที่คุณต้องการจาก[หน้า releases](https://github.com/ELEGOO-3D/ElegooSlicer/releases)
    - *เพื่อความสะดวก ยังมี portable build ให้ใช้งานด้วย*
    - *หากคุณมีปัญหาในการรัน build คุณอาจต้องติดตั้ง runtime ต่อไปนี้:*
      - [MicrosoftEdgeWebView2RuntimeInstallerX64](https://github.com/SoftFever/OrcaSlicer/releases/download/v1.0.10-sf2/MicrosoftEdgeWebView2RuntimeInstallerX64.exe)
          - [รายละเอียดของ runtime นี้](https://aka.ms/webview2)
          - [ลิงก์ดาวน์โหลดทางเลือกจาก Microsoft](https://go.microsoft.com/fwlink/p/?LinkId=2124703)
      - [vcredist2019_x64](https://github.com/SoftFever/OrcaSlicer/releases/download/v1.0.10-sf2/vcredist2019_x64.exe)
          -  [ลิงก์ดาวน์โหลดทางเลือกจาก Microsoft](https://aka.ms/vs/17/release/vc_redist.x64.exe)
          -  ไฟล์นี้อาจมีอยู่ในคอมพิวเตอร์ของคุณแล้วหากคุณติดตั้ง Visual Studio ตรวจสอบที่ตำแหน่ง: `%VCINSTALLDIR%Redist\MSVC\v142`

**Mac**:
1. ดาวน์โหลด DMG สำหรับคอมพิวเตอร์ของคุณ: เวอร์ชัน `arm64` สำหรับ Apple Silicon และ `x86_64` สำหรับ Intel CPU
2. ลาก OrcaSlicer.app ไปยังโฟลเดอร์ Application
3. *หากคุณต้องการรัน build จาก PR คุณต้องทำตามคำแนะนำด้านล่างด้วย:*
    <details quarantine>
    <summary>รายละเอียด</summary>

    - ตัวเลือกที่ 1 (คุณต้องทำเพียงครั้งเดียว หลังจากนั้นแอปจะเปิดได้ตามปกติ):
      - ขั้นตอนที่ 1: กด _cmd_ ค้างไว้แล้วคลิกขวาที่แอป จากเมนูบริบทเลือก **Open**
      - ขั้นตอนที่ 2: หน้าต่างเตือนจะปรากฏขึ้น คลิก _Open_

    - ตัวเลือกที่ 2:
      รันคำสั่งนี้ใน terminal: `xattr -dr com.apple.quarantine /Applications/ElegooSlicer.app`
      ```console
          softfever@mac:~$ xattr -dr com.apple.quarantine /Applications/ElegooSlicer.app
      ```
    - ตัวเลือกที่ 3:
        - ขั้นตอนที่ 1: เปิดแอป หน้าต่างเตือนจะปรากฏขึ้น
            ![image](./SoftFever_doc/mac_cant_open.png)
        - ขั้นตอนที่ 2: ใน `System Settings` -> `Privacy & Security` คลิก `Open Anyway`:
            ![image](./SoftFever_doc/mac_security_setting.png)
    </details>

# วิธีการ Compile
- Windows 64-bit
  - เครื่องมือที่ต้องการ: Visual Studio 2019, Cmake, git, git-lfs, Strawberry Perl
      - คุณต้องการ cmake เวอร์ชัน 3.14 หรือใหม่กว่า ซึ่งสามารถดาวน์โหลดได้[จากเว็บไซต์](https://cmake.org/download/)
      - Strawberry Perl [มีให้ที่ GitHub repository](https://github.com/StrawberryPerl/Perl-Dist-Strawberry/releases/)
  - รัน `build_release.bat` ใน `x64 Native Tools Command Prompt for VS 2019`
  - หมายเหตุ: อย่าลืมรัน `git lfs pull` หลังจาก clone repository เพื่อดาวน์โหลดเครื่องมือบน Windows

- Mac 64-bit
  - เครื่องมือที่ต้องการ: Xcode, Cmake, git, gettext, libtool, automake, autoconf, texinfo
      - คุณสามารถติดตั้งส่วนใหญ่ได้โดยรัน `brew install cmake gettext libtool automake autoconf texinfo`
  - รัน `build_release_macos.sh`
  - การ build และ debug ใน Xcode:
      - รัน `Xcode.app`
      - เปิด ``build_`arch`/OrcaSlicer.Xcodeproj``
      - แถบเมนู: Product => Scheme => OrcaSlicer
      - แถบเมนู: Product => Scheme => Edit Scheme...
          - Run => แท็บ Info => Build Configuration: `RelWithDebInfo`
          - Run => แท็บ Options => Document Versions: ยกเลิกการเลือก `Allow debugging when browsing versions`
      - แถบเมนู: Product => Run


# วิธีการรายงานปัญหา

เราแนะนำให้เข้าร่วมช่อง Discord BETA ของเราเพื่อให้ feedback แบบ real-time และร่วมอภิปราย คุณยังสามารถรายงานปัญหาและติดตามความคืบหน้าได้ที่ GitHub Issues

<a href="https://github.com/ELEGOO-3D/ElegooSlicer/issues">
    <img alt="GitHub Issues or Pull Requests by label" src="https://img.shields.io/github/issues/ELEGOO-3D/ElegooSlicer/bug">
</a>

# สัญญาอนุญาต
ElegooSlicer อยู่ภายใต้สัญญาอนุญาต GNU Affero General Public License เวอร์ชัน 3 ElegooSlicer พัฒนาต่อยอดจาก Orca Slicer โดย SoftFever

Orca Slicer อยู่ภายใต้สัญญาอนุญาต GNU Affero General Public License เวอร์ชัน 3 Orca Slicer พัฒนาต่อยอดจาก Bambu Studio โดย BambuLab

Bambu Studio อยู่ภายใต้สัญญาอนุญาต GNU Affero General Public License เวอร์ชัน 3 Bambu Studio พัฒนาต่อยอดจาก PrusaSlicer โดย PrusaResearch

PrusaSlicer อยู่ภายใต้สัญญาอนุญาต GNU Affero General Public License เวอร์ชัน 3 PrusaSlicer เป็นของ Prusa Research PrusaSlicer พัฒนาต่อยอดจาก Slic3r โดย Alessandro Ranellucci

Slic3r อยู่ภายใต้สัญญาอนุญาต GNU Affero General Public License เวอร์ชัน 3 Slic3r สร้างโดย Alessandro Ranellucci ด้วยความช่วยเหลือจากผู้มีส่วนร่วมอื่นๆ อีกมากมาย

สัญญาอนุญาต GNU Affero General Public License เวอร์ชัน 3 รับประกันว่าหากคุณใช้ส่วนใดส่วนหนึ่งของซอฟต์แวร์นี้ในทางใดก็ตาม (แม้แต่เบื้องหลังเว็บเซิร์ฟเวอร์) ซอฟต์แวร์ของคุณจะต้องเผยแพร่ภายใต้สัญญาอนุญาตเดียวกัน

Orca Slicer รวมถึงการทดสอบ pressure advance calibration pattern ที่ดัดแปลงมาจาก generator ของ Andrew Ellis ซึ่งอยู่ภายใต้สัญญาอนุญาต GNU General Public License เวอร์ชัน 3 generator ของ Ellis เองก็ดัดแปลงมาจาก generator ที่พัฒนาโดย Sineos สำหรับ Marlin ซึ่งอยู่ภายใต้สัญญาอนุญาต GNU General Public License เวอร์ชัน 3

ปลั๊กอิน Bambu networking อิงจากไลบรารีที่ไม่ฟรีจาก BambuLab เป็นส่วนเสริมของ Orca Slicer และให้ฟังก์ชันการทำงานขยายสำหรับผู้ใช้เครื่องพิมพ์ Bambulab

อยู่ภายใต้สัญญาอนุญาต [AGPL-3.0](LICENSE.txt)
