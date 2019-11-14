#ifndef  USER_H
#define USER_H

#include "File_Manager.h"
#include <string>
using namespace std;

class User {
public:
	static const int EAX = 0;

	enum ErrorCode {
		U_NOERROR = 0,  	/* 没有错误码*/
		U_ENOENT = 2,	    /* 不存在文件或目录 */
		U_EBADF = 9,	    /* 错误文件号 */
		U_EACCES = 13,	    /* 没有权限 */
		U_ENOTDIR = 20,	    /* 不是目录 */
		U_ENFILE = 23,	    /* 文件表溢出 */
		U_EMFILE = 24,	    /* 打开太多文件 */
		U_EFBIG = 27,	    /* 文件太大 */
		U_ENOSPC = 28,	    /* 设备没有空间 */
	};

public:
	INode* cdir;
	INode* pdir;		
	DirectoryEntry dent;					
	char dbuf[DirectoryEntry::DIRSIZ];	  
	string curDirPath;						
	string dirp;
	long arg[5];				
	unsigned int	ar0[5];	    
	ErrorCode u_error;			
	OpenFiles ofiles;		   
	IOParameter IOParam;	    
	FileManager* fileManager;
	string ls;

public:
	User();
	~User();

	void U_Ls();
	void U_Cd(string dirName);
	void U_Mkdir(string dirName);
	void U_Create(string fileName, string mode);
	void U_Delete(string fileName);
	void U_Open(string fileName, string mode);
	void U_Close(string fd);
	void U_Seek(string fd, string offset, string origin);
	void U_Write(string fd, string inFile, string size);
	void U_Read(string fd, string outFile, string size);

private:
	bool Judge_Error();
	void EchoError(enum ErrorCode err);
	int INodeMode(string mode);
	int FileMode(string mode);
	bool checkPathName(string path);

};

#endif
