#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include "bits/stdc++.h"
using namespace std;

class Device_Driver {
public:
	static const char* DISK_FILE_NAME;

private:
	FILE* fp;

public:
	Device_Driver();
	~Device_Driver();
	bool Exists();
	void Construct();
	void write(const void* buffer, unsigned int size,
		int offset = -1, unsigned int origin = SEEK_SET);
	void read(void* buffer, unsigned int size,
		int offset = -1, unsigned int origin = SEEK_SET);
};
