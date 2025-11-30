#!/usr/bin/env python3
"""
生成合成的 ARGB8888 raw 图像（用于占位测试），文件名默认 `test.argb`。
生成示例：
  python3 gen_synthetic_argb.py --out test.argb --w 640 --h 480

生成规则：灰色背景，中间画一个椭圆（脸），并在椭圆内画两个深色矩形（眼睛），用于验证眼区裁剪与亮度判断。
"""
import argparse
from PIL import Image, ImageDraw


def gen(out, w, h):
    img = Image.new('RGBA', (w, h), (120, 120, 120, 255))
    draw = ImageDraw.Draw(img)
    # face ellipse
    fx0 = int(w * 0.25)
    fy0 = int(h * 0.15)
    fx1 = int(w * 0.75)
    fy1 = int(h * 0.85)
    draw.ellipse([fx0, fy0, fx1, fy1], fill=(200, 180, 160, 255))
    # left eye (dark)
    lx0 = int(w * 0.36)
    ly0 = int(h * 0.38)
    lx1 = lx0 + int(w * 0.06)
    ly1 = ly0 + int(h * 0.03)
    draw.rectangle([lx0, ly0, lx1, ly1], fill=(20, 20, 20, 255))
    # right eye
    rx0 = int(w * 0.58)
    ry0 = int(h * 0.38)
    rx1 = rx0 + int(w * 0.06)
    ry1 = ry0 + int(h * 0.03)
    draw.rectangle([rx0, ry0, rx1, ry1], fill=(20, 20, 20, 255))
    # mouth (darker rectangle)
    mx0 = int(w * 0.44)
    my0 = int(h * 0.65)
    mx1 = mx0 + int(w * 0.12)
    my1 = my0 + int(h * 0.03)
    draw.rectangle([mx0, my0, mx1, my1], fill=(60, 40, 40, 255))

    # write ARGB raw (A R G B)
    pix = img.getdata()
    with open(out, 'wb') as f:
        for (r, g, b, a) in pix:
            f.write(bytes((a, r, g, b)))
    print('Wrote', out)


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--out', dest='out', default='test.argb')
    p.add_argument('--w', dest='w', type=int, default=640)
    p.add_argument('--h', dest='h', type=int, default=480)
    args = p.parse_args()
    gen(args.out, args.w, args.h)
