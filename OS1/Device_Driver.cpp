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

/* ��龵���ļ��Ƿ���� */
bool Device_Driver::Exists() {
	return fp != NULL;
}

/* �򿪾����ļ� */
void Device_Driver::Construct() {
	fp = fopen(DISK_FILE_NAME, "wb+");
	if (fp == NULL) {
		printf("�򿪻��½��ļ�%sʧ�ܣ�", DISK_FILE_NAME);
		exit(-1);
	}
}

/* ʵ��д���̺��� */
void Device_Driver::write(const void* buffer, unsigned int size, int offset, unsigned int origin) {
	if (offset >= 0) {
		fseek(fp, offset, origin);
	}
	fwrite(buffer, size, 1, fp);
}

/* ʵ��д���̺��� */
void Device_Driver::read(void* buffer, unsigned int size, int offset, unsigned int origin) {
	if (offset >= 0) {
		fseek(fp, offset, origin);
	}
	fread(buffer, size, 1, fp);
}