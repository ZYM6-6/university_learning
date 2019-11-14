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

/* ����Inode�����е�������̿���������ȡ��Ӧ���ļ����� */
void INode::ReadI() {
	User& u = g_User;
	Buf_Manager& bufferManager = g_Buf_Manager;
	int lbn, bn;
	int offset, nbytes;
	Buf* pBuf;

	/* ��Ҫ���ֽ���Ϊ�㣬�򷵻� */
	if (0 == u.IOParam.count) {
		return;
	}
	this->i_flag |= INode::IACC;

	while (User::U_NOERROR == u.u_error && u.IOParam.count) {
		lbn = bn = u.IOParam.offset / INode::BLOCK_SIZE;
		offset = u.IOParam.offset % INode::BLOCK_SIZE;

		/* ���͵��û������ֽ�������ȡ�������ʣ���ֽ����뵱ǰ�ַ�������Ч�ֽ�����Сֵ */
		nbytes = Common::min(INode::BLOCK_SIZE - offset /* ������Ч�ֽ��� */, u.IOParam.count);
		int remain = this->i_size - u.IOParam.offset;
		if (remain <= 0) {
			return;
		}

		/* ���͵��ֽ�������ȡ����ʣ���ļ��ĳ��� */
		nbytes = Common::min(nbytes, remain);
		if ((bn = this->Bmap(lbn)) == 0) {
			return;
		}

		pBuf = bufferManager.Buf_Read(bn);
		this->i_lastr = lbn;

		/* ������������ʼ��λ�� */
		unsigned char* start = pBuf->Address + offset;
		Common::memcpy(u.IOParam.base, start, nbytes);
		//printf("nbytes:%s\n", u.IOParam.base);
		u.IOParam.base += nbytes;
		u.IOParam.offset += nbytes;
		u.IOParam.count -= nbytes;

		bufferManager.Buf_Relse(pBuf);
	}
}

/* ����Inode�����е�������̿�������������д���ļ� */
void INode::WriteI() {
	int lbn, bn;
	int offset, nbytes;
	Buf* pBuf;
	User& u = g_User;
	Buf_Manager& bufferManager = g_Buf_Manager;

	this->i_flag |= (INode::IACC | INode::IUPD);

	/* ��Ҫд�ֽ���Ϊ�㣬�򷵻� */
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
			/* ���д������������һ���ַ��飬��Ϊ����仺�� */
			pBuf = bufferManager.GetBlock(bn);
		}
		else {
			/* д�����ݲ���һ���ַ��飬�ȶ���д���������ַ����Ա�������Ҫ��д�����ݣ� */
			pBuf = bufferManager.Buf_Read(bn);
		}

		/* ���������ݵ���ʼдλ�� д����: ���û�Ŀ�����������ݵ������� */
		unsigned char* start = pBuf->Address + offset;
		Common::memcpy(start, u.IOParam.base, nbytes);
		u.IOParam.base += nbytes;
		u.IOParam.offset += nbytes;
		u.IOParam.count -= nbytes;

		if (u.u_error != User::U_NOERROR) {
			bufferManager.Buf_Relse(pBuf);
		}

		/* ��������Ϊ�ӳ�д�������ڽ���I/O�������ַ�������������� */
		bufferManager.Buf_Dwrite(pBuf);

		/* ��ͨ�ļ��������� */
		if (this->i_size < u.IOParam.offset) {
			this->i_size = u.IOParam.offset;
		}
		this->i_flag |= INode::IUPD;
	}
}

/* ���������Inode�ַ�������Ϣ�������ڴ�Inode�� */
void INode::ICopy(Buf* pb, int inumber) {
	DiskINode& dINode = *(DiskINode*)(pb->Address + (inumber%FileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskINode));
	i_mode = dINode.d_mode;
	i_nlink = dINode.d_nlink;
	i_uid = dINode.d_uid;
	i_gid = dINode.d_gid;
	i_size = dINode.d_size;
	Common::memcpy(i_addr, dINode.d_addr, sizeof(i_addr));
}

/* ���ļ����߼����ת���ɶ�Ӧ�������̿�� */
int INode::Bmap(int lbn) {
	/*
	* Unix V6++���ļ������ṹ��(С�͡����ͺ;����ļ�)
	* (1) i_addr[0] - i_addr[5]Ϊֱ���������ļ����ȷ�Χ��0 - 6���̿飻
	*
	* (2) i_addr[6] - i_addr[7]���һ�μ�����������ڴ��̿�ţ�ÿ���̿�
	* �ϴ��128���ļ������̿�ţ������ļ����ȷ�Χ��7 - (128 * 2 + 6)���̿飻
	*
	* (3) i_addr[8] - i_addr[9]��Ŷ��μ�����������ڴ��̿�ţ�ÿ�����μ��
	* �������¼128��һ�μ�����������ڴ��̿�ţ������ļ����ȷ�Χ��
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

	/* �����С���ļ����ӻ���������i_addr[0-5]�л�������̿�ż��� */
	if (lbn < 6) {
		phyBlkno = this->i_addr[lbn];

		/* ������߼���Ż�û����Ӧ�������̿����֮��Ӧ�������һ������顣*/
		if (phyBlkno == 0 && (pFirstBuf = fileSystem.Alloc()) != NULL) {
			phyBlkno = pFirstBuf->block_num;
			bufferManager.Buf_Dwrite(pFirstBuf);
			this->i_addr[lbn] = phyBlkno;
			this->i_flag |= INode::IUPD;
		}
		return phyBlkno;
	}

	/* lbn >= 6 ���͡������ļ� */
	if (lbn < INode::LARGE_FILE_BLOCK) {
		index = (lbn - INode::SMALL_FILE_BLOCK) / INode::ADDRESS_PER_INDEX_BLOCK + 6;
	}
	else {
		/* �����ļ�: ���Ƚ���263 - (128 * 128 * 2 + 128 * 2 + 6)���̿�֮�� */
		index = (lbn - INode::LARGE_FILE_BLOCK) / (INode::ADDRESS_PER_INDEX_BLOCK * INode::ADDRESS_PER_INDEX_BLOCK) + 8;
	}

	phyBlkno = this->i_addr[index];
	if (phyBlkno) {
		pFirstBuf = bufferManager.Buf_Read(phyBlkno);
	}
	else {
		/* ������Ϊ�㣬���ʾ��������Ӧ�ļ��������� */
		this->i_flag |= INode::IUPD;
		if ((pFirstBuf = fileSystem.Alloc()) == 0) {
			return 0;
		}
		this->i_addr[index] = pFirstBuf->block_num;
	}

	iTable = (int *)pFirstBuf->Address;
	if (index >= 8) {
		/*
		* ���ھ����ļ��������pFirstBuf���Ƕ��μ��������
		* ��������߼���ţ����ɶ��μ���������ҵ�һ�μ��������
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

/* ���Inode�����е����� */
void INode::Clean() {
	/*
	* Inode::Clean()�ض�����IAlloc()������·���DiskInode��ԭ�����ݣ�
	* �����ļ���Ϣ��Clean()�����в�Ӧ�����i_dev, i_number, i_flag, i_count,
	* ���������ڴ�Inode����DiskInode�����ľ��ļ���Ϣ����Inode�๹�캯����Ҫ
	* �����ʼ��Ϊ��Чֵ��
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

/* �������Inode�����ķ���ʱ�䡢�޸�ʱ�� */
void INode::IUpdate(int time) {
	Buf* pBuf;
	DiskINode dINode;
	FileSystem& fileSystem = g_FileSystem;
	Buf_Manager& bufferManager = g_Buf_Manager;

	/*
	 *��IUPD��IACC��־֮һ�����ã�����Ҫ������ӦDiskInode
	 * Ŀ¼����������������;����Ŀ¼�ļ���IACC��IUPD��־
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

/* ����������ļ�ʱ���ڴ��ļ����������з���һ�����б��� */
int OpenFiles::AllocFreeSlot() {
	User& u = g_User;
	for (int i = 0; i < OpenFiles::MAX_FILES; i++) {
		/* ���̴��ļ������������ҵ�������򷵻�֮ */
		if (this->processOpenFileTable[i] == NULL) {
			u.ar0[User::EAX] = i;
			return i;
		}
	}

	u.ar0[User::EAX] = -1;   /* Open1����Ҫһ����־�������ļ��ṹ����ʧ��ʱ�����Ի���ϵͳ��Դ*/
	u.u_error = User::U_EMFILE;
	return -1;
}

/* �����û�ϵͳ�����ṩ���ļ�����������fd���ҵ���Ӧ�Ĵ��ļ����ƿ�File�ṹ */
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

/* Ϊ�ѷ��䵽�Ŀ���������fd���ѷ���Ĵ��ļ����п���File������������ϵ */
void OpenFiles::SetF(int fd, File* pFile) {
	if (fd < 0 || fd >= OpenFiles::MAX_FILES) {
		return;
	}
	this->processOpenFileTable[fd] = pFile;
}
