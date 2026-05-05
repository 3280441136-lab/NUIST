# NUIST Tri-Modal Intrusion Detector

这是一个基于 Waveshare ESP32-P4-WIFI6 的三模态行人入侵检测报警装置工程。

当前阶段只做第一件事：把 ESP-IDF 工程规范起来，让队友从 GitHub 克隆后能看懂、能用 VS Code 编译、能烧录，并能在串口日志里确认程序已经跑进 `app_main()`。传感器、摄像头和 AI 后面再逐步接入。

## 硬件

- 开发板：Waveshare ESP32-P4-WIFI6
- 主控：ESP32-P4
- 联网芯片：ESP32-C6
- Flash：32MB Nor Flash
- PSRAM：32MB PSRAM

硬件规格以 Waveshare 官方页面为准：ESP32-P4-WIFI6 板载 ESP32-P4、ESP32-C6、32MB PSRAM 和 32MB Nor Flash。

ESP32-P4 还要特别注意芯片版本：这块板使用 P4 v1.x，所以工程配置为支持 `rev <3.0`，最低支持版本选择 `Rev v1.0`。ESP-IDF 里 P4 v1.x 和 v3.x 的底层硬件差异很大，不能混着配置。

## 工程文件怎么理解

如果你以前用 Keil，可以这样类比：

- `CMakeLists.txt` 类似 Keil 的 `.uvprojx` 工程文件，告诉编译器这个工程怎么编译。
- `main/main.c` 类似你以前写用户代码的 `main.c`，但 ESP-IDF 的入口函数叫 `app_main()`。
- `sdkconfig.defaults` 是团队共享的默认配置，类似大家约定好用同一块芯片、同一套基础工程选项。
- `sdkconfig` 是 ESP-IDF 在你电脑上生成的本地配置文件，不提交到 Git。
- `build/` 是编译输出目录，类似 Keil 的 `Objects/`，不提交到 Git。

## 推荐开发方式：VS Code

对新手最推荐的方式是使用 VS Code + Espressif IDF 插件。

1. 用 VS Code 打开本工程文件夹。
2. 确认左下角 ESP-IDF 插件已经选择好目标芯片 `esp32p4`。
3. 点击底部工具栏的小扳手图标进行 Build。
4. 点击烧录按钮进行 Flash。
5. 点击 Monitor 查看串口日志。

这就像 Keil 里点 Build 按钮一样：先让 IDE 帮你把环境、路径、编译器都串起来，不需要一开始就在普通 Windows PowerShell 里手动配置。

注意：如果你本地已经有旧的 `sdkconfig`，ESP-IDF 会优先沿用它。若串口里仍显示 2MB Flash、PSRAM 未启用，或 P4 版本不是 `Rev v1.0`，请在 VS Code 的 SDK Configuration Editor 里检查 Flash、PSRAM 和芯片版本配置；必要时使用 ESP-IDF 插件提供的清理构建功能后重新 Build。

## 串口日志怎么看

ESP-IDF 官方启动过程本来就会打印很多系统日志，例如 bootloader、Flash、分区表、heap 初始化等信息。



## Git 协作规则

提交到 Git：

- `README.md`
- `CMakeLists.txt`
- `main/CMakeLists.txt`
- `main/main.c`
- `sdkconfig.defaults`
- `.gitignore`

不要提交到 Git：

- `build/`
- `sdkconfig`
- `sdkconfig.old`
- `.vscode/` 里的个人路径配置
- `.bin`、`.elf`、`.map` 等编译产物

## 下一步

1. 在 VS Code 里点小扳手，确认工程能编译。
2. 烧录当前日志版程序，确认串口里能看到 `NUIST` 应用日志。
3. 再接一个最简单的 GPIO 输出或输入，学习 ESP-IDF 的 GPIO API。
4. 后续按 PIR、毫米波、LED/蜂鸣器、摄像头、AI 的顺序推进。
