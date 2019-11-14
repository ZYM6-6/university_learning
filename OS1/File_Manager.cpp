#include "Buf.h"
#include "File_Manager.h"
#include "User.h"

extern Buf_Manager g_Buf_Manager;
extern FileSystem g_FileSystem;
extern INodeTable g_INodeTable;
extern OpenFileTable g_OpenFileTable;
extern User g_User;

FileManager::FileManager() {
	fileSystem = &g_FileSystem;
	openFileTable = &g_OpenFileTable;
	inodeTable = &g_INodeTable;
	rootDirINode = inodeTable->IGet(FileSystem::ROOT_INODE_NO);
	rootDirINode->i_count += 0xff;
	//rootDirINode = NULL;
}

FileManager::~FileManager() {

}

/*
* ���ܣ����ļ�
* Ч�����������ļ��ṹ���ڴ�i�ڵ㿪�� ��i_count Ϊ������i_count ++��
* */
void FileManager::Open() {
	User &u = g_User;
	INode* pINode;
	pINode = this->NameI(FileManager::OPEN);
	if (NULL == pINode) {
		return;
	}
	this->Open1(pINode, u.arg[1], 0);
}

void FileManager::Creat() {
	INode* pINode;
	User& u = g_User;
	//unsigned int newACCMode = u.arg[1] & (INode::IRWXU | INode::IRWXG | INode::IRWXO);
	unsigned int newACCMode = u.arg[1];

	/* ����Ŀ¼��ģʽΪ1����ʾ����������Ŀ¼����д�������� */
	pINode = this->NameI(FileManager::CREATE);

	/* û���ҵ���Ӧ��INode����NameI���� */
	if (NULL == pINode) {
		if (u.u_error)
			return;
		pINode = this->MakNode(newACCMode);
		if (NULL == pINode)
			return;

		/* ������������ֲ����ڣ�ʹ�ò���trf = 2������open1()��*/
		this->Open1(pINode, File::FWRITE, 2);
		return;
	}

	/* ���NameI()�������Ѿ�����Ҫ�������ļ�������ո��ļ������㷨ITrunc()��*/
	this->Open1(pINode, File::FWRITE, 1);
	pINode->i_mode |= newACCMode;
}

/* ����NULL��ʾĿ¼����ʧ�ܣ�δ�ҵ�u.dirp��ָ��Ŀ¼ȫ·��
 * �����Ǹ�ָ�룬ָ���ļ����ڴ��i�ڵ�
 */
INode* FileManager::NameI(enum DirectorySearchMode mode) {

	Buf_Manager& bufferManager = g_Buf_Manager;
	User &u = g_User;
	INode* pINode = u.cdir;
	Buf* pBuf;
	int freeEntryOffset;
	unsigned int index = 0, nindex = 0;

	if ('/' == u.dirp[0]) {
		nindex = ++index + 1;
		pINode = this->rootDirINode;
	}


	/* ���ѭ��ÿ�δ���pathname��һ��·������ */
	while (1) {
		if (u.u_error != User::U_NOERROR) {
			break;
		}
		if (nindex >= u.dirp.length()) {
			return pINode;
		}

		/* ���Ҫ���������Ĳ���Ŀ¼�ļ����ͷ����INode��Դ���˳� */
		if ((pINode->i_mode&INode::IFMT) != INode::IFDIR) {
			u.u_error = User::U_ENOTDIR;
			break;
		}

		nindex = u.dirp.find_first_of('/', index);
		Common::memset(u.dbuf, 0, sizeof(u.dbuf));
		Common::memcpy(u.dbuf, u.dirp.data() + index, (nindex == (unsigned int)string::npos ? u.dirp.length() : nindex) - index);
		index = nindex + 1;

		/* �ڲ�ѭ�����ֶ���u.dbuf[]�е�·���������������Ѱƥ���Ŀ¼�� */
		u.IOParam.offset = 0;
		/* ����ΪĿ¼����� �����հ׵�Ŀ¼��*/
		u.IOParam.count = pINode->i_size / sizeof(DirectoryEntry);
		freeEntryOffset = 0;
		pBuf = NULL;

		/* ��һ��Ŀ¼��Ѱ�� */
		while (1) {

			/* ��Ŀ¼���Ѿ�������� */
			if (0 == u.IOParam.count) {
				if (NULL != pBuf) {
					bufferManager.Buf_Relse(pBuf);
				}

				/* ����Ǵ������ļ� */
				if (FileManager::CREATE == mode && nindex >= u.dirp.length()) {
					u.pdir = pINode;
					if (freeEntryOffset) {
						u.IOParam.offset = freeEntryOffset - sizeof(DirectoryEntry);
					}
					else {
						pINode->i_flag |= INode::IUPD;
					}
					return NULL;
				}
				u.u_error = User::U_ENOENT;
				goto out;
			}

			/* �Ѷ���Ŀ¼�ļ��ĵ�ǰ�̿飬��Ҫ������һĿ¼�������̿� */
			if (0 == u.IOParam.offset%INode::BLOCK_SIZE) {
				if (pBuf) {
					bufferManager.Buf_Relse(pBuf);
				}
				int phyBlkno = pINode->Bmap(u.IOParam.offset / INode::BLOCK_SIZE);
				pBuf = bufferManager.Buf_Read(phyBlkno);
				//pBuf->debug();
			}

			Common::memcpy(&u.dent, pBuf->Address + (u.IOParam.offset % INode::BLOCK_SIZE), sizeof(u.dent));
			u.IOParam.offset += sizeof(DirectoryEntry);
			u.IOParam.count--;

			/* ����ǿ���Ŀ¼���¼����λ��Ŀ¼�ļ���ƫ���� */
			if (0 == u.dent.m_ino) {
				if (0 == freeEntryOffset) {
					freeEntryOffset = u.IOParam.offset;
				}
				continue;
			}

			if (!Common::memcmp(u.dbuf, &u.dent.name, sizeof(DirectoryEntry) - 4)) {
				break;
			}
		}

		if (pBuf) {
			bufferManager.Buf_Relse(pBuf);
		}

		/* �����ɾ���������򷵻ظ�Ŀ¼INode����Ҫɾ���ļ���INode����u.dent.m_ino�� */
		if (FileManager::DELETE == mode && nindex >= u.dirp.length()) {
			return pINode;
		}

		/*
		* ƥ��Ŀ¼��ɹ������ͷŵ�ǰĿ¼INode������ƥ��ɹ���
		* Ŀ¼��m_ino�ֶλ�ȡ��Ӧ��һ��Ŀ¼���ļ���INode��
		*/
		this->inodeTable->IPut(pINode);
		pINode = this->inodeTable->IGet(u.dent.m_ino);

		if (NULL == pINode) {
			return NULL;
		}
	}

out:
	this->inodeTable->IPut(pINode);
	return NULL;
}

/*
* trf == 0��open����
* trf == 1��creat���ã�creat�ļ���ʱ��������ͬ�ļ������ļ�
* trf == 2��creat���ã�creat�ļ���ʱ��δ������ͬ�ļ������ļ��������ļ�����ʱ��һ������
* mode���������ļ�ģʽ����ʾ�ļ������� ����д���Ƕ�д
*/
void FileManager::Open1(INode* pINode, int mode, int trf) {
	User& u = g_User;

	/* ��creat�ļ���ʱ��������ͬ�ļ������ļ����ͷŸ��ļ���ռ�ݵ������̿� */
	if (1 == trf) {
		pINode->ITrunc();
	}

	/* ������ļ����ƿ�File�ṹ */
	File* pFile = this->openFileTable->FAlloc();
	if (NULL == pFile) {
		this->inodeTable->IPut(pINode);
		return;
	}

	/* ���ô��ļ���ʽ������File�ṹ���ڴ�INode�Ĺ�����ϵ */
	pFile->flag = mode & (File::FREAD | File::FWRITE);
	pFile->inode = pINode;

	/* Ϊ�򿪻��ߴ����ļ��ĸ�����Դ���ѳɹ����䣬�������� */
	if (u.u_error == 0) {
		return;
	}
	else {  /* ����������ͷ���Դ */
		/* �ͷŴ��ļ������� */
		int fd = u.ar0[User::EAX];
		if (fd != -1) {
			u.ofiles.SetF(fd, NULL);
			/* �ݼ�File�ṹ��INode�����ü��� ,File�ṹû���� f_countΪ0�����ͷ�File�ṹ��*/
			pFile->count--;
		}
		this->inodeTable->IPut(pINode);
	}
}

/* ��creat���á�
 * Ϊ�´������ļ�д�µ�i�ڵ�͸�Ŀ¼���µ�Ŀ¼��(��Ӧ������User�ṹ��)
 * ���ص�pINode�����������ڴ�i�ڵ㣬���е�i_count�� 1��
 */
INode* FileManager::MakNode(unsigned int mode) {
	INode* pINode;
	User& u = g_User;

	/* ����һ������DiskInode������������ȫ����� */
	pINode = this->fileSystem->IAlloc();
	if (NULL == pINode) {
		return NULL;
	}

	pINode->i_flag |= (INode::IACC | INode::IUPD);
	pINode->i_mode = mode | INode::IALLOC;
	pINode->i_nlink = 1;

	/* ��Ŀ¼��д��u.u_u_dent�����д��Ŀ¼�ļ� */
	this->WriteDir(pINode);
	return pINode;
}

/* ��creat���ӵ��á�
 * �������Լ���Ŀ¼��д����Ŀ¼���޸ĸ�Ŀ¼�ļ���i�ڵ� ������д�ش��̡�
 */
void FileManager::WriteDir(INode* pINode) {
	User& u = g_User;
	//cout << "i_number=" << pINode->i_number << endl;
	/* ����Ŀ¼����INode��Ų��� */
	u.dent.m_ino = pINode->i_number;

	/* ����Ŀ¼����pathname�������� */
	Common::memcpy(u.dent.name, u.dbuf, DirectoryEntry::DIRSIZ);

	u.IOParam.count = DirectoryEntry::DIRSIZ + 4;
	u.IOParam.base = (unsigned char *)&u.dent;

	/* ��Ŀ¼��д�븸Ŀ¼�ļ� */
	u.pdir->WriteI();
	this->inodeTable->IPut(u.pdir);
}

void FileManager::Close() {
	User& u = g_User;
	int fd = u.arg[0];

	/* ��ȡ���ļ����ƿ�File�ṹ */
	File* pFile = u.ofiles.GetF(fd);
	if (NULL == pFile) {
		return;
	}

	/* �ͷŴ��ļ�������fd���ݼ�File�ṹ���ü��� */
	u.ofiles.SetF(fd, NULL);
	this->openFileTable->CloseF(pFile);
}

void FileManager::UnLink() {
	//ע��ɾ���ļ����д���й¶
	INode* pINode;
	INode* pDeleteINode;
	User& u = g_User;

	pDeleteINode = this->NameI(FileManager::DELETE);
	if (NULL == pDeleteINode) {
		return;
	}

	pINode = this->inodeTable->IGet(u.dent.m_ino);
	if (NULL == pINode) {
		return;
	}

	/* д��������Ŀ¼�� */
	u.IOParam.offset -= (DirectoryEntry::DIRSIZ + 4);
	u.IOParam.base = (unsigned char *)&u.dent;
	u.IOParam.count = DirectoryEntry::DIRSIZ + 4;

	u.dent.m_ino = 0;
	pDeleteINode->WriteI();

	/* �޸�inode�� */
	pINode->i_nlink--;
	pINode->i_flag |= INode::IUPD;

	this->inodeTable->IPut(pDeleteINode);
	this->inodeTable->IPut(pINode);
}

void FileManager::Seek() {
	File* pFile;
	User& u = g_User;
	int fd = u.arg[0];

	pFile = u.ofiles.GetF(fd);
	if (NULL == pFile) {
		return;  /* ��FILE�����ڣ�GetF��������� */
	}

	int offset = u.arg[1];

	switch (u.arg[2]) {
		/* ��дλ������Ϊoffset */
	case 0:
		pFile->offset = offset;
		break;
		/* ��дλ�ü�offset(�����ɸ�) */
	case 1:
		pFile->offset += offset;
		break;
		/* ��дλ�õ���Ϊ�ļ����ȼ�offset */
	case 2:
		pFile->offset = pFile->inode->i_size + offset;
		break;
	default:
		cout << " origin " << u.arg[2] << " is undefined ! \n";
		break;
	}
}

void FileManager::Read() {
	/* ֱ�ӵ���Rdwr()�������� */
	this->Rdwr(File::FREAD);
}

void FileManager::Write() {
	/* ֱ�ӵ���Rdwr()�������� */
	this->Rdwr(File::FWRITE);
}

void FileManager::Rdwr(enum File::FileFlags mode) {
	File* pFile;
	User& u = g_User;

	/* ����Read()/Write()��ϵͳ���ò���fd��ȡ���ļ����ƿ�ṹ */
	pFile = u.ofiles.GetF(u.arg[0]);	/* fd */
	if (NULL == pFile) {
		/* �����ڸô��ļ���GetF�Ѿ����ù������룬�������ﲻ��Ҫ�������� */
		/*	u.u_error = User::EBADF;	*/
		return;
	}

	/* ��д��ģʽ����ȷ */
	if ((pFile->flag & mode) == 0) {
		u.u_error = User::U_EACCES;
		return;
	}

	u.IOParam.base = (unsigned char *)u.arg[1];     /* Ŀ�껺������ַ */
	u.IOParam.count = u.arg[2];		/* Ҫ���/д���ֽ��� */

	u.IOParam.offset = pFile->offset;   /* �����ļ���ʼ��λ�� */
	
	if (File::FREAD == mode) {
		//printf("��д��λ��%d\n", pFile->offset);
		pFile->inode->ReadI();
	}
	else {
		pFile->inode->WriteI();
	}

	/* ���ݶ�д�������ƶ��ļ���дƫ��ָ�� */
	pFile->offset += (u.arg[2] - u.IOParam.count);

	/* ����ʵ�ʶ�д���ֽ������޸Ĵ��ϵͳ���÷���ֵ�ĺ���ջ��Ԫ */
	u.ar0[User::EAX] = u.arg[2] - u.IOParam.count;
}

void FileManager::Ls() {
	User &u = g_User;
	Buf_Manager& bufferManager = g_Buf_Manager;

	INode* pINode = u.cdir;
	Buf* pBuf = NULL;
	u.IOParam.offset = 0;
	u.IOParam.count = pINode->i_size / sizeof(DirectoryEntry);

	while (u.IOParam.count) {
		if (0 == u.IOParam.offset%INode::BLOCK_SIZE) {
			if (pBuf) {
				bufferManager.Buf_Relse(pBuf);
			}
			int phyBlkno = pINode->Bmap(u.IOParam.offset / INode::BLOCK_SIZE);
			pBuf = bufferManager.Buf_Read(phyBlkno);
		}
		Common::memcpy(&u.dent, pBuf->Address + (u.IOParam.offset % INode::BLOCK_SIZE), sizeof(u.dent));
		u.IOParam.offset += sizeof(DirectoryEntry);
		u.IOParam.count--;

		if (0 == u.dent.m_ino)
			continue;
		u.ls += u.dent.name;
		u.ls += "\n";
	}

	if (pBuf) {
		bufferManager.Buf_Relse(pBuf);
	}
}

/* �ı䵱ǰ����Ŀ¼ */
void FileManager::ChDir() {
	INode* pINode;
	User& u = g_User;

	pINode = this->NameI(FileManager::OPEN);
	if (NULL == pINode) {
		return;
	}

	/* ���������ļ�����Ŀ¼�ļ� */
	if ((pINode->i_mode & INode::IFMT) != INode::IFDIR) {
		u.u_error = User::U_ENOTDIR;
		this->inodeTable->IPut(pINode);
		return;
	}

	u.cdir = pINode;
	/* ·�����ǴӸ�Ŀ¼'/'��ʼ����������u.u_curdir������ϵ�ǰ·������ */
	if (u.dirp[0] != '/') {
		u.curDirPath += u.dirp;
	}
	else {
		/* ����ǴӸ�Ŀ¼'/'��ʼ����ȡ��ԭ�й���Ŀ¼ */
		u.curDirPath = u.dirp;
	}
	if (u.curDirPath.back() != '/')
		u.curDirPath.push_back('/');
}



OpenFileTable::OpenFileTable() {
}

OpenFileTable::~OpenFileTable() {
}


void OpenFileTable::Format() {
	File emptyFile;
	for (int i = 0; i < OpenFileTable::MAX_FILES; ++i) {
		Common::memcpy(m_File + i, &emptyFile, sizeof(File));
	}
}

/*���ã����̴��ļ������������ҵĿ�����֮�±�д�� ar0[EAX]*/
File* OpenFileTable::FAlloc() {
	User& u = g_User;
	int fd = u.ofiles.AllocFreeSlot();
	if (fd < 0) {
		return NULL;
	}
	for (int i = 0; i < OpenFileTable::MAX_FILES; ++i) {
		/* count==0��ʾ������� */
		if (this->m_File[i].count == 0) {
			u.ofiles.SetF(fd, &this->m_File[i]);
			this->m_File[i].count++;
			this->m_File[i].offset = 0;
			return (&this->m_File[i]);
		}
	}
	u.u_error = User::U_ENFILE;
	return NULL;
}

/* �Դ��ļ����ƿ�File�ṹ�����ü���count��1�������ü���countΪ0�����ͷ�File�ṹ��*/
void OpenFileTable::CloseF(File* pFile) {
	pFile->count--;
	if (pFile->count <= 0) {
		g_INodeTable.IPut(pFile->inode);
	}
}

INodeTable::INodeTable() {
	fileSystem = &g_FileSystem;
}

INodeTable::~INodeTable() {

}

void INodeTable::Format() {
	INode emptyINode;
	for (int i = 0; i < INodeTable::NINODE; ++i) {
		Common::memcpy(m_INode + i, &emptyINode, sizeof(INode));
	}
}

//�����Ϊinumber�����INode�Ƿ����ڴ濽����������򷵻ظ��ڴ�INode���ڴ�INode���е�����
 
int INodeTable::IsLoaded(int inumber) {
	for (int i = 0; i < NINODE; ++i) {
		if (m_INode[i].i_number == inumber && m_INode[i].i_count) {
			return i;
		}
	}
	return -1;
}

/* ���ڴ�INode����Ѱ��һ�����е��ڴ�INode */
INode* INodeTable::GetFreeINode() {
	for (int i = 0; i < INodeTable::NINODE; i++) {
		if (this->m_INode[i].i_count == 0) {
			return m_INode + i;
		}
	}
	return NULL;
}

//�������INode��Ż�ȡ��ӦINode����INode���ڴ��У����ظ��ڴ�INode���������ڴ��У���������ڴ�����������ظ��ڴ�INode
INode* INodeTable::IGet(int inumber) {
	INode* pINode;
	int index = IsLoaded(inumber);
	if (index >= 0) {
		pINode = m_INode + index;
		++pINode->i_count;
		return pINode;
	}

	pINode = GetFreeINode();
	if (NULL == pINode) {
		cout << "INode Table Overflow !" << endl;
		g_User.u_error = User::U_ENFILE;
		return NULL;
	}

	pINode->i_number = inumber;
	pINode->i_count++;
	Buf_Manager& bmg = g_Buf_Manager;
	Buf* pBuffer = bmg.Buf_Read(FileSystem::INODE_ZONE_START_SECTOR + inumber / FileSystem::INODE_NUMBER_PER_SECTOR);
	pINode->ICopy(pBuffer, inumber);
	bmg.Buf_Relse(pBuffer);
	return pINode;
}

/*
 * ���ٸ��ڴ�INode�����ü����������INode�Ѿ�û��Ŀ¼��ָ������
 * ���޽������ø�INode�����ͷŴ��ļ�ռ�õĴ��̿顣
 */
void INodeTable::IPut(INode* pINode) {
	/* ��ǰ����Ϊ���ø��ڴ�INode��Ψһ���̣���׼���ͷŸ��ڴ�INode */
	if (pINode->i_count == 1) {
		/* ���ļ��Ѿ�û��Ŀ¼·��ָ���� */
		if (pINode->i_nlink <= 0) {
			/* �ͷŸ��ļ�ռ�ݵ������̿� */
			pINode->ITrunc();
			pINode->i_mode = 0;
			/* �ͷŶ�Ӧ�����INode */
			this->fileSystem->IFree(pINode->i_number);
		}

		/* �������INode��Ϣ */
		pINode->IUpdate((int)Common::time(NULL));

		/* ����ڴ�INode�����б�־λ */
		pINode->i_flag = 0;
		/* �����ڴ�inode���еı�־֮һ����һ����i_count == 0 */
		pINode->i_number = -1;
	}

	pINode->i_count--;
}

/* �����б��޸Ĺ����ڴ�INode���µ���Ӧ���INode�� */
void INodeTable::UpdateINodeTable() {
	for (int i = 0; i < INodeTable::NINODE; ++i) {
		if (this->m_INode[i].i_count) {
			this->m_INode[i].IUpdate((int)Common::time(NULL));
		}
	}
}