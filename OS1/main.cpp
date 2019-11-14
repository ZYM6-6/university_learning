#include "INode.h"
#include "File_System.h"

#include "File_Manager.h"
#include "User.h"
#include <iostream>
#include <unordered_map>
using namespace std;

Device_Driver g_Device_Driver;
Buf_Manager g_Buf_Manager;
OpenFileTable g_OpenFileTable;
SuperBlock g_SuperBlock;
FileSystem g_FileSystem;
INodeTable g_INodeTable;
FileManager g_FileManager;
User g_User;

void show_man(string command) {

	static string man =
		"����			:  man -��ʾ���߰����ֲ� \n"
		"˵��			:  �����û�ʹ����Ӧ���� \n"
		"ʹ�÷���		:  man [-����] \n"
		"����			:  [����] ���£�  \n"
		"                 man          :  �ֲ� \n"
		"                 fformat      :  ��ʽ�� \n"
		"                 exit         :  ��ȷ�˳� \n"
		"                 create       :  �½��ļ� \n"
		"                 write        :  д���ļ� \n"
		"                 read         :  ��ȡ�ļ� \n"
		"                 delete       :  ɾ���ļ� \n"
		"                 open         :  ���ļ� \n"
		"                 close        :  �ر��ļ� \n"
		"                 seek         :  �ƶ���дָ�� \n"
		"                 mkdir        :  �½�Ŀ¼ \n"
		"                 cd           :  �ı�Ŀ¼ \n"
		"                 ls           :  �г�Ŀ¼���ļ� \n"
		"                 test         :  ���� \n"
		"ʹ��ʾ��		:  man -exit \n"
		;

	static string fformat =
		"����       :  fformat -�����ļ�ϵͳ��ʽ�� \n"
		"˵��		:  ���ļ�ϵͳ���и�ʽ�������ã��� \n"
		"ʹ�÷���   :  fformat \n"
		;

	static string exit =
		"����       :  exit -�˳��ļ�ϵͳ \n"
		"˵��		:  ��Ҫ�˳�����Ҫͨ��exit��� \n"
		"ʹ�÷���   :  exit \n"
		;
	static string delet =
		"����       :  delete -ɾ���ļ� \n"
		"˵��		:  ɾ��һ���ļ��� \n"
		"ʹ�÷���   :  delete <�ļ���> \n"
		"����		:  <�ļ���> ���԰������·�������·�� \n"
		"ʹ��ʾ��   :  delete (/·��/)fileName \n"
		"����		:  �ļ���������Ŀ¼·�������ڣ�Ŀ¼������Ŀ¼�� \n"
		;

	static string open =
		"����       :  open -���ļ� \n"
		"˵��		:  ��Unix|Linux����open����һ���ļ���\n"
		"ʹ�÷���   :  open <�ļ���> <ѡ��> \n"
		"����		:  <�ļ���> ���԰������·�������·�� \n"
		"                 <ѡ��> -r ֻ������ \n"
		"                 <ѡ��> -w ֻд���� \n"
		"ʹ��ʾ��   :  open (/·��/)fileName -r \n"
		"����		:  �ļ���������Ŀ¼·�������ڣ�Ŀ¼������Ŀ¼�� \n"
		;

	static string close =
		"����       :  close -�ر��ļ� \n"
		"˵��		:  ��Unix|Linux����close���ر�һ���ļ��� \n"
		"ʹ�÷���   :  close <file descriptor> \n"
		"����		:  <file descriptor> �ļ������� \n"
		"ʹ��ʾ��   :  close 1 \n"
		"����		:  �ļ������������ڻ򳬳���Χ \n"
		;

	static string seek =
		"����       :  seek -д���ļ� \n"
		"˵��		:  ��Unix|Linux����fseek��д��һ���Ѿ��򿪵��ļ��� \n"
		"ʹ�÷���	:  seek <file descriptor> <offset> <origin> \n"
		"����		:  <file descriptor> open���ص��ļ������� \n"
		"                 <offset> ָ���� <origin> ��ʼ��ƫ���� �����ɸ� \n"
		"                 <origin> ָ����ʼλ�� ��Ϊ0(SEEK_SET), 1(SEEK_CUR), 2(SEEK_END) \n"
		"ʹ��ʾ��   :  seek 1 500 0 \n"
		"����		:  �ļ������������ڻ򳬳���Χ��δ��ȷָ������ \n"
		;

	static string write =
		"����       :  write -д���ļ� \n"
		"˵��		:  ��Unix|Linux����write��д��һ���Ѿ��򿪵��ļ��С� \n"
		"ʹ�÷���   :  write <file descriptor> <InFileName> <size> \n"
		"����		:  <file descriptor> open���ص��ļ������� \n"
		"                 <InFileName> ָ��д������Ϊ�ļ�InFileName�е����� \n"
		"                 <size> ָ��д���ֽ�������СΪ <size> \n"
		"ʹ��ʾ��   :  write 1 InFileName 123 \n"
		"����		:  �ļ������������ڻ򳬳���Χ��δ��ȷָ������ \n"
		;

	static string read =
		"����       :  read -��ȡ�ļ� \n"
		"˵��		:  ��Unix|Linux����read����һ���Ѿ��򿪵��ļ��ж�ȡ�� \n"
		"ʹ�÷���   :  read <file descriptor> [-o <OutFileName>] <size> \n"
		"����		:  <file descriptor> open���ص��ļ������� \n"
		"                 [-o <OutFileName>] -o ָ�������ʽΪ�ļ����ļ���Ϊ <OutFileName> Ĭ��Ϊshell \n"
		"                 <size> ָ����ȡ�ֽ�������СΪ <size> \n"
		"ʹ��ʾ��   :  read 1 -o OutFileName 123 \n"
		"                 read 1 123 \n"
		"����		:  �ļ������������ڻ򳬳���Χ��δ��ȷָ������ \n"
		;
	static string mkdir =
		"����       :  mkdir -����Ŀ¼ \n"
		"˵��		:  �½�һ��Ŀ¼�� \n"
		"ʹ�÷���   :  mkdir <Ŀ¼��> \n"
		"����		:  <Ŀ¼��> ���������·����Ҳ�����Ǿ���·�� \n"
		"ʹ��ʾ��   :  mkdir (/·��/)dirName \n"
		"����		:  Ŀ¼��������Ŀ¼·�������ڣ�Ŀ¼������Ŀ¼�� \n"
		;

	static string ls =
		"����       :  ls -��Ŀ¼���� \n"
		"˵��		:  �г���ǰĿ¼�а������ļ�����Ŀ¼����\n"
		"ʹ�÷���   :  ls \n"
		;

	static string cd =
		"����       :  cd -�ı䵱ǰĿ¼ \n"
		"˵��		:  �ı䵱ǰ����Ŀ¼�� \n"
		"ʹ�÷���   :  cd <Ŀ¼��> \n"
		"����		:  <Ŀ¼��>Ĭ��Ϊ��ǰĿ¼��\n"
		"                 <Ŀ¼��> ���������·����Ҳ�����Ǿ���·�� \n"
		"ʹ��ʾ��   :  cd (/·��/)\n"
		"����		:  Ŀ¼��������Ŀ¼·�������ڣ�Ŀ¼������Ŀ¼�� \n"
		;

	static string create =
		"����       :  create -�½��ļ� \n"
		"˵��		:  �½�һ���ļ��� \n"
		"ʹ�÷���   :  create <�ļ���> <ѡ��> \n"
		"����		:  <�ļ���> ���԰������·�������·�� \n"
		"                 <ѡ��> -r ֻ������ \n"
		"                 <ѡ��> -w ֻд���� \n"
		"ʹ��ʾ��   :  create (/·��/)newFileName -rw \n"
		"����		:  �ļ���������Ŀ¼·�������ڣ�Ŀ¼������Ŀ¼�� \n"
		;



	static string test =
		"����       :  test -�Զ����� \n"
		"˵��		:  �������ԣ���ϵͳ�������ڰ������ԡ�\n"
		"ʹ�÷���   :  test \n"
		;

	static unordered_map<string, string*>manMap({
		{ "man", &man },
		{ "-fformat", &fformat },
		{ "-exit", &exit },
		{ "-open", &open },
		{ "-close", &close },
		{ "-seek", &seek },
		{ "-write", &write },
		{ "-read", &read },
		{ "-mkdir", &mkdir },
		{ "-cd", &cd },
		{ "-ls", &ls },
		{ "-create", &create },
		{ "-delete", &delet },
		{ "-test", &test },
		});

	auto it = manMap.find(command);
	if (it == manMap.end()) {
		cout << "shell : " << command << " : don't find this commond \n";
		return;
	}
	cout << *it->second;
}

bool autoTest(string& in) {
	static int testNo = 0;
	static char CMD[][50] = {
		"mkdir /dir1",
		"mkdir dir2",
		"create file1 -rw",
		"create file2 -rw",
		"ls",
		"delete /file2",
		"ls",
		"mkdir /dir1/dir11",
		"create /dir1/file11 -rw",
		"cd dir1",
		"ls",
		"cd ..",
		"ls",
		"open file1 -rw",
		"write 6 file1-2000.txt 800",
		"seek 6 500 0",
		"read 6 20",
		"seek 6 500 0",
		"read 6 -o readOut1.txt 20",
		"seek 6 -20 1",
		"read 6 100",
		"close 6",
		"cd dir1",
		"ls",
		"open file11 -rw",
		"write 6 file11-100000.txt 100000",
		"seek 6 0 0",
		"read 6 200",
		"read 6 200",
		"seek 6 40000 0",
		"read 6 200",
		"#"
	};
	int inNums = sizeof(CMD) / sizeof(char*);
	in = CMD[testNo % inNums];
	return ++testNo <= inNums;
}

void inArgs(const string& in, vector<string>& args) {
	args.clear();
	string str;
	unsigned int p, q;
	for (p = 0, q = 0; q < in.length(); p = q + 1) {
		q = in.find_first_of(" \n", p);
		str = in.substr(p, q - p);
		if (!str.empty()) {
			args.push_back(str);
		}
		if (q == string::npos)
			return;
	}
}

int judgement(string in)
{
	if (in == "man")
		return 0;
	else if (in == "fformat")
	{
		cout << "[unix-Zhang Yiming ]$ ��ʽ�����" << endl;
		return 1;
	}
	else if (in == "exit") {
		return 2;
	}
	else if (in == "test") {
		return 3;
	}
	else if (in == "create") {
		return 4;
	}
	else if (in == "delete") {
		return 5;
	}
	else if (in == "open") {
		return 6;
	}
	else if (in == "close") {
		return 7;
	}
	else if (in == "seek") {
		return 8;
	}
	else if (in == "read") {
		return 9;
	}
	else if (in == "write") {
		return 10;
	}
	else if (in == "mkdir") {
		return 11;
		
	}
	else if (in == "ls") {
		return 12;
	}
	else if (in == "cd") {
		return 13;
	}
	return 14;
}

int main() {
	User* user = &g_User;

	string line = "man";
	vector<string> args;
	string in, parameter1, parameter2, parameter3;
	int flag = 0;
	cout << "++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
	cout << "+                                                +" << endl;
	cout << "+         ������ʹ�õ���UNIX�ļ�ϵͳ             +" << endl;
	cout << "+                                                +" << endl;
	cout << "++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
	for(;1;)
	 {
		if (line == "")
		{
			cout << "[unix-Zhang Yiming " << user->curDirPath << " ]$ ";
			if (flag) {
				if (autoTest(line) && line != "#") {
					cout << line;
					getchar();
				}
				else {
					cout << "test finished ... \n";
					flag = 0;
					line = "";
				}
			}
			else {
				getline(cin, line);
			}
		}
		else {
			inArgs(line, args);
			in = args[0];
			parameter1 = args.size() > 1 ? args[1] : "";
			parameter2 = args.size() > 2 ? args[2] : "";
			parameter3 = args.size() > 3 ? args[3] : "";
			int h = judgement(in);
			switch (h) {
			case 0:
				show_man(parameter1.empty() ? "man" : parameter1);
				break;
			case 1:
				g_OpenFileTable.Format();
				g_INodeTable.Format();
				g_Buf_Manager.Format_Buf();
				g_FileSystem.FormatDevice();
				exit(0);
				break;
			case 2:
				exit(0);
				break;
			case 3:
				flag = 1;
				cout << "test begin ... \njust press enter to continue ... \n";
				break;
			case 4:
				user->U_Create(parameter1, parameter2 + parameter3);
				break;
			case 5:
				user->U_Delete(parameter1);
				break;
			case 6:
				user->U_Open(parameter1, line);
				break;
			case 7:
				user->U_Close(parameter1);
				break;
			case 8:
				user->U_Seek(parameter1, parameter2, parameter3);
				break;
			case 9:
				if (parameter2 == "-o")
					user->U_Read(parameter1, parameter3, args[4]);
				else {
					user->U_Read(parameter1, "", parameter2);
				}
				break;
			case 10:
				user->U_Write(parameter1, parameter2, parameter3);
				break;
			case 11:
				user->U_Mkdir(args[1]);
				break;
			case 12:
				user->U_Ls();
				break;
			case 13:
				user->U_Cd(parameter1);
				break;
			case 14:
				cout << "shell : " << in << " : don't find this commond \n";
				break;
			}		
			cout << "[unix-Zhang Yiming " << user->curDirPath << " ]$ ";
			if (flag) {
				if (autoTest(line) && line != "#") {
					cout << line;
					getchar();
				}
				else {
					cout << "autoTest finished ... \n";
					flag = 0;
					line = "";
				}
			}
			else {
				getline(cin, line);
			}
		}
	}
	return 0;
}