#ifndef OPENFILEMANAGER_H
#define OPENFILEMANAGER_H

#include "INode.h"
#include "File_System.h"

/*
 * ���ļ�������(OpenFileManager)�����ں��жԴ��ļ������Ĺ���Ϊ���̴��ļ�������
 * �����ݽṹ֮��Ĺ�����ϵ��
 * ������ϵָ����u���д��ļ�������ָ����ļ����е�File���ļ����ƽṹ���Լ���File
 * �ṹָ���ļ���Ӧ���ڴ�INode��
 */
class OpenFileTable {
public:
	static const int MAX_FILES = 100;	/* ���ļ����ƿ�File�ṹ������ */

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