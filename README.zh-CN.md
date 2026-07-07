# 78HAM NRL ESP32

`78ham-nrlesp32` 是一个面向 NRLLink 的开源 ESP32-S3 网络电台终端固件。

当前主线已经切换到 **ESP-IDF**，早期 Arduino / PlatformIO 原型仍保留在仓库中作为参考和备用。

## 硬件目标

当前设计面向以下硬件：

- 主控：ESP32-S3-N16R8
- 屏幕：1.77 寸 240x320 COG / ST7789V SPI 屏
- 麦克风：INMP441 I2S 数字麦克风
- 功放：MAX98357AETE I2S 数字功放
- 按键：PTT、音量+、音量-、频道上、频道下、开机/功能键
- 供电：首版先直接供电，暂不做电池管理
- 用途：纯网络 NRLLink 终端，不桥接外部模拟电台

## 当前状态

ESP-IDF 主线已经具备基础骨架：

- NVS 初始化和默认配置
- NRL2 / NRL21 48 字节包头封装
- Type 2 心跳包
- Type 7 频道/群组切换包
- Type 8 Opus 包解析入口
- WiFi STA 连接框架
- UDP socket，预留语音优先级 `IP_TOS=0xC0`
- ST7789V SPI 屏幕初始化
- 6 个按键 GPIO 初始化
- 音频模块占位

尚未完成：

- INMP441 采集
- MAX98357A 播放
- Opus 编解码实装
- AP 热点网页配网
- TFT 中文 UI / 二维码配网显示
- 完整编译验证

## 目录结构

```text
78ham-nrlesp32/
  CMakeLists.txt              ESP-IDF 工程入口
  sdkconfig.defaults          ESP-IDF 默认配置
  partitions.csv              分区表
  main/                       ESP-IDF 主线源码
    app_main.cpp              固件入口
    app_config.*              配置结构和 NVS 存储
    board_pins.h              硬件管脚定义
    nrl21.*                   NRL2/NRL21 编解码
    network.*                 WiFi + UDP + 心跳
    display.*                 ST7789V 显示
    keys.*                    按键初始化
    audio.*                   音频模块占位
  src/                        早期 Arduino/PlatformIO 原型
  include/                    早期 Arduino/PlatformIO 配置头
  docs/                       接线和架构文档
```

## 屏幕接线

当前 ST7789V 7 线屏丝印：

```text
CS DC RST SDA SCK VC GND
```

接线：

| 屏幕 | ESP32-S3 |
|---|---:|
| `CS` | `GPIO10` |
| `DC` | `GPIO9` |
| `RST` | `GPIO8` |
| `SDA` | `GPIO11` |
| `SCK` | `GPIO12` |
| `VC` | `3.3V` |
| `GND` | `GND` |

该屏幕没有单独背光脚，`VC` 同时给背光供电。

更多接线说明见 [docs/wiring.md](docs/wiring.md)。

## ESP-IDF 构建

先打开 ESP-IDF PowerShell 环境，然后执行：

```powershell
cd C:\Users\Admin\Documents\GitHub\78ham-nrlesp32
idf.py set-target esp32s3
idf.py build
```

烧录和串口监视：

```powershell
idf.py -p COMx flash monitor
```

当前机器还没有检测到 ESP-IDF 环境，所以仓库内代码尚未完成实际 `idf.py build` 验证。

## NRLLink 协议

设备使用 NRL2 / NRL21 UDP 协议接入 `nrllink` 后端，默认端口为 `60050`。

首版目标包类型：

| Type | 用途 | 状态 |
|---:|---|---|
| `2` | 心跳 | 已写入骨架 |
| `7` | 切换群组/频道 | 已写入骨架 |
| `8` | Opus 16k 语音 | 入口已预留，编解码待接入 |

语音目标：

```text
INMP441 -> I2S RX -> 16k PCM -> Opus encode -> NRL2 Type 8 -> UDP
UDP -> NRL2 Type 8 -> Opus decode -> 16k PCM -> I2S TX -> MAX98357A
```

## 配网设计

计划中的首次配网逻辑：

```text
首次启动或配置无效
  -> 开启 78HAM-ESP32-XXXXXX 热点
  -> 生成并保存 10 位随机密码
  -> 屏幕显示热点信息和二维码
  -> 用户访问 192.168.4.1
  -> 保存 WiFi、呼号、服务器、频道
  -> 连接 WiFi 并进入主界面
```

频道设计采用“只保存要在设备上显示的频道”，不保存服务器全部频道，节省内存并简化 UI。

## 开源协议

本项目使用 MIT License，详见 [LICENSE](LICENSE)。
