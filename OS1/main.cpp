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
		"命令			:  man -显示在线帮助手册 \n"
		"说明			:  帮助用户使用相应命令 \n"
		"使用方法		:  man [-命令] \n"
		"参数			:  [命令] 如下：  \n"
		"                 man          :  手册 \n"
		"                 fformat      :  格式化 \n"
		"                 exit         :  正确退出 \n"
		"                 create       :  新建文件 \n"
		"                 write        :  写入文件 \n"
		"                 read         :  读取文件 \n"
		"                 delete       :  删除文件 \n"
		"                 open         :  打开文件 \n"
		"                 close        :  关闭文件 \n"
		"                 seek         :  移动读写指针 \n"
		"                 mkdir        :  新建目录 \n"
		"                 cd           :  改变目录 \n"
		"                 ls           :  列出目录及文件 \n"
		"                 test         :  测试 \n"
		"使用示例		:  man -exit \n"
		;

	static string fformat =
		"命令       :  fformat -进行文件系统格式化 \n"
		"说明		:  将文件系统进行格式化（慎用）。 \n"
		"使用方法   :  fformat \n"
		;

	static string exit =
		"命令       :  exit -退出文件系统 \n"
		"说明		:  若要退出程序，要通过exit命令。 \n"
		"使用方法   :  exit \n"
		;
	static string delet =
		"命令       :  delete -删除文件 \n"
		"说明		:  删除一个文件。 \n"
		"使用方法   :  delete <文件名> \n"
		"参数		:  <文件名> 可以包含相对路径或绝对路径 \n"
		"使用示例   :  delete (/路径/)fileName \n"
		"错误		:  文件名过长，目录路径不存在，目录超出根目录等 \n"
		;

	static string open =
		"命令       :  open -打开文件 \n"
		"说明		:  类Unix|Linux函数open，打开一个文件。\n"
		"使用方法   :  open <文件名> <选项> \n"
		"参数		:  <文件名> 可以包含相对路径或绝对路径 \n"
		"                 <选项> -r 只读属性 \n"
		"                 <选项> -w 只写属性 \n"
		"使用示例   :  open (/路径/)fileName -r \n"
		"错误		:  文件名过长，目录路径不存在，目录超出根目录等 \n"
		;

	static string close =
		"命令       :  close -关闭文件 \n"
		"说明		:  类Unix|Linux函数close，关闭一个文件。 \n"
		"使用方法   :  close <file descriptor> \n"
		"参数		:  <file descriptor> 文件描述符 \n"
		"使用示例   :  close 1 \n"
		"错误		:  文件描述符不存在或超出范围 \n"
		;

	static string seek =
		"命令       :  seek -写入文件 \n"
		"说明		:  类Unix|Linux函数fseek，写入一个已经打开的文件中 \n"
		"使用方法	:  seek <file descriptor> <offset> <origin> \n"
		"参数		:  <file descriptor> open返回的文件描述符 \n"
		"                 <offset> 指定从 <origin> 开始的偏移量 可正可负 \n"
		"                 <origin> 指定起始位置 可为0(SEEK_SET), 1(SEEK_CUR), 2(SEEK_END) \n"
		"使用示例   :  seek 1 500 0 \n"
		"错误		:  文件描述符不存在或超出范围，未正确指定参数 \n"
		;

	static string write =
		"命令       :  write -写入文件 \n"
		"说明		:  类Unix|Linux函数write，写入一个已经打开的文件中。 \n"
		"使用方法   :  write <file descriptor> <InFileName> <size> \n"
		"参数		:  <file descriptor> open返回的文件描述符 \n"
		"                 <InFileName> 指定写入内容为文件InFileName中的内容 \n"
		"                 <size> 指定写入字节数，大小为 <size> \n"
		"使用示例   :  write 1 InFileName 123 \n"
		"错误		:  文件描述符不存在或超出范围，未正确指定参数 \n"
		;

	static string read =
		"命令       :  read -读取文件 \n"
		"说明		:  类Unix|Linux函数read，从一个已经打开的文件中读取。 \n"
		"使用方法   :  read <file descriptor> [-o <OutFileName>] <size> \n"
		"参数		:  <file descriptor> open返回的文件描述符 \n"
		"                 [-o <OutFileName>] -o 指定输出方式为文件，文件名为 <OutFileName> 默认为shell \n"
		"                 <size> 指定读取字节数，大小为 <size> \n"
		"使用示例   :  read 1 -o OutFileName 123 \n"
		"                 read 1 123 \n"
		"错误		:  文件描述符不存在或超出范围，未正确指定参数 \n"
		;
	static string mkdir =
		"命令       :  mkdir -建立目录 \n"
		"说明		:  新建一个目录。 \n"
		"使用方法   :  mkdir <目录名> \n"
		"参数		:  <目录名> 可以是相对路径，也可以是绝对路径 \n"
		"使用示例   :  mkdir (/路径/)dirName \n"
		"错误		:  目录名过长，目录路径不存在，目录超出根目录等 \n"
		;

	static string ls =
		"命令       :  ls -列目录内容 \n"
		"说明		:  列出当前目录中包含的文件名和目录名。\n"
		"使用方法   :  ls \n"
		;

	static string cd =
		"命令       :  cd -改变当前目录 \n"
		"说明		:  改变当前工作目录。 \n"
		"使用方法   :  cd <目录名> \n"
		"参数		:  <目录名>默认为当前目录；\n"
		"                 <目录名> 可以是相对路径，也可以是绝对路径 \n"
		"使用示例   :  cd (/路径/)\n"
		"错误		:  目录名过长，目录路径不存在，目录超出根目录等 \n"
		;

	static string create =
		"命令       :  create -新建文件 \n"
		"说明		:  新建一个文件。 \n"
		"使用方法   :  create <文件名> <选项> \n"
		"参数		:  <文件名> 可以包含相对路径或绝对路径 \n"
		"                 <选项> -r 只读属性 \n"
		"                 <选项> -w 只写属性 \n"
		"使用示例   :  create (/路径/)newFileName -rw \n"
		"错误		:  文件名过长，目录路径不存在，目录超出根目录等 \n"
		;



	static string test =
		"命令       :  test -自动测试 \n"
		"说明		:  帮助测试，在系统启动初期帮助测试。\n"
		"使用方法   :  test \n"
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
		cout << "[unix-Zhang Yiming ]$ 格式化完成" << endl;
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
	cout << "+         您现在使用的是UNIX文件系统             +" << endl;
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