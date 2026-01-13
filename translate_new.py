#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Script to translate untranslated messages in Thai PO file"""

import re

# Translation dictionary for new messages
translations = {
    "Alt+": "Alt+",
    "Remap": "กำหนดใหม่",
    "Fixed step drag": "ลากแบบขั้นตอนคงที่",
    "Single sided scaling": "ปรับขนาดด้านเดียว",
    "Reset current rotation to the value when open the rotation tool.": "รีเซ็ตการหมุนปัจจุบันเป็นค่าเมื่อเปิดเครื่องมือหมุน",
    "Cancel a feature until exit": "ยกเลิกฟีเจอร์จนกว่าจะออก",
    "Please confirm explosion ratio = 1, and please select at least one object.": "กรุณายืนยันอัตราส่วนระเบิด = 1 และเลือกวัตถุอย่างน้อยหนึ่งชิ้น",
    " (Moving)": " (กำลังเคลื่อนที่)",
    "Select 2 faces on objects and \\n make objects assemble together.": "เลือก 2 หน้าบนวัตถุและ\\n ประกอบวัตถุเข้าด้วยกัน",
    "Select 2 points or circles on objects and \\n specify distance between them.": "เลือก 2 จุดหรือวงกลมบนวัตถุและ\\n ระบุระยะห่างระหว่างกัน",
    "Face": "หน้า",
    " (Fixed)": " (คงที่)",
    "Feature 1 has been reset, \\nfeature 2 has been feature 1": "ฟีเจอร์ 1 ถูกรีเซ็ตแล้ว\\nฟีเจอร์ 2 กลายเป็นฟีเจอร์ 1",
    "Warning: please select Plane's feature.": "คำเตือน: กรุณาเลือกฟีเจอร์ระนาบ",
    "Warning: please select Point's or Circle's feature.": "คำเตือน: กรุณาเลือกฟีเจอร์จุดหรือวงกลม",
    "Warning: please select two different meshes.": "คำเตือน: กรุณาเลือกเมชที่แตกต่างกันสองชิ้น",
    "Parallel": "ขนาน",
    "Flip by Face 2": "พลิกตามหน้า 2",
    "Remove paint-on fuzzy skin": "ลบผิวฟัซซี่แบบทาสี",
    "Failed to download the plug-in. Please check your firewall settings and vpn software, check and retry.": "ดาวน์โหลดปลั๊กอินไม่สำเร็จ กรุณาตรวจสอบการตั้งค่าไฟร์วอลล์และซอฟต์แวร์ VPN แล้วลองใหม่",
    "Failed to install the plug-in. Please check whether it is blocked or deleted by anti-virus software.": "ติดตั้งปลั๊กอินไม่สำเร็จ กรุณาตรวจสอบว่าถูกบล็อคหรือลบโดยซอฟต์แวร์ป้องกันไวรัสหรือไม่",
    "Both [Extrusion] and [Combined] modes of Fuzzy Skin require the Arachne Wall Generator to be enabled.": "โหมด [การอัดรีด] และ [รวม] ของผิวฟัซซี่ต้องเปิดใช้งานตัวสร้างผนัง Arachne",
    "This parameter expects a valid template.": "พารามิเตอร์นี้ต้องการเทมเพลตที่ถูกต้อง",
    "Pick": "เลือก",
    "number keys": "ปุ่มตัวเลข",
    "number keys can quickly change the color of objects": "ปุ่มตัวเลขสามารถเปลี่ยนสีวัตถุได้อย่างรวดเร็ว",
    "Automatically switch between orthographic and perspective when changing from top/bottom/side views.": "สลับอัตโนมัติระหว่างมุมมองออร์โธกราฟิกและเปอร์สเปกทีฟเมื่อเปลี่ยนจากมุมมองบน/ล่าง/ด้านข้าง",
    "Cornering": "การเลี้ยว",
    "Input Shaping Frequency": "ความถี่การปรับรูปอินพุต",
    "Input Shaping Damping/zeta factor": "ค่าหน่วงการปรับรูปอินพุต/ปัจจัยซีตา",
    "Input Shaping": "การปรับรูปอินพุต",
    "No Storage": "ไม่มีที่เก็บข้อมูล",
    "How to use LAN only mode": "วิธีใช้โหมด LAN เท่านั้น",
    "Tab": "แท็บ",
    "INFO:": "ข้อมูล:",
    "Swap pan and rotate mouse buttons": "สลับปุ่มเมาส์แพนและหมุน",
    "Ask When Relevant": "ถามเมื่อเกี่ยวข้อง",
    "Always Ask": "ถามเสมอ",
    "Load Behaviour": "พฤติกรรมการโหลด",
    "This is a default preset.": "นี่คือค่าพรีเซ็ตเริ่มต้น",
    "It can't be deleted or modified.": "ไม่สามารถลบหรือแก้ไขได้",
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
    "Current preset is inherited from": "ค่าพรีเซ็ตปัจจุบันสืบทอดจาก",
    "Any modifications should be saved as a new preset inherited from this one.": "การแก้ไขใดๆ ควรบันทึกเป็นค่าพรีเซ็ตใหม่ที่สืบทอดจากค่านี้",
    "To do that please specify a new name for the preset.": "โปรดระบุชื่อใหม่สำหรับค่าพรีเซ็ต",
    "Modifications to the current profile will be saved.": "การแก้ไขโปรไฟล์ปัจจุบันจะถูกบันทึก",
    "A copy of the current system preset will be created, which will be detached from the system preset.": "จะสร้างสำเนาของค่าพรีเซ็ตระบบปัจจุบัน ซึ่งจะแยกออกจากค่าพรีเซ็ตระบบ",
    "The current custom preset will be detached from the parent system preset.": "ค่าพรีเซ็ตกำหนดเองปัจจุบันจะถูกแยกออกจากค่าพรีเซ็ตระบบหลัก",
    "Should printer/filament/process settings be loaded when opening a .3mf?": "ควรโหลดการตั้งค่าเครื่องพิมพ์/เส้นพลาสติก/กระบวนการเมื่อเปิดไฟล์ .3mf หรือไม่?",
    "Add model files (stl/step) to recent file list.": "เพิ่มไฟล์โมเดล (stl/step) ลงในรายการไฟล์ล่าสุด",
    "Show the step mesh parameter setting dialog.": "แสดงกล่องโต้ตอบการตั้งค่าพารามิเตอร์เมช step",
}

def main():
    po_file = r"D:\ElegooSlicer\localization\i18n\th\ElegooSlicer_th.po"

    with open(po_file, 'r', encoding='utf-8') as f:
        content = f.read()

    # Count translations made
    count = 0

    for eng, thai in translations.items():
        # Pattern to find msgid and empty msgstr
        pattern = rf'(msgid "{re.escape(eng)}"\nmsgstr )"'
        replacement = rf'\1"{thai}"'
        new_content, n = re.subn(pattern, replacement, content)
        if n > 0:
            content = new_content
            count += n
            print(f"Translated: {eng[:50]}...")

    with open(po_file, 'w', encoding='utf-8') as f:
        f.write(content)

    print(f"\nTotal translations made: {count}")

if __name__ == "__main__":
    main()
