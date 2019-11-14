#include "Device_Driver.h"

const char * Device_Driver::DISK_FILE_NAME = "1652325-zym.img";

Device_Driver::Device_Driver() {
	fp = fopen(DISK_FILE_NAME, "rb+");
}

Device_Driver::~Device_Driver() {
	if (fp) {
		fclose(fp);
	}
}

/* 检查镜像文件是否存在 */
bool Device_Driver::Exists() {
	return fp != NULL;
}

/* 打开镜像文件 */
void Device_Driver::Construct() {
	fp = fopen(DISK_FILE_NAME, "wb+");
	if (fp == NULL) {
		printf("打开或新建文件%s失败！", DISK_FILE_NAME);
		exit(-1);
	}
}

/* 实际写磁盘函数 */
void Device_Driver::write(const void* buffer, unsigned int size, int offset, unsigned int origin) {
	if (offset >= 0) {
		fseek(fp, offset, origin);
	}
	fwrite(buffer, size, 1, fp);
}

/* 实际写磁盘函数 */
void Device_Driver::read(void* buffer, unsigned int size, int offset, unsigned int origin) {
	if (offset >= 0) {
		fseek(fp, offset, origin);
	}
	fread(buffer, size, 1, fp);
}