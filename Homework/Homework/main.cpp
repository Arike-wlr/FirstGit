#define _CRT_SECURE_NO_WARNINGS
#include<iostream>
#include<cstring>
#include"function.h"
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
struct node
{
	int no;
	string phone;
	string name;
	string content;
	string comment;
};
struct follow
{
	string userphone;
	string fophone;
	string foname;
	follow* next;
};
struct comments {
	int no;
	string phone;
	string content;
	string name;
	string comment;
	comments* next;
};
void remove(Node* head)
{
	while (head != NULL)
	{
		Node* p = head;
		head = head->next;
		delete p;
	}
	return;
}
void remove(comments* head1)
{
	while (head1 != NULL)
	{
		comments* p = head1;
		head1 = head1->next;
		delete p;
	}
	return;
}
int main()
{
	char exitc[2] = { '#' };
	string exits = "#";
	cout << "��ӭ������������С���飬���¼/ע�ᣡ" << endl;
	cout << "�����˳����򣬿���ʱ����#�����˳�" << endl;
	struct User
	{
		char phonenum[12];
		char pwd[100];
		string name;
	}use;
	User now;
	do {

		cout << "�������ֻ��ţ�";
		 cin >> use.phonenum;use.phonenum[11] = '\0';
		if (strcmp(use.phonenum, exitc) == 0) return 0;
		//for (int i = 0;i < 11;i++) { cout<< phonenum[i]; }
		if (!phone(use.phonenum)) cout << "��Ǹ��������ֻ�������" << endl;
		else break;
	}
	while (!phone(use.phonenum));
	//�����������ֻ���
	// ���¼���ֻ����Ƿ��Ѿ�ע��
	User u1;
	ifstream in_file("users.txt", ios::in);
	if (!in_file)exit(-1);
	in_file >> u1.phonenum;
	bool mark=false;//�����mark�����ж��Ƿ���ע���
	//if (in_file.good())cout << "!";
	//if (in_file.fail())cout << "?";
	while (!in_file.fail())
	{
		//cout << strcmp(use.phonenum, u1.phonenum);
		if (strcmp(use.phonenum, u1.phonenum) == 0) {
			cout << "���ֻ����Ѿ�ע�ᣬ���¼��" << endl;
			int flag = 0;//�����flag�����ж����������ٴ�
			in_file >> u1.pwd;
			while (flag <= 3) {
				cout << "���������룺";
				char inpwd[100];
				cin >> inpwd;
				if (strcmp(inpwd, exitc) == 0) return 0;
				if (strcmp(inpwd, u1.pwd) == 0) {
					cout << "\033c";
					in_file.ignore();
					getline(in_file,u1.name);
					cout << "��¼�ɹ�����ӭ����" <<u1.name<<'!'<<endl;
					for (int i = 0;i < 11;i++)
						now.phonenum[i] = u1.phonenum[i];
					now.phonenum[11] = '\0';
					now.name = u1.name;
					mark = true;
					break;//����е�ë��,���������ǳ��д��ո�զ�죿��11.30
				}//��ignore��getline���ɹ���112.1
				else {
					cout << "���벻��ȷ��";
					flag++;
					cout << "���������" << flag << "�Σ����󳬹�3�ν��Զ��˳���¼��";
				}
			}if (mark)break;
			if (flag == 4)return 0;
		}
		in_file >> u1.pwd;
		in_file.ignore();
		getline(in_file, u1.name);
		in_file >> u1.phonenum;
	}
	in_file.close();
	if (!mark) {
		cout << "���ֻ�����δע�ᣬ";
		//������������
		int flag;//�����flag�����ж��Ƿ����
		do {
			cout << "����������,����Ӧ�������ֺʹ�Сд��ĸ��";
			cin >> use.pwd;
			if (strcmp(use.pwd, exitc) == 0) return 0;			//cout << use.pwd;
			flag = 0;
			password(use.pwd, &flag);
		} while (flag == 0);
		do {
			cout << "���ٴ��������룺";
			char pwdagain[100];
			cin >> pwdagain;
			cin.get();
			if (strcmp(pwdagain, exitc) == 0) return 0;
			if (strcmp(pwdagain, use.pwd) != 0)cout << "�����������벻һ�£�" << endl;
			else break;
		} while (true);
		cout << "�������óɹ���" << endl;
		//���������ǳ�
		cout << "�������ǳƣ�";
		getline(cin,use.name);
		if (use.name == exits) return 0;
		//��������֤��
			char verify[10];
			char input[10];
			do {
				srand(time(NULL));
				for (int i = 0;i < 4;i++)
				{
					verify[i] = randomlet();
				}
				verify[4] = '\0';
				cout << "��������֤��" << verify << ":";
				cin >> input;
				if (strcmp(verify, exitc) == 0) return 0;
				if (strcmp(verify, input) != 0) cout << "��֤�벻��ȷ�����������룡" << endl;
			} while (strcmp(verify, input) != 0);
			for (int i = 0;i < 11;i++)
				now.phonenum[i] = use.phonenum[i];
			now.phonenum[11] = '\0';
			now.name = use.name;
		cout << "\033c";//��������
		cout << "ע��ɹ�����ӭ����" <<use.name<<'!'<< endl;
		//�����ǰ����û�����Ϣд��ԭ�ļ�
		ofstream out_file("users.txt", ios::out | ios::app);
		if (!out_file)exit(-1);
			out_file <<endl<< use.phonenum << ' ' << use.pwd << ' ' << use.name <<endl;
		out_file.close();
	}//�����ǵڶ�����
	char choice;
	do {
		cout << "�������������롰1����" << endl;
		cout << "ɾ�����������롰2����" << endl;
		cout << "�ظ����������롰3����" << endl;
		cout << "�鿴�����û��ķ������������롰4����" << endl;
		cout << "�鿴��ע�û��ķ������������롰5����" << endl;
		cout << "�˳��밴��#����";
		cin >> choice;
		cout << "\033c";
		if (choice == '1')
		{
			cout << "������Ҫ���������ݣ�" << endl;
			string content;
			cin.get();
			getline(cin, content);
			do {
				cout << "ȷ�Ϸ����𣿷����밴��1����ȡ���밴��2��" << endl;
				cin >> choice;
				if (choice == '1') {
					ofstream out_file("contents.txt", ios::out | ios::app);
					if (!out_file)exit(-1);
					time_t curtime;
					time(&curtime);
					string time;
					time = ctime(&curtime);
					out_file << now.phonenum << endl << now.name << endl << time << content << endl;
					out_file.close();
					cout << "\033c";
					cout << "�����ɹ���" << endl;
				}
				else if (choice == '2') {
					cout << "\033c" << "��ȡ������" << endl; break;
				}
				else if (choice == '#')return 0;
				else cout << "��������ȷ����ţ���" << endl;
			} while (choice != '1' && choice != '2');
		}
		else if (choice == '2')
		{int ch;
				L:Node* head = NULL;string nowphone;
				string nowname;
				string nowtime;
				string nowcontent;
				ifstream in_file("contents.txt", ios::in);
				if (!in_file)exit(-1);
				in_file >> nowphone;
				while (!in_file.fail())
				{
					Node* p = new Node;
					p->phone = nowphone;
					in_file.get();
					getline(in_file, nowname);
					p->name = nowname;
					getline(in_file, nowtime);
					p->time = nowtime;
					getline(in_file, nowcontent);
					p->content = nowcontent;
					p->next = head;
					head = p;
					in_file >> nowphone;
				}
				in_file.close();
				Node* p;
				/*for (p = head;p != NULL;p = p->next)
					cout << p->phone << endl;*/
				cout << "�������������������ݣ�" << endl;
				int many = 0;//���ڼ����û��Ƿ񷢲�������
				for (p = head;p != NULL;p = p->next)
				{
					//cout << p->phone << endl << p->name << endl << p->time << endl << p->content << endl;
					if (p->phone == now.phonenum) {
						cout << p->name << endl << p->time << endl << p->content << endl << endl;
						many++;
					}
				}
				if (many != 0)
				{
					Node* pre = NULL;Node* curr = NULL;bool nul = false;
					
					do {
						do
						{cout << "��ѡ����Ҫɾ�������ݣ�������������µ����,���롮0������,���롮-1���˳�����:";
							many = 0;
							cin >> ch;
							if (ch < -1)cout << "��������ȷ����ţ�";
							else if (ch == -1)return 0;
						} while (ch < -1);
						if (ch == 0) break;
						pre = head;
						if (head->phone == now.phonenum)
						{
							curr = head;
							for (int i = 1;i < ch && pre->next != NULL;)
							{
								if (pre->next->phone == now.phonenum) { i++;curr = pre->next; }
								if (i < ch)pre = pre->next;
							}
						}
						else {
							for (int i = 1;i <= ch && pre->next != NULL;)
							{
								if (pre->next->phone == now.phonenum) { i++;curr = pre->next; }
								if (i <= ch)pre = pre->next;
							}
						}
						if (pre->next == NULL&&ch!=1) {
							nul = true;cout << "�޵�" << ch << "���������ݣ����������룡" << endl;
						}
						else {
							cout << "�Ƿ�ȷ��ɾ����ȷ�ϰ���1����ȡ������2����";
							cin >> choice;
							if (choice == '1') {
								curr->phone = "##";
								ofstream out_file("contents.txt", ios::out);
								while (head->next != NULL) {
									for (pre = head;pre->next->next != NULL;pre = pre->next);
									Node* cur = pre->next;
									pre->next = NULL;
									out_file << cur->phone << endl << cur->name << endl << cur->time << endl << cur->content << endl;
									delete cur;
								}
								out_file << head->phone << endl << head->name << endl << head->time << endl << head->content << endl;
								out_file.close();
								cout << "\033c";
								cout << "ɾ���ɹ���" << endl;
								remove(head);goto L;
							}
							else if (choice == '2') {
								remove(head);cout << "\033c";
								cout << "��ȡ������" << endl;
								goto L;
							}
							else if (choice == '#')return 0;
						}
					} while (nul);
					if (ch == 0) {
						cout << "\033c";continue;
					}
				}
				else {
					cout << "����û�з���������Ŷ~" << endl;continue;
				}
			remove(head);
		}
		else if (choice == '3') {
			L2:comments *head1 = NULL, *tail = NULL;
			int no;
			string phone;
			string content;
			string name;
			string comment;
			ifstream in_file2("comments.txt", ios::in);
			if (!in_file2)exit(-1);
			in_file2 >> no;
			while (!in_file2.fail())
			{
				comments* q = new comments;
				q->next = NULL;
				q->no = no;
				in_file2 >> phone;
				q->phone = phone;
				in_file2.get();
				getline(in_file2, content);
				q->content = content;
				getline(in_file2, name);
				q->name = name;
				getline(in_file2, comment);
				q->comment = comment;
				if (head1 == NULL) {
					head1 = q;tail = q;
				}
				else {
					tail->next = q;
					tail = q;
				}
				in_file2 >> no;
			}
			in_file2.close();
		L7:cout << "���������������������д������۵ģ�" << endl;
			Node* head = NULL;
			string nowphone;
			string nowname;
			string nowtime;
			string nowcontent;
			ifstream in_file("contents.txt", ios::in);
			if (!in_file)exit(-1);
			in_file >> nowphone;
			while (!in_file.fail())
			{
				Node* p = new Node;
				p->phone = nowphone;
				in_file.get();
				getline(in_file, nowname);
				p->name = nowname;
				getline(in_file, nowtime);
				p->time = nowtime;
				getline(in_file, nowcontent);
				p->content = nowcontent;
				p->next = head;
				head = p;
				in_file >> nowphone;
			}
			in_file.close();
			comments* p = NULL;comments* pre = NULL;comments* curr = NULL;bool nul = false;bool bo = false;
			Node* q = NULL;
			int many = 0,j=1;//���ڼ���
			for (p = head1;p != NULL;p = p->next)
			{
				bo = false;
				for (q = head;q != NULL;q = q->next) {
					if (p->content == q->content && q->phone == "##") { bo = true; break; }
				}if (bo)continue;
				if (p->phone == now.phonenum&&p->no==1) {cout << j<<' '<<now.name << endl << p->content << endl << endl;
				many++;j++;
				}
			}
			int ch;
			if (many != 0) {
				do {
					do {
						cout << "��ѡ������Ҫ�鿴���۵ķ������ݣ�����������µ���ţ����롮0�����أ����롮-1���˳����򣩣�";
						many = 0;
						cin >> ch;
						if (ch < -1)cout << "��������ȷ����ţ�";
						else if (ch == 0)break;
						else if (ch == -1)return 0;
					} while (ch < -1);
					if (ch == 0) {
						cout << "\033c"<<"�ѷ���"<<endl;break;
					}
					pre = head1;
					if (head1->phone == now.phonenum && ch == 1)
					{
						curr = head1;
						for (q = head;q != NULL;q = q->next) {
							if (curr->content == q->content && q->phone == "##")
								curr = curr->next;
						}
					}
					else if(head1->phone == now.phonenum){
						for (q = head;q != NULL;q = q->next)
							if (pre->content == q->content && q->phone == "##") goto D;
						for (int i = 1;i <ch && pre->next != NULL;)
						{
							bo = false;
							for (q = head;q != NULL;q = q->next) {
								if (pre->next->content == q->content && q->phone == "##") { bo = true; break; }
							}if (bo) {
								pre = pre->next;continue;
							}
							if (pre->next->phone == now.phonenum && pre->next->no == 1) { i++;curr = pre->next; }
							if (i < ch)pre = pre->next;
						}
					}
					else {
						D:for (int i = 1;i <= ch && pre->next != NULL;)
						{
							bo = false;
							for (q = head;q != NULL;q = q->next) {
								if (pre->next->content == q->content && q->phone == "##") { bo = true; break; }
							}if (bo) {
								pre = pre->next;continue;
							}
							if (pre->next->phone == now.phonenum && pre->next->no == 1) { i++;curr = pre->next; }
							if (i <= ch)pre = pre->next;
						}
					}//�ҵ���ch�������۵�����,����pre��curr��currָ����Ҫ�ظ����۵�����*/
					
					if (pre->next == NULL&&ch!=1) {
						nul = true;
						cout << "�޵�" << ch << "���������ݣ����������룡" << endl;
					}
					else {
						cout << "\033c";
						cout << curr->phone <<endl << now.name << endl << curr->content << endl<<endl;
						for (p = head1;p != NULL;p = p->next)
						{
							if (p->phone == now.phonenum&&p->content==curr->content) {
								cout << p->no << ' ' << p->name << endl << p->comment << endl << endl;
							}
						}
						bool null = false;
						do
						{
							do {
								cout << "��������Ҫ�ظ����������(���ذ�'0',�˳���'-1')��";
								cin >> ch;
								if (ch < -1)cout << "��������ȷ�ı�ţ�" << endl;
								else if (ch == -1)return 0;
							} while (ch < -1);
							comments* q = NULL;p = head1;
							if (ch == 0) {
							cout << "\033c" << "�ѷ���" << endl;goto L7;
								}
							else if (ch == 1&&head1->phone==now.phonenum)q = head1;
							else {
								for (;p->next != NULL;p = p->next) {
									if (p->next->phone == now.phonenum && p->next->content == curr->content && p->next->no == ch) {
										curr = p->next;break;
									}
								}
								q = p->next;
							}//q��Ҫ�ظ�����������
							if (p->next == NULL&&ch!=1) { null = true;cout << "�޵�" << ch << "�����ۣ����������룡" << endl; }
							else {  
								cout << "������ظ������ݣ�";
								string reply;
								cin.get();
								getline(cin, reply);
								string r = "   //�����ظ���";
								do {
									cout << "ȷ�ϻظ��𣿣�ȷ�ϰ���1����ȡ������2������";
									cin >> choice;
									if (choice == '1') {
									q->comment += (r+reply);
										ofstream out_file("comments.txt", ios::out);
										for (p = head1;p != NULL;p = p->next)
											out_file << p->no << ' ' << p->phone << endl << p->content << endl << p->name << endl << p->comment << endl;
										out_file.close();
										cout << "\033c";
										cout << "�ظ��ɹ���" << endl;
										remove(head1);goto L2;
									}
									else if (choice == '2') { cout << "\033c";cout << "��ȡ������" << endl;remove(head1);goto L2; }
									else if (choice == '#')return 0;
									else if (choice >= '3' || choice <= '0')cout << "��������ȷ��ѡ�";
								} while (choice >= '3' || choice <= '0');
							}
						} while (null);
					}
				} while (nul);
			}
			else { cout << "�����������ݻ�û������Ŷ~" << endl;continue; }
		}
		else if (choice == '4') {
			Node* head = NULL;
			string nowphone;
			string nowname;
			string nowtime;
			string nowcontent;
			ifstream in_file("contents.txt", ios::in);
			if (!in_file)exit(-1);
			in_file >> nowphone;
			while (!in_file.fail())
			{
				Node* p = new Node;
				p->phone = nowphone;
				in_file.get();
				getline(in_file, nowname);
				p->name = nowname;
				getline(in_file, nowtime);
				p->time = nowtime;
				getline(in_file, nowcontent);
				p->content = nowcontent;
				p->next = head;
				head = p;
				in_file >> nowphone;
			}
			in_file.close();
			L3:cout << "�����������û����������ݣ�" << endl;Node* p;
			int j = 1;
			for (p = head;p != NULL;p = p->next)
			{
				if (p->phone == "##")continue;
				cout <<j<<' '<< p->name << endl << p->time << endl << p->content << endl << endl;
				j++;
			}int ch;bool nul = false;
			do{
			do {
				cout << "��������Ҫ�鿴���ݵı�ţ�������������µ����,���롮0������,���롮-1���˳����򣩣�";
				cin >> ch;
				if (ch < -1)cout << "��������ȷ����ţ�";
				else if (ch == 0) {
					cout << "\033c";break;
				}
				else if (ch == -1)return 0;
			} while (ch < 1);
			if (ch == 0) break;
			Node* pre = head;Node* curr = NULL;
			if (ch == 1) { curr = head;for (;curr->phone == "##";curr = curr->next) ; }
			else {
				if (head->phone !="##") {
					for (int i = 1;i < ch && pre->next != NULL;)
					{
						if (pre->next->phone != "##") { i++;curr = pre->next; }
						if (i < ch)pre = pre->next;
					}
				}
				else {
					for (int i = 1;i <= ch && pre->next != NULL;)
					{
					if (pre->next->phone != "##") { i++;curr = pre->next; }
					if (i <= ch)pre = pre->next;
				}
				}
			}
			if (pre->next == NULL&&ch!=1) {
				nul = true;cout << "�޵�" << ch << "���������ݣ����������룡" << endl;
			}
			else {
				cout << "\033c";
				cout << curr->phone << endl << curr->name << endl << curr->content << endl;
				node u1;//��Ȼ��������û�а����������ù����� /�ã�ɾ��next
				int preno=0;
				ifstream in_file("comments.txt", ios::in);
				in_file >> u1.no;
				while (!in_file.fail())
				{
					in_file >> u1.phone;
					in_file.get();
					getline(in_file, u1.content);
					getline(in_file, u1.name);
					getline(in_file, u1.comment);
					if (u1.content == curr->content) {
						cout <<u1.no<< ' '<<u1.name <<endl<< u1.comment << endl<<endl;
						preno = u1.no;
					}
					in_file >> u1.no;
				}
				in_file.close();
				cout << "���Ƿ�Ҫ���ۣ����밴��1�������밴��2����";
				cin >> choice;
				if (choice == '1') {
					cout << "�������������ݣ�";
					string comment;
					cin.get();
					getline(cin, comment);
					int no = preno + 1;
					ofstream out_file("comments.txt", ios::out | ios::app);
					if (!out_file)exit(-1);
					out_file << no << ' ' << curr->phone << endl << curr->content << endl << now.name << endl << comment << endl << endl;
					out_file.close();
					cout << "\033c";
					cout << "���۳ɹ���" << endl;
					int m = 0;
					follow f1;
					ifstream in_file1("follows.txt", ios::in);
					in_file1 >> f1.userphone;
					while (!in_file1.fail())
					{
						in_file1 >> f1.fophone;
						in_file1.get();
						getline(in_file1, f1.foname);
						if (f1.userphone == now.phonenum && f1.fophone == curr->phone) {
							m = 1;break;
						}
						in_file1 >> f1.userphone;
					}in_file1.close();
					if (m == 0) {
						cout << "�Ƿ��ע���û���(��ע�밴��1��������ע�밴��2������";
						cin >> choice;
						if (choice == '1') {
							ofstream out_file("follows.txt", ios::out | ios::app);
							out_file << now.phonenum << endl << curr->phone << endl << curr->name << endl;
							out_file.close();
							cout << "\033c";
							cout << "��ע�ɹ���" << endl;
							goto L3;
						}
						else if (choice == '2') {
							cout << "\033c" << "��ѡ�񲻹�ע���ѷ���" << endl;goto L3;
						}
						else if (choice == '#')return 0;
					}
					else { cout << "���ѹ�ע���û����ѷ���" << endl;goto L3; }
				}
				else { cout << "\033c" << "�ѷ���"<<endl;goto L3; }
			}
			} while (nul);
		remove(head);}
		else if (choice == '5') {
			
			follow* head = NULL;
			mark = 0;
			follow f1;
			ifstream in_file("follows.txt", ios::in);
			in_file >> f1.userphone;
			while (!in_file.fail())
			{follow* p = new follow;
				p->userphone = f1.userphone;
				in_file >> f1.fophone;
				p->fophone = f1.fophone;
				in_file.get();
				getline(in_file, f1.foname);
				p->foname = f1.foname;
				if (f1.userphone == now.phonenum )mark++;
				p->next = head;
				head = p;
				in_file >> f1.userphone;
			}in_file.close();
			if (mark == 0)cout << "����û�й�ע���û�Ŷ����ȥ��ע��~"<<endl;
			else {L5:cout << "����������ע���û���"<<endl;
				follow* p = head;int j = 1;
				for (;p != NULL;p = p->next)
				{
					if (p->userphone == now.phonenum) { cout << j << ' ' << p->fophone << ' ' << p->foname << endl << endl;j++; }
				}int ch;bool nu = false;
				do {
					do {
						cout << "��ѡ������Ҫ�鿴���û���ţ�������������µ����,���롮0������,���롮-1���˳����򣩣�";
						cin >> ch;
						if (ch < -1)cout << "��������ȷ����ţ�";
					} while (ch < -1);
					if (ch == 0) {
						cout << "\033c"<<"�ѷ���"<<endl;break;
					}
					else if (ch == -1)return 0;
					else {
						follow* pre = head;follow* curr = NULL;
						if (ch == 1) { curr = head;for (;curr->userphone != now.phonenum;curr = curr->next); }
						else {
							if (head->userphone == now.phonenum) {
								for (int i = 1;i < ch && pre->next != NULL;) {
									if (pre->next->userphone == now.phonenum) { i++;curr = pre->next; }
									if (i < ch)pre = pre->next;
								}
							}
							else {
								for (int i = 1;i <= ch && pre->next != NULL;) {
									if (pre->next->userphone == now.phonenum) { i++;curr = pre->next; }
									if (i <= ch)pre = pre->next;
								}
							}
						}
						if (pre->next == NULL&&ch!=1) { cout << "�޵�" << ch << "����ע�û������������룡" << endl;nu = true; }
						else {
							cout << "\033c";
							cout << "�����Ǹ��û����еķ������ݣ�" << endl;
							Node fo;Node* h=NULL;
							ifstream in_file1("contents.txt", ios::in);
							in_file1 >> fo.phone;
							while (!in_file1.fail())
							{
								Node* p = new Node;
								p->phone = fo.phone;
								in_file1.get();
								getline(in_file1, fo.name);
								p->name = fo.name;
								getline(in_file1, fo.time);
								p->time = fo.time;
								getline(in_file1, fo.content);
								p->content = fo.content;
								p->next = h;
								h = p;
								in_file1 >> fo.phone;
							}in_file1.close();
							L4:int j = 1;for (Node* q = h;q != NULL;q = q->next) {
								if (q->phone == curr->fophone)
								{
									cout << j << ' ' << q->time << endl << q->content << endl << endl;j++;
								}
							}bool n = false;
							do {
								do {
									cout << "��ѡ����Ҫ���۵�������ţ����������,���롮0������,���롮-1���˳�����";
									cin >> ch;
									if (ch < -1)cout << "��������ȷ����ţ�";
								} while (ch < -1);
								if (ch == 0) {
									cout << "\033c";cout << "�ѷ���"<<endl;goto L5;
								}
								else if (ch == -1) return 0;
								else {
									Node* pre1 = h;Node* current = NULL;
									if (ch == 1) { current = h;for (;current->phone !=curr->fophone;current = current->next); }
									else {
										if (h->phone ==curr->fophone) {
											for (int i = 1;i < ch && pre1->next != NULL;)
											{
												if (pre1->next->phone ==curr->fophone ) { i++;current = pre1->next; }
												if (i < ch)pre1 = pre1->next;
											}
										}
										else {
											for (int i = 1;i <= ch && pre1->next != NULL;)
											{
												if (pre1->next->phone ==curr->fophone) { i++;current = pre1->next; }
												if (i <= ch)pre1 = pre1->next;
											}
										}
									}if (pre1->next == NULL&&ch!=1) {
										n = true;cout << "�޵�" << ch << "���������ݣ����������룡" << endl;
									}
									else {
										cout << "\033c";
										cout << current->phone << endl << current->name << endl << current->content << endl;
										node u1;
										int preno = 0;
										ifstream in_file("comments.txt", ios::in);
										in_file >> u1.no;
										while (!in_file.fail())
										{
											in_file >> u1.phone;
											in_file.get();
											getline(in_file, u1.content);
											getline(in_file, u1.name);
											getline(in_file, u1.comment);
											if (u1.content == current->content) {
												cout << u1.no << ' ' << u1.name << endl << u1.comment << endl << endl;
												preno = u1.no;
											}
											in_file >> u1.no;
										}
										in_file.close();
										cout << "�������������ݣ�";
										string comment;
										cin.get();
										getline(cin, comment);
										int no = preno + 1;
										ofstream out_file("comments.txt", ios::out | ios::app);
										if (!out_file)exit(-1);
										out_file << no << ' ' << current->phone << endl << current->content << endl << now.name << endl << comment << endl << endl;
										out_file.close();
										cout << "\033c";
										cout << "���۳ɹ���" << endl;
										goto L4;
									}
								}
							} while (n);
						}
					}
				} while (nu);
			}
		}
		else if (choice == '#')return 0;
		else  cout << "��������ȷ����ţ���" << endl; 
	}
	while (choice!= '#');
	return 0;
}