#include <rtthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "nna_config.h"

/*
 * 轻量级疲劳检测 Demo（基于人脸检测 + 眼区亮度/闭合率估计）
 * 目标：在没有关键点模型或资源受限时，提供一个可运行的粗略疲劳检测方法。
 *
 * 思路：
 * 1) 使用人脸检测模型得到人脸框（相对坐标）
 * 2) 按固定比例在脸框上估计左右眼区域并裁剪
 * 3) 将裁剪区域从 ARGB 转为灰度，计算平均亮度与方差作为“睁眼指标”
 * 4) 基于阈值判断是否疑似闭眼/打瞌睡（演示用，生产应做时间序列与自适应阈值）
 */

static inline float clipf(float v, float lo, float hi)
{
    if (v < lo)
        return lo;
    if (v > hi)
        return hi;
    return v;
}

/* 从 ARGB8888 raw buffer 裁剪并转换为灰度图，返回新分配的灰度 buffer（uint8_t），以及宽高
 * src_buf: 指向 ARGB 原始数据 (A R G B)
 * src_w, src_h: 原始图像尺寸
 * stride_bytes: 原始每行字节数
 * crop_x,y,w,h: 裁剪区域（像素坐标）
 */
static uint8_t *crop_and_gray(const uint8_t *src_buf, int src_w, int src_h, int stride_bytes, int crop_x, int crop_y, int crop_w, int crop_h)
{
    if (!src_buf || crop_w <= 0 || crop_h <= 0)
        return NULL;
    if (crop_x < 0)
        crop_x = 0;
    if (crop_y < 0)
        crop_y = 0;
    if (crop_x + crop_w > src_w)
        crop_w = src_w - crop_x;
    if (crop_y + crop_h > src_h)
        crop_h = src_h - crop_y;

    uint8_t *out = (uint8_t *)malloc(crop_w * crop_h);
    if (!out)
        return NULL;

    for (int ry = 0; ry < crop_h; ry++)
    {
        const uint8_t *row = src_buf + (crop_y + ry) * stride_bytes + crop_x * 4;
        for (int rx = 0; rx < crop_w; rx++)
        {
            uint8_t a = row[0];
            uint8_t r = row[1];
            uint8_t g = row[2];
            uint8_t b = row[3];
            /* 灰度：标准加权 */
            uint8_t gray = (uint8_t)clipf((0.299f * r + 0.587f * g + 0.114f * b), 0.0f, 255.0f);
            out[ry * crop_w + rx] = gray;
            row += 4;
        }
    }
    return out;
}

/* 计算灰度图像的均值和标准差 */
static void calc_mean_std(const uint8_t *buf, int w, int h, float *mean, float *std)
{
    if (!buf || w <= 0 || h <= 0)
    {
        *mean = 0.0f;
        *std = 0.0f;
        return;
    }
    uint64_t sum = 0;
    uint64_t sum2 = 0;
    int n = w * h;
    for (int i = 0; i < n; i++)
    {
        uint8_t v = buf[i];
        sum += v;
        sum2 += ((uint64_t)v * (uint64_t)v);
    }
    *mean = (float)sum / (float)n;
    float var = ((float)sum2 / (float)n) - (*mean) * (*mean);
    *std = var > 0.0f ? sqrtf(var) : 0.0f;
}

/* 全局可调阈值（可通过 finsh 命令调节） */
static float g_eye_mean_thresh = 60.0f; /* 0-255 */
static float g_eye_std_thresh = 10.0f;
static float g_eye_w_ratio = 0.30f;
static float g_eye_h_ratio = 0.15f;
static float g_eye_y_ratio = 0.22f;
static float g_left_eye_x_ratio = 0.12f;
static float g_right_eye_x_ratio = 0.58f;

/* 运行一次检测（可以从 finsh 调用） */
int run_fatigue_once(void)
{
    rt_kprintf("fatigue_detection run_once\n");

    NN_HANDLE det_hdl = NULL;
    const char *det_model = "/rom/models/face_det.nbg";

    if (nna_init(0, &det_hdl, PERSON_DET, (FH_CHAR *)det_model, 640, 640) != 0)
    {
        rt_kprintf("face det init failed\n");
        return -1;
    }

    extern FH_VOID *readFile_with_len(FH_CHAR *filename, FH_UINT32 *len);
    FH_UINT32 img_len = 0;
    FH_VOID *img_buf = readFile_with_len((FH_CHAR *)"/rom/models/test.argb", &img_len);
    if (!img_buf)
    {
        rt_kprintf("no test image, exit\n");
        nna_release(0, &det_hdl);
        return -2;
    }

    const int src_w = 640;
    const int src_h = 480;
    const int stride_bytes = src_w * 4;

    NN_INPUT_DATA in = {0};
    in.data.base = 0;
    in.data.vbase = img_buf;
    in.data.size = img_len;
    in.width = src_w;
    in.height = src_h;
    in.stride = stride_bytes;
    in.imageType = IMAGE_TYPE_RGB888;

    NN_RESULT det_out = {0};
    int ret = nna_get_obj_det_result(det_hdl, in, &det_out);
    if (ret != 0)
    {
        rt_kprintf("det inference failed %d\n", ret);
    }
    else
    {
        rt_kprintf("det num=%d\n", det_out.result_det.nn_result_num);
        for (int i = 0; i < det_out.result_det.nn_result_num; i++)
        {
            NN_RESULT_DET_INFO *info = &det_out.result_det.nn_result_info[i];
            float cx = info->box.x * in.width;
            float cy = info->box.y * in.height;
            float w = info->box.w * in.width;
            float h = info->box.h * in.height;
            float left = cx - w / 2.0f;
            float top = cy - h / 2.0f;

            rt_kprintf("face[%d] box pix=(%f,%f,%f,%f) conf=%f\n", i, cx, cy, w, h, info->conf);

            int eye_w = (int)clipf(w * g_eye_w_ratio, 4.0f, w);
            int eye_h = (int)clipf(h * g_eye_h_ratio, 4.0f, h);
            int eye_y = (int)clipf(top + h * g_eye_y_ratio, 0, src_h - 1);
            int left_eye_x = (int)clipf(left + w * g_left_eye_x_ratio, 0, src_w - 1);
            int right_eye_x = (int)clipf(left + w * g_right_eye_x_ratio, 0, src_w - 1);

            uint8_t *left_eye = crop_and_gray((const uint8_t *)img_buf, src_w, src_h, stride_bytes, left_eye_x, eye_y, eye_w, eye_h);
            uint8_t *right_eye = crop_and_gray((const uint8_t *)img_buf, src_w, src_h, stride_bytes, right_eye_x, eye_y, eye_w, eye_h);

            float mean_l = 0, std_l = 0, mean_r = 0, std_r = 0;
            if (left_eye)
                calc_mean_std(left_eye, eye_w, eye_h, &mean_l, &std_l);
            if (right_eye)
                calc_mean_std(right_eye, eye_w, eye_h, &mean_r, &std_r);

            rt_kprintf("face[%d] eye mean L=%f std=%f R=%f std=%f\n", i, mean_l, std_l, mean_r, std_r);

            if (mean_l < g_eye_mean_thresh && mean_r < g_eye_mean_thresh)
            {
                rt_kprintf("face[%d] maybe eyes closed (mean L=%f R=%f)\n", i, mean_l, mean_r);
            }
            else if ((std_l < g_eye_std_thresh) && (std_r < g_eye_std_thresh))
            {
                rt_kprintf("face[%d] low texture in eye region (maybe closed)\n", i);
            }

            if (left_eye)
                free(left_eye);
            if (right_eye)
                free(right_eye);
        }
    }

    nna_release(0, &det_hdl);
    free(img_buf);

    return 0;
}

/* finsh 命令：设置与查看阈值 */
int set_eye_thresh(int mean_thresh, int std_thresh)
{
    g_eye_mean_thresh = (float)mean_thresh;
    g_eye_std_thresh = (float)std_thresh;
    rt_kprintf("set_eye_thresh: mean=%f std=%f\n", g_eye_mean_thresh, g_eye_std_thresh);
    return 0;
}
FINSH_FUNCTION_EXPORT(set_eye_thresh, set_eye_thresh <mean> <std> - set eye mean and std thresholds);

int show_eye_thresh(void)
{
    rt_kprintf("eye_mean_thresh=%f eye_std_thresh=%f\n", g_eye_mean_thresh, g_eye_std_thresh);
    rt_kprintf("eye_w_ratio=%f eye_h_ratio=%f eye_y_ratio=%f left_x_ratio=%f right_x_ratio=%f\n", g_eye_w_ratio, g_eye_h_ratio, g_eye_y_ratio, g_left_eye_x_ratio, g_right_eye_x_ratio);
    return 0;
}
FINSH_FUNCTION_EXPORT(show_eye_thresh, show_eye_thresh - show current thresholds and ratios);

int run_fatigue_demo(void)
{
    return run_fatigue_once();
}
FINSH_FUNCTION_EXPORT(run_fatigue_demo, run_fatigue_demo - run one-shot fatigue detection using /rom/models/test.argb);

void user_main(void)
{
    rt_kprintf("fatigue_detection_demo (lightweight) ready. Use finsh commands: run_fatigue_demo, set_eye_thresh, show_eye_thresh\n");
    /* Keep thread alive */
    while (1)
    {
        rt_thread_delay(RT_TICK_PER_SECOND * 60);
    }
}
