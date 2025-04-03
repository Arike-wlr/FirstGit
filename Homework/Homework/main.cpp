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
	cout << "欢迎您来到单机版小红书，请登录/注册！" << endl;
	cout << "若想退出程序，可随时按‘#’键退出" << endl;
	struct User
	{
		char phonenum[12];
		char pwd[100];
		string name;
	}use;
	User now;
	do {

		cout << "请输入手机号：";
		 cin >> use.phonenum;use.phonenum[11] = '\0';
		if (strcmp(use.phonenum, exitc) == 0) return 0;
		//for (int i = 0;i < 11;i++) { cout<< phonenum[i]; }
		if (!phone(use.phonenum)) cout << "抱歉您输入的手机号有误！" << endl;
		else break;
	}
	while (!phone(use.phonenum));
	//以上是输入手机号
	// 以下检查手机号是否已经注册
	User u1;
	ifstream in_file("users.txt", ios::in);
	if (!in_file)exit(-1);
	in_file >> u1.phonenum;
	bool mark=false;//这里的mark用于判断是否已注册过
	//if (in_file.good())cout << "!";
	//if (in_file.fail())cout << "?";
	while (!in_file.fail())
	{
		//cout << strcmp(use.phonenum, u1.phonenum);
		if (strcmp(use.phonenum, u1.phonenum) == 0) {
			cout << "该手机号已经注册，请登录！" << endl;
			int flag = 0;//这里的flag用于判断密码输错多少次
			in_file >> u1.pwd;
			while (flag <= 3) {
				cout << "请输入密码：";
				char inpwd[100];
				cin >> inpwd;
				if (strcmp(inpwd, exitc) == 0) return 0;
				if (strcmp(inpwd, u1.pwd) == 0) {
					cout << "\033c";
					in_file.ignore();
					getline(in_file,u1.name);
					cout << "登录成功！欢迎您，" <<u1.name<<'!'<<endl;
					for (int i = 0;i < 11;i++)
						now.phonenum[i] = u1.phonenum[i];
					now.phonenum[11] = '\0';
					now.name = u1.name;
					mark = true;
					break;//这段有点毛病,如果输入的昵称中带空格咋办？？11.30
				}//用ignore和getline！成功啦112.1
				else {
					cout << "密码不正确！";
					flag++;
					cout << "已输错密码" << flag << "次，错误超过3次将自动退出登录！";
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
		cout << "该手机号尚未注册，";
		//以下设置密码
		int flag;//这里的flag用于判断是否输对
		do {
			cout << "请设置密码,密码应包含数字和大小写字母：";
			cin >> use.pwd;
			if (strcmp(use.pwd, exitc) == 0) return 0;			//cout << use.pwd;
			flag = 0;
			password(use.pwd, &flag);
		} while (flag == 0);
		do {
			cout << "请再次输入密码：";
			char pwdagain[100];
			cin >> pwdagain;
			cin.get();
			if (strcmp(pwdagain, exitc) == 0) return 0;
			if (strcmp(pwdagain, use.pwd) != 0)cout << "两次输入密码不一致！" << endl;
			else break;
		} while (true);
		cout << "密码设置成功！" << endl;
		//以下设置昵称
		cout << "请设置昵称：";
		getline(cin,use.name);
		if (use.name == exits) return 0;
		//以下是验证码
			char verify[10];
			char input[10];
			do {
				srand(time(NULL));
				for (int i = 0;i < 4;i++)
				{
					verify[i] = randomlet();
				}
				verify[4] = '\0';
				cout << "请输入验证码" << verify << ":";
				cin >> input;
				if (strcmp(verify, exitc) == 0) return 0;
				if (strcmp(verify, input) != 0) cout << "验证码不正确，请重新输入！" << endl;
			} while (strcmp(verify, input) != 0);
			for (int i = 0;i < 11;i++)
				now.phonenum[i] = use.phonenum[i];
			now.phonenum[11] = '\0';
			now.name = use.name;
		cout << "\033c";//清屏操作
		cout << "注册成功！欢迎您，" <<use.name<<'!'<< endl;
		//以下是把新用户的信息写入原文件
		ofstream out_file("users.txt", ios::out | ios::app);
		if (!out_file)exit(-1);
			out_file <<endl<< use.phonenum << ' ' << use.pwd << ' ' << use.name <<endl;
		out_file.close();
	}//以下是第二部分
	char choice;
	do {
		cout << "发布内容请输入“1”，" << endl;
		cout << "删除内容请输入“2”，" << endl;
		cout << "回复评论请输入“3”，" << endl;
		cout << "查看其他用户的发布内容请输入“4”，" << endl;
		cout << "查看关注用户的发布内容请输入“5”，" << endl;
		cout << "退出请按“#”：";
		cin >> choice;
		cout << "\033c";
		if (choice == '1')
		{
			cout << "请输入要发布的内容：" << endl;
			string content;
			cin.get();
			getline(cin, content);
			do {
				cout << "确认发布吗？发布请按‘1’，取消请按‘2’" << endl;
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
					cout << "发布成功！" << endl;
				}
				else if (choice == '2') {
					cout << "\033c" << "已取消发布" << endl; break;
				}
				else if (choice == '#')return 0;
				else cout << "请输入正确的序号！！" << endl;
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
				cout << "以下是您发布过的内容：" << endl;
				int many = 0;//用于计数用户是否发布过内容
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
						{cout << "请选择需要删除的内容（输入其从上往下的序号,输入‘0’返回,输入‘-1’退出程序）:";
							many = 0;
							cin >> ch;
							if (ch < -1)cout << "请输入正确的序号！";
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
							nul = true;cout << "无第" << ch << "个发布内容，请重新输入！" << endl;
						}
						else {
							cout << "是否确认删除？确认按‘1’，取消按‘2’：";
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
								cout << "删除成功！" << endl;
								remove(head);goto L;
							}
							else if (choice == '2') {
								remove(head);cout << "\033c";
								cout << "已取消操作" << endl;
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
					cout << "您还没有发布过内容哦~" << endl;continue;
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
		L7:cout << "以下是您发布过的内容中带有评论的：" << endl;
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
			int many = 0,j=1;//用于计数
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
						cout << "请选择您想要查看评论的发布内容（输入从上往下的序号，输入‘0’返回，输入‘-1’退出程序）：";
						many = 0;
						cin >> ch;
						if (ch < -1)cout << "请输入正确的序号！";
						else if (ch == 0)break;
						else if (ch == -1)return 0;
					} while (ch < -1);
					if (ch == 0) {
						cout << "\033c"<<"已返回"<<endl;break;
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
					}//找到第ch个有评论的内容,已用pre和curr，curr指向需要回复评论的帖子*/
					
					if (pre->next == NULL&&ch!=1) {
						nul = true;
						cout << "无第" << ch << "个发布内容，请重新输入！" << endl;
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
								cout << "请输入需要回复的评论序号(返回按'0',退出按'-1')：";
								cin >> ch;
								if (ch < -1)cout << "请输入正确的编号！" << endl;
								else if (ch == -1)return 0;
							} while (ch < -1);
							comments* q = NULL;p = head1;
							if (ch == 0) {
							cout << "\033c" << "已返回" << endl;goto L7;
								}
							else if (ch == 1&&head1->phone==now.phonenum)q = head1;
							else {
								for (;p->next != NULL;p = p->next) {
									if (p->next->phone == now.phonenum && p->next->content == curr->content && p->next->no == ch) {
										curr = p->next;break;
									}
								}
								q = p->next;
							}//q是要回复的那条评论
							if (p->next == NULL&&ch!=1) { null = true;cout << "无第" << ch << "条评论，请重新输入！" << endl; }
							else {  
								cout << "请输入回复的内容：";
								string reply;
								cin.get();
								getline(cin, reply);
								string r = "   //博主回复：";
								do {
									cout << "确认回复吗？（确认按‘1’，取消按‘2’）：";
									cin >> choice;
									if (choice == '1') {
									q->comment += (r+reply);
										ofstream out_file("comments.txt", ios::out);
										for (p = head1;p != NULL;p = p->next)
											out_file << p->no << ' ' << p->phone << endl << p->content << endl << p->name << endl << p->comment << endl;
										out_file.close();
										cout << "\033c";
										cout << "回复成功！" << endl;
										remove(head1);goto L2;
									}
									else if (choice == '2') { cout << "\033c";cout << "已取消操作" << endl;remove(head1);goto L2; }
									else if (choice == '#')return 0;
									else if (choice >= '3' || choice <= '0')cout << "请输入正确的选项！";
								} while (choice >= '3' || choice <= '0');
							}
						} while (null);
					}
				} while (nul);
			}
			else { cout << "您发布的内容还没有评论哦~" << endl;continue; }
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
			L3:cout << "以下是所有用户发布的内容：" << endl;Node* p;
			int j = 1;
			for (p = head;p != NULL;p = p->next)
			{
				if (p->phone == "##")continue;
				cout <<j<<' '<< p->name << endl << p->time << endl << p->content << endl << endl;
				j++;
			}int ch;bool nul = false;
			do{
			do {
				cout << "请输入需要查看内容的编号（输入其从上往下的序号,输入‘0’返回,输入‘-1’退出程序）：";
				cin >> ch;
				if (ch < -1)cout << "请输入正确的序号！";
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
				nul = true;cout << "无第" << ch << "个发布内容，请重新输入！" << endl;
			}
			else {
				cout << "\033c";
				cout << curr->phone << endl << curr->name << endl << curr->content << endl;
				node u1;//虽然是链表但我没有把它当链表用哈哈哈 /好，删了next
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
				cout << "您是否要评论？是请按‘1’，否请按‘2’：";
				cin >> choice;
				if (choice == '1') {
					cout << "请输入评论内容：";
					string comment;
					cin.get();
					getline(cin, comment);
					int no = preno + 1;
					ofstream out_file("comments.txt", ios::out | ios::app);
					if (!out_file)exit(-1);
					out_file << no << ' ' << curr->phone << endl << curr->content << endl << now.name << endl << comment << endl << endl;
					out_file.close();
					cout << "\033c";
					cout << "评论成功！" << endl;
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
						cout << "是否关注该用户？(关注请按‘1’，不关注请按‘2’）：";
						cin >> choice;
						if (choice == '1') {
							ofstream out_file("follows.txt", ios::out | ios::app);
							out_file << now.phonenum << endl << curr->phone << endl << curr->name << endl;
							out_file.close();
							cout << "\033c";
							cout << "关注成功！" << endl;
							goto L3;
						}
						else if (choice == '2') {
							cout << "\033c" << "您选择不关注，已返回" << endl;goto L3;
						}
						else if (choice == '#')return 0;
					}
					else { cout << "您已关注该用户，已返回" << endl;goto L3; }
				}
				else { cout << "\033c" << "已返回"<<endl;goto L3; }
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
			if (mark == 0)cout << "您还没有关注的用户哦，快去关注吧~"<<endl;
			else {L5:cout << "以下是您关注的用户："<<endl;
				follow* p = head;int j = 1;
				for (;p != NULL;p = p->next)
				{
					if (p->userphone == now.phonenum) { cout << j << ' ' << p->fophone << ' ' << p->foname << endl << endl;j++; }
				}int ch;bool nu = false;
				do {
					do {
						cout << "请选择您想要查看的用户序号（输入其从上往下的序号,输入‘0’返回,输入‘-1’退出程序）：";
						cin >> ch;
						if (ch < -1)cout << "请输入正确的序号！";
					} while (ch < -1);
					if (ch == 0) {
						cout << "\033c"<<"已返回"<<endl;break;
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
						if (pre->next == NULL&&ch!=1) { cout << "无第" << ch << "个关注用户，请重新输入！" << endl;nu = true; }
						else {
							cout << "\033c";
							cout << "以下是该用户所有的发布内容：" << endl;
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
									cout << "请选择想要评论的内容序号（输入其序号,输入‘0’返回,输入‘-1’退出程序）";
									cin >> ch;
									if (ch < -1)cout << "请输入正确的序号！";
								} while (ch < -1);
								if (ch == 0) {
									cout << "\033c";cout << "已返回"<<endl;goto L5;
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
										n = true;cout << "无第" << ch << "个发布内容，请重新输入！" << endl;
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
										cout << "请输入评论内容：";
										string comment;
										cin.get();
										getline(cin, comment);
										int no = preno + 1;
										ofstream out_file("comments.txt", ios::out | ios::app);
										if (!out_file)exit(-1);
										out_file << no << ' ' << current->phone << endl << current->content << endl << now.name << endl << comment << endl << endl;
										out_file.close();
										cout << "\033c";
										cout << "评论成功！" << endl;
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
		else  cout << "请输入正确的序号！！" << endl; 
	}
	while (choice!= '#');
	return 0;
}