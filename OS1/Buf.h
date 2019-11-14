#pragma once
#ifndef BUFFER_H
#define BUFFER_H

#include "Device_Driver.h"
#include <bits/stdc++.h>
#include <unordered_map>

using namespace std;

class Buf {
public:

	/* flags中标志位,枚举 */
	enum Buf_flag {
		Buf_WRITE = 0x1,		
		Buf_READ = 0x2,		    
		Buf_DONE = 0x4,		    
		Buf_ERROR = 0x8,		
		Buf_BUSY = 0x10,		
		Buf_WANTED = 0x20,	    
		BBuf_ASYNC = 0x40,		
		Buf_DELWRI = 0x80		
	};

public:
	unsigned int flags;
	Buf*	forw;
	Buf*	back;
	int		count;		    
	unsigned char* Address;	
	int		block_num;		 
	int		u_error;	    
	int		resid;	
	int no;
public:
	Buf();
	~Buf();
	void Debug_Mark();
	void Debug_Content();
};




class Buf_Manager {
public:
	static const int Num_Buf = 100;			
	static const int Buf_Size = 512;		

private:
	Buf* Buf_List;							
	Buf Buf_Block[Num_Buf];					
	unsigned char BUF[Num_Buf][Buf_Size];	
	unordered_map<int, Buf*> map;
	Device_Driver* deviceDriver;

public:
	Buf_Manager();
	~Buf_Manager();
	Buf* GetBlock(int block_num);
	void Buf_Relse(Buf* bp);
	Buf* Buf_Read(int blkno);
	void Buf_Write(Buf* bp);
	void Buf_Dwrite(Buf* bp);
	void Buf_Clear(Buf* bp);
	void Buf_Flush();
	void Format_Buf();

private:
	void InitList();
	void DetachNode(Buf* buffer);
	void InsertTail(Buf* buffer);
	void debug();
};



class Common {
public:
	static void memcpy(void *dest, const void *src, size_t n);
	static void memset(void *s, int ch, size_t n);
	static int memcmp(const void *buf1, const void *buf2, unsigned int count);
	static int min(int a, int b);
	static time_t time(time_t* t);
};
#endif