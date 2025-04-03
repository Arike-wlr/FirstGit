#include<iostream>
#include<cstring>
#include<cstdlib>
#include<ctime>
#include<fstream>
#include<string>
using namespace std;
struct Node
{
	string phone;
	string name;
	string time;
	string content;
	Node* next;
};
char randomlet()
{
	char randlet = 'A' + rand() % 26;
	return randlet;
}

bool phone(char phonenum[])
{
	if (strlen(phonenum) != 11) return false;
	if (phonenum[0] != '1')return false;
	else {
		if (phonenum[1] != '3' && phonenum[1] !=' 4' && phonenum[1] != '5' && phonenum[1] != '7' && phonenum[1] != '8')return false;
		else if (phonenum[1] == '4' && (phonenum[2] != '7' && phonenum[2] != '9'))return false;
		else if (phonenum[1] == '5' && phonenum[2] == '4')return false;
		else if (phonenum[1] == '7' && (phonenum[2] != '3' && phonenum[2] != '7'))return false;
		else if (phonenum[1] == '8' && (phonenum[2] != '0' && phonenum[2] != '1' && phonenum[2] != '5' && phonenum[2] != '6' && phonenum[2] != 8 && phonenum[2] != '9'))return false;
		else return true;
	}
}
void password(char pwd[],int* f)
{
	int num = 0, bw = 0, sw = 0;
	if (strlen(pwd) < 8) { cout << "密码长度必须大于8位！" << endl;return; }
	else {
		for (int i = 0;pwd[i] != '\0';i++)
		{
			if (pwd[i] >= '0' && pwd[i] <= '9')num++;
			else if (pwd[i] >= 'A' && pwd[i] <= 'Z')bw++;
			else if (pwd[i] >= 'a' && pwd[i] <= 'z')sw++;
			else {
				cout << "密码含有非法字符！" << endl;return;
			}
		}
			//cout << num << ' ' << bw << ' ' << sw << endl;
			if (num == 0) { cout << "密码中必须包含数字！" << endl;return ; }
			else if (bw == 0) { cout << "密码中必须包含大写字母！" << endl;return; }
			else if (sw == 0) { cout << "密码中必须包含小写字母！" << endl;return; }
			else {
				*f = 1;return;
			}
		
	}
}
/*Node* rc()
{
	Node* head = NULL;
	string nowphone;
	string nowname;
	string nowtime;
	string nowcontent;
	ifstream in_file("contents", ios::in);
	if (!in_file)exit(-1);
	in_file >> nowphone;
	while (!in_file.fail())
	{
		Node* p = new Node;
		p->phone = nowphone;
		in_file >> nowname;
		p->name = nowname;
		in_file >> nowtime;
		p->time = nowtime;
		in_file >> nowcontent;
		p->content = nowcontent;
		p->next = head;
		head = p;
		in_file >> nowphone;
	}
	in_file.close();
	return head;
}*/
/*void output(Node* head)
{
	for (Node* p = head;p != NULL;p = p->next)
	{
		if (p->phone == now_phone);
		cout << p->phone << endl << p->name << endl << p->time << endl << p->content << endl;
	}
	return;
}*/
