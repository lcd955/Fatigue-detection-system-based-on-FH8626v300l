#!/usr/bin/env python3
"""
简单工具：把常见图片（png/jpg）转换为 ARGB8888 raw（A R G B 每像素 4 字节）
用法示例：
  pip3 install pillow
  python3 convert_to_argb.py --in test.jpg --out test.argb --w 640 --h 480
输出文件可直接放到固件 rootfs 下供 demo 使用。
"""
import argparse
from PIL import Image

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--in', dest='infile', required=True, help='输入图片文件')
    p.add_argument('--out', dest='outfile', required=True, help='输出 ARGB raw 文件')
    p.add_argument('--w', dest='w', type=int, required=True, help='输出宽度')
    p.add_argument('--h', dest='h', type=int, required=True, help='输出高度')
    args = p.parse_args()

    img = Image.open(args.infile).convert('RGBA')
    img = img.resize((args.w, args.h), Image.BILINEAR)
    pix = img.getdata()

    with open(args.outfile, 'wb') as f:
        for (r, g, b, a) in pix:
            # 写入顺序 A R G B
            f.write(bytes((a, r, g, b)))

    print('Wrote', args.outfile)

if __name__ == '__main__':
    main()
