#ifndef INODE_H
#define INODE_H

#include "Buf.h"

class INode {
public:
	// INodeFlag�б�־λ
	enum INodeFlag {
		IUPD = 0x2,	
		IACC = 0x4
	};

	static const unsigned int IALLOC = 0x8000;		/* �ļ���ʹ�� */
	static const unsigned int IFMT = 0x6000;		/* �ļ��������� */
	
	static const unsigned int IFDIR = 0x4000;		/* �ļ����ͣ�Ŀ¼�ļ� */
	static const unsigned int IFBLK = 0x6000;		/* ���豸���������ļ���Ϊ0��ʾ���������ļ� */
	static const unsigned int ILARG = 0x1000;		/* �ļ��������ͣ����ͻ�����ļ� */
	static const unsigned int IREAD = 0x100;		/* ���ļ��Ķ�Ȩ�� */
	static const unsigned int IWRITE = 0x80;		/* ���ļ���дȨ�� */
	static const unsigned int IEXEC = 0x40;			/* ���ļ���ִ��Ȩ�� */
	static const unsigned int IRWXU = (IREAD | IWRITE | IEXEC);		/* �ļ������ļ��Ķ���д��ִ��Ȩ�� */
	static const unsigned int IRWXG = ((IRWXU) >> 3);			    /* �ļ���ͬ���û����ļ��Ķ���д��ִ��Ȩ�� */
	static const unsigned int IRWXO = ((IRWXU) >> 6);			    /* �����û����ļ��Ķ���д��ִ��Ȩ�� */

	static const int BLOCK_SIZE = 512;		/* �ļ��߼����С: 512�ֽ� */
	static const int ADDRESS_PER_INDEX_BLOCK = BLOCK_SIZE / sizeof(int);	/* ÿ�����������(��������)�����������̿�� */

	static const int SMALL_FILE_BLOCK = 6;	/* С���ļ���ֱ������������Ѱַ���߼���� */
	static const int LARGE_FILE_BLOCK = 128 * 2 + 6;	/* �����ļ�����һ�μ������������Ѱַ���߼���� */
	static const int HUGE_FILE_BLOCK = 128 * 128 * 2 + 128 * 2 + 6;	/* �����ļ��������μ����������Ѱַ�ļ��߼���� */

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
		FREAD = 0x1,			/* ���������� */
		FWRITE = 0x2,			/* д�������� */
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