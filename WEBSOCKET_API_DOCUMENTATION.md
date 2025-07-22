# Elegoo Link WebSocket API 接口文档

## 目录

- [1. 概述](#概述)
  - [版本信息](#版本信息)
  - [WebSocket 连接地址规则](#websocket-连接地址规则)
- [2. 消息格式](#消息格式)
  - [基础消息结构](#基础消息结构)
  - [字段说明](#字段说明)
- [3. 方法类型 (MethodType)](#方法类型-methodtype)
  - [系统相关方法 (10-99)](#系统相关方法-10-99)
  - [设备命令相关方法 (1000-1999)](#设备命令相关方法-1000-1999)
    - [设备连接与管理 (1000-1099)](#设备连接与管理-1000-1099)
    - [打印任务控制 (1100-1199)](#打印任务控制-1100-1199)
    - [硬件设置与控制 (1200-1299)](#硬件设置与控制-1200-1299)
    - [文件管理 (1300-1399)](#文件管理-1300-1399)
    - [任务管理 (1400-1499)](#任务管理-1400-1499)
  - [通知消息 (2000+)](#通知消息-2000)
- [4. 响应状态码](#响应状态码)
- [5. 设备类型与状态](#设备类型与状态)
  - [设备类型 (DeviceType)](#设备类型-devicetype)
  - [连接状态 (ConnectionStatus)](#连接状态-connectionstatus)
  - [机器主状态 (MachineMainStatus)](#机器主状态-machinemainStatus)
  - [机器子状态 (MachineSubStatus)](#机器子状态-machinesubstatus)
  - [机器异常状态 (MachineExceptionStatus)](#机器异常状态-machineexceptionstatus)
  - [设备能力 (DeviceCapabilities)](#设备能力-devicecapabilities)
    - [存储组件信息 (StorageComponent)](#存储组件信息-storagecomponent)
    - [风扇组件信息 (FanComponent)](#风扇组件信息-fancomponent)
    - [温度控制组件信息 (TemperatureComponent)](#温度控制组件信息-temperaturecomponent)
    - [灯光组件信息 (LightComponent)](#灯光组件信息-lightcomponent)
    - [摄像头能力 (CameraCapabilities)](#摄像头能力-cameracapabilities)
    - [系统能力 (SystemCapabilities)](#系统能力-systemcapabilities)
    - [设备属性 (DeviceAttributes)](#设备属性-deviceattributes)
- [6. API 详细说明](#api-详细说明)
  - [6.1 系统相关接口](#1-系统相关接口)
    - [版本协商](#11-版本协商)
    - [设备发现](#12-设备发现)
    - [获取设备列表](#13-获取设备列表)
  - [6.2 设备连接与控制](#2-设备连接与控制)
    - [连接设备](#21-连接设备)
    - [获取设备状态](#22-获取设备状态)
  - [6.3 打印控制](#3-打印控制)
    - [开始打印](#31-开始打印)
    - [暂停/恢复/停止打印](#32-暂停恢复停止打印)
  - [6.4 轴控制](#4-轴控制)
    - [轴回零](#41-轴回零)
    - [移动轴](#42-移动轴)
  - [6.5 温度与风扇控制](#5-温度与风扇控制)
    - [设置温度](#51-设置温度)
    - [设置风扇速度](#52-设置风扇速度)
    - [设置灯光亮度](#53-设置灯光亮度)
    - [设置打印速度](#54-设置打印速度)
  - [6.6 文件管理](#6-文件管理)
    - [获取文件列表](#61-获取文件列表)
    - [上传文件](#62-上传文件)
    - [获取磁盘信息](#63-获取磁盘信息)
  - [6.7 打印任务管理](#7-打印任务管理)
    - [获取打印任务列表](#71-获取打印任务列表)
    - [获取任务详情](#72-获取任务详情)
  - [6.8 其他功能](#8-其他功能)
    - [视频流](#81-视频流)
    - [进料/出料](#82-进料出料)
- [7. 错误处理](#错误处理)
  - [通用错误响应格式](#通用错误响应格式)
  - [常见错误情况](#常见错误情况)
- [8. 最佳实践](#最佳实践)
- [9. 示例代码](#示例代码)
  - [JavaScript WebSocket 客户端示例](#javascript-websocket-客户端示例)
- [10. 常用字段取值说明](#常用字段取值说明)
  - [存储位置 (storageLocation)](#存储位置-storagelocation)
  - [温度组件名称](#温度组件名称-temperatures-对象的-key)
  - [风扇名称](#风扇名称-fans-对象的-key)
  - [灯光名称](#灯光名称-lights-对象的-key)
  - [打印速度挡位](#打印速度挡位-speedmode)
  - [热床类型](#热床类型-heatedbedtype)
  - [打印任务状态](#打印任务状态-taskstatus)
  - [延时摄影状态](#延时摄影状态-timelapsevideoStatus)

---

## 概述

Elegoo Link WebSocket API 是一个用于与 Elegoo 3D 打印机进行通信的统一接口协议。该协议支持设备发现、连接管理、打印控制、文件传输等功能。

### 版本信息
- **当前版本**: 1.0
- **最低支持版本**: 1.0
- **协议类型**: WebSocket / MQTT

### WebSocket 连接地址规则

WebSocket 连接地址遵循以下格式：

```
ws://<设备IP地址>:6382/ws
```

**示例：**
- `ws://192.168.1.100:6382/ws`
- `ws://10.0.0.50:6382/ws`

**说明：**
- **协议**: 使用 `ws://` 协议（如果设备支持加密连接，可能使用 `wss://`）
- **IP地址**: 设备在网络中的IP地址，通过设备发现或手动配置获得
- **端口**: 固定使用 `6382` 端口
- **路径**: 固定使用 `/ws` 路径

**连接流程：**
1. 通过设备发现获取设备IP地址
2. 构建WebSocket连接URL：`ws://<设备IP>:6382/ws`
3. 建立WebSocket连接
4. 进行版本协商
5. 根据设备的授权模式进行认证（如果需要）
6. 开始API调用

## 消息格式

### 基础消息结构

所有消息都遵循以下 JSON 格式：

```json
{
  "id": "唯一消息ID",
  "version": "1.0",
  "method": 1000,
  "data": {}
}
```

### 字段说明

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| id | string | 是 | 唯一消息标识符 |
| version | string | 是 | 协议版本号，默认 "1.0" |
| method | number | 是 | 方法类型，见下方方法列表 |
| data | object | 否 | 消息数据，根据不同方法类型变化 |

## 方法类型 (MethodType)

### 系统相关方法 (10-99)

| 方法名 | 值 | 说明 |
|--------|-----|------|
| UNKNOWN | 0 | 未知命令 |
| VERSION_NEGOTIATION | 10 | 版本协商请求 |
| VERSION_INFO_REQUEST | 11 | 请求版本信息 |
| START_DEVICE_DISCOVERY | 20 | 开始设备发现 |
| STOP_DEVICE_DISCOVERY | 21 | 停止设备发现 |
| GET_DEVICE_LIST | 22 | 获取设备列表 |

### 设备命令相关方法 (1000-1999)

#### 设备连接与管理 (1000-1099)
| 方法名 | 值 | 说明 |
|--------|-----|------|
| DEVICE_COMMAND_START | 1000 | 设备命令起始编号 |
| CONNECT | 1000 | 连接设备 |
| DISCONNECT | 1001 | 断开连接 |
| DEVICE_ATTRIBUTES_ASYNC | 1002 | 获取设备属性，获取之后将会通过消息通知上报 |
| DEVICE_STATUS_ASYNC | 1003 | 获取设备状态，获取之后将会通过消息通知上报 |
| DEVICE_ATTRIBUTES | 1004 | 同步获取设备属性（暂未实现） |
| DEVICE_STATUS | 1005 | 同步获取设备状态（暂未实现） |
| SET_MACHINE_NAME | 1006 | 设置机器名字 |
| GET_DISK_INFO | 1007 | 获取磁盘信息 |
| VIDEO_STREAM | 1008 | 获取相机视频流 |
| EXPORT_TIMELAPSE_VIDEO | 1009 | 导出延时摄影视频 |

#### 打印任务控制 (1100-1199)
| 方法名 | 值 | 说明 |
|--------|-----|------|
| START_PRINT | 1100 | 开始打印 |
| PAUSE_PRINT | 1101 | 暂停打印 |
| RESUME_PRINT | 1102 | 恢复打印 |
| STOP_PRINT | 1103 | 停止打印 |

#### 硬件设置与控制 (1200-1299)
| 方法名 | 值 | 说明 |
|--------|-----|------|
| HOME_AXES | 1200 | 回零轴 |
| MOVE_AXES | 1201 | 移动轴 |
| SET_LIGHT | 1202 | 设置灯光亮度 |
| SET_TEMPERATURE | 1203 | 设置温度 |
| SET_PRINT_SPEED | 1204 | 设置打印速度挡位 |
| SET_FAN_SPEED | 1205 | 设置风扇速度 |
| LOAD_FILAMENT | 1206 | 进料 |
| UNLOAD_FILAMENT | 1207 | 出料 |

#### 文件管理 (1300-1399)
| 方法名 | 值 | 说明 |
|--------|-----|------|
| UPLOAD_FILE | 1300 | 上传文件 |
| CANCEL_FILE_UPLOAD | 1301 | 取消文件上传 |
| DOWNLOAD_FILE | 1302 | 下载文件（暂未实现） |
| CANCEL_FILE_DOWNLOAD | 1303 | 取消文件下载 |
| GET_DOWNLOAD_URL | 1304 | 获取下载URL |
| GET_FILE_LIST | 1305 | 获取文件列表 |
| GET_FILE_DETAIL | 1306 | 获取文件详情 |
| DELETE_FILE | 1307 | 删除文件 |

#### 任务管理 (1400-1499)
| 方法名 | 值 | 说明 |
|--------|-----|------|
| PRINT_TASK_LIST | 1400 | 获取打印任务列表 |
| PRINT_TASK_DETAIL | 1401 | 获取打印任务详情 |
| DELETE_PRINT_TASK | 1402 | 删除打印任务 |

#### 其他功能
| 方法名 | 值 | 说明 |
|--------|-----|------|
| DEVICE_COMMAND_END | 1999 | 设备命令结束编号 |

### 通知消息 (2000+)

| 方法名 | 值 | 说明 |
|--------|-----|------|
| ON_DEVICE_STATUS | 2000 | 设备状态更新通知 |
| ON_DEVICE_ATTRIBUTES | 2001 | 设备属性更新通知 |
| ON_DEVICE_ERROR | 2002 | 设备错误通知 |
| ON_CONNECTION_STATUS | 2003 | 连接状态变化通知 |
| ON_FILE_TRANSFER_PROGRESS | 2004 | 文件传输进度通知 |
| ON_DEVICE_DISCOVERY | 2005 | 设备发现通知 |

## 响应状态码

| 状态码 | 名称 | 说明 |
|--------|------|------|
| **成功状态** | | |
| 0 | SUCCESS | 操作成功 |
| **通用错误 (1-99)** | | |
| 1 | UNKNOWN_ERROR | 未知错误 |
| 2 | INTERNAL_ERROR | 内部错误 |
| 3 | INVALID_PARAMETER | 参数无效（值错误、类型错误等） |
| 4 | INVALID_FORMAT | 格式错误（JSON结构、数据格式等） |
| 5 | TIMEOUT | 操作超时（包括连接、文件传输等） |
| 6 | CANCELLED | 操作被取消 |
| 7 | PERMISSION_DENIED | 权限不足 |
| 8 | NOT_FOUND | 资源未找到（设备、文件等） |
| 9 | ALREADY_EXISTS | 资源已存在 |
| 10 | INSUFFICIENT_SPACE | 存储空间不足 |
| 11 | BAD_REQUEST | 错误请求 |
| 12 | CONNECTION_ERROR | 连接错误（包括连接失败、丢失） |
| 13 | NETWORK_ERROR | 网络错误 |
| 14 | SERVICE_UNAVAILABLE | 服务不可用（服务器忙、维护等） |
| **版本兼容性相关错误 (100-199)** | | |
| 100 | VERSION_NOT_SUPPORTED | 版本不支持 |
| 101 | VERSION_TOO_OLD | 版本过低 |
| 102 | VERSION_TOO_NEW | 版本过高 |
| **鉴权相关错误 (200-299)** | | |
| 200 | UNAUTHORIZED | 未授权访问 |
| 201 | AUTHENTICATION_FAILED | 登录验证失败 |
| 202 | TOKEN_EXPIRED | Token已过期 |
| 203 | TOKEN_INVALID | Token无效 |
| **文件传输相关错误 (300-399)** | | |
| 300 | FILE_TRANSFER_FAILED | 文件传输失败（包括上传、下载） |
| 301 | FILE_NOT_FOUND | 文件未找到 |
| **设备业务错误 (1000-1999)** | | |
| 1000 | DEVICE_BUSY | 设备忙碌 |
| 1001 | DEVICE_OFFLINE | 设备离线 |
| 1002 | DEVICE_INITIALIZATION_ERROR | 设备初始化错误 |
| 1003 | DEVICE_COMMAND_FAILED | 设备命令执行失败 |
| 1004 | DEVICE_ALREADY_CONNECTED | 设备已连接或正在连接中 |

## 设备类型与状态

### 设备类型 (DeviceType)

| 类型 | 说明 |
|------|------|
| ELEGOO_FDM_CC | Elegoo FDM 3D打印机 CC系列 |
| ELEGOO_FDM_CCS | Elegoo FDM 3D打印机 CCS系列 |
| UNKNOWN | 未知设备 |

### 连接状态 (ConnectionStatus)

| 状态 | 说明 |
|------|------|
| DISCONNECTED | 已断开连接 |
| CONNECTED | 已连接 |

### 机器主状态 (MachineMainStatus)

| 状态 | 值 | 说明 |
|------|-----|------|
| OFFLINE | -1 | 离线状态 |
| IDLE | 0 | 空闲状态 |
| PRINTING | 1 | 打印中 |
| SELF_CHECKING | 2 | 自检中 |
| AUTO_LEVELING | 3 | 自动调平 |
| PID_CALIBRATING | 4 | PID校准中 |
| RESONANCE_TESTING | 5 | 共振测试中 |
| UPDATING | 6 | 升级中 |
| FILE_COPYING | 7 | 文件拷贝中 |
| FILE_TRANSFERRING | 8 | 文件传输中 |
| HOMING | 9 | 回零中 |
| PREHEATING | 10 | 预热中 |
| FILAMENT_OPERATING | 11 | 耗材操作中 |
| EXTRUDER_OPERATING | 12 | 挤出机操作中 |
| PRINT_COMPLETED | 13 | 打印完成 |
| RFID_RECOGNIZING | 14 | RFID识别中 |
| EXCEPTION | 99 | 设备异常状态 |
| UNKNOWN | 100 | 未知状态 |

### 机器子状态 (MachineSubStatus)

| 状态 | 值 | 说明 |
|------|-----|------|
| NONE | 0 | 无子状态 |
| UNKNOWN | 1 | 未知状态 |
| **打印相关子状态 (PRINTING = 1)** | | |
| P_PREHEATING | 101 | 预热中 |
| P_HOMING | 102 | 回零中 |
| P_AUTO_LEVELING | 103 | 自动调平中 |
| P_PRINTING | 104 | 打印中 |
| P_PAUSING | 105 | 暂停中 |
| P_LOAD_FILAMENT | 106 | 暂停-进料中 |
| P_UNLOAD_FILAMENT | 107 | 暂停-退料中 |
| P_RESUME_PRINTING | 108 | 暂停后恢复打印中 |
| P_PAUSED | 109 | 已暂停 |
| P_STOPPING | 110 | 停止中 |
| P_STOPPED | 111 | 已停止 |
| P_OUT_OF_FILAMENT | 112 | 断料触发提示（弹窗）状态 |
| P_RESUME_AFTER_OUT_OF_FILAMENT | 113 | 断料后恢复打印 |
| P_POWER_LOSS_RECOVERY_PROMPT | 114 | 断电续打提示（弹窗）状态 |
| P_POWER_LOSS_RECOVERY | 115 | 断电续打恢复中状态 |
| P_COLOR_CHANGE | 116 | 换色中 |
| P_COLOR_CHANGE_COMPLETED | 117 | 换色完成 |
| P_COLOR_CHANGE_PROMPT | 118 | 换色提示（弹窗状态） |
| P_COLOR_CHANGE_FAILED | 119 | 换色失败提示（弹窗状态） |
| **自检相关子状态 (SELF_CHECKING = 2)** | | |
| SC_PID_CALIBRATION | 201 | PID校准中 |
| SC_PID_CALIBRATION_COMPLETED | 202 | PID校准完成 |
| SC_RESONANCE_TEST | 203 | 共振测试中 |
| SC_RESONANCE_TEST_COMPLETED | 204 | 共振测试完成 |
| SC_AUTO_LEVELING | 205 | 自动调平中 |
| SC_AUTO_LEVELING_COMPLETED | 206 | 自动调平完成 |
| SC_COMPLETED | 207 | 自检完成 |
| **自动调平相关子状态 (AUTO_LEVELING = 3)** | | |
| AL_AUTO_LEVELING | 301 | 自动调平中 |
| AL_AUTO_LEVELING_COMPLETED | 302 | 自动调平完成（弹窗）状态 |
| **PID校准相关子状态 (PID_CALIBRATING = 4)** | | |
| PC_PID_CALIBRATION | 401 | PID校准中 |
| PC_PID_CALIBRATION_COMPLETED | 402 | PID校准完成 |
| **共振测试相关子状态 (RESONANCE_TESTING = 5)** | | |
| RT_RESONANCE_TEST | 501 | 共振测试中 |
| RT_RESONANCE_TEST_COMPLETED | 502 | 共振测试完成 |
| **升级相关子状态 (UPDATING = 6)** | | |
| U_UPDATING | 601 | 升级中 |
| U_UPDATING_COMPLETED | 602 | 升级完成 |
| **文件操作相关子状态** | | |
| CF_COPYING_FILE | 701 | 文件拷贝中 (FILE_COPYING = 7) |
| CF_COPYING_FILE_COMPLETED | 702 | 文件拷贝完成 |
| UF_UPLOADING_FILE | 801 | 文件传输中 (FILE_TRANSFERRING = 8) |
| UF_UPLOADING_FILE_COMPLETED | 802 | 文件传输完成 |
| **回零相关子状态 (HOMING = 9)** | | |
| H_HOMING | 901 | 回零中 |
| H_HOMING_COMPLETED | 902 | 回零完成 |
| **预热相关子状态 (PREHEATING = 10)** | | |
| PRE_EXTRUDER_PREHEATING | 1001 | 挤出机预热中 |
| PRE_EXTRUDER_PREHEATING_COMPLETED | 1002 | 挤出机预热完成 |
| PRE_HEATED_BED_PREHEATING | 1003 | 热床预热中 |
| PRE_HEATED_BED_PREHEATING_COMPLETED | 1004 | 热床预热完成 |
| **耗材操作相关子状态 (FILAMENT_OPERATING = 11)** | | |
| FO_FILAMENT_LOADING | 1101 | 进料中 |
| FO_FILAMENT_LOADING_COMPLETED | 1102 | 进料完成 |
| FO_FILAMENT_UNLOADING | 1103 | 退料中 |
| FO_FILAMENT_UNLOADING_COMPLETED | 1104 | 退料完成 |
| FO_FILAMENT_CUTTING | 1105 | 切（刀）料中 |
| FO_FILAMENT_CUTTING_COMPLETED | 1106 | 切（刀）料完成 |
| **挤出机操作相关子状态 (EXTRUDER_OPERATING = 12)** | | |
| EO_EXTRUDER_LOADING | 1201 | 挤出机进料（正转）中 |
| EO_EXTRUDER_LOADING_COMPLETED | 1202 | 挤出机进料（正转）完成 |
| EO_EXTRUDER_UNLOADING | 1203 | 挤出机退料（反转）中 |
| EO_EXTRUDER_UNLOADING_COMPLETED | 1204 | 挤出机退料（反转）完成 |
| EO_EXTRUDER_TEMPERATURE_PROMPT | 1205 | 温度提示（弹窗状态） |
| **异常状态** | | |
| P_EXCEPTION_STOPPED | 9999 | 异常停止状态 |

### 机器异常状态 (MachineExceptionStatus)

| 异常 | 值 | 说明 |
|------|-----|------|
| **风扇异常** | | |
| BOARD_FAN_ERROR | 2001 | 主板风扇异常 |
| THROAT_FAN_ERROR | 2002 | 喉管风扇异常 |
| MODEL_FAN_ERROR | 2003 | 模型风扇异常 |
| **温度异常** | | |
| HEATED_BED_HEATING_ERROR | 8001 | 热床加热异常 |
| HEATED_BED_THERMISTOR_ERROR | 8002 | 热床热敏异常 |
| HOTEND_HEATING_ERROR | 8003 | 热端加热异常 |
| HOTEND_THERMISTOR_ERROR | 8004 | 热端热敏异常 |
| **传感器异常** | | |
| ACCELERATION_SENSOR_ERROR | 8005 | 加速度传感器异常 |
| Z_AXIS_HOME_ERROR | 8006 | Z轴归零异常 |

### 设备能力 (DeviceCapabilities)

设备能力描述了设备支持的各种功能和组件。

#### 存储组件信息 (StorageComponent)

| 字段 | 类型 | 说明 |
|------|------|------|
| name | string | 存储设备名称（如: "local", "udisk", "sdcard"） |
| removable | boolean | 是否为可移动设备 |

#### 风扇组件信息 (FanComponent)

| 字段 | 类型 | 说明 |
|------|------|------|
| name | string | 风扇名称（如: "model", "heatsink", "controller", "box", "aux"） |
| controllable | boolean | 是否可控制 |
| minSpeed | number | 最小速度 (0-100) |
| maxSpeed | number | 最大速度 (0-100) |
| supportsRpmReading | boolean | 是否支持转速读取 |

#### 温度控制组件信息 (TemperatureComponent)

| 字段 | 类型 | 说明 |
|------|------|------|
| name | string | 组件名称（如: "heatedBed", "extruder", "chamber"） |
| controllable | boolean | 是否可控制温度 |
| supportsTemperatureReading | boolean | 是否支持温度读取 |
| minTemperature | number | 最低温度 (摄氏度) |
| maxTemperature | number | 最高温度 (摄氏度) |

#### 灯光组件信息 (LightComponent)

| 字段 | 类型 | 说明 |
|------|------|------|
| name | string | 灯光名称（例如: "main"） |
| type | string | 灯光类型（例如: "rgb", "single-color"） |
| minBrightness | number | 最小亮度 (0-255) |
| maxBrightness | number | 最大亮度 (0-255) |

#### 摄像头能力 (CameraCapabilities)

| 字段 | 类型 | 说明 |
|------|------|------|
| supportsCamera | boolean | 是否支持摄像头 |
| supportsWebRTC | boolean | 是否支持WebRTC视频流 |
| supportsTimeLapse | boolean | 是否支持延时摄影 |

#### 系统能力 (SystemCapabilities)

| 字段 | 类型 | 说明 |
|------|------|------|
| canSetMachineName | boolean | 是否支持设置机器名称 |
| canGetDiskInfo | boolean | 是否支持获取磁盘信息 |

#### 设备属性 (DeviceAttributes)

DeviceAttributes 继承自 DeviceInfo，并包含详细的设备能力描述：

| 字段 | 类型 | 说明 |
|------|------|------|
| capabilities | DeviceCapabilities | 设备能力详细描述 |
| capabilities.storageComponents | array | 存储组件列表 |
| capabilities.fanComponents | array | 风扇组件列表 |
| capabilities.temperatureComponents | array | 温度控制组件列表 |
| capabilities.lightComponents | array | 灯光组件列表 |
| capabilities.camera | CameraCapabilities | 摄像头相关能力 |
| capabilities.system | SystemCapabilities | 系统相关能力 |

## API 详细说明

### 1. 系统相关接口

#### 1.1 版本协商

**请求**
```json
{
  "id": "msg_001",
  "version": "1.0",
  "method": 10,
  "data": {
    "clientVersion": "1.0",
    "supportedVersions": ["1.0"],
    "clientType": "desktop",
    "clientInfo": {
      "platform": "Windows",
      "appVersion": "1.0.0"
    }
  }
}
```

**请求字段说明**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| clientVersion | string | 是 | 客户端当前版本 |
| supportedVersions | array | 是 | 客户端支持的版本列表 |
| clientType | string | 是 | 客户端类型（如"desktop", "mobile", "web"） |
| clientInfo | object | 否 | 客户端额外信息 |
| clientInfo.platform | string | 否 | 客户端平台（如"Windows", "macOS", "Linux"） |
| clientInfo.appVersion | string | 否 | 应用程序版本 |

**响应**
```json
{
  "id": "msg_001",
  "version": "1.0",
  "method": 10,
  "data": {
    "code": 0,
    "message": "Version negotiation successful",
    "negotiatedVersion": "1.0",
    "serverVersion": "1.0",
    "supportedVersions": ["1.0"]
  }
}
```

**响应字段说明**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| code | number | 是 | 响应状态码 |
| message | string | 是 | 响应消息 |
| negotiatedVersion | string | 是 | 协商后使用的版本 |
| serverVersion | string | 是 | 服务端当前版本 |
| supportedVersions | array | 是 | 服务端支持的版本列表 |

#### 1.2 设备发现

**开始设备发现**
```json
{
  "id": "msg_002",
  "version": "1.0",
  "method": 20,
  "data": {
    "timeoutMs": 3000,
    "broadcastInterval": 2,
    "enableAutoRetry": false,
    "preferredListenPorts": [8080, 8081]
  }
}
```

**请求字段说明**

| 字段 | 类型 | 必填 | 默认值 | 说明 |
|------|------|------|--------|------|
| timeoutMs | number | 否 | 3000 | 设备发现超时时间（毫秒） |
| broadcastInterval | number | 否 | 2 | 广播重发间隔（秒） |
| enableAutoRetry | boolean | 否 | false | 是否启用自动重试 |
| preferredListenPorts | array | 否 | [] | 首选监听端口列表 |

**设备发现响应**
```json
{
  "id": "msg_002",
  "version": "1.0",
  "method": 20,
  "data": {
    "code": 0,
    "message": "Device discovery successful",
    "devices": [
      {
        "deviceId": "elegoo_printer_001",
        "name": "Elegoo Neptune 4 Pro",
        "brand": "Elegoo",
        "model": "Neptune 4 Pro",
        "firmwareVersion": "1.0.2",
        "manufacturer": "Elegoo",
        "serialNumber": "EN4P202301001",
        "protocolType": "WebSocket",
        "deviceType": "ELEGOO_FDM_CCS",
        "ipAddress": "192.168.1.100",
        "webUrl": "http://192.168.1.100",
        "connectionUrl": "ws://192.168.1.100:6382/ws",
        "authMode": "token",
        "extraInfo": {}
      }
    ]
  }
}
```

**设备信息字段说明**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| deviceId | string | 是 | 设备唯一标识符 |
| name | string | 是 | 设备名称 |
| brand | string | 是 | 设备品牌 |
| model | string | 是 | 设备型号 |
| firmwareVersion | string | 是 | 固件版本号 |
| manufacturer | string | 是 | 制造商 |
| serialNumber | string | 是 | 设备序列号 |
| protocolType | string | 是 | 协议类型（"MQTT" 或 "WebSocket"） |
| deviceType | string | 是 | 设备类型（"ELEGOO_FDM_CC", "ELEGOO_FDM_CCS", "UNKNOWN"） |
| ipAddress | string | 是 | 设备IP地址 |
| webUrl | string | 否 | 设备Web界面地址（如果有的话，例如: http://192.168.1.100） |
| connectionUrl | string | 是 | WebSocket或MQTT连接地址 |
| authMode | string | 否 | 授权模式（"token", "basic" 或空表示无需授权） |
| extraInfo | object | 否 | 额外设备信息 |

#### 1.3 获取设备列表

**请求**
```json
{
  "id": "msg_003",
  "version": "1.0",
  "method": 22,
  "data": {}
}
```

**请求字段说明**

无需额外参数，data 字段可为空对象。

**响应**
```json
{
  "id": "msg_003",
  "version": "1.0",
  "method": 22,
  "data": {
    "code": 0,
    "message": "Device list retrieved successfully",
    "devices": [
      {
        "deviceId": "elegoo_printer_001",
        "name": "Elegoo Neptune 4 Pro",
        "brand": "Elegoo",
        "model": "Neptune 4 Pro",
        "firmwareVersion": "1.2.3",
        "manufacturer": "Elegoo",
        "serialNumber": "SN123456789",
        "protocolType": 1,
        "deviceType": 0,
        "ipAddress": "192.168.1.100",
        "webUrl": "http://192.168.1.100",
        "connectionUrl": "ws://192.168.1.100:6382/ws",
        "authMode": "token",
        "extraInfo": {}
      }
    ]
  }
}
```

**响应字段说明**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| code | number | 是 | 响应状态码 |
| message | string | 是 | 响应消息 |
| devices | array | 是 | 设备列表 |

**devices 数组中每个设备对象的字段说明**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| deviceId | string | 是 | 设备唯一标识符 |
| name | string | 是 | 设备名称 |
| brand | string | 是 | 设备品牌 |
| model | string | 是 | 设备型号 |
| firmwareVersion | string | 是 | 固件版本号 |
| manufacturer | string | 是 | 制造商 |
| serialNumber | string | 是 | 设备序列号 |
| protocolType | number | 是 | 协议类型（0=MQTT, 1=WebSocket） |
| deviceType | number | 是 | 设备类型（0=ELEGOO_FDM_CC, 1=ELEGOO_FDM_CCS, 2=UNKNOWN） |
| ipAddress | string | 是 | 设备IP地址 |
| webUrl | string | 否 | 设备Web界面地址（如果有的话，例如: http://192.168.1.100） |
| connectionUrl | string | 是 | WebSocket或MQTT连接地址 |
| authMode | string | 否 | 授权模式（"token", "basic" 或空表示无需授权） |
| extraInfo | object | 否 | 额外设备信息 |

### 2. 设备连接与控制

#### 2.1 连接设备

**请求**
```json
{
  "id": "msg_003",
  "version": "1.0",
  "method": 1000,
  "data": {
    "deviceId": "elegoo_printer_001",
    "username": "admin",
    "password": "password",
    "token": "auth_token_here",
    "extraParams": {}
  }
}
```

**请求字段说明**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| deviceId | string | 是 | 设备唯一标识符 |
| username | string | 否 | 用户名（basic授权模式时使用） |
| password | string | 否 | 密码（basic授权模式时使用） |
| token | string | 否 | 授权令牌（token授权模式时使用） |
| extraParams | object | 否 | 额外连接参数 |

**响应**
```json
{
  "id": "msg_003",
  "version": "1.0",
  "method": 1000,
  "data": {
    "code": 0,
    "message": "Connected successfully"
  }
}
```

#### 2.2 获取设备状态

**请求**
```json
{
  "id": "msg_004",
  "version": "1.0",
  "method": 1003,
  "data": {
    "deviceId": "elegoo_printer_001"
  }
}
```

**请求字段说明**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| deviceId | string | 是 | 设备唯一标识符 |

**状态通知**
```json
{
  "id": "notification_002",
  "version": "1.0",
  "method": 2000,
  "data": {
    "deviceId": "elegoo_printer_001",
    "temperatures": {
      "hotend": {
        "current": 210.5,
        "target": 210.0,
        "highest": 215.0,
        "lowest": 20.0
      },
      "bed": {
        "current": 60.2,
        "target": 60.0,
        "highest": 65.0,
        "lowest": 20.0
      }
    },
    "fans": {
      "part_cooling": {
        "speed": 100,
        "rpm": 4500
      },
      "hotend": {
        "speed": 80,
        "rpm": 3600
      }
    },
    "printAxesInfo": {
      "x": 120.5,
      "y": 100.3,
      "z": 10.2,
      "xHomed": true,
      "yHomed": true,
      "zHomed": true,
      "homedAxes": "xyz"
    },
    "machineStatus": {
      "status": 2,
      "subStatus": 203,
      "exceptionStatus": [],
      "supportProgress": true,
      "progress": 45
    },
    "printStatus": {
      "filename": "test_model.gcode",
      "totalTime": 7200,
      "currentTime": 3240,
      "estimatedTime": 7180,
      "totalLayer": 200,
      "currentLayer": 90,
      "progress": 45.0
    }
  }
}
```

**状态通知字段说明**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| deviceId | string | 是 | 设备唯一标识符 |
| temperatures | object | 是 | 温度信息集合 |
| temperatures.{key}.current | number | 是 | 当前温度（key值 "heatedBed", "extruder", "box"，分别表示热床温度，打印头温度，机箱温度） |
| temperatures.{key}.target | number | 是 | 目标温度 |
| temperatures.{key}.highest | number | 是 | 历史最高温度 |
| temperatures.{key}.lowest | number | 是 | 历史最低温度 |
| fans | object | 是 | 风扇信息集合 |
| fans.{key}.speed | number | 是 | 风扇速度百分比 (0-100)（key值 "model", "heatsink", "controller", "box", "aux"，分别表示模型冷却风扇、热端散热风扇、控制器风扇、机箱风扇、辅助风扇） |
| fans.{key}.rpm | number | 是 | 风扇转速 (RPM) |
| printAxesInfo | object | 是 | 打印轴信息 |
| printAxesInfo.x | number | 是 | X轴当前位置 |
| printAxesInfo.y | number | 是 | Y轴当前位置 |
| printAxesInfo.z | number | 是 | Z轴当前位置 |
| printAxesInfo.xHomed | boolean | 是 | X轴是否已归零 |
| printAxesInfo.yHomed | boolean | 是 | Y轴是否已归零 |
| printAxesInfo.zHomed | boolean | 是 | Z轴是否已归零 |
| printAxesInfo.homedAxes | string | 是 | 已归零的轴信息（如"xyz"） |
| machineStatus | object | 是 | 机器状态信息 |
| machineStatus.status | number | 是 | 主状态码（参考MachineMainStatus枚举） |
| machineStatus.subStatus | number | 是 | 子状态码（参考MachineSubStatus枚举） |
| machineStatus.exceptionStatus | array | 是 | 异常状态码列表 |
| machineStatus.supportProgress | boolean | 是 | 是否支持进度显示 |
| machineStatus.progress | number | 是 | 当前进度百分比 (0-100) |
| printStatus | object | 是 | 打印状态信息 |
| printStatus.filename | string | 是 | 当前打印文件名 |
| printStatus.totalTime | number | 是 | 总打印时长（秒） |
| printStatus.currentTime | number | 是 | 当前已打印时长（秒） |
| printStatus.estimatedTime | number | 是 | 预计总打印时长（秒） |
| printStatus.totalLayer | number | 是 | 总层数 |
| printStatus.currentLayer | number | 是 | 当前层数 |
| printStatus.progress | number | 是 | 打印进度百分比 |

### 3. 打印控制

#### 3.1 开始打印

**请求**
```json
{
  "id": "msg_005",
  "version": "1.0",
  "method": 1012,
  "data": {
    "deviceId": "elegoo_printer_001",
    "storageLocation": "SD",
    "fileName": "test_model.gcode",
    "autoBedLeveling": true,
    "heatedBedType": 1,
    "enableTimeLapse": true
  }
}
```

**请求字段说明**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| deviceId | string | 是 | 设备唯一标识符 |
| storageLocation | string | 是 | 存储位置（"local"本地存储、"udisk"U盘、"sdcard"SD卡） |
| fileName | string | 是 | 要打印的文件名 |
| autoBedLeveling | boolean | 否 | 是否启用自动调平 |
| heatedBedType | number | 否 | 热床类型（0表示光滑低温板，1表示纹理高温板） |
| enableTimeLapse | boolean | 否 | 是否启用延时摄影 |

#### 3.2 暂停/恢复/停止打印

**暂停打印**
```json
{
  "id": "msg_006",
  "version": "1.0",
  "method": 1013,
  "data": {
    "deviceId": "elegoo_printer_001"
  }
}
```

**恢复打印**
```json
{
  "id": "msg_007",
  "version": "1.0",
  "method": 1014,
  "data": {
    "deviceId": "elegoo_printer_001"
  }
}
```

**停止打印**
```json
{
  "id": "msg_008",
  "version": "1.0",
  "method": 1015,
  "data": {
    "deviceId": "elegoo_printer_001"
  }
}
```

### 4. 轴控制

#### 4.1 轴回零

**请求**
```json
{
  "id": "msg_009",
  "version": "1.0",
  "method": 1016,
  "data": {
    "deviceId": "elegoo_printer_001",
    "axes": "xyz"
  }
}
```

**请求字段说明**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| deviceId | string | 是 | 设备唯一标识符 |
| axes | string | 是 | 要回零的轴（如"x", "y", "z", "xyz"等组合） |

#### 4.2 移动轴

**请求**
```json
{
  "id": "msg_010",
  "version": "1.0",
  "method": 1017,
  "data": {
    "deviceId": "elegoo_printer_001",
    "axes": "x",
    "distance": 10.0
  }
}
```

**请求字段说明**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| deviceId | string | 是 | 设备唯一标识符 |
| axes | string | 是 | 要移动的轴（如"x", "y", "z"） |
| distance | number | 是 | 移动距离（毫米，正值为正方向，负值为负方向） |

### 5. 温度与风扇控制

#### 5.1 设置温度

**请求**
```json
{
  "id": "msg_011",
  "version": "1.0",
  "method": 1019,
  "data": {
    "deviceId": "elegoo_printer_001",
    "temperatures": {
      "hotend": 210.0,
      "bed": 60.0
    }
  }
}
```

**请求字段说明**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| deviceId | string | 是 | 设备唯一标识符 |
| temperatures | object | 是 | 温度设置映射表，key为温度组件名（如"heatedBed"热床、"extruder"挤出机），value为目标温度 |

#### 5.2 设置风扇速度

**请求**
```json
{
  "id": "msg_012",
  "version": "1.0",
  "method": 1021,
  "data": {
    "deviceId": "elegoo_printer_001",
    "fans": {
      "part_cooling": 100,
      "hotend": 80
    }
  }
}
```

**请求字段说明**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| deviceId | string | 是 | 设备唯一标识符 |
| fans | object | 是 | 风扇速度设置映射表，key为风扇名（如"model"模型冷却风扇、"box"机箱风扇、"aux"辅助风扇），value为速度百分比(0-100) |

#### 5.3 设置灯光亮度

**请求**
```json
{
  "id": "msg_012_1",
  "version": "1.0",
  "method": 1018,
  "data": {
    "deviceId": "elegoo_printer_001",
    "lights": {
      "main": 100
    }
  }
}
```

**请求字段说明**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| deviceId | string | 是 | 设备唯一标识符 |
| lights | object | 是 | 灯光亮度设置映射表，key为灯光名（如"main"主灯光），value为亮度百分比(0-100，0表示关闭，100表示全亮) |

#### 5.4 设置打印速度

**请求**
```json
{
  "id": "msg_012_2",
  "version": "1.0",
  "method": 1020,
  "data": {
    "deviceId": "elegoo_printer_001",
    "speedMode": 1
  }
}
```

**请求字段说明**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| deviceId | string | 是 | 设备唯一标识符 |
| speedMode | number | 是 | 打印速度挡位（0=静音、1=均衡、2=运动、3=狂暴） |

### 6. 文件管理

#### 6.1 获取文件列表

**请求**
```json
{
  "id": "msg_013",
  "version": "1.0",
  "method": 1025,
  "data": {
    "deviceId": "elegoo_printer_001",
    "storageLocation": "local",
  }
}
```

**请求字段说明**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| deviceId | string | 是 | 设备唯一标识符 |
| storageLocation | string | 是 | 存储位置（"local"本地存储、"udisk"U盘、"sdcard"SD卡） |
| directoryPath | string | 是 | 目录路径 |

**响应**
```json
{
  "id": "msg_013",
  "version": "1.0",
  "method": 1025,
  "data": {
    "code": 0,
    "message": "Success",
    "fileList": ["model1.gcode", "model2.gcode", "config.ini"]
  }
}
```

#### 6.2 上传文件

**请求**
```json
{
  "id": "msg_014",
  "version": "1.0",
  "method": 1007,
  "data": {
    "deviceId": "elegoo_printer_001",
    "storageLocation": "SD",
    "localFilePath": "C:/models/test.gcode",
    "overwriteExisting": true,
    "timeout": 600,
    "extraHeaders": {},
    "extraParams": {}
  }
}
```

**请求字段说明**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| deviceId | string | 是 | 设备唯一标识符 |
| storageLocation | string | 是 | 存储位置（"local"本地存储、"udisk"U盘、"sdcard"SD卡） |
| localFilePath | string | 是 | 本地文件路径 |
| overwriteExisting | boolean | 否 | 是否覆盖已存在的文件 |
| timeout | number | 否 | 上传超时时间（秒） |
| extraHeaders | object | 否 | 额外HTTP头信息 |
| extraParams | object | 否 | 额外上传参数 |

**进度通知**
```json
{
  "id": "notification_003",
  "version": "1.0",
  "method": 2004,
  "data": {
    "deviceId": "elegoo_printer_001",
    "fileName": "test.gcode",
    "totalBytes": 1048576,
    "transferredBytes": 524288,
    "percentage": 50.0,
    "transferSpeed": 102400
  }
}
```

**进度通知字段说明**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| deviceId | string | 是 | 设备唯一标识符 |
| fileName | string | 是 | 文件名 |
| totalBytes | number | 是 | 文件总大小（字节） |
| transferredBytes | number | 是 | 已传输大小（字节） |
| percentage | number | 是 | 传输进度百分比 |
| transferSpeed | number | 是 | 传输速度（字节/秒） |

#### 6.3 获取磁盘信息

**请求**
```json
{
  "id": "msg_014_1",
  "version": "1.0",
  "method": 1028,
  "data": {
    "deviceId": "elegoo_printer_001"
  }
}
```

**响应**
```json
{
  "id": "msg_014_1",
  "version": "1.0",
  "method": 1028,
  "data": {
    "code": 0,
    "message": "Success",
    "diskInfos": {
      "local": {
        "totalCapacity": 31457280000,
        "usedSpace": 15728640000
      },
      "udisk": {
        "totalCapacity": 8589934592,
        "usedSpace": 2147483648
      },
      "sdcard": {
        "totalCapacity": 16777216000,
        "usedSpace": 8388608000
      }
    }
  }
}
```

**响应字段说明**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| diskInfos | object | 是 | 磁盘信息映射表，key为存储介质名（如"local"本地存储、"udisk"U盘、"sdcard"SD卡），value为磁盘信息对象 |
| diskInfos.{key}.totalCapacity | number | 是 | 总容量（字节） |
| diskInfos.{key}.usedSpace | number | 是 | 已使用空间（字节） |

### 7. 打印任务管理

#### 7.1 获取打印任务列表

**请求**
```json
{
  "id": "msg_015",
  "version": "1.0",
  "method": 1022,
  "data": {
    "deviceId": "elegoo_printer_001"
  }
}
```

**响应**
```json
{
  "id": "msg_015",
  "version": "1.0",
  "method": 1022,
  "data": {
    "code": 0,
    "message": "Success",
    "taskIds": ["task_001", "task_002", "task_003"]
  }
}
```

#### 7.2 获取任务详情

**请求**
```json
{
  "id": "msg_016",
  "version": "1.0",
  "method": 1023,
  "data": {
    "deviceId": "elegoo_printer_001",
    "taskIds": ["task_001", "task_002"]
  }
}
```

**响应**
```json
{
  "id": "msg_016",
  "version": "1.0",
  "method": 1023,
  "data": {
    "code": 0,
    "message": "Success",
    "taskDetails": [
      {
        "taskId": "task_001",
        "thumbnail": "base64_image_data",
        "taskName": "test_model.gcode",
        "beginTime": 1641024000,
        "endTime": 1641031200,
        "taskStatus": 2,
        "alreadyPrintLayer": 200,
        "md5": "abc123def456",
        "currentLayerTotalVolume": 15.5,
        "timeLapseVideoStatus": 1,
        "timeLapseVideoUrl": "http://192.168.1.100/videos/task_001.mp4",
        "timeLapseVideoSize": 52428800,
        "timeLapseVideoDuration": 120
      }
    ]
  }
}
```

### 8. 其他功能

#### 8.1 视频流

**请求**
```json
{
  "id": "msg_017",
  "version": "1.0",
  "method": 1029,
  "data": {
    "deviceId": "elegoo_printer_001"
  }
}
```

**响应**
```json
{
  "id": "msg_017",
  "version": "1.0",
  "method": 1029,
  "data": {
    "code": 0,
    "message": "Success",
    "streamUrl": "rtsp://192.168.1.100:554/stream"
  }
}
```

#### 8.2 进料/出料

**进料**
```json
{
  "id": "msg_018",
  "version": "1.0",
  "method": 1031,
  "data": {
    "deviceId": "elegoo_printer_001"
  }
}
```

**出料**
```json
{
  "id": "msg_019",
  "version": "1.0",
  "method": 1032,
  "data": {
    "deviceId": "elegoo_printer_001"
  }
}
```

## 错误处理

### 通用错误响应格式

```json
{
  "id": "msg_xxx",
  "version": "1.0",
  "method": <原请求方法>,
  "data": {
    "code": 2,
    "message": "Internal server error"
  }
}
```

### 常见错误情况

1. **版本不兼容**: 返回状态码 100（VERSION_NOT_SUPPORTED）、101（VERSION_TOO_OLD）或 102（VERSION_TOO_NEW）
2. **设备未连接**: 返回状态码 8（NOT_FOUND）或 1001（DEVICE_OFFLINE）
3. **权限不足**: 返回状态码 7（PERMISSION_DENIED）或 200（UNAUTHORIZED）
4. **设备忙碌**: 返回状态码 1000（DEVICE_BUSY）
5. **文件传输错误**: 返回状态码 300（FILE_TRANSFER_FAILED）或 301（FILE_NOT_FOUND）
6. **参数无效**: 返回状态码 3（INVALID_PARAMETER）
7. **连接错误**: 返回状态码 12（CONNECTION_ERROR）

## 最佳实践

1. **消息ID唯一性**: 确保每个请求的ID都是唯一的
2. **版本协商**: 连接时首先进行版本协商
3. **错误处理**: 正确处理各种错误状态码
4. **设备状态监听**: 及时处理设备状态变更通知
5. **超时处理**: 为长时间操作设置合适的超时时间
6. **重连机制**: 在连接断开时实现自动重连
7. **进度监听**: 对于文件传输等长时间操作，监听进度通知

## 示例代码

### JavaScript WebSocket 客户端示例

```javascript
class ElegooWebSocketClient {
  constructor(deviceIp) {
    // 使用标准的WebSocket地址格式
    const url = `ws://${deviceIp}:6382/ws`;
    this.ws = new WebSocket(url);
    this.messageId = 0;
    this.pendingRequests = new Map();
    
    this.ws.onmessage = this.handleMessage.bind(this);
    this.ws.onopen = this.handleOpen.bind(this);
    this.ws.onclose = this.handleClose.bind(this);
    this.ws.onerror = this.handleError.bind(this);
  }
  
  generateMessageId() {
    return `msg_${++this.messageId}`;
  }
  
  sendRequest(method, data = {}) {
    const message = {
      id: this.generateMessageId(),
      version: "1.0",
      method: method,
      data: data
    };
    
    return new Promise((resolve, reject) => {
      this.pendingRequests.set(message.id, { resolve, reject });
      this.ws.send(JSON.stringify(message));
    });
  }
  
  handleMessage(event) {
    const message = JSON.parse(event.data);
    
    // 处理响应消息
    if (this.pendingRequests.has(message.id)) {
      const { resolve } = this.pendingRequests.get(message.id);
      this.pendingRequests.delete(message.id);
      resolve(message);
      return;
    }
    
    // 处理通知消息
    this.handleNotification(message);
  }
  
  handleNotification(message) {
    switch (message.method) {
      case 2000: // ON_DEVICE_STATUS
        this.onDeviceStatus(message.data);
        break;
      case 2004: // ON_FILE_TRANSFER_PROGRESS
        this.onFileTransferProgress(message.data);
        break;
      // ... 其他通知处理
    }
  }
  
  // API 方法
  async connectDevice(deviceId, credentials) {
    return this.sendRequest(1000, {
      deviceId: deviceId,
      ...credentials
    });
  }
  
  async startPrint(deviceId, printConfig) {
    return this.sendRequest(1012, {
      deviceId: deviceId,
      ...printConfig
    });
  }
  
  async getDeviceStatus(deviceId) {
    return this.sendRequest(1003, { deviceId });
  }
}

// 使用示例
const client = new ElegooWebSocketClient('192.168.1.100');
```

## 常用字段取值说明

### 存储位置 (storageLocation)
| 值 | 说明 |
|-----|------|
| local | 本地存储 |
| udisk | U盘 |
| sdcard | SD卡 |

### 温度组件名称 (temperatures 对象的 key)
| 值 | 说明 |
|-----|------|
| heatedBed | 热床 |
| extruder | 挤出机/热端 |
| box | 机箱温度 |

### 风扇名称 (fans 对象的 key)
| 值 | 说明 |
|-----|------|
| model | 模型冷却风扇 |
| heatsink | 热端散热风扇 |
| controller | 控制器风扇 |
| box | 机箱风扇 |
| aux | 辅助风扇 |

### 灯光名称 (lights 对象的 key)
| 值 | 说明 |
|-----|------|
| main | 主灯光 |

### 打印速度挡位 (speedMode)
| 值 | 说明 |
|-----|------|
| 0 | 静音 |
| 1 | 均衡 |
| 2 | 运动 |
| 3 | 狂暴 |

### 热床类型 (heatedBedType)
| 值 | 说明 |
|-----|------|
| 0 | 光滑低温板 |
| 1 | 纹理高温板 |

### 打印任务状态 (taskStatus)
| 值 | 说明 |
|-----|------|
| 0 | 其他状态 |
| 1 | 完成 |
| 2 | 异常状态 |
| 3 | 停止 |

### 延时摄影状态 (timeLapseVideoStatus)
| 值 | 说明 |
|-----|------|
| 0 | 未拍摄延时摄影文件 |
| 1 | 存在延时摄影文件 |
| 2 | 延时摄影文件已删除 |
| 3 | 延时摄影生成中 |
| 4 | 延时摄影生成失败 |

---

*本文档基于 Elegoo Link SDK v1.0 生成，如有疑问请参考源代码或联系技术支持。*
