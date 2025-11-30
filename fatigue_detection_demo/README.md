fatigue_detection_demo
======================

目的：在 FH8626V300 平台上使用板载 NNA 做驾驶/作业场景的疲劳检测示例（骨架）。

思路（推荐）：
- 使用人脸检测模型 `face_det.nbg` 找到人脸框。
- 对每个检测到的人脸，使用面部 68 点关键点模型 `face_68_kpt.nbg`（`NN_TYPE = FACE_68_KPT_DET`）得到面部关键点。
- 基于 68 点关键点计算 EAR（Eye Aspect Ratio）和 MAR（Mouth Aspect Ratio），结合时间序列判断疲劳（长期低 EAR 或张嘴频繁视为疲劳/打哈欠）。

目录说明：
- `Makefile`：应用构建片段，集成到 RT-Thread 顶层构建系统。
- `resource/`：放置模型文件和测试图像（请把 `.nbg` 模型放到 ROMFS 或 rootfs 中）。
- `src/application.c`：示例程序骨架，包含模型加载、推理流程和疲劳判定逻辑（需根据你实际模型输出做少量适配）。

使用步骤（示例）：
1. 把 `resource/` 下的模型复制到你的固件 rootfs 输入目录（见 `Makefile` 的 `copy_resources`）。
2. 在 RT-Thread 顶层运行 `make` 构建固件。
3. 烧写固件并在串口终端观察输出。

注意：
- 示例代码中包含 `TODO` 注释，说明需要根据模型的实际输出格式（NNA 返回的后处理数据）来实现关键点解析。当前代码提供计算 EAR/MAR 的逻辑和判定策略。
- 如果你打算使用其他模型（比如直接输出疲劳/清醒概率的二分类模型），只需替换模型并在代码中调整后处理即可。

WSL2 (Ubuntu 22.04) 下完整构建、打包与测试步骤
--------------------------------------------------

以下步骤假设你在 WSL2 的 Ubuntu 22.04 中操作，并且源码树位于 WSL 可访问路径（例如 `~/workspace/FH8626V300_RT_V1.0.0_20250627/rt-thread`）。

1) 准备交叉编译工具链（若已有可跳过）

 - 推荐使用 arm-none-eabi 或厂商提供的交叉工具链。若仓库已有 `toolchain`，请按仓库说明设置 `CROSS_COMPILE`。

2) 把模型和测试图像放到 `resource/`

 - 把你的 `face_det.nbg` 放到 `rt-thread/app/fatigue_detection_demo/resource/`。
 - 如需合成 ARGB raw 测试图像，可在 WSL 中使用示例脚本生成：

```bash
# 进入项目根（包含 rt-thread 目录）
cd ~/workspace/FH8626V300_RT_V1.0.0_20250627

# 安装 pillow（仅在 WSL 中用于生成 raw 测试图像）
python3 -m pip install --user pillow

# 生成 ARGB raw（例如把 local/test.jpg 缩放到 640x480）
python3 rt-thread/app/fatigue_detection_demo/resource/convert_to_argb.py --in local/test.jpg --out rt-thread/app/fatigue_detection_demo/resource/test.argb --w 640 --h 480
```

3) 把 `resource/` 里的文件复制到固件 rootfs 输入目录（build/rootfs 或 build/root）

```bash
cd ~/workspace/FH8626V300_RT_V1.0.0_20250627
make copy_resources || cp -r rt-thread/app/fatigue_detection_demo/resource/* build/rootfs/ || cp -r rt-thread/app/fatigue_detection_demo/resource/* build/root/
```

4) 构建固件

```bash
# 在 rt-thread 根目录直接 make（会调用交叉编译器）
make -j$(nproc)
```

注意：如果需要指定交叉前缀，可在 make 前导出：

```bash
export CROSS_COMPILE=arm-none-eabi-
make -j4
```

5) 烧写与启动

- 烧写方式依板子与 bootloader 有所不同；常见流程：通过串口/USB 升级工具、厂商烧录工具或使用 `make flash`（若仓库提供）。请查看仓库顶层 `README` 或 `make_*` 脚本。
- 烧写后，用串口终端（115200 8N1）查看输出，你应该看到 demo 的打印信息，例如 `fatigue_detection_demo start`。

常见问题排查
- 如果 `nna_init` 返回失败：检查模型路径是否存在于 rootfs（固件镜像内），并确认模型是设备/SDK 支持的 `.nbg` 格式。
- 如果 `make` 报找不到交叉编译器：安装或指定正确的 `CROSS_COMPILE` 前缀并确认工具链在 PATH 中。
- 如果看不到 `test.argb`：确认 `copy_resources` 步骤已成功把文件复制进 `build/rootfs` 或 `build/root`，并最终打包到镜像中。

已完成项（生成合成图与 finsh 调参支持）
------------------------------------

1) 合成 ARGB raw 生成脚本

- 我在 `resource/` 下加入 `gen_synthetic_argb.py`，在 WSL (Ubuntu 22.04) 中你可以运行：

```bash
python3 rt-thread/app/fatigue_detection_demo/resource/gen_synthetic_argb.py --out rt-thread/app/fatigue_detection_demo/resource/test.argb --w 640 --h 480
```

该脚本会生成一个灰色背景、中心带“人脸”椭圆与“眼睛”暗条的 `test.argb`，用于占位测试和调试裁剪/亮度检测逻辑。

2) finsh 命令（板上调参）

- 我已在程序中加入以下 finsh 命令：
	- `run_fatigue_demo`：运行一次检测并打印结果（读取 `/rom/models/test.argb`）。
	- `set_eye_thresh <mean> <std>`：设置亮度均值阈值与方差阈值（整数）。
	- `show_eye_thresh`：打印当前阈值与眼区裁剪比例。

示例（串口 finsh）：

```bash
# 运行一次检测并查看输出
run_fatigue_demo

# 查看当前阈值
show_eye_thresh

# 调整阈值（例如把 mean 降低到 50）
set_eye_thresh 50 8

# 再次运行
run_fatigue_demo
```

说明：finsh 命令依赖 RT-Thread 的 shell/finsh 支持（默认仓库已启用）。你可以在串口终端直接执行上述命令进行实时调参。


