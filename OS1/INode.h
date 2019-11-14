#ifndef INODE_H
#define INODE_H

#include "Buf.h"

class INode {
public:
	// INodeFlag中标志位
	enum INodeFlag {
		IUPD = 0x2,	
		IACC = 0x4
	};

	static const unsigned int IALLOC = 0x8000;		/* 文件被使用 */
	static const unsigned int IFMT = 0x6000;		/* 文件类型掩码 */
	
	static const unsigned int IFDIR = 0x4000;		/* 文件类型：目录文件 */
	static const unsigned int IFBLK = 0x6000;		/* 块设备特殊类型文件，为0表示常规数据文件 */
	static const unsigned int ILARG = 0x1000;		/* 文件长度类型：大型或巨型文件 */
	static const unsigned int IREAD = 0x100;		/* 对文件的读权限 */
	static const unsigned int IWRITE = 0x80;		/* 对文件的写权限 */
	static const unsigned int IEXEC = 0x40;			/* 对文件的执行权限 */
	static const unsigned int IRWXU = (IREAD | IWRITE | IEXEC);		/* 文件主对文件的读、写、执行权限 */
	static const unsigned int IRWXG = ((IRWXU) >> 3);			    /* 文件主同组用户对文件的读、写、执行权限 */
	static const unsigned int IRWXO = ((IRWXU) >> 6);			    /* 其他用户对文件的读、写、执行权限 */

	static const int BLOCK_SIZE = 512;		/* 文件逻辑块大小: 512字节 */
	static const int ADDRESS_PER_INDEX_BLOCK = BLOCK_SIZE / sizeof(int);	/* 每个间接索引表(或索引块)包含的物理盘块号 */

	static const int SMALL_FILE_BLOCK = 6;	/* 小型文件：直接索引表最多可寻址的逻辑块号 */
	static const int LARGE_FILE_BLOCK = 128 * 2 + 6;	/* 大型文件：经一次间接索引表最多可寻址的逻辑块号 */
	static const int HUGE_FILE_BLOCK = 128 * 128 * 2 + 128 * 2 + 6;	/* 巨型文件：经二次间接索引最大可寻址文件逻辑块号 */

public:
	unsigned int i_flag;
	unsigned int i_mode;
	int		i_count;
	int		i_nlink;
	short	i_dev;	
	int		i_number;
	short	i_uid;	
	short	i_gid;
	int		i_size;	
	int		i_addr[10];	
	int		i_lastr;

public:
	INode();
	~INode();
	void ReadI();
	void WriteI();
	int Bmap(int lbn);
	void IUpdate(int time);
	void ITrunc();
	void Clean();
	void ICopy(Buf* bp, int inumber);
};

class DiskINode {
public:
	unsigned int d_mode;
	int		d_nlink;
	short	d_uid;
	short	d_gid;
	int		d_size;	
	int		d_addr[10];
	int		d_atime;
	int		d_mtime;

public:
	DiskINode();
	~DiskINode();
};

class File {
public:
	enum FileFlags {
		FREAD = 0x1,			/* 读请求类型 */
		FWRITE = 0x2,			/* 写请求类型 */
	};

public:
	File();
	~File();

	unsigned int flag;	
	int		count;
	INode*	inode;
	int		offset;	
};

class OpenFiles {
public:
	static const int MAX_FILES = 100;

private:
	File *processOpenFileTable[MAX_FILES];

public:
	OpenFiles();
	~OpenFiles();
	int AllocFreeSlot();

	File* GetF(int fd);
	void SetF(int fd, File* pFile);
};


class IOParameter {
public:
	unsigned char* base;	
	int offset;	
	int count;	
};


#endif