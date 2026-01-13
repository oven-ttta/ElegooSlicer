#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Complete remaining Thai translations for ElegooSlicer"""

import re

# All remaining translations
translations = {
    # Assembly/Selection messages
    "Please confirm explosion ratio = 1, and please select at least one object.": "กรุณายืนยันอัตราส่วนระเบิด = 1 และเลือกวัตถุอย่างน้อยหนึ่งชิ้น",
    "Please confirm explosion ratio = 1 and select at least two volumes.": "กรุณายืนยันอัตราส่วนระเบิด = 1 และเลือกปริมาตรอย่างน้อยสองชิ้น",
    "(Moving)": "(กำลังเคลื่อนที่)",
    "Point and point assembly": "ประกอบจุดต่อจุด",
    "Face and face assembly": "ประกอบหน้าต่อหน้า",

    # Plugin errors
    "Failed to download the plug-in. Please check your firewall settings and vpn software, check and retry.": "ดาวน์โหลดปลั๊กอินไม่สำเร็จ กรุณาตรวจสอบการตั้งค่าไฟร์วอลล์และซอฟต์แวร์ VPN แล้วลองใหม่",
    "Failed to install the plug-in. Please check whether it is blocked or deleted by anti-virus software.": "ติดตั้งปลั๊กอินไม่สำเร็จ กรุณาตรวจสอบว่าถูกบล็อคหรือลบโดยซอฟต์แวร์ป้องกันไวรัสหรือไม่",

    # Fuzzy Skin
    "Both [Extrusion] and [Combined] modes of Fuzzy Skin require the Arachne Wall Generator to be enabled.": "โหมด [การอัดรีด] และ [รวม] ของผิวฟัซซี่ต้องเปิดใช้งานตัวสร้างผนัง Arachne",

    # Pattern
    "Invalid pattern. Use N, N#K, or a comma-separated list with optional #K per entry. Examples: 5, 5#2, 1,7,9, 5,9#2,18.": "รูปแบบไม่ถูกต้อง ใช้ N, N#K หรือรายการคั่นด้วยจุลภาคพร้อม #K ตัวเลือก ตัวอย่าง: 5, 5#2, 1,7,9, 5,9#2,18",

    # View settings
    "Automatically switch between orthographic and perspective when changing from top/bottom/side views.": "สลับอัตโนมัติระหว่างมุมมองออร์โธกราฟิกและเปอร์สเปกทีฟเมื่อเปลี่ยนจากมุมมองบน/ล่าง/ด้านข้าง",

    # Server/Network
    "The server is unable to respond. Please click the link below to check the server status.": "เซิร์ฟเวอร์ไม่สามารถตอบสนองได้ กรุณาคลิกลิงก์ด้านล่างเพื่อตรวจสอบสถานะเซิร์ฟเวอร์",
    "If the server is in a fault state, you can temporarily use offline printing or local network printing.": "หากเซิร์ฟเวอร์อยู่ในสถานะผิดพลาด คุณสามารถใช้การพิมพ์ออฟไลน์หรือการพิมพ์ผ่านเครือข่ายท้องถิ่นชั่วคราวได้",
    "An unknown error occurred. Please try again later.": "เกิดข้อผิดพลาดที่ไม่ทราบสาเหตุ กรุณาลองใหม่ภายหลัง",
    "Server error. Please try again later.": "ข้อผิดพลาดเซิร์ฟเวอร์ กรุณาลองใหม่ภายหลัง",
    "Too many requests. Please try again later.": "คำขอมากเกินไป กรุณาลองใหม่ภายหลัง",
    "Forbidden access. Please try again later.": "ไม่อนุญาตให้เข้าถึง กรุณาลองใหม่ภายหลัง",
    "Insufficient memory. Please try again later.": "หน่วยความจำไม่เพียงพอ กรุณาลองใหม่ภายหลัง",

    # File import
    "The 3MF file you are importing may be incompatible, continuing could cause errors. Would you like to import the entire file or just the model data?": "ไฟล์ 3MF ที่คุณกำลังนำเข้าอาจไม่เข้ากัน การดำเนินการต่ออาจทำให้เกิดข้อผิดพลาด คุณต้องการนำเข้าไฟล์ทั้งหมดหรือเฉพาะข้อมูลโมเดล?",
    "This project was created with an OrcaSlicer 2.3.1-alpha and uses infill rotation template settings that may not work properly with your current infill pattern. This could result in weak support or print quality issues.": "โปรเจกต์นี้สร้างด้วย OrcaSlicer 2.3.1-alpha และใช้การตั้งค่าเทมเพลตการหมุนอินฟิลที่อาจทำงานไม่ถูกต้องกับรูปแบบอินฟิลปัจจุบันของคุณ อาจส่งผลให้การรองรับอ่อนแอหรือมีปัญหาคุณภาพการพิมพ์",
    "Would you like OrcaSlicer to automatically fix this by clearing the rotation template settings?": "คุณต้องการให้ OrcaSlicer แก้ไขปัญหานี้โดยอัตโนมัติด้วยการล้างการตั้งค่าเทมเพลตการหมุนหรือไม่?",

    # Calibration
    "No accelerations provided for calibration. Use default acceleration value ": "ไม่มีค่าความเร่งสำหรับการปรับเทียบ ใช้ค่าความเร่งเริ่มต้น ",

    # Mouse settings
    "If enabled, swaps the left and right mouse buttons pan and rotate functions.": "หากเปิดใช้งาน จะสลับฟังก์ชันแพนและหมุนของปุ่มเมาส์ซ้ายและขวา",

    # STEP import
    "If enabled,a parameter settings dialog will appear during STEP file import.": "หากเปิดใช้งาน กล่องโต้ตอบการตั้งค่าพารามิเตอร์จะปรากฏขึ้นระหว่างการนำเข้าไฟล์ STEP",

    # Prime tower
    "Enabling both precise Z height and the prime tower may cause the size of prime tower to increase. Do you still want to enable?": "การเปิดใช้งานทั้งความสูง Z ที่แม่นยำและหอไพรม์อาจทำให้ขนาดหอไพรม์เพิ่มขึ้น คุณยังต้องการเปิดใช้งานหรือไม่?",

    # Infill warning
    "Infill patterns are typically designed to handle rotation automatically to ensure proper printing and achieve their intended effects (e.g., Gyroid, Cubic). Rotating the current sparse infill pattern may lead to insufficient support. Please proceed with caution and thoroughly check for any potential printing issues.Are you sure you want to enable this option?": "รูปแบบอินฟิลโดยทั่วไปได้รับการออกแบบให้จัดการการหมุนโดยอัตโนมัติเพื่อให้แน่ใจว่าการพิมพ์ถูกต้องและบรรลุผลตามที่ต้องการ (เช่น Gyroid, Cubic) การหมุนรูปแบบอินฟิลกระจายปัจจุบันอาจทำให้การรองรับไม่เพียงพอ กรุณาดำเนินการด้วยความระมัดระวังและตรวจสอบปัญหาการพิมพ์ที่อาจเกิดขึ้นอย่างละเอียด คุณแน่ใจหรือไม่ว่าต้องการเปิดใช้งานตัวเลือกนี้?",

    # Presets
    "A copy of the current system preset will be created, which will be detached from the system preset.": "จะสร้างสำเนาของค่าพรีเซ็ตระบบปัจจุบัน ซึ่งจะแยกออกจากค่าพรีเซ็ตระบบ",
    "The current custom preset will be detached from the parent system preset.": "ค่าพรีเซ็ตกำหนดเองปัจจุบันจะถูกแยกออกจากค่าพรีเซ็ตระบบหลัก",
    "Any modifications should be saved as a new preset inherited from this one.": "การแก้ไขใดๆ ควรบันทึกเป็นค่าพรีเซ็ตใหม่ที่สืบทอดจากค่านี้",

    # Extruder warnings
    "One or more object were assigned an extruder that the printer does not have.": "วัตถุหนึ่งหรือมากกว่าถูกกำหนดหัวฉีดที่เครื่องพิมพ์ไม่มี",
    "Printing with multiple extruders of differing nozzle diameters. If support is to be printed with the current filament (support_filament == 0 or support_interface_filament == 0), all nozzles have to be of the same diameter.": "การพิมพ์ด้วยหัวฉีดหลายหัวที่มีเส้นผ่านศูนย์กลางต่างกัน หากต้องพิมพ์ฐานรองรับด้วยเส้นพลาสติกปัจจุบัน (support_filament == 0 หรือ support_interface_filament == 0) หัวฉีดทั้งหมดต้องมีเส้นผ่านศูนย์กลางเท่ากัน",
    "The precise wall option will be ignored for outer-inner or inner-outer-inner wall sequences.": "ตัวเลือกผนังแม่นยำจะถูกละเว้นสำหรับลำดับผนังนอก-ใน หรือ ใน-นอก-ใน",

    # Printer settings
    "Setting the number of printheads on the OrangeStorm Giga printer quickly configures all related settings.": "การตั้งค่าจำนวนหัวพิมพ์บนเครื่องพิมพ์ OrangeStorm Giga จะกำหนดค่าที่เกี่ยวข้องทั้งหมดอย่างรวดเร็ว",
    "Default bed type for the printer (supports both numeric and string format).": "ประเภทฐานพิมพ์เริ่มต้นสำหรับเครื่องพิมพ์ (รองรับทั้งรูปแบบตัวเลขและสตริง)",

    # Fan speed
    "Enable this option to allow adjustment of the part cooling fan speed for specifically for overhangs, internal and external bridges. Setting the fan speed specifically for these features can improve overall print quality and reduce warping.": "เปิดใช้งานตัวเลือกนี้เพื่ออนุญาตการปรับความเร็วพัดลมทำความเย็นชิ้นงานสำหรับส่วนยื่น สะพานภายในและภายนอกโดยเฉพาะ การตั้งความเร็วพัดลมสำหรับฟีเจอร์เหล่านี้สามารถปรับปรุงคุณภาพการพิมพ์โดยรวมและลดการบิดงอ",
    "When the overhang exceeds this specified threshold, force the cooling fan to run at the 'Overhang Fan Speed' set below. This threshold is expressed as a percentage, indicating the portion of each line's width that is unsupported by the layer beneath it. Setting this value to 0% forces the cooling fan to run for all outer walls, regardless of the overhang degree.": "เมื่อส่วนยื่นเกินเกณฑ์ที่ระบุนี้ บังคับให้พัดลมทำความเย็นทำงานที่ 'ความเร็วพัดลมส่วนยื่น' ที่ตั้งไว้ด้านล่าง เกณฑ์นี้แสดงเป็นเปอร์เซ็นต์ ระบุสัดส่วนของความกว้างแต่ละเส้นที่ไม่ได้รับการรองรับจากชั้นด้านล่าง การตั้งค่านี้เป็น 0% บังคับให้พัดลมทำความเย็นทำงานสำหรับผนังนอกทั้งหมด โดยไม่คำนึงถึงระดับส่วนยื่น",

    # Boolean expressions
    "A boolean expression using the configuration values of an active printer profile. If this expression evaluates to true, this profile is considered compatible with the active printer profile.": "นิพจน์บูลีนที่ใช้ค่าการกำหนดค่าของโปรไฟล์เครื่องพิมพ์ที่ใช้งานอยู่ หากนิพจน์นี้ประเมินเป็นจริง โปรไฟล์นี้จะถือว่าเข้ากันได้กับโปรไฟล์เครื่องพิมพ์ที่ใช้งานอยู่",
    "A boolean expression using the configuration values of an active print profile. If this expression evaluates to true, this profile is considered compatible with the active print profile.": "นิพจน์บูลีนที่ใช้ค่าการกำหนดค่าของโปรไฟล์การพิมพ์ที่ใช้งานอยู่ หากนิพจน์นี้ประเมินเป็นจริง โปรไฟล์นี้จะถือว่าเข้ากันได้กับโปรไฟล์การพิมพ์ที่ใช้งานอยู่",

    # Infill
    "Aligns infill and surface fill directions to follow the model's orientation on the build plate. When enabled, fill directions rotate with the model to maintain optimal strength characteristics.": "จัดตำแหน่งทิศทางอินฟิลและการเติมพื้นผิวให้ตามทิศทางของโมเดลบนแผ่นสร้าง เมื่อเปิดใช้งาน ทิศทางการเติมจะหมุนตามโมเดลเพื่อรักษาคุณสมบัติความแข็งแรงที่เหมาะสม",
    "Insert solid infill at specific layers. Use N to insert every Nth layer, N#K to insert K consecutive solid layers every N layers (K is optional, e.g. '5#' equals '5#1'), or a comma-separated list (e.g. 1,7,9) to insert at explicit layers. Layers are 1-based.": "แทรกอินฟิลแข็งที่ชั้นเฉพาะ ใช้ N เพื่อแทรกทุกชั้นที่ N, N#K เพื่อแทรกชั้นอินฟิลแข็งติดต่อกัน K ชั้นทุก N ชั้น (K เป็นตัวเลือก เช่น '5#' เท่ากับ '5#1') หรือรายการคั่นด้วยจุลภาค (เช่น 1,7,9) เพื่อแทรกที่ชั้นที่ระบุ ชั้นเริ่มต้นที่ 1",
    "Using multiple lines for the infill pattern, if supported by infill pattern.": "ใช้หลายเส้นสำหรับรูปแบบอินฟิล หากรูปแบบอินฟิลรองรับ",

    # Lattice angles
    "The angle of the first set of Lateral lattice elements in the Z direction. Zero is vertical.": "มุมของชุดองค์ประกอบโครงตาข่ายด้านข้างชุดแรกในทิศทาง Z ศูนย์คือแนวตั้ง",
    "The angle of the second set of Lateral lattice elements in the Z direction. Zero is vertical.": "มุมของชุดองค์ประกอบโครงตาข่ายด้านข้างชุดที่สองในทิศทาง Z ศูนย์คือแนวตั้ง",
    "The angle of the infill angled lines. 60° will result in a pure honeycomb.": "มุมของเส้นอินฟิลเอียง 60° จะได้รังผึ้งบริสุทธิ์",

    # Junction Deviation
    "Marlin Firmware Junction Deviation (replaces the traditional XY Jerk setting).": "การเบี่ยงเบนจุดเชื่อมต่อเฟิร์มแวร์ Marlin (แทนที่การตั้งค่า XY Jerk แบบดั้งเดิม)",
    "Maximum junction deviation (M205 J, only apply if JD > 0 for Marlin Firmware)": "การเบี่ยงเบนจุดเชื่อมต่อสูงสุด (M205 J ใช้เฉพาะเมื่อ JD > 0 สำหรับเฟิร์มแวร์ Marlin)",

    # Noise types
    "Perlin": "เพอร์ลิน",
    "Ridged Multifractal": "มัลติแฟรกทัลสันนูน",
    "Voronoi": "โวโรนอย",

    # Noise settings
    "The base size of the coherent noise features, in mm. Higher values will result in larger features.": "ขนาดฐานของฟีเจอร์สัญญาณรบกวนเชื่อมโยง หน่วยเป็นมม. ค่าที่สูงกว่าจะให้ฟีเจอร์ที่ใหญ่กว่า",
    "The number of octaves of coherent noise to use. Higher values increase the detail of the noise, but also increase computation time.": "จำนวนอ็อกเทฟของสัญญาณรบกวนเชื่อมโยงที่จะใช้ ค่าที่สูงกว่าเพิ่มรายละเอียดของสัญญาณรบกวน แต่ก็เพิ่มเวลาคำนวณด้วย",
    "The decay rate for higher octaves of the coherent noise. Lower values will result in smoother noise.": "อัตราการลดลงสำหรับอ็อกเทฟที่สูงกว่าของสัญญาณรบกวนเชื่อมโยง ค่าที่ต่ำกว่าจะให้สัญญาณรบกวนที่เรียบกว่า",

    # Infill shift
    "Infill shift step": "ขั้นตอนเลื่อนอินฟิล",
    "This parameter adds a slight displacement to each layer of infill to create a cross texture.": "พารามิเตอร์นี้เพิ่มการเลื่อนเล็กน้อยให้แต่ละชั้นอินฟิลเพื่อสร้างพื้นผิวไขว้",

    # Skin/Skeleton
    "The parameter sets the depth of skin.": "พารามิเตอร์ตั้งค่าความลึกของผิว",
    "The parameter sets the overlapping depth between the interior and skin.": "พารามิเตอร์ตั้งค่าความลึกทับซ้อนระหว่างภายในและผิว",
    "Adjust the line width of the selected skin paths.": "ปรับความกว้างเส้นของเส้นทางผิวที่เลือก",
    "Adjust the line width of the selected skeleton paths.": "ปรับความกว้างเส้นของเส้นทางโครงกระดูกที่เลือก",
    "Symmetric infill Y axis": "อินฟิลสมมาตรแกน Y",
    "If the model has two parts that are symmetric about the Y axis, and you want these parts to have symmetric textures, please click this option on one of the parts.": "หากโมเดลมีสองส่วนที่สมมาตรรอบแกน Y และคุณต้องการให้ส่วนเหล่านี้มีพื้นผิวสมมาตร กรุณาคลิกตัวเลือกนี้บนหนึ่งในส่วนนั้น",

    # Edge distance
    "The distance to keep from the edges. A value of 0 sets this to half of the nozzle diameter.": "ระยะห่างจากขอบ ค่า 0 จะตั้งเป็นครึ่งหนึ่งของเส้นผ่านศูนย์กลางหัวฉีด",

    # Resonance avoidance
    "Maximum speed of resonance avoidance.": "ความเร็วสูงสุดของการหลีกเลี่ยงการสั่นพ้อง",
    "Apply only on external features": "ใช้เฉพาะกับฟีเจอร์ภายนอก",
    "Applies extrusion rate smoothing only on external perimeters and overhangs. This can help reduce artefacts due to sharp speed transitions on externally visible overhangs without impacting the print speed of features that will not be visible to the user.": "ใช้การปรับอัตราการอัดรีดให้ราบรื่นเฉพาะบนเส้นรอบวงภายนอกและส่วนยื่น ช่วยลดสิ่งประดิษฐ์จากการเปลี่ยนความเร็วกะทันหันบนส่วนยื่นที่มองเห็นได้ภายนอกโดยไม่กระทบความเร็วพิมพ์ของฟีเจอร์ที่ผู้ใช้จะมองไม่เห็น",

    # Skirt/Draft shield
    "Limits the skirt/draft shield loops to one wall after the first layer. This is useful, on occasion, to conserve filament but may cause the draft shield/skirt to warp / crack.": "จำกัดวงรอบกระโปรง/โล่กันลมเป็นผนังเดียวหลังชั้นแรก มีประโยชน์บางครั้งในการประหยัดเส้นพลาสติก แต่อาจทำให้โล่กันลม/กระโปรงบิดงอ/แตกร้าว",

    # Tool change
    "Auto-generate Tool Change Command": "สร้างคำสั่งเปลี่ยนเครื่องมืออัตโนมัติ",
    "Enable this option to automatically generate the tool change (Tn) command in the G-code when switching extruders/tools.": "เปิดใช้งานตัวเลือกนี้เพื่อสร้างคำสั่งเปลี่ยนเครื่องมือ (Tn) ในG-codeโดยอัตโนมัติเมื่อสลับหัวฉีด/เครื่องมือ",

    # Support
    "If threshold angle is zero, support will be generated for overhangs whose overlap is below the threshold. The smaller this value is, the steeper the overhang that can be printed without support.": "หากมุมเกณฑ์เป็นศูนย์ ฐานรองรับจะถูกสร้างสำหรับส่วนยื่นที่มีการทับซ้อนต่ำกว่าเกณฑ์ ค่ายิ่งเล็ก ส่วนยื่นที่ชันยิ่งสามารถพิมพ์ได้โดยไม่ต้องมีฐานรองรับ",

    # Prime tower
    "Positive values can increase the size of the rib wall, while negative values can reduce the size.However, the size of the rib wall can not be smaller than that determined by the cleaning volume.": "ค่าบวกสามารถเพิ่มขนาดผนังซี่โครง ในขณะที่ค่าลบสามารถลดขนาดได้ อย่างไรก็ตาม ขนาดผนังซี่โครงไม่สามารถเล็กกว่าที่กำหนดโดยปริมาตรการทำความสะอาด",
    "The wall of prime tower will fillet.": "ผนังหอไพรม์จะมีมุมโค้ง",

    # CLI options
    "Export slicing data to a folder.": "ส่งออกข้อมูลการสไลซ์ไปยังโฟลเดอร์",
    "Load cached slicing data from directory.": "โหลดข้อมูลการสไลซ์ที่แคชจากไดเรกทอรี",
    "Slice the plates: 0-all plates, i-plate i, others-invalid": "สไลซ์แผ่น: 0-ทุกแผ่น, i-แผ่น i, อื่นๆ-ไม่ถูกต้อง",
    "Show command help.": "แสดงความช่วยเหลือคำสั่ง",
    "UpToDate": "อัปเดตล่าสุด",
    "Update the configs values of 3mf to latest.": "อัปเดตค่าการกำหนดค่าของ 3mf เป็นล่าสุด",
    "downward machines check": "ตรวจสอบเครื่องจักรที่เข้ากันได้ย้อนหลัง",
    "Load first filament as default for those not loaded.": "โหลดเส้นพลาสติกแรกเป็นค่าเริ่มต้นสำหรับที่ยังไม่โหลด",
    "mtcpp": "mtcpp",
    "mstpp": "mstpp",
    "max slicing time per plate in seconds.": "เวลาสไลซ์สูงสุดต่อแผ่นเป็นวินาที",
    "Output Model Info": "ข้อมูลโมเดลที่ส่งออก",
    "Send progress to pipe.": "ส่งความคืบหน้าไปยังไปป์",
    "Repetition count of the whole model.": "จำนวนการทำซ้ำของโมเดลทั้งหมด",
    "Arrange the supplied models in a plate and merge them in a single model in order to perform actions once.": "จัดเรียงโมเดลที่ให้มาในแผ่นและรวมเป็นโมเดลเดียวเพื่อดำเนินการครั้งเดียว",
    "Scale the model by a float factor.": "ปรับขนาดโมเดลด้วยตัวคูณทศนิยม",
    "Load process/machine settings from the specified file.": "โหลดการตั้งค่ากระบวนการ/เครื่องจักรจากไฟล์ที่ระบุ",
    "Load filament settings from the specified file list.": "โหลดการตั้งค่าเส้นพลาสติกจากรายการไฟล์ที่ระบุ",
    "Load uptodate process/machine settings when using uptodate": "โหลดการตั้งค่ากระบวนการ/เครื่องจักรล่าสุดเมื่อใช้ uptodate",
    "Load uptodate process/machine settings from the specified file when using uptodate.": "โหลดการตั้งค่ากระบวนการ/เครื่องจักรล่าสุดจากไฟล์ที่ระบุเมื่อใช้ uptodate",
    "Load uptodate filament settings when using uptodate": "โหลดการตั้งค่าเส้นพลาสติกล่าสุดเมื่อใช้ uptodate",
    "Load uptodate filament settings from the specified file when using uptodate.": "โหลดการตั้งค่าเส้นพลาสติกล่าสุดจากไฟล์ที่ระบุเมื่อใช้ uptodate",
    "If enabled, check whether current machine downward compatible with the machines in the list.": "หากเปิดใช้งาน ตรวจสอบว่าเครื่องจักรปัจจุบันเข้ากันได้ย้อนหลังกับเครื่องจักรในรายการหรือไม่",
    "The machine settings list needs to do downward checking.": "รายการการตั้งค่าเครื่องจักรต้องทำการตรวจสอบความเข้ากันได้ย้อนหลัง",
    "Load assemble object list from config file.": "โหลดรายการวัตถุประกอบจากไฟล์การกำหนดค่า",
    "Output directory for the exported files.": "ไดเรกทอรีที่ส่งออกสำหรับไฟล์ที่ส่งออก",
    "If enabled, this slicing will be considered using timelapse.": "หากเปิดใช้งาน การสไลซ์นี้จะพิจารณาใช้ไทม์แลปส์",
    "If enabled, Arrange will allow multiple colors on one plate.": "หากเปิดใช้งาน Arrange จะอนุญาตหลายสีบนแผ่นเดียว",
    "If enabled, Arrange will allow rotation when placing objects.": "หากเปิดใช้งาน Arrange จะอนุญาตการหมุนเมื่อวางวัตถุ",
    "If enabled, Arrange will avoid extrusion calibrate region when placing objects.": "หากเปิดใช้งาน Arrange จะหลีกเลี่ยงพื้นที่ปรับเทียบการอัดรีดเมื่อวางวัตถุ",
    "Skip the modified G-code in 3mf from Printer or filament Presets.": "ข้าม G-code ที่แก้ไขใน 3mf จากค่าพรีเซ็ตเครื่องพิมพ์หรือเส้นพลาสติก",
    "MakerLab name to generate this 3mf.": "ชื่อ MakerLab เพื่อสร้าง 3mf นี้",
    "MakerLab version to generate this 3mf.": "เวอร์ชัน MakerLab เพื่อสร้าง 3mf นี้",
    "metadata name list": "รายการชื่อข้อมูลเมตา",
    "metadata name list added into 3mf.": "รายการชื่อข้อมูลเมตาที่เพิ่มเข้า 3mf",
    "metadata value list": "รายการค่าข้อมูลเมตา",
    "metadata value list added into 3mf.": "รายการค่าข้อมูลเมตาที่เพิ่มเข้า 3mf",
    "Allow 3mf with newer version to be sliced": "อนุญาตให้สไลซ์ 3mf ที่มีเวอร์ชันใหม่กว่า",
    "Allow 3mf with newer version to be sliced.": "อนุญาตให้สไลซ์ 3mf ที่มีเวอร์ชันใหม่กว่า",
    "Comma-separated list of printing accelerations": "รายการความเร่งการพิมพ์คั่นด้วยจุลภาค",
    "Comma-separated list of printing speeds": "รายการความเร็วการพิมพ์คั่นด้วยจุลภาค",

    # Input Shaping tests
    "Input shaping Frequency test": "ทดสอบความถี่การปรับรูปอินพุต",
    "Damp: ": "ค่าหน่วง: ",
    "Input shaping Damp test": "ทดสอบค่าหน่วงการปรับรูปอินพุต",
    "Damp": "ค่าหน่วง",
    "Note: Use previously calculated frequencies.": "หมายเหตุ: ใช้ความถี่ที่คำนวณไว้ก่อนหน้า",
    "Junction Deviation test": "ทดสอบการเบี่ยงเบนจุดเชื่อมต่อ",
    "End junction deviation: ": "การเบี่ยงเบนจุดเชื่อมต่อสิ้นสุด: ",
    "Note: Lower values = sharper corners but slower speeds": "หมายเหตุ: ค่าต่ำกว่า = มุมคมกว่าแต่ความเร็วช้ากว่า",
    "NOTE: High values may cause Layer shift": "หมายเหตุ: ค่าสูงอาจทำให้เกิดการเลื่อนชั้น",

    # BigTraffic/Brim
    "BigTraffic": "BigTraffic",
    "Adjust head diameter": "ปรับเส้นผ่านศูนย์กลางหัว",
    "Warning: The brim type is not set to \"painted\", the brim ears will not take effect!": "คำเตือน: ประเภทขอบไม่ได้ตั้งเป็น \"ทาสี\" หูขอบจะไม่มีผล!",

    # Printer errors
    "Missing bed leveling data. Please level the bed.": "ข้อมูลการปรับระดับฐานขาดหาย กรุณาปรับระดับฐาน",
    "Invalid PIN: Please check the printer's region matches your account region, or check that the PIN was entered correctly.": "PIN ไม่ถูกต้อง: กรุณาตรวจสอบว่าภูมิภาคของเครื่องพิมพ์ตรงกับภูมิภาคบัญชีของคุณ หรือตรวจสอบว่าป้อน PIN ถูกต้อง",
    "Printer network error. Please try again later or restart the printer.": "ข้อผิดพลาดเครือข่ายเครื่องพิมพ์ กรุณาลองใหม่ภายหลังหรือรีสตาร์ทเครื่องพิมพ์",
    "Invalid printer data received. Please try again later or restart the printer.": "ได้รับข้อมูลเครื่องพิมพ์ไม่ถูกต้อง กรุณาลองใหม่ภายหลังหรือรีสตาร์ทเครื่องพิมพ์",
    "Printer already exists, please check.": "เครื่องพิมพ์มีอยู่แล้ว กรุณาตรวจสอบ",
    "Unmapped filament detected, printing cannot be started.": "ตรวจพบเส้นพลาสติกที่ไม่ได้แมป ไม่สามารถเริ่มพิมพ์ได้",
    "Operation failed. Please check if multiple clients are open and continue on the main client.": "การดำเนินการล้มเหลว กรุณาตรวจสอบว่าเปิดไคลเอนต์หลายตัวหรือไม่และดำเนินการต่อบนไคลเอนต์หลัก",
    "The file is too large to upload. Please try another way to start the transfer or print.": "ไฟล์ใหญ่เกินไปสำหรับอัปโหลด กรุณาลองวิธีอื่นเพื่อเริ่มการถ่ายโอนหรือพิมพ์",
}

def translate_po_file(po_path):
    """Read PO file and apply translations"""
    with open(po_path, 'r', encoding='utf-8') as f:
        content = f.read()

    count = 0
    for eng, thai in translations.items():
        # Pattern for single line msgid with empty msgstr
        pattern = rf'(msgid "{re.escape(eng)}"\nmsgstr )""\n'
        if re.search(pattern, content):
            replacement = rf'\1"{thai}"\n'
            content, n = re.subn(pattern, replacement, content, count=1)
            if n > 0:
                count += n
                print(f"Translated: {eng[:50]}...")

    with open(po_path, 'w', encoding='utf-8') as f:
        f.write(content)

    return count

if __name__ == "__main__":
    po_file = r"D:\ElegooSlicer\localization\i18n\th\ElegooSlicer_th.po"
    translated = translate_po_file(po_file)
    print(f"\nTotal translations applied: {translated}")
