# Fatigue-detection-system-based-on-FH8626v300l
【FH8626v300l】基于rtthread与FH8626v300l的疲劳检测系统
## 系统功能概述
随着高性能嵌入式芯片以及人工智能技术与视觉神经网络的高速发展，智能检测技术已在智能安防、智慧医疗、智能驾驶、工业检测等领域得到广泛应用。

疲劳检测系统是一款基于人脸检测和行为分析的智能应用，旨在通过实时监测用户的面部状态，判断其疲劳程度，若发现疲劳，则进行框选提示。系统主要目标在于应用于驾驶安全、工业生产监控等场景，帮助用户及时发现疲劳状态，避免安全隐患。
本作品主要依赖fh8626v300l mediademo模块进行开发，结合富瀚微提供的nn检测模型，首先框选出人脸，然后基于用户的特征点进行疲劳状态分析，若用户疲劳状态高，则进行提升。



## 系统实现功能
RTSP视频流传输：通过VLC软件播放视频流
疲劳检测：检测到疲劳状态画框的方式标注

### 人脸检测
- **功能描述**：通过加载预训练的神经网络模型（**case30083_persondet.nbg**），检测视频流或图像中的人脸区域。
- **实现方式**：
  - 使用芯片内置的 NNA（神经网络加速器）进行模型推理。
  - 支持多种输入格式的图像数据，经过预处理后输入模型。
  - 输出人脸的位置信息（边界框）。

### 疲劳状态分析
- **功能描述**：基于人脸检测结果，进一步分析用户的面部特征（如眼睛、嘴巴的状态），判断是否存在疲劳迹象。
- **实现方式**：
  - 通过算法分析眼睛闭合时间、眨眼频率等指标。
  - 检测打哈欠等行为，结合时间维度进行综合判断。
  - 提供疲劳等级评分，输出报警信号。


---

## RT-Thread 使用情况

### 多任务管理
- 系统通过 RT-Thread app/media_demo文件夹进行开发：
  - 人脸检测任务：负责调用模型推理接口，处理视频流数据。
  - 疲劳分析任务：独立线程处理检测结果，进行疲劳状态分析。
  - 数据展示任务：将检测结果发送到显示设备或存储设备。

### 设备驱动
- 使用 RT-Thread 的设备驱动框架管理硬件设备：
  - 摄像头驱动：采集视频流数据。
  - 显示设备驱动：实时显示检测结果。
  - 报警设备驱动：在检测到疲劳状态时触发报警。

### 内存管理
- 系统通过 RT-Thread 的动态内存分配机制优化资源使用：
  - 为每个任务分配独立的内存区域，避免任务间干扰。
  - 使用内存池管理大规模图像数据，提升内存利用率。

---

## 硬件框架说明

### 核心硬件
串口、开发主板、电源线、网线、一对sensor板（CV2005），硬件环境搭建请看官网说明[硬件准备 (yinxiang.com)](https://static.app.yinxiang.com/verse/share/c4hFYioqQFS-PTj_JmDGcg/I1Yp9Gn3Ske2TUum9z1F9A/?fromNote=I1Yp9Gn3Ske2TUum9z1F9A&flatten=false)

---

## 软件框架说明
开发平台Ubuntu22.04 wsl2+vscode连接
### 系统架构
疲劳检测系统的软件架构采用模块化设计，主要包括以下部分：
1. **模型推理模块**：
   - 加载并运行人脸检测模型。
2. **数据处理模块**：
   - 图像数据的预处理和格式转换。
   - 检测结果的后处理（如边界框绘制）。
3. **疲劳分析模块**：
   - 分析面部特征，判断疲劳状态。
   - 输出疲劳等级评分和报警信号。
4. **任务调度模块**：
   - 基于 RT-Thread 的任务管理机制，调度各模块任务。
5. **设备驱动模块**：
   - 管理摄像头、显示设备和报警设备。

---

## 软件模块说明

### 模型推理模块
- **模型文件**：case30083_persondet.nbg，富瀚微官方提供的nbg文件，本来想自己训练的，但是训练需要咨询官方要对应资料，详情可看[RT-Thread-关于fh8626v300开发版深度学习模型转换问题RT-Thread问答社区 - RT-Thread](https://club.rt-thread.org/ask/question/7d5714677e788d44.html)。
- **实现文件**：
  - `src/application.c`：调用模型推理接口，处理视频流数据。
- **功能特点**
  - 输出人脸的边界框坐标。

### 数据处理模块
- **实现文件**：
  - `src/application.c`：对检测结果进行后处理。
- **功能特点**：
  - 高效的数据预处理和格式转换。
  - 支持实时处理

### 疲劳分析模块
- **实现文件**：
  - `src/application.c`：分析面部特征，判断疲劳状态。
- **功能特点**：
  - 支持多种疲劳状态的检测（如眼睛闭合、打哈欠）。
  - 提供疲劳等级评分和报警信号。

### 任务调度模块
- **实现方式**：
  - 基于 RT-Thread 的多任务机制，调度各模块任务。
- **功能特点**：
  - 支持任务优先级设置，保证关键任务的实时性。
  - 提供任务间通信机制，确保数据流畅传递。

### 设备驱动模块
- **实现方式**：
  - 使用 RT-Thread 的设备驱动框架管理硬件设备。
- **功能特点**：
  - 支持摄像头、显示设备和报警设备的接入。
  - 提供统一的设备接口，便于扩展。

---
## 代码模块
### 1. **人脸检测**
```c
if (nna_init(0, &det_hdl, PERSON_DET, (FH_CHAR *)det_model, 640, 640) != 0)
{
    rt_kprintf("face det init failed\n");
    return -1;
}
```
- **功能**：初始化人脸检测模型，加载 **case30083_persondet.nbg**。

---

### 2. **图像裁剪与灰度转换**
```c
uint8_t *crop_and_gray(const uint8_t *src_buf, int src_w, int src_h, int stride_bytes, int crop_x, int crop_y, int crop_w, int crop_h)
{
    // ...existing code...
    uint8_t gray = (uint8_t)clipf((0.299f * r + 0.587f * g + 0.114f * b), 0.0f, 255.0f);
    out[ry * crop_w + rx] = gray;
    // ...existing code...
}
```
- **功能**：从原始 ARGB 图像中裁剪出眼部区域，并转换为灰度图。
- **算法**：使用标准加权公式将 RGB 转换为灰度值。

---

### 3. **灰度图统计分析**
```c
static void calc_mean_std(const uint8_t *buf, int w, int h, float *mean, float *std)
{
    // ...existing code...
    *mean = (float)sum / (float)n;
    float var = ((float)sum2 / (float)n) - (*mean) * (*mean);
    *std = var > 0.0f ? sqrtf(var) : 0.0f;
}
```
- **功能**：计算灰度图的均值和标准差。
- **用途**：均值用于判断眼睛亮度，标准差用于判断眼部纹理特征。

---

### 4. **疲劳状态判断**
```c
if (mean_l < g_eye_mean_thresh && mean_r < g_eye_mean_thresh)
{
    rt_kprintf("face[%d] maybe eyes closed (mean L=%f R=%f)\n", i, mean_l, mean_r);
}
else if ((std_l < g_eye_std_thresh) && (std_r < g_eye_std_thresh))
{
    rt_kprintf("face[%d] low texture in eye region (maybe closed)\n", i);
}
```
- **功能**：基于灰度均值和标准差判断眼睛是否闭合。
- **逻辑**：
  - 均值低于阈值：可能闭眼。
  - 标准差低于阈值：眼部纹理较少，可能闭眼。

---

### 5. **全局参数设置**
```c
static float g_eye_mean_thresh = 60.0f; /* 0-255 */
static float g_eye_std_thresh = 10.0f;
static float g_eye_w_ratio = 0.30f;
static float g_eye_h_ratio = 0.15f;
static float g_eye_y_ratio = 0.22f;
static float g_left_eye_x_ratio = 0.12f;
static float g_right_eye_x_ratio = 0.58f;
```
- **功能**：定义全局参数，包括眼部区域比例和检测阈值。
- **可调性**：通过 `finsh` 命令动态调整参数。


---

### 7. **主线程**
```c
void user_main(void)
{
    rt_kprintf("fatigue_detection_demo (lightweight) ready. Use finsh commands: run_fatigue_demo, set_eye_thresh, show_eye_thresh\n");
    while (1)
    {
        rt_thread_delay(RT_TICK_PER_SECOND * 60);
    }
}
```
- **功能**：初始化系统并保持线程存活。
- **提示**：通过 `finsh` 命令运行检测或调整参数。


---

## 参考资料
1、FH8626V300L_double_cv2005_常电模式出图说明.pdf
2、[FullHan-FH8626V300L上手指南 · 语雀 (yuque.com)](https://www.yuque.com/muchen-fyewf/qev0eo/bzooutzs01dad1zn)
3、AOV DEMO使用指南.pdf
4、RT-Thread SDK开发指南.pdf
5、智能模型的编译[手把手带你玩转智能模型——RT-Thread×富瀚微FH8626V300L初级智能案例实战 | 技术集结 (qq.com)](https://mp.weixin.qq.com/s/ZLvFkoEEyIfYHXJ5X6z6Qw)
6、https://static.app.yinxiang.com/
