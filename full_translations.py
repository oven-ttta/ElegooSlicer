#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Complete Thai translations for ElegooSlicer"""

import re

translations = {
    # Basic UI
    "Alt+": "Alt+",
    "Remap": "กำหนดใหม่",
    "Fixed step drag": "ลากแบบขั้นตอนคงที่",
    "Single sided scaling": "ปรับขนาดด้านเดียว",
    "Reset current rotation to the value when open the rotation tool.": "รีเซ็ตการหมุนปัจจุบันเป็นค่าเมื่อเปิดเครื่องมือหมุน",
    "Cancel a feature until exit": "ยกเลิกฟีเจอร์จนกว่าจะออก",
    "Please confirm explosion ratio = 1, and please select at least one object.": "กรุณายืนยันอัตราส่วนระเบิด = 1 และเลือกวัตถุอย่างน้อยหนึ่งชิ้น",
    " (Moving)": " (กำลังเคลื่อนที่)",
    "Face": "หน้า",
    " (Fixed)": " (คงที่)",
    "Warning: please select Plane's feature.": "คำเตือน: กรุณาเลือกฟีเจอร์ระนาบ",
    "Warning: please select Point's or Circle's feature.": "คำเตือน: กรุณาเลือกฟีเจอร์จุดหรือวงกลม",
    "Warning: please select two different meshes.": "คำเตือน: กรุณาเลือกเมชที่แตกต่างกันสองชิ้น",
    "Parallel": "ขนาน",
    "Flip by Face 2": "พลิกตามหน้า 2",
    "Remove paint-on fuzzy skin": "ลบผิวฟัซซี่แบบทาสี",
    "Pick": "เลือก",
    "number keys": "ปุ่มตัวเลข",
    "number keys can quickly change the color of objects": "ปุ่มตัวเลขสามารถเปลี่ยนสีวัตถุได้อย่างรวดเร็ว",
    "Tab": "แท็บ",
    "INFO:": "ข้อมูล:",
    "No Storage": "ไม่มีที่เก็บข้อมูล",
    "How to use LAN only mode": "วิธีใช้โหมด LAN เท่านั้น",

    # Settings
    "Cornering": "การเลี้ยว",
    "Input Shaping Frequency": "ความถี่การปรับรูปอินพุต",
    "Input Shaping Damping/zeta factor": "ค่าหน่วงการปรับรูปอินพุต/ปัจจัยซีตา",
    "Input Shaping": "การปรับรูปอินพุต",
    "Ask When Relevant": "ถามเมื่อเกี่ยวข้อง",
    "Always Ask": "ถามเสมอ",
    "Load Behaviour": "พฤติกรรมการโหลด",
    "Cool Plate (SuperTack)": "แผ่นเย็น (SuperTack)",
    "Dependencies": "การพึ่งพา",
    "Resonance Avoidance Speed": "ความเร็วหลีกเลี่ยงการสั่นพ้อง",
    "Z-Hop": "ยก Z",
    "Air Pump": "ปั๊มลม",
    "Laser 10 W": "เลเซอร์ 10 W",
    "Laser 40 W": "เลเซอร์ 40 W",
    "Overhangs and external bridges fan speed": "ความเร็วพัดลมสำหรับส่วนยื่นและสะพานภายนอก",
    "Zig Zag": "ซิกแซก",
    "Locked Zag": "แซกล็อค",
    "TPMS-D": "TPMS-D",
    "TPMS-FK": "TPMS-FK",

    # Presets
    "This is a default preset.": "นี่คือค่าพรีเซ็ตเริ่มต้น",
    "Current preset is inherited from": "ค่าพรีเซ็ตปัจจุบันสืบทอดจาก",
    "It can't be deleted or modified.": "ไม่สามารถลบหรือแก้ไขได้",
    "Any modifications should be saved as a new preset inherited from this one.": "การแก้ไขใดๆ ควรบันทึกเป็นค่าพรีเซ็ตใหม่ที่สืบทอดจากค่านี้",
    "To do that please specify a new name for the preset.": "โปรดระบุชื่อใหม่สำหรับค่าพรีเซ็ต",
    "Modifications to the current profile will be saved.": "การแก้ไขโปรไฟล์ปัจจุบันจะถูกบันทึก",
    "A copy of the current system preset will be created, which will be detached from the system preset.": "จะสร้างสำเนาของค่าพรีเซ็ตระบบปัจจุบัน ซึ่งจะแยกออกจากค่าพรีเซ็ตระบบ",
    "The current custom preset will be detached from the parent system preset.": "ค่าพรีเซ็ตกำหนดเองปัจจุบันจะถูกแยกออกจากค่าพรีเซ็ตระบบหลัก",

    # Mouse/Camera
    "Swap pan and rotate mouse buttons": "สลับปุ่มเมาส์แพนและหมุน",
    "If enabled, swaps the left and right mouse buttons pan and rotate functions.": "หากเปิดใช้งาน จะสลับฟังก์ชันแพนและหมุนของปุ่มเมาส์ซ้ายและขวา",
    "Multiplies the orbit speed for finer or coarser camera movement.": "คูณความเร็วโคจรเพื่อการเคลื่อนที่กล้องที่ละเอียดหรือหยาบขึ้น",
    "Automatically switch between orthographic and perspective when changing from top/bottom/side views.": "สลับอัตโนมัติระหว่างมุมมองออร์โธกราฟิกและเปอร์สเปกทีฟเมื่อเปลี่ยนจากมุมมองบน/ล่าง/ด้านข้าง",

    # File Import
    "Show the step mesh parameter setting dialog.": "แสดงกล่องโต้ตอบการตั้งค่าพารามิเตอร์เมช step",
    "If enabled,a parameter settings dialog will appear during STEP file import.": "หากเปิดใช้งาน กล่องโต้ตอบการตั้งค่าพารามิเตอร์จะปรากฏขึ้นระหว่างการนำเข้าไฟล์ STEP",
    "Should printer/filament/process settings be loaded when opening a .3mf?": "ควรโหลดการตั้งค่าเครื่องพิมพ์/เส้นพลาสติก/กระบวนการเมื่อเปิดไฟล์ .3mf หรือไม่?",
    "Add model files (stl/step) to recent file list.": "เพิ่มไฟล์โมเดล (stl/step) ลงในรายการไฟล์ล่าสุด",
    "The 3MF file you are importing may be incompatible, continuing could cause errors. Would you like to import the entire file or just the model data?": "ไฟล์ 3MF ที่คุณกำลังนำเข้าอาจไม่เข้ากัน การดำเนินการต่ออาจทำให้เกิดข้อผิดพลาด คุณต้องการนำเข้าไฟล์ทั้งหมดหรือเฉพาะข้อมูลโมเดล?",
    "This project was created with an OrcaSlicer 2.3.1-alpha and uses infill rotation template settings that may not work properly with your current infill pattern. This could result in weak support or print quality issues.": "โปรเจกต์นี้สร้างด้วย OrcaSlicer 2.3.1-alpha และใช้การตั้งค่าเทมเพลตการหมุนอินฟิลที่อาจทำงานไม่ถูกต้องกับรูปแบบอินฟิลปัจจุบันของคุณ อาจส่งผลให้การรองรับอ่อนแอหรือมีปัญหาคุณภาพการพิมพ์",
    "Would you like OrcaSlicer to automatically fix this by clearing the rotation template settings?": "คุณต้องการให้ OrcaSlicer แก้ไขปัญหานี้โดยอัตโนมัติด้วยการล้างการตั้งค่าเทมเพลตการหมุนหรือไม่?",

    # Calibration
    "No accelerations provided for calibration. Use default acceleration value ": "ไม่มีค่าความเร่งสำหรับการปรับเทียบ ใช้ค่าความเร่งเริ่มต้น ",
    "No speeds provided for calibration. Use default optimal speed ": "ไม่มีค่าความเร็วสำหรับการปรับเทียบ ใช้ค่าความเร็วที่เหมาะสมเริ่มต้น ",

    # Plugin errors
    "Failed to download the plug-in. Please check your firewall settings and vpn software, check and retry.": "ดาวน์โหลดปลั๊กอินไม่สำเร็จ กรุณาตรวจสอบการตั้งค่าไฟร์วอลล์และซอฟต์แวร์ VPN แล้วลองใหม่",
    "Failed to install the plug-in. Please check whether it is blocked or deleted by anti-virus software.": "ติดตั้งปลั๊กอินไม่สำเร็จ กรุณาตรวจสอบว่าถูกบล็อคหรือลบโดยซอฟต์แวร์ป้องกันไวรัสหรือไม่",

    # Fuzzy Skin
    "Both [Extrusion] and [Combined] modes of Fuzzy Skin require the Arachne Wall Generator to be enabled.": "โหมด [การอัดรีด] และ [รวม] ของผิวฟัซซี่ต้องเปิดใช้งานตัวสร้างผนัง Arachne",

    # Template/Pattern
    "This parameter expects a valid template.": "พารามิเตอร์นี้ต้องการเทมเพลตที่ถูกต้อง",
    "Invalid pattern. Use N, N#K, or a comma-separated list with optional #K per entry. Examples: 5, 5#2, 1,7,9, 5,9#2,18.": "รูปแบบไม่ถูกต้อง ใช้ N, N#K หรือรายการคั่นด้วยจุลภาคพร้อม #K ตัวเลือกต่อรายการ ตัวอย่าง: 5, 5#2, 1,7,9, 5,9#2,18",

    # Server/Network
    "The server is unable to respond. Please click the link below to check the server status.": "เซิร์ฟเวอร์ไม่สามารถตอบสนองได้ กรุณาคลิกลิงก์ด้านล่างเพื่อตรวจสอบสถานะเซิร์ฟเวอร์",
    "If the server is in a fault state, you can temporarily use offline printing or local network printing.": "หากเซิร์ฟเวอร์อยู่ในสถานะผิดพลาด คุณสามารถใช้การพิมพ์ออฟไลน์หรือการพิมพ์ผ่านเครือข่ายท้องถิ่นชั่วคราวได้",

    # Warnings
    "Enabling both precise Z height and the prime tower may cause the size of prime tower to increase. Do you still want to enable?": "การเปิดใช้งานทั้งความสูง Z ที่แม่นยำและหอไพรม์อาจทำให้ขนาดหอไพรม์เพิ่มขึ้น คุณยังต้องการเปิดใช้งานหรือไม่?",
    "One or more object were assigned an extruder that the printer does not have.": "วัตถุหนึ่งหรือมากกว่าถูกกำหนดหัวฉีดที่เครื่องพิมพ์ไม่มี",
    "The precise wall option will be ignored for outer-inner or inner-outer-inner wall sequences.": "ตัวเลือกผนังแม่นยำจะถูกละเว้นสำหรับลำดับผนังนอก-ใน หรือ ใน-นอก-ใน",

    # Descriptions
    "Setting the number of printheads on the OrangeStorm Giga printer quickly configures all related settings.": "การตั้งค่าจำนวนหัวพิมพ์บนเครื่องพิมพ์ OrangeStorm Giga จะกำหนดค่าที่เกี่ยวข้องทั้งหมดอย่างรวดเร็ว",
    "Default bed type for the printer (supports both numeric and string format).": "ประเภทฐานพิมพ์เริ่มต้นสำหรับเครื่องพิมพ์ (รองรับทั้งรูปแบบตัวเลขและสตริง)",
    "Enable this option to allow adjustment of the part cooling fan speed for specifically for overhangs, internal and external bridges. Setting the fan speed specifically for these features can improve overall print quality and reduce warping.": "เปิดใช้งานตัวเลือกนี้เพื่ออนุญาตการปรับความเร็วพัดลมทำความเย็นชิ้นงานสำหรับส่วนยื่น สะพานภายในและภายนอกโดยเฉพาะ การตั้งความเร็วพัดลมสำหรับฟีเจอร์เหล่านี้สามารถปรับปรุงคุณภาพการพิมพ์โดยรวมและลดการบิดงอ",
    "A boolean expression using the configuration values of an active printer profile. If this expression evaluates to true, this profile is considered compatible with the active printer profile.": "นิพจน์บูลีนที่ใช้ค่าการกำหนดค่าของโปรไฟล์เครื่องพิมพ์ที่ใช้งานอยู่ หากนิพจน์นี้ประเมินเป็นจริง โปรไฟล์นี้จะถือว่าเข้ากันได้กับโปรไฟล์เครื่องพิมพ์ที่ใช้งานอยู่",
    "A boolean expression using the configuration values of an active print profile. If this expression evaluates to true, this profile is considered compatible with the active print profile.": "นิพจน์บูลีนที่ใช้ค่าการกำหนดค่าของโปรไฟล์การพิมพ์ที่ใช้งานอยู่ หากนิพจน์นี้ประเมินเป็นจริง โปรไฟล์นี้จะถือว่าเข้ากันได้กับโปรไฟล์การพิมพ์ที่ใช้งานอยู่",
    "Aligns infill and surface fill directions to follow the model's orientation on the build plate. When enabled, fill directions rotate with the model to maintain optimal strength characteristics.": "จัดตำแหน่งทิศทางอินฟิลและการเติมพื้นผิวให้ตามทิศทางของโมเดลบนแผ่นสร้าง เมื่อเปิดใช้งาน ทิศทางการเติมจะหมุนตามโมเดลเพื่อรักษาคุณสมบัติความแข็งแรงที่เหมาะสม",
    "Using multiple lines for the infill pattern, if supported by infill pattern.": "ใช้หลายเส้นสำหรับรูปแบบอินฟิล หากรูปแบบอินฟิลรองรับ",

    # Infill rotation
    "Insert solid infill at specific layers. Use N to insert every Nth layer, N#K to insert K consecutive solid layers every N layers (K is optional, e.g. '5#' equals '5#1'), or a comma-separated list (e.g. 1,7,9) to insert at explicit layers. Layers are 1-based.": "แทรกอินฟิลแข็งที่ชั้นเฉพาะ ใช้ N เพื่อแทรกทุกชั้นที่ N, N#K เพื่อแทรกชั้นอินฟิลแข็งติดต่อกัน K ชั้นทุก N ชั้น (K เป็นตัวเลือก เช่น '5#' เท่ากับ '5#1') หรือรายการคั่นด้วยจุลภาค (เช่น 1,7,9) เพื่อแทรกที่ชั้นที่ระบุ ชั้นเริ่มต้นที่ 1",

    # Format string
    "Resources path does not exist or is not a directory: %s": "เส้นทางทรัพยากรไม่มีอยู่หรือไม่ใช่ไดเรกทอรี: %s",
    "Access code:%s IP address:%s": "รหัสเข้าถึง:%s ที่อยู่ IP:%s",
    "For constant flow rate, hold %1% while dragging.": "สำหรับอัตราการไหลคงที่ กด %1% ค้างขณะลาก",

    # Extruder warnings
    "Printing with multiple extruders of differing nozzle diameters. If support is to be printed with the current filament (support_filament == 0 or support_interface_filament == 0), all nozzles have to be of the same diameter.": "การพิมพ์ด้วยหัวฉีดหลายหัวที่มีเส้นผ่านศูนย์กลางต่างกัน หากต้องพิมพ์ฐานรองรับด้วยเส้นพลาสติกปัจจุบัน (support_filament == 0 หรือ support_interface_filament == 0) หัวฉีดทั้งหมดต้องมีเส้นผ่านศูนย์กลางเท่ากัน",

    # Lattice angles
    "The angle of the first set of Lateral lattice elements in the Z direction. Zero is vertical.": "มุมของชุดองค์ประกอบโครงตาข่ายด้านข้างชุดแรกในทิศทาง Z ศูนย์คือแนวตั้ง",
    "The angle of the second set of Lateral lattice elements in the Z direction. Zero is vertical.": "มุมของชุดองค์ประกอบโครงตาข่ายด้านข้างชุดที่สองในทิศทาง Z ศูนย์คือแนวตั้ง",
    "The angle of the infill angled lines. 60° will result in a pure honeycomb.": "มุมของเส้นอินฟิลเอียง 60° จะได้รังผึ้งบริสุทธิ์",

    # Junction Deviation
    "Marlin Firmware Junction Deviation (replaces the traditional XY Jerk setting).": "การเบี่ยงเบนจุดเชื่อมต่อเฟิร์มแวร์ Marlin (แทนที่การตั้งค่า XY Jerk แบบดั้งเดิม)",

    # Fan speed descriptions
    "The part cooling fan speed used for all internal bridges. Set to -1 to use the overhang fan speed settings instead.\\n\\nReducing the internal bridges fan speed, compared to your regular fan speed, can help reduce part warping due to excessive cooling applied over a large surface for a prolonged period of time.": "ความเร็วพัดลมทำความเย็นชิ้นงานที่ใช้สำหรับสะพานภายในทั้งหมด ตั้งเป็น -1 เพื่อใช้การตั้งค่าความเร็วพัดลมส่วนยื่นแทน\\n\\nการลดความเร็วพัดลมสะพานภายในเมื่อเทียบกับความเร็วพัดลมปกติ สามารถช่วยลดการบิดงอของชิ้นงานเนื่องจากการทำความเย็นมากเกินไปบนพื้นผิวขนาดใหญ่เป็นเวลานาน",
    "This part cooling fan speed is applied when ironing. Setting this parameter to a lower than regular speed reduces possible nozzle clogging due to the low volumetric flow rate, making the interface smoother.\\nSet to -1 to disable it.": "ความเร็วพัดลมทำความเย็นชิ้นงานนี้ใช้เมื่อรีดผิว การตั้งพารามิเตอร์นี้ต่ำกว่าความเร็วปกติช่วยลดการอุดตันหัวฉีดที่อาจเกิดขึ้นเนื่องจากอัตราการไหลเชิงปริมาตรต่ำ ทำให้พื้นผิวเรียบขึ้น\\nตั้งเป็น -1 เพื่อปิดใช้งาน",

    # Infill rotation warning
    "Infill patterns are typically designed to handle rotation automatically to ensure proper printing and achieve their intended effects (e.g., Gyroid, Cubic). Rotating the current sparse infill pattern may lead to insufficient support. Please proceed with caution and thoroughly check for any potential printing issues.Are you sure you want to enable this option?": "รูปแบบอินฟิลโดยทั่วไปได้รับการออกแบบให้จัดการการหมุนโดยอัตโนมัติเพื่อให้แน่ใจว่าการพิมพ์ถูกต้องและบรรลุผลตามที่ต้องการ (เช่น Gyroid, Cubic) การหมุนรูปแบบอินฟิลกระจายปัจจุบันอาจทำให้การรองรับไม่เพียงพอ กรุณาดำเนินการด้วยความระมัดระวังและตรวจสอบปัญหาการพิมพ์ที่อาจเกิดขึ้นอย่างละเอียด คุณแน่ใจหรือไม่ว่าต้องการเปิดใช้งานตัวเลือกนี้?",

    # Lock depth warning
    "Lock depth should smaller than skin depth.\\nReset to 50% of skin depth.": "ความลึกล็อคควรน้อยกว่าความลึกผิว\\nรีเซ็ตเป็น 50% ของความลึกผิว",

    # Fuzzy skin dialog
    "Change these settings automatically?\\nYes - Enable Arachne Wall Generator\\nNo  - Disable Arachne Wall Generator and set [Displacement] mode of the Fuzzy Skin": "เปลี่ยนการตั้งค่าเหล่านี้โดยอัตโนมัติหรือไม่?\\nใช่ - เปิดใช้งานตัวสร้างผนัง Arachne\\nไม่ - ปิดใช้งานตัวสร้างผนัง Arachne และตั้งโหมด [การเลื่อน] ของผิวฟัซซี่",

    # Action confirmation
    "This action is not revertible.\\nDo you want to proceed?": "การดำเนินการนี้ไม่สามารถย้อนกลับได้\\nคุณต้องการดำเนินการต่อหรือไม่?",
}

def translate_po_file(po_path):
    """Read PO file and apply translations"""
    with open(po_path, 'r', encoding='utf-8') as f:
        content = f.read()

    count = 0
    for eng, thai in translations.items():
        # Handle multiline messages
        eng_escaped = eng.replace('\\n', '\n')
        eng_escaped = re.escape(eng_escaped)
        eng_escaped = eng_escaped.replace(r'\ ', ' ')

        # Try to find and replace empty msgstr
        # Pattern for single line
        pattern = rf'(msgid "{re.escape(eng)}"\nmsgstr )""\n'
        if re.search(pattern, content):
            replacement = rf'\1"{thai}"\n'
            content, n = re.subn(pattern, replacement, content, count=1)
            if n > 0:
                count += n
                print(f"Translated: {eng[:60]}...")

    with open(po_path, 'w', encoding='utf-8') as f:
        f.write(content)

    return count

if __name__ == "__main__":
    po_file = r"D:\ElegooSlicer\localization\i18n\th\ElegooSlicer_th.po"
    translated = translate_po_file(po_file)
    print(f"\nTotal translations applied: {translated}")
