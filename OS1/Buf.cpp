#include "Buf.h"
#include <iostream>
#include <iomanip>
#include <ctype.h>

#include "Device_Driver.h"
extern Device_Driver g_Device_Driver;
using namespace std;

Buf::Buf() {
	flags = 0;
	forw = NULL;
	back = NULL;
	count = 0;
	Address = NULL;
	block_num = -1;
	u_error = -1;
	resid = 0;

	no = 0;
}

Buf::~Buf() {

}


void Buf::Debug_Content() {
	Debug_Mark();
	for (int x = 0; x < Buf_Manager::Buf_Size; x += 32) {
		cout << "  " << setfill('0') << setw(4) << hex << x << ": ";
		int j = x;
		while (j < x + 32)
		{
			cout << ((j + 1 - x % 8) ? " " : " - ");
			cout << setfill('0') << setw(2) << hex << (unsigned int)(unsigned char)Address[j];
			j++;
		}
		cout << "  " << setw(4) << *(int*)(Address + x) << " ";
		j = x + 4;
		while (j < x + 32)
		{
			cout << (isprint(Address[j]) ? (char)Address[j] : '.');
			j++;
		}
		cout << endl;
	}
	cout << dec;
}

void Buf::Debug_Mark() {
	cout << "no = " << no;
	cout << " block_num = " << block_num;
	cout << " flag = " << (flags&Buf_DONE ? " DONE " : " ");
	cout << (flags&Buf_DELWRI ? " DELWRI " : " ");
	cout<< endl;
}



/*
 *	Bufferֻ�õ���������־��Buf_DONE��Buf_DELWRI���ֱ��ʾ�Ѿ����IO���ӳ�д�ı�־��
 *	����Buffer���κα�־
*/
Buf_Manager::Buf_Manager() {
	Buf_List = new Buf;
	InitList();
	deviceDriver = &g_Device_Driver;
}

Buf_Manager::~Buf_Manager() {
	Buf_Flush();
	delete Buf_List;
}

void Buf_Manager::Format_Buf() {
	Buf empty_Buf;
	for (int i = 0; i < Num_Buf; ++i) {
		Common::memcpy(Buf_Block + i, &empty_Buf, sizeof(Buf));
	}
	InitList();
}

void Buf_Manager::InitList() {
	for (int i = 0; i < Num_Buf; ++i) {
		if (i) {
			Buf_Block[i].forw = Buf_Block + i - 1;
		}
		else {
			Buf_Block[i].forw = Buf_List;
			Buf_List->back = Buf_Block + i;
		}

		if (i + 1 < Num_Buf) {
			Buf_Block[i].back = Buf_Block + i + 1;
		}
		else {
			Buf_Block[i].back = Buf_List;
			Buf_List->forw = Buf_Block + i;
		}
		Buf_Block[i].Address = BUF[i];
		Buf_Block[i].no = i;
	}
}

/* ����LRU Cache �㷨��ÿ�δ�ͷ��ȡ����ʹ�ú�ŵ�β��
*/
void Buf_Manager::DetachNode(Buf* buffer) {
	if (buffer->back == NULL) {
		return;
	}
	buffer->forw->back = buffer->back;
	buffer->back->forw = buffer->forw;
	buffer->back = NULL;
	buffer->forw = NULL;
}

void Buf_Manager::InsertTail(Buf* buffer) {
	if (buffer->back != NULL) {
		return;
	}
	buffer->forw = Buf_List->forw;
	buffer->back = Buf_List;
	Buf_List->forw->back = buffer;
	Buf_List->forw = buffer;
}

/* ����һ�黺�棬�ӻ��������ȡ�������ڶ�д�豸�ϵĿ�block_num��*/
Buf* Buf_Manager::GetBlock(int block_num) {
	Buf* buffer;
	if (map.find(block_num) != map.end()) {
		buffer = map[block_num];
		DetachNode(buffer);
		return buffer;
	}
	buffer = Buf_List->back;
	if (buffer == Buf_List) {
		cout << "��Buffer�ɹ�ʹ��" << endl;
		return NULL;
	}
	DetachNode(buffer);
	map.erase(buffer->block_num);
	if (buffer->flags&Buf::Buf_DELWRI) {
		deviceDriver->write(buffer->Address, Buf_Size, buffer->block_num*Buf_Size);
	}
	buffer->flags &= ~(Buf::Buf_DELWRI | Buf::Buf_DONE);
	buffer->block_num = block_num;
	map[block_num] = buffer;
	return buffer;
}

/* �ͷŻ�����ƿ�buf */
void Buf_Manager::Buf_Relse(Buf* buffer) {
	InsertTail(buffer);
}

/* ��һ�����̿飬block_numΪĿ����̿��߼���š� */
Buf* Buf_Manager::Buf_Read(int block_num) {
	Buf* buffer = GetBlock(block_num);
	//buffer->debugMark();
	//buffer->debugContent();
	if (buffer->flags&(Buf::Buf_DONE | Buf::Buf_DELWRI)) {
		return buffer;
	}
	deviceDriver->read(buffer->Address, Buf_Size, buffer->block_num*Buf_Size);
	buffer->flags |= Buf::Buf_DONE;
	return buffer;
}

/* дһ�����̿� */
void Buf_Manager::Buf_Write(Buf *buffer) {
	//buffer->debugMark();
	//buffer->debugContent();
	buffer->flags &= ~(Buf::Buf_DELWRI);
	deviceDriver->write(buffer->Address, Buf_Size, buffer->block_num*Buf_Size);
	buffer->flags |= (Buf::Buf_DONE);
	this->Buf_Relse(buffer);
}

/* �ӳ�д���̿� */
void Buf_Manager::Buf_Dwrite(Buf* buffer_p) {
	buffer_p->flags |= (Buf::Buf_DELWRI | Buf::Buf_DONE);
	this->Buf_Relse(buffer_p);
	return;
}

/* ��ջ��������� */
void Buf_Manager::Buf_Clear(Buf *buffer_p) {
	Common::memset(buffer_p->Address, 0, Buf_Manager::Buf_Size);
	return;
}

/* ���������ӳ�д�Ļ���ȫ����������� */
void Buf_Manager::Buf_Flush() {
	Buf* buffer = NULL;
	for (int i = 0; i < Num_Buf; ++i) {
		buffer = Buf_Block + i;
		if ((buffer->flags & Buf::Buf_DELWRI)) {
			buffer->flags &= ~(Buf::Buf_DELWRI);
			deviceDriver->write(buffer->Address, Buf_Size, buffer->block_num*Buf_Size);
			buffer->flags |= (Buf::Buf_DONE);
		}
	}
}

void Buf_Manager::debug() {
	for (Buf* buffer = Buf_List->back; buffer != Buf_List; buffer = buffer->back) {
		buffer->Debug_Mark();
	}
	cout << endl;
}



void Common::memcpy(void *dest, const void *src, size_t n) {
	::memcpy(dest, src, n);
}

void Common::memset(void *s, int ch, size_t n) {
	::memset(s, ch, n);
}

int Common::memcmp(const void *buf1, const void *buf2, unsigned int count) {
	return ::memcmp(buf1, buf2, count);
}

int Common::min(int a, int b) {
	if (a < b)
		return a;
	return b;
}

time_t Common::time(time_t* t) {
	return ::time(t);
}
