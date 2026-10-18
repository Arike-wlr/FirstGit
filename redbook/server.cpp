// ============================================================
//  单机版小红书 · C++ 后端
//  一个零依赖的 HTTP 服务，继续用 users/contents/comments/follows
//  四个 txt 当数据库，文件格式与 2024 年那份 main.cpp 完全一致。
//
//  编译： g++ -O2 -std=c++17 server.cpp -o server.exe -lws2_32
//  运行： server.exe [端口] [数据目录]
// ============================================================
#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

// ---------------- 全局 ----------------
static string DATA_DIR = "data";        // 四个 txt 所在目录
static string WEB_ROOT = ".";           // index.html 所在目录
static const int MAX_BODY = 1 << 20;

// ---------------- 编码：文件是 GBK，浏览器要 UTF-8 ----------------
static string cpToUtf8(const string& in, UINT cp) {
    if (in.empty()) return in;
    int n = MultiByteToWideChar(cp, 0, in.c_str(), (int)in.size(), nullptr, 0);
    wstring w(n, 0);
    MultiByteToWideChar(cp, 0, in.c_str(), (int)in.size(), &w[0], n);
    int m = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    string o(m, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &o[0], m, nullptr, nullptr);
    return o;
}
static string utf8ToCp(const string& in, UINT cp) {
    if (in.empty()) return in;
    int n = MultiByteToWideChar(CP_UTF8, 0, in.c_str(), (int)in.size(), nullptr, 0);
    wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, in.c_str(), (int)in.size(), &w[0], n);
    int m = WideCharToMultiByte(cp, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    string o(m, 0);
    WideCharToMultiByte(cp, 0, w.c_str(), (int)w.size(), &o[0], m, nullptr, nullptr);
    return o;
}
static string gbkToUtf8(const string& s) { return cpToUtf8(s, 936); }
static string utf8ToGbk(const string& s) { return utf8ToCp(s, 936); }

// ---------------- 小工具 ----------------
static string trim(const string& s) {
    size_t a = 0, b = s.size();
    while (a < b && isspace((unsigned char)s[a])) a++;
    while (b > a && isspace((unsigned char)s[b - 1])) b--;
    return s.substr(a, b - a);
}
static vector<string> splitLines(const string& s) {
    vector<string> out;
    size_t i = 0;
    while (i <= s.size()) {
        size_t j = s.find('\n', i);
        string line = (j == string::npos) ? s.substr(i) : s.substr(i, j - i);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        out.push_back(line);
        if (j == string::npos) break;
        i = j + 1;
    }
    return out;
}
static string readFileUtf8(const string& path) {
    ifstream f(path.c_str(), ios::binary);
    if (!f) return "";
    string raw((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    return gbkToUtf8(raw);
}
static bool writeFileUtf8(const string& path, const string& utf8) {
    ofstream f(path.c_str(), ios::binary | ios::trunc);
    if (!f) return false;
    string gbk = utf8ToGbk(utf8);
    f.write(gbk.data(), (streamsize)gbk.size());
    return f.good();
}
static string jesc(const string& s) {
    string o;
    o.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
            case '"':  o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n";  break;
            case '\r': o += "\\r";  break;
            case '\t': o += "\\t";  break;
            default:
                if (c < 0x20) { char b[8]; snprintf(b, sizeof b, "\\u%04x", c); o += b; }
                else o += (char)c;
        }
    }
    return o;
}
static string jget(const string& body, const string& key) {
    string pat = "\"" + key + "\"";
    size_t p = body.find(pat);
    if (p == string::npos) return "";
    p = body.find(':', p + pat.size());
    if (p == string::npos) return "";
    p++;
    while (p < body.size() && isspace((unsigned char)body[p])) p++;
    if (p >= body.size()) return "";
    if (body[p] == '"') {
        p++;
        string o;
        while (p < body.size() && body[p] != '"') {
            if (body[p] == '\\' && p + 1 < body.size()) {
                char n = body[p + 1];
                if (n == 'n') o += '\n';
                else if (n == 't') o += '\t';
                else if (n == 'r') o += '\r';
                else o += n;
                p += 2;
            } else {
                o += body[p++];
            }
        }
        return o;
    }
    size_t e = p;
    while (e < body.size() && body[e] != ',' && body[e] != '}' && body[e] != ']') e++;
    return trim(body.substr(p, e - p));
}

// ---------------- 时间：ctime 字符串 <-> ISO ----------------
static string isoFromCtime(const string& raw) {
    char wd[16] = {0}, mon[8] = {0};
    int d = 0, h = 0, mi = 0, s = 0, y = 0;
    if (sscanf(raw.c_str(), "%15s %7s %d %d:%d:%d %d", wd, mon, &d, &h, &mi, &s, &y) < 7) return "";
    static const char* M[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    int m = 1;
    for (int i = 0; i < 12; i++) if (_strnicmp(mon, M[i], 3) == 0) m = i + 1;
    char buf[40];
    snprintf(buf, sizeof buf, "%04d-%02d-%02dT%02d:%02d:%02d", y, m, d, h, mi, s);
    return buf;
}
static time_t isoToTime(const string& iso) {
    if (iso.size() < 19) return 0;
    struct tm t;
    memset(&t, 0, sizeof t);
    t.tm_year = atoi(iso.substr(0, 4).c_str()) - 1900;
    t.tm_mon  = atoi(iso.substr(5, 2).c_str()) - 1;
    t.tm_mday = atoi(iso.substr(8, 2).c_str());
    t.tm_hour = atoi(iso.substr(11, 2).c_str());
    t.tm_min  = atoi(iso.substr(14, 2).c_str());
    t.tm_sec  = atoi(iso.substr(17, 2).c_str());
    t.tm_isdst = -1;
    return mktime(&t);
}
static string nowCtime() {
    time_t t = time(nullptr);
    string s = ctime(&t);           // 自带结尾换行，与原程序写入格式一致
    return s;
}
static string isoAdd(const string& iso, int minutes) {
    time_t t = isoToTime(iso);
    if (!t) return iso;
    t += (time_t)minutes * 60;
    struct tm tmv;
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    tmv = *localtime(&t);
#endif
    char buf[40];
    strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%S", &tmv);
    return buf;
}

// ---------------- 数据模型 ----------------
struct User    { string phone, pwd, name; };
struct Post    { string phone, name, timeRaw, content; };
struct Comment { int no = 0; string phone, postContent, name, text; };
struct Follow  { string up, fp, fname; };

static vector<User> loadUsers() {
    vector<User> v;
    for (const string& ln : splitLines(readFileUtf8(DATA_DIR + "/users.txt"))) {
        string line = trim(ln);
        if (line.empty()) continue;
        size_t a = line.find(' ');
        if (a == string::npos) continue;
        size_t b = line.find(' ', a + 1);
        User u;
        u.phone = line.substr(0, a);
        if (b == string::npos) { u.pwd = line.substr(a + 1); }
        else { u.pwd = line.substr(a + 1, b - a - 1); u.name = trim(line.substr(b + 1)); }
        if (u.phone.empty()) continue;
        v.push_back(u);
    }
    return v;
}
static void saveUsers(const vector<User>& v) {
    ostringstream o;
    for (const auto& u : v) o << u.phone << ' ' << u.pwd << ' ' << u.name << '\n';
    writeFileUtf8(DATA_DIR + "/users.txt", o.str());
}
static vector<Post> loadPosts() {
    vector<Post> v;
    vector<string> L = splitLines(readFileUtf8(DATA_DIR + "/contents.txt"));
    size_t i = 0;
    while (i < L.size()) {
        while (i < L.size() && trim(L[i]).empty()) i++;
        if (i + 3 >= L.size()) break;
        Post p;
        p.phone   = trim(L[i]);
        p.name    = trim(L[i + 1]);
        p.timeRaw = trim(L[i + 2]);
        p.content = L[i + 3];
        i += 4;
        if (p.phone.empty()) continue;
        v.push_back(p);
    }
    return v;
}
static void savePosts(const vector<Post>& v) {
    ostringstream o;
    for (const auto& p : v)
        o << p.phone << '\n' << p.name << '\n' << p.timeRaw << '\n' << p.content << '\n';
    writeFileUtf8(DATA_DIR + "/contents.txt", o.str());
}
static vector<Comment> loadComments() {
    vector<Comment> v;
    vector<string> L = splitLines(readFileUtf8(DATA_DIR + "/comments.txt"));
    size_t i = 0;
    while (i < L.size()) {
        while (i < L.size() && trim(L[i]).empty()) i++;
        if (i + 3 >= L.size()) break;
        string head = trim(L[i]);
        size_t sp = head.find(' ');
        Comment c;
        c.no = atoi(head.substr(0, sp == string::npos ? head.size() : sp).c_str());
        if (sp != string::npos) c.phone = trim(head.substr(sp + 1));
        c.postContent = L[i + 1];
        c.name = trim(L[i + 2]);
        c.text = L[i + 3];
        i += 4;
        v.push_back(c);
    }
    return v;
}
static void saveComments(const vector<Comment>& v) {
    ostringstream o;
    for (const auto& c : v)
        o << c.no << ' ' << c.phone << '\n' << c.postContent << '\n'
          << c.name << '\n' << c.text << "\n\n";
    writeFileUtf8(DATA_DIR + "/comments.txt", o.str());
}
static vector<Follow> loadFollows() {
    vector<Follow> v;
    vector<string> L = splitLines(readFileUtf8(DATA_DIR + "/follows.txt"));
    vector<string> t;
    for (const string& ln : L) if (!trim(ln).empty()) t.push_back(trim(ln));
    for (size_t i = 0; i + 2 < t.size(); i += 3)
        v.push_back(Follow{t[i], t[i + 1], t[i + 2]});
    return v;
}
static void saveFollows(const vector<Follow>& v) {
    ostringstream o;
    for (const auto& f : v) o << f.up << '\n' << f.fp << '\n' << f.fname << '\n';
    writeFileUtf8(DATA_DIR + "/follows.txt", o.str());
}

// ---------------- 校验（与原 function.cpp 同规则） ----------------
static bool validPhone(const string& p) {
    if (p.size() != 11) return false;
    for (char c : p) if (!isdigit((unsigned char)c)) return false;
    if (p[0] != '1') return false;
    char s = p[1], t = p[2];
    if (!strchr("34578", s)) return false;
    if (s == '4' && !strchr("79", t)) return false;
    if (s == '5' && t == '4') return false;
    if (s == '7' && !strchr("37", t)) return false;
    if (s == '8' && !strchr("01569", t)) return false;
    return true;
}
static string pwdError(const string& p) {
    if (p.size() < 8) return "密码长度必须大于 8 位";
    for (char c : p) if (!isalnum((unsigned char)c)) return "密码含有非法字符";
    bool num = false, up = false, low = false;
    for (char c : p) { if (isdigit((unsigned char)c)) num = true; else if (isupper((unsigned char)c)) up = true; else low = true; }
    if (!num) return "密码中必须有数字";
    if (!up)  return "密码中必须有大写字母";
    if (!low) return "密码中必须有小写字母";
    return "";
}

// ---------------- JSON 输出 ----------------
static string jsonState() {
    vector<User> users = loadUsers();
    vector<Post> posts = loadPosts();
    vector<Comment> comments = loadComments();
    vector<Follow> follows = loadFollows();

    // 帖子 id = 文件里的序号；已删除（##）的不输出
    vector<pair<string, string>> live;   // (id, content)
    ostringstream po;
    po << "[";
    bool first = true;
    for (size_t i = 0; i < posts.size(); i++) {
        if (posts[i].phone == "##") continue;
        string id = "p" + to_string(i + 1);
        live.push_back(make_pair(id, posts[i].content));
        if (!first) po << ",";
        first = false;
        po << "{\"id\":\"" << id << "\",\"phone\":\"" << jesc(posts[i].phone) << "\",\"name\":\""
           << jesc(posts[i].name) << "\",\"time\":\"" << isoFromCtime(posts[i].timeRaw)
           << "\",\"content\":\"" << jesc(posts[i].content) << "\"}";
    }
    po << "]";

    // 评论：靠「评论记录里的 postContent」找回所属帖子，时间按原帖时间顺延
    ostringstream co;
    co << "[";
    first = true;
    for (size_t i = 0; i < comments.size(); i++) {
        string pid;
        string ptime;
        int k = 0;
        for (size_t j = 0; j < posts.size(); j++) {
            if (posts[j].phone == "##") continue;
            if (posts[j].content == comments[i].postContent) {
                pid = "p" + to_string(j + 1);
                ptime = isoFromCtime(posts[j].timeRaw);
                break;
            }
        }
        if (pid.empty()) continue;
        for (size_t j = 0; j < i; j++)
            if (comments[j].postContent == comments[i].postContent) k++;
        if (!first) co << ",";
        first = false;
        co << "{\"id\":\"c" << (i + 1) << "\",\"postId\":\"" << pid << "\",\"phone\":\""
           << jesc(comments[i].phone) << "\",\"name\":\"" << jesc(comments[i].name)
           << "\",\"text\":\"" << jesc(comments[i].text) << "\",\"time\":\""
           << isoAdd(ptime, (k + 1) * 7) << "\"}";
    }
    co << "]";

    ostringstream uo, fo;
    uo << "[";
    first = true;
    for (const auto& u : users) {
        if (!first) uo << ",";
        first = false;
        uo << "{\"phone\":\"" << jesc(u.phone) << "\",\"pwd\":\"" << jesc(u.pwd)
           << "\",\"name\":\"" << jesc(u.name) << "\"}";
    }
    uo << "]";
    fo << "[";
    first = true;
    for (const auto& f : follows) {
        if (!first) fo << ",";
        first = false;
        fo << "{\"userphone\":\"" << jesc(f.up) << "\",\"fophone\":\"" << jesc(f.fp)
           << "\",\"foname\":\"" << jesc(f.fname) << "\"}";
    }
    fo << "]";

    ostringstream o;
    o << "{\"mode\":\"cpp\",\"users\":" << uo.str() << ",\"posts\":" << po.str()
      << ",\"comments\":" << co.str() << ",\"follows\":" << fo.str()
      << ",\"files\":{\"users\":" << users.size() << ",\"contents\":" << posts.size()
      << ",\"comments\":" << comments.size() << ",\"follows\":" << follows.size() << "}}";
    return o.str();
}
static string ok(const string& extra = "") {
    if (extra.empty()) return "{\"ok\":true}";
    return "{\"ok\":true," + extra + "}";
}
static string fail(const string& msg) {
    return "{\"ok\":false,\"error\":\"" + jesc(msg) + "\"}";
}

// ---------------- 业务 ----------------
static string apiLogin(const string& body) {
    string phone = trim(jget(body, "phone"));
    string pwd = jget(body, "pwd");
    if (!validPhone(phone)) return fail("手机号格式不正确");
    for (const auto& u : loadUsers()) {
        if (u.phone != phone) continue;
        if (u.pwd != pwd) return fail("密码不正确");
        return ok("\"user\":{\"phone\":\"" + jesc(u.phone) + "\",\"name\":\"" + jesc(u.name) + "\"}");
    }
    return fail("该手机号尚未注册");
}
static string apiRegister(const string& body) {
    string phone = trim(jget(body, "phone"));
    string pwd = jget(body, "pwd");
    string name = trim(jget(body, "name"));
    if (!validPhone(phone)) return fail("手机号格式不正确");
    string e = pwdError(pwd);
    if (!e.empty()) return fail(e);
    if (name.empty()) return fail("昵称不能为空");
    vector<User> users = loadUsers();
    for (const auto& u : users) if (u.phone == phone) return fail("该手机号已经注册");
    users.push_back(User{phone, pwd, name});
    saveUsers(users);
    return ok("\"user\":{\"phone\":\"" + jesc(phone) + "\",\"name\":\"" + jesc(name) + "\"}");
}
static string apiCreatePost(const string& body) {
    string phone = trim(jget(body, "phone"));
    string content = trim(jget(body, "content"));
    if (content.empty()) return fail("内容不能为空");
    if (content.size() > 1200) return fail("内容太长了");
    string name;
    bool found = false;
    for (const auto& u : loadUsers())
        if (u.phone == phone) { name = u.name; found = true; break; }
    if (!found) return fail("用户不存在，请重新登录");

    vector<Post> posts = loadPosts();
    Post p;
    p.phone = phone;
    p.name = name;
    p.timeRaw = trim(nowCtime());
    p.content = content;
    posts.push_back(p);
    savePosts(posts);
    return ok("\"post\":{\"id\":\"p" + to_string(posts.size()) + "\"}");
}
static string apiDeletePost(const string& body) {
    string id = jget(body, "id");
    if (id.size() < 2 || id[0] != 'p') return fail("参数不正确");
    size_t idx = (size_t)atoi(id.c_str() + 1);
    vector<Post> posts = loadPosts();
    if (idx < 1 || idx > posts.size()) return fail("动态不存在");
    // 与原程序一致：把手机号改成 ## 当墓碑，而不是真的删掉
    posts[idx - 1].phone = "##";
    savePosts(posts);
    return ok();
}
static string apiCreateComment(const string& body) {
    string postId = jget(body, "postId");
    string phone = trim(jget(body, "phone"));
    string text = trim(jget(body, "text"));
    string parentId = jget(body, "parentId");
    if (text.empty()) return fail("评论不能为空");
    if (text.size() > 600) return fail("评论太长了");

    vector<Post> posts = loadPosts();
    size_t idx = (size_t)atoi(postId.c_str() + 1);
    if (idx < 1 || idx > posts.size()) return fail("动态不存在");
    const Post& post = posts[idx - 1];
    if (post.phone == "##") return fail("动态已被删除");

    string name;
    bool found = false;
    for (const auto& u : loadUsers())
        if (u.phone == phone) { name = u.name; found = true; break; }
    if (!found) return fail("用户不存在，请重新登录");

    vector<Comment> cs = loadComments();

    // 回复某人：和原程序一样，把「   //博主回复：」拼到那条评论后面
    if (!parentId.empty() && parentId.size() > 1 && parentId[0] == 'c') {
        size_t ci = (size_t)atoi(parentId.c_str() + 1);
        if (ci < 1 || ci > cs.size()) return fail("要回复的评论不存在");
        cs[ci - 1].text += "   //博主回复：" + text;
        saveComments(cs);
        return ok();
    }

    int maxNo = 0;
    for (const auto& c : cs) if (c.postContent == post.content) maxNo = max(maxNo, c.no);
    Comment c;
    c.no = maxNo + 1;
    c.phone = phone;
    c.postContent = post.content;
    c.name = name;
    c.text = text;
    cs.push_back(c);
    saveComments(cs);
    return ok();
}
static string apiDeleteComment(const string& body) {
    string id = jget(body, "id");
    if (id.size() < 2 || id[0] != 'c') return fail("参数不正确");
    size_t idx = (size_t)atoi(id.c_str() + 1);
    vector<Comment> cs = loadComments();
    if (idx < 1 || idx > cs.size()) return fail("评论不存在");
    cs.erase(cs.begin() + (idx - 1));
    saveComments(cs);
    return ok();
}
static string apiFollow(const string& body) {
    string phone = trim(jget(body, "phone"));
    string target = trim(jget(body, "target"));
    string action = jget(body, "action");
    if (phone.empty() || target.empty()) return fail("参数不正确");
    if (phone == target) return fail("不能关注自己");

    vector<Follow> fs = loadFollows();
    auto it = find_if(fs.begin(), fs.end(),
        [&](const Follow& f) { return f.up == phone && f.fp == target; });
    if (action == "remove") {
        if (it != fs.end()) fs.erase(it);
        saveFollows(fs);
        return ok();
    }
    if (it != fs.end()) return ok();       // 已经关注了
    string fname;
    for (const auto& u : loadUsers()) if (u.phone == target) { fname = u.name; break; }
    fs.push_back(Follow{phone, target, fname});
    saveFollows(fs);
    return ok();
}

// ---------------- HTTP ----------------
static const char* statusText(int code) {
    switch (code) {
        case 200: return "OK";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        default:  return "OK";
    }
}
static void sendAll(SOCKET s, const string& data) {
    size_t sent = 0;
    while (sent < data.size()) {
        int n = send(s, data.data() + sent, (int)(data.size() - sent), 0);
        if (n <= 0) return;
        sent += (size_t)n;
    }
}
static void respond(SOCKET s, int code, const string& type, const string& body) {
    ostringstream o;
    o << "HTTP/1.1 " << code << ' ' << statusText(code) << "\r\n"
      << "Content-Type: " << type << "\r\n"
      << "Content-Length: " << body.size() << "\r\n"
      << "Access-Control-Allow-Origin: *\r\n"
      << "Access-Control-Allow-Methods: GET,POST,OPTIONS\r\n"
      << "Access-Control-Allow-Headers: Content-Type\r\n"
      << "Cache-Control: no-store\r\n"
      << "Connection: close\r\n\r\n";
    sendAll(s, o.str());
    if (!body.empty()) sendAll(s, body);
}
static string urlDecode(const string& s) {
    string o;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '%' && i + 2 < s.size()) {
            int v = 0;
            sscanf(s.substr(i + 1, 2).c_str(), "%x", &v);
            o += (char)v;
            i += 2;
        } else if (s[i] == '+') o += ' ';
        else o += s[i];
    }
    return o;
}
static string serveFile(const string& rel) {
    ifstream f((WEB_ROOT + "/" + rel).c_str(), ios::binary);
    if (!f) return "";
    string s((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    return s;
}
static void handleClient(SOCKET c) {
    string req;
    char buf[8192];
    size_t headerEnd = string::npos;
    long long contentLength = 0;
    while (true) {
        int n = recv(c, buf, sizeof(buf), 0);
        if (n <= 0) break;
        req.append(buf, n);
        if (headerEnd == string::npos) {
            headerEnd = req.find("\r\n\r\n");
            if (headerEnd != string::npos) {
                string head = req.substr(0, headerEnd);
                size_t p = head.find("Content-Length:");
                if (p != string::npos) contentLength = atoll(head.c_str() + p + 15);
            }
        }
        if (headerEnd != string::npos && req.size() >= headerEnd + 4 + (size_t)contentLength) break;
        if (req.size() > (size_t)MAX_BODY) break;
    }
    if (headerEnd == string::npos) { closesocket(c); return; }

    string head = req.substr(0, headerEnd);
    string body = req.substr(headerEnd + 4);
    if (contentLength > 0 && body.size() > (size_t)contentLength) body.resize((size_t)contentLength);

    istringstream hs(head);
    string method, target, version;
    hs >> method >> target >> version;
    string path = target;
    size_t q = path.find('?');
    if (q != string::npos) path = path.substr(0, q);

    if (method == "OPTIONS") { respond(c, 204, "text/plain", ""); closesocket(c); return; }

    if (method == "GET" && (path == "/" || path == "/index.html")) {
        string html = serveFile("index.html");
        if (html.empty()) { respond(c, 404, "text/plain; charset=utf-8", "index.html not found"); }
        else respond(c, 200, "text/html; charset=utf-8", html);
        closesocket(c);
        return;
    }
    if (method == "GET" && path == "/api/state") {
        respond(c, 200, "application/json; charset=utf-8", jsonState());
        closesocket(c);
        return;
    }
    if (method == "POST") {
        string out;
        if (path == "/api/login")            out = apiLogin(body);
        else if (path == "/api/register")    out = apiRegister(body);
        else if (path == "/api/post/create") out = apiCreatePost(body);
        else if (path == "/api/post/delete") out = apiDeletePost(body);
        else if (path == "/api/comment/create") out = apiCreateComment(body);
        else if (path == "/api/comment/delete") out = apiDeleteComment(body);
        else if (path == "/api/follow")      out = apiFollow(body);
        else {
            respond(c, 404, "application/json; charset=utf-8", fail("接口不存在"));
            closesocket(c);
            return;
        }
        respond(c, 200, "application/json; charset=utf-8", out);
        closesocket(c);
        return;
    }
    respond(c, 404, "text/plain; charset=utf-8", "not found");
    closesocket(c);
}

int main(int argc, char** argv) {
    SetConsoleOutputCP(CP_UTF8);

    int port = 8787;
    if (argc >= 2) port = atoi(argv[1]);
    if (port <= 0) port = 8787;
    if (argc >= 3) DATA_DIR = argv[2];

    if (!fs::exists(DATA_DIR)) fs::create_directories(DATA_DIR);

    // 首次运行：如果数据目录里没有 txt，就从原始的 Homework 目录拷一份过来
    const char* need[] = {"users.txt", "contents.txt", "comments.txt", "follows.txt"};
    bool anyMissing = false;
    for (const char* f : need) if (!fs::exists(DATA_DIR + "/" + f)) anyMissing = true;
    if (anyMissing) {
        string src = "../Homework/Homework";
        if (!fs::exists(src + "/users.txt")) src = "../../Homework/Homework";
        if (fs::exists(src + "/users.txt")) {
            for (const char* f : need)
                if (!fs::exists(DATA_DIR + "/" + f))
                    fs::copy_file(src + "/" + f, DATA_DIR + "/" + f, fs::copy_options::overwrite_existing);
            cout << "[init] 已从 " << src << " 复制四个 txt 到 " << DATA_DIR << "\n";
        } else {
            cout << "[warn] 没找到原始数据，将以空库启动\n";
        }
    }

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cout << "[error] WSAStartup 失败\n";
        return 1;
    }
    SOCKET srv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (srv == INVALID_SOCKET) { cout << "[error] socket 创建失败\n"; return 1; }

    int yes = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof yes);

    sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((u_short)port);

    if (bind(srv, (sockaddr*)&addr, sizeof addr) == SOCKET_ERROR) {
        cout << "[error] 端口 " << port << " 绑定失败（可能已被占用）\n";
        return 1;
    }
    listen(srv, 32);

    cout << "==========================================\n";
    cout << "  单机版小红书 · C++ 后端\n";
    cout << "  http://localhost:" << port << "\n";
    cout << "  数据目录: " << fs::absolute(DATA_DIR).string() << "\n";
    cout << "  Ctrl+C 停止\n";
    cout << "==========================================\n";
    cout.flush();

    while (true) {
        SOCKET c = accept(srv, nullptr, nullptr);
        if (c == INVALID_SOCKET) continue;
        handleClient(c);
    }
    closesocket(srv);
    WSACleanup();
    return 0;
}
