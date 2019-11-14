#ifndef  USER_H
#define USER_H

#include "File_Manager.h"
#include <string>
using namespace std;

class User {
public:
	static const int EAX = 0;

	enum ErrorCode {
		U_NOERROR = 0,  	/* û�д�����*/
		U_ENOENT = 2,	    /* �������ļ���Ŀ¼ */
		U_EBADF = 9,	    /* �����ļ��� */
		U_EACCES = 13,	    /* û��Ȩ�� */
		U_ENOTDIR = 20,	    /* ����Ŀ¼ */
		U_ENFILE = 23,	    /* �ļ������ */
		U_EMFILE = 24,	    /* ��̫���ļ� */
		U_EFBIG = 27,	    /* �ļ�̫�� */
		U_ENOSPC = 28,	    /* �豸û�пռ� */
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
