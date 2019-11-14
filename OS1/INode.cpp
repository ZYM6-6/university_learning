#include "Buf.h"
#include "INode.h"
#include "File_System.h"
#include "User.h"

extern Buf_Manager g_Buf_Manager;
extern FileSystem g_FileSystem;
extern User g_User;

DiskINode::DiskINode() {
	this->d_mode = 0;
	this->d_nlink = 0;
	this->d_uid = -1;
	this->d_gid = -1;
	this->d_size = 0;
	Common::memset(d_addr, 0, sizeof(d_addr));
	this->d_atime = 0;
	this->d_mtime = 0;
}

DiskINode::~DiskINode() {
}

INode::INode() {
	this->i_flag = 0;
	this->i_mode = 0;
	this->i_count = 0;
	this->i_nlink = 0;
	this->i_number = -1;
	this->i_uid = -1;
	this->i_gid = -1;
	this->i_size = 0;
	this->i_lastr = -1;
	Common::memset(i_addr, 0, sizeof(i_addr));
}

INode::~INode() {
}

/* 根据Inode对象中的物理磁盘块索引表，读取相应的文件数据 */
void INode::ReadI() {
	User& u = g_User;
	Buf_Manager& bufferManager = g_Buf_Manager;
	int lbn, bn;
	int offset, nbytes;
	Buf* pBuf;

	/* 需要读字节数为零，则返回 */
	if (0 == u.IOParam.count) {
		return;
	}
	this->i_flag |= INode::IACC;

	while (User::U_NOERROR == u.u_error && u.IOParam.count) {
		lbn = bn = u.IOParam.offset / INode::BLOCK_SIZE;
		offset = u.IOParam.offset % INode::BLOCK_SIZE;

		/* 传送到用户区的字节数量，取读请求的剩余字节数与当前字符块内有效字节数较小值 */
		nbytes = Common::min(INode::BLOCK_SIZE - offset /* 块内有效字节数 */, u.IOParam.count);
		int remain = this->i_size - u.IOParam.offset;
		if (remain <= 0) {
			return;
		}

		/* 传送的字节数量还取决于剩余文件的长度 */
		nbytes = Common::min(nbytes, remain);
		if ((bn = this->Bmap(lbn)) == 0) {
			return;
		}

		pBuf = bufferManager.Buf_Read(bn);
		this->i_lastr = lbn;

		/* 缓存中数据起始读位置 */
		unsigned char* start = pBuf->Address + offset;
		Common::memcpy(u.IOParam.base, start, nbytes);
		//printf("nbytes:%s\n", u.IOParam.base);
		u.IOParam.base += nbytes;
		u.IOParam.offset += nbytes;
		u.IOParam.count -= nbytes;

		bufferManager.Buf_Relse(pBuf);
	}
}

/* 根据Inode对象中的物理磁盘块索引表，将数据写入文件 */
void INode::WriteI() {
	int lbn, bn;
	int offset, nbytes;
	Buf* pBuf;
	User& u = g_User;
	Buf_Manager& bufferManager = g_Buf_Manager;

	this->i_flag |= (INode::IACC | INode::IUPD);

	/* 需要写字节数为零，则返回 */
	if (0 == u.IOParam.count) {
		return;
	}

	while (User::U_NOERROR == u.u_error && u.IOParam.count) {
		lbn = u.IOParam.offset / INode::BLOCK_SIZE;
		offset = u.IOParam.offset % INode::BLOCK_SIZE;
		nbytes = Common::min(INode::BLOCK_SIZE - offset, u.IOParam.count);
		if ((bn = this->Bmap(lbn)) == 0) {
			return;
		}

		if (INode::BLOCK_SIZE == nbytes) {
			/* 如果写入数据正好满一个字符块，则为其分配缓存 */
			pBuf = bufferManager.GetBlock(bn);
		}
		else {
			/* 写入数据不满一个字符块，先读后写（读出该字符块以保护不需要重写的数据） */
			pBuf = bufferManager.Buf_Read(bn);
		}

		/* 缓存中数据的起始写位置 写操作: 从用户目标区拷贝数据到缓冲区 */
		unsigned char* start = pBuf->Address + offset;
		Common::memcpy(start, u.IOParam.base, nbytes);
		u.IOParam.base += nbytes;
		u.IOParam.offset += nbytes;
		u.IOParam.count -= nbytes;

		if (u.u_error != User::U_NOERROR) {
			bufferManager.Buf_Relse(pBuf);
		}

		/* 将缓存标记为延迟写，不急于进行I/O操作将字符块输出到磁盘上 */
		bufferManager.Buf_Dwrite(pBuf);

		/* 普通文件长度增加 */
		if (this->i_size < u.IOParam.offset) {
			this->i_size = u.IOParam.offset;
		}
		this->i_flag |= INode::IUPD;
	}
}

/* 将包含外存Inode字符块中信息拷贝到内存Inode中 */
void INode::ICopy(Buf* pb, int inumber) {
	DiskINode& dINode = *(DiskINode*)(pb->Address + (inumber%FileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskINode));
	i_mode = dINode.d_mode;
	i_nlink = dINode.d_nlink;
	i_uid = dINode.d_uid;
	i_gid = dINode.d_gid;
	i_size = dINode.d_size;
	Common::memcpy(i_addr, dINode.d_addr, sizeof(i_addr));
}

/* 将文件的逻辑块号转换成对应的物理盘块号 */
int INode::Bmap(int lbn) {
	/*
	* Unix V6++的文件索引结构：(小型、大型和巨型文件)
	* (1) i_addr[0] - i_addr[5]为直接索引表，文件长度范围是0 - 6个盘块；
	*
	* (2) i_addr[6] - i_addr[7]存放一次间接索引表所在磁盘块号，每磁盘块
	* 上存放128个文件数据盘块号，此类文件长度范围是7 - (128 * 2 + 6)个盘块；
	*
	* (3) i_addr[8] - i_addr[9]存放二次间接索引表所在磁盘块号，每个二次间接
	* 索引表记录128个一次间接索引表所在磁盘块号，此类文件长度范围是
	* (128 * 2 + 6 ) < size <= (128 * 128 * 2 + 128 * 2 + 6)
	*/
	User& u = g_User;
	Buf_Manager& bufferManager = g_Buf_Manager;
	FileSystem& fileSystem = g_FileSystem;
	Buf* pFirstBuf, *pSecondBuf;
	int phyBlkno, index;
	int *iTable;

	if (lbn >= INode::HUGE_FILE_BLOCK) {
		u.u_error = User::U_EFBIG;
		return 0;
	}

	/* 如果是小型文件，从基本索引表i_addr[0-5]中获得物理盘块号即可 */
	if (lbn < 6) {
		phyBlkno = this->i_addr[lbn];

		/* 如果该逻辑块号还没有相应的物理盘块号与之对应，则分配一个物理块。*/
		if (phyBlkno == 0 && (pFirstBuf = fileSystem.Alloc()) != NULL) {
			phyBlkno = pFirstBuf->block_num;
			bufferManager.Buf_Dwrite(pFirstBuf);
			this->i_addr[lbn] = phyBlkno;
			this->i_flag |= INode::IUPD;
		}
		return phyBlkno;
	}

	/* lbn >= 6 大型、巨型文件 */
	if (lbn < INode::LARGE_FILE_BLOCK) {
		index = (lbn - INode::SMALL_FILE_BLOCK) / INode::ADDRESS_PER_INDEX_BLOCK + 6;
	}
	else {
		/* 巨型文件: 长度介于263 - (128 * 128 * 2 + 128 * 2 + 6)个盘块之间 */
		index = (lbn - INode::LARGE_FILE_BLOCK) / (INode::ADDRESS_PER_INDEX_BLOCK * INode::ADDRESS_PER_INDEX_BLOCK) + 8;
	}

	phyBlkno = this->i_addr[index];
	if (phyBlkno) {
		pFirstBuf = bufferManager.Buf_Read(phyBlkno);
	}
	else {
		/* 若该项为零，则表示不存在相应的间接索引表块 */
		this->i_flag |= INode::IUPD;
		if ((pFirstBuf = fileSystem.Alloc()) == 0) {
			return 0;
		}
		this->i_addr[index] = pFirstBuf->block_num;
	}

	iTable = (int *)pFirstBuf->Address;
	if (index >= 8) {
		/*
		* 对于巨型文件的情况，pFirstBuf中是二次间接索引表，
		* 还需根据逻辑块号，经由二次间接索引表找到一次间接索引表
		*/
		index = ((lbn - INode::LARGE_FILE_BLOCK) / INode::ADDRESS_PER_INDEX_BLOCK) % INode::ADDRESS_PER_INDEX_BLOCK;
		phyBlkno = iTable[index];

		if (phyBlkno) {
			bufferManager.Buf_Relse(pFirstBuf);
			pSecondBuf = bufferManager.Buf_Read(phyBlkno);
		}
		else {
			if ((pSecondBuf = fileSystem.Alloc()) == NULL) {
				bufferManager.Buf_Relse(pFirstBuf);
				return 0;
			}
			iTable[index] = pSecondBuf->block_num;
			bufferManager.Buf_Dwrite(pFirstBuf);
		}

		pFirstBuf = pSecondBuf;
		iTable = (int *)pSecondBuf->Address;
	}

	if (lbn < INode::LARGE_FILE_BLOCK) {
		index = (lbn - INode::SMALL_FILE_BLOCK) % INode::ADDRESS_PER_INDEX_BLOCK;
	}
	else {
		index = (lbn - INode::LARGE_FILE_BLOCK) % INode::ADDRESS_PER_INDEX_BLOCK;
	}

	if ((phyBlkno = iTable[index]) == 0 && (pSecondBuf = fileSystem.Alloc()) != NULL) {
		phyBlkno = pSecondBuf->block_num;
		iTable[index] = phyBlkno;
		bufferManager.Buf_Dwrite(pSecondBuf);
		bufferManager.Buf_Dwrite(pFirstBuf);
	}
	else {
		bufferManager.Buf_Relse(pFirstBuf);
	}
	return phyBlkno;
}

/* 清空Inode对象中的数据 */
void INode::Clean() {
	/*
	* Inode::Clean()特定用于IAlloc()中清空新分配DiskInode的原有数据，
	* 即旧文件信息。Clean()函数中不应当清除i_dev, i_number, i_flag, i_count,
	* 这是属于内存Inode而非DiskInode包含的旧文件信息，而Inode类构造函数需要
	* 将其初始化为无效值。
	*/

	// this->i_flag = 0;
	this->i_mode = 0;
	//this->i_count = 0;
	this->i_nlink = 0;
	//this->i_dev = -1;
	//this->i_number = -1;
	this->i_uid = -1;
	this->i_gid = -1;
	this->i_size = 0;
	this->i_lastr = -1;
	Common::memset(i_addr, 0, sizeof(i_addr));
}

/* 更新外存Inode的最后的访问时间、修改时间 */
void INode::IUpdate(int time) {
	Buf* pBuf;
	DiskINode dINode;
	FileSystem& fileSystem = g_FileSystem;
	Buf_Manager& bufferManager = g_Buf_Manager;

	/*
	 *当IUPD和IACC标志之一被设置，才需要更新相应DiskInode
	 * 目录搜索，不会设置所途径的目录文件的IACC和IUPD标志
	 */
	if (this->i_flag&(INode::IUPD | INode::IACC)) {
		pBuf = bufferManager.Buf_Read(FileSystem::INODE_ZONE_START_SECTOR + this->i_number / FileSystem::INODE_NUMBER_PER_SECTOR);
		dINode.d_mode = this->i_mode;
		dINode.d_nlink = this->i_nlink;
		dINode.d_uid = this->i_uid;
		dINode.d_gid = this->i_gid;
		dINode.d_size = this->i_size;
		memcpy(dINode.d_addr, i_addr, sizeof(dINode.d_addr));
		if (this->i_flag & INode::IACC) {
			dINode.d_atime = time;
		}
		if (this->i_flag & INode::IUPD) {
			dINode.d_mtime = time;
		}

		unsigned char* p = pBuf->Address + (this->i_number % FileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskINode);
		DiskINode* pNode = &dINode;
		Common::memcpy(p, pNode, sizeof(DiskINode));
		bufferManager.Buf_Write(pBuf);
	}
}

void INode::ITrunc() {
	Buf_Manager &bufferManager = g_Buf_Manager;
	FileSystem& fileSystem = g_FileSystem;
	Buf* pFirstBuf, *pSecondBuf;

	for (int i = 9; i >= 0; --i) {
		if (this->i_addr[i]) {
			if (i >= 6) {
				pFirstBuf = bufferManager.Buf_Read(this->i_addr[i]);
				int *pFirst = (int*)pFirstBuf->Address;
				for (int j = BLOCK_SIZE / sizeof(int) - 1; j >= 0; --j) {
					if (pFirst[j]) {
						if (i >= 8) {
							pSecondBuf = bufferManager.Buf_Read(pFirst[j]);
							int* pSecond = (int*)pSecondBuf->Address;
							for (int k = BLOCK_SIZE / sizeof(int) - 1; k >= 0; --k) {
								if (pSecond[k]) {
									fileSystem.Free(pSecond[k]);
								}
							}
							bufferManager.Buf_Relse(pSecondBuf);
						}
						fileSystem.Free(pFirst[j]);
					}
				}
				bufferManager.Buf_Relse(pFirstBuf);
			}
			fileSystem.Free(this->i_addr[i]);
			this->i_addr[i] = 0;
		}
	}
	this->i_size = 0;
	this->i_flag |= INode::IUPD;
	this->i_mode &= ~(INode::ILARG);
	this->i_nlink = 1;
}


extern User g_User;

File::File() {
	flag = 0;
	count = 0;
	inode = NULL;
	offset = 0;
}

File::~File() {
}

OpenFiles::OpenFiles() {
	Common::memset(processOpenFileTable, NULL, sizeof(processOpenFileTable));
}

OpenFiles::~OpenFiles() {
}

/* 进程请求打开文件时，在打开文件描述符表中分配一个空闲表项 */
int OpenFiles::AllocFreeSlot() {
	User& u = g_User;
	for (int i = 0; i < OpenFiles::MAX_FILES; i++) {
		/* 进程打开文件描述符表中找到空闲项，则返回之 */
		if (this->processOpenFileTable[i] == NULL) {
			u.ar0[User::EAX] = i;
			return i;
		}
	}

	u.ar0[User::EAX] = -1;   /* Open1，需要一个标志。当打开文件结构创建失败时，可以回收系统资源*/
	u.u_error = User::U_EMFILE;
	return -1;
}

/* 根据用户系统调用提供的文件描述符参数fd，找到对应的打开文件控制块File结构 */
File* OpenFiles::GetF(int fd) {
	User& u = g_User;
	File* pFile;

	if (fd < 0 || fd >= OpenFiles::MAX_FILES) {
		u.u_error = User::U_EBADF;
		return NULL;
	}

	pFile = this->processOpenFileTable[fd];
	if (pFile == NULL) {
		u.u_error = User::U_EBADF;
	}
	return pFile;
}

/* 为已分配到的空闲描述符fd和已分配的打开文件表中空闲File对象建立勾连关系 */
void OpenFiles::SetF(int fd, File* pFile) {
	if (fd < 0 || fd >= OpenFiles::MAX_FILES) {
		return;
	}
	this->processOpenFileTable[fd] = pFile;
}
