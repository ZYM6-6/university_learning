#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "File_System.h"
#include "INode.h"

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


class FileManager
{
public:
	enum DirectorySearchMode
	{
		OPEN = 0,		
		CREATE = 1,		
		DELETE = 2		
	};

public:
	
	INode* rootDirINode;
	FileSystem* fileSystem;
	INodeTable* inodeTable;
	OpenFileTable* openFileTable;

public:
	FileManager();
	~FileManager();
	void Open();
	void Creat();
	void Open1(INode* pINode, int mode, int trf);
	void Close();
	void Seek();
	void Read();
	void Write();
	void Rdwr(enum File::FileFlags mode);
	INode* NameI(enum DirectorySearchMode mode);
	INode* MakNode(unsigned int mode);
	void UnLink();
	void WriteDir(INode* pINode);
	void ChDir();
	void Ls();
};





#endif