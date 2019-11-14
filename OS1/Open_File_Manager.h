#ifndef OPENFILEMANAGER_H
#define OPENFILEMANAGER_H

#include "INode.h"
#include "File_System.h"

/*
 * 打开文件管理类(OpenFileManager)负责内核中对打开文件机构的管理，为进程打开文件建立内
 * 核数据结构之间的勾连关系。
 * 勾连关系指进程u区中打开文件描述符指向打开文件表中的File打开文件控制结构，以及从File
 * 结构指向文件对应的内存INode。
 */
class OpenFileTable {
public:
	static const int MAX_FILES = 100;	/* 打开文件控制块File结构的数量 */

public:
	File m_File[MAX_FILES];

public:
	OpenFileTable();
	~OpenFileTable();
	File* FAlloc();
	void CloseF(File* pFile);
	void Format();
};

class INodeTable {
public:
	static const int NINODE = 100;

private:
	INode m_INode[NINODE];	
	FileSystem* fileSystem;	

public:
	INodeTable();
	~INodeTable();
	INode* IGet(int inumber);
	void IPut(INode* pNode);
	void UpdateINodeTable();
	int IsLoaded(int inumber);
	INode* GetFreeINode();
	void Format();
};

#endif