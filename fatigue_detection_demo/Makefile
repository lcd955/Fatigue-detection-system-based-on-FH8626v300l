# Simple Makefile fragment for building this app in the existing RT-Thread build system
# Copy this folder into rt-thread/app/ and then from rt-thread 根目录运行 make

APPS += fatigue_detection_demo

# 应用路径
fatigue_detection_demo_DIR := $(RTT_ROOT)/app/fatigue_detection_demo

.PHONY: copy_resources
copy_resources:
	@echo "尝试把 resource/ 下的文件复制到常见的固件 rootfs 目录 (build/rootfs 或 build/root) 。"
	@if [ -d build/rootfs ]; then \
		cp -r app/fatigue_detection_demo/resource/* build/rootfs/ ; \
		echo "copied to build/rootfs/"; \
	elif [ -d build/root ]; then \
		cp -r app/fatigue_detection_demo/resource/* build/root/ ; \
		echo "copied to build/root/"; \
	else \
		echo "未找到 build/rootfs 或 build/root，请手动把 app/fatigue_detection_demo/resource/ 下的文件复制到你的 rootfs 输入目录。"; \
	fi
