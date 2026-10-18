// ============================================================
//  单机版小红书 · C++ 后端（SQLite 版）
//
//  原来是四个 txt 当数据库；这一版把它们搬进 SQLite，
//  txt 只作为「首次导入」的只读数据源，运行期一个字节都不写。
//
//  编译： gcc -O2 -c sqlite3.c -o sqlite3.o
//         g++ -O2 -std=c++17 server.cpp sqlite3.o -o server.exe -lws2_32
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

#include "sqlite3.h"

using namespace std;
namespace fs = std::filesystem;

// ---------------- 全局 ----------------
static string DATA_DIR = "data";          // txt 种子 + 数据库所在目录
static string WEB_ROOT = ".";
static string DB_PATH  = "data/redbook.db";
static sqlite3* g_db = nullptr;
static const int MAX_BODY = 1 << 20;

// ---------------- 编码（读老 txt 用） ----------------
static string gbkToUtf8(const string& in) {
    if (in.empty()) return in;
    int n = MultiByteToWideChar(936, 0, in.c_str(), (int)in.size(), nullptr, 0);
    if (n <= 0) return in;
    wstring w(n, 0);
    MultiByteToWideChar(936, 0, in.c_str(), (int)in.size(), &w[0], n);
    int m = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    string o(m, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &o[0], m, nullptr, nullptr);
    return o;
}
static string readFileUtf8(const string& path) {
    ifstream f(path.c_str(), ios::binary);
    if (!f) return "";
    string raw((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    return gbkToUtf8(raw);
}

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

// ---------------- 时间 ----------------
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
static string fmtIso(time_t t) {
    struct tm tmv;
    localtime_s(&tmv, &t);
    char buf[40];
    strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%S", &tmv);
    return buf;
}
static string nowIso() { return fmtIso(time(nullptr)); }
static string isoAdd(const string& iso, int minutes) {
    time_t t = isoToTime(iso);
    if (!t) return iso;
    return fmtIso(t + (time_t)minutes * 60);
}

// ---------------- SQLite 薄封装 ----------------
struct Stmt {
    sqlite3_stmt* s = nullptr;
    explicit Stmt(const string& sql) {
        if (sqlite3_prepare_v2(g_db, sql.c_str(), -1, &s, nullptr) != SQLITE_OK) {
            cerr << "[sql] prepare failed: " << sqlite3_errmsg(g_db) << "\n";
            s = nullptr;
        }
    }
    ~Stmt() { if (s) sqlite3_finalize(s); }
    void bind(int i, const string& v) { sqlite3_bind_text(s, i, v.c_str(), -1, SQLITE_TRANSIENT); }
    void bind(int i, long long v) { sqlite3_bind_int64(s, i, v); }
    bool row() { return s && sqlite3_step(s) == SQLITE_ROW; }
    bool done() { return s && sqlite3_step(s) == SQLITE_DONE; }
    string txt(int c) const {
        const char* p = (const char*)sqlite3_column_text(s, c);
        return p ? p : "";
    }
    long long num(int c) const { return sqlite3_column_int64(s, c); }
};
static void dbExec(const string& sql) {
    char* err = nullptr;
    if (sqlite3_exec(g_db, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        cerr << "[sql] " << (err ? err : "?") << "\n";
        if (err) sqlite3_free(err);
    }
}
static long long scalar(const string& sql, const string& arg = "") {
    Stmt st(sql);
    if (!arg.empty()) st.bind(1, arg);
    if (st.row()) return st.num(0);
    return 0;
}

// ---------------- 建表 + 从 txt 导入 ----------------
static void ensureSchema() {
    dbExec("PRAGMA journal_mode=WAL;");
    dbExec(
        "CREATE TABLE IF NOT EXISTS users("
        "  phone TEXT PRIMARY KEY,"
        "  pwd   TEXT NOT NULL,"
        "  name  TEXT NOT NULL);"
        "CREATE TABLE IF NOT EXISTS posts("
        "  id      INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  phone   TEXT NOT NULL,"
        "  name    TEXT NOT NULL,"
        "  time    TEXT NOT NULL,"
        "  content TEXT NOT NULL,"
        "  deleted INTEGER NOT NULL DEFAULT 0);"
        "CREATE TABLE IF NOT EXISTS comments("
        "  id        INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  post_id   INTEGER NOT NULL,"
        "  phone     TEXT,"
        "  name      TEXT,"
        "  text      TEXT NOT NULL,"
        "  time      TEXT NOT NULL,"
        "  parent_id INTEGER,"
        "  deleted   INTEGER NOT NULL DEFAULT 0);"
        "CREATE TABLE IF NOT EXISTS follows("
        "  user_phone   TEXT NOT NULL,"
        "  follow_phone TEXT NOT NULL,"
        "  follow_name  TEXT NOT NULL,"
        "  PRIMARY KEY(user_phone, follow_phone));"
        "CREATE INDEX IF NOT EXISTS idx_comments_post ON comments(post_id);"
        "CREATE INDEX IF NOT EXISTS idx_posts_time ON posts(time);");
}

// 把 "评论正文博主回复：回复1博主回复：回复2" 拆开
static void splitReplies(const string& raw, string& main, vector<string>& replies) {
    string t = raw;
    // 老数据里可能写成 "   //博主回复："
    size_t pos = 0;
    while ((pos = t.find("//博主回复", pos)) != string::npos) t.erase(pos, 2);
    const string KEY = "博主回复：";
    main = t;
    size_t p = t.find(KEY);
    if (p == string::npos) { main = trim(t); return; }
    main = trim(t.substr(0, p));
    size_t cur = p + KEY.size();
    while (true) {
        size_t next = t.find(KEY, cur);
        string part = trim(t.substr(cur, next == string::npos ? string::npos : next - cur));
        if (!part.empty()) replies.push_back(part);
        if (next == string::npos) break;
        cur = next + KEY.size();
    }
    if (main.empty() && !replies.empty()) { main = replies.front(); replies.erase(replies.begin()); }
}

static void importFromTxt() {
    if (scalar("SELECT COUNT(*) FROM users") > 0) return;   // 已经导过
    cout << "[db] 空库，开始从 txt 导入…\n";

    sqlite3_exec(g_db, "BEGIN;", nullptr, nullptr, nullptr);

    int nUsers = 0;
    {
        vector<string> L = splitLines(readFileUtf8(DATA_DIR + "/users.txt"));
        for (const string& ln : L) {
            string line = trim(ln);
            if (line.empty()) continue;
            size_t a = line.find(' ');
            if (a == string::npos) continue;
            size_t b = line.find(' ', a + 1);
            string phone = line.substr(0, a);
            string pwd, name;
            if (b == string::npos) pwd = line.substr(a + 1);
            else { pwd = line.substr(a + 1, b - a - 1); name = trim(line.substr(b + 1)); }
            if (phone.empty()) continue;
            Stmt st("INSERT OR IGNORE INTO users(phone,pwd,name) VALUES(?,?,?)");
            st.bind(1, phone); st.bind(2, pwd); st.bind(3, name);
            st.done();
            nUsers++;
        }
    }

    int nPosts = 0, nDeleted = 0;
    {
        vector<string> L = splitLines(readFileUtf8(DATA_DIR + "/contents.txt"));
        size_t i = 0;
        while (i < L.size()) {
            while (i < L.size() && trim(L[i]).empty()) i++;
            if (i + 3 >= L.size()) break;
            string phone = trim(L[i]), name = trim(L[i + 1]), timeRaw = trim(L[i + 2]), content = L[i + 3];
            i += 4;
            if (phone.empty()) continue;
            bool del = (phone == "##");
            Stmt st("INSERT INTO posts(phone,name,time,content,deleted) VALUES(?,?,?,?,?)");
            st.bind(1, phone); st.bind(2, name); st.bind(3, isoFromCtime(timeRaw));
            st.bind(4, content); st.bind(5, del ? 1LL : 0LL);
            st.done();
            nPosts++;
            if (del) nDeleted++;
        }
    }

    int nComments = 0;
    {
        vector<string> L = splitLines(readFileUtf8(DATA_DIR + "/comments.txt"));
        size_t i = 0;
        int k = 0;
        while (i < L.size()) {
            while (i < L.size() && trim(L[i]).empty()) i++;
            if (i + 3 >= L.size()) break;
            string head = trim(L[i]);
            size_t sp = head.find(' ');
            string phone = (sp == string::npos) ? "" : trim(head.substr(sp + 1));
            string postContent = L[i + 1], cname = trim(L[i + 2]), rawText = L[i + 3];
            i += 4;

            long long pid = 0;
            {
                Stmt q("SELECT id FROM posts WHERE content=? AND deleted=0 ORDER BY id LIMIT 1");
                q.bind(1, postContent);
                if (q.row()) pid = q.num(0);
            }
            if (!pid) continue;
            string ptime, pname;
            {
                Stmt q("SELECT time,name FROM posts WHERE id=?");
                q.bind(1, pid);
                if (q.row()) { ptime = q.txt(0); pname = q.txt(1); }
            }
            string main;
            vector<string> replies;
            splitReplies(rawText, main, replies);
            string t = isoAdd(ptime, (k + 1) * 7);
            k++;
            long long cid = 0;
            {
                Stmt st("INSERT INTO comments(post_id,phone,name,text,time,parent_id) VALUES(?,?,?,?,?,NULL)");
                st.bind(1, pid); st.bind(2, phone); st.bind(3, cname); st.bind(4, main); st.bind(5, t);
                st.done();
                cid = sqlite3_last_insert_rowid(g_db);
            }
            int j = 1;
            for (const string& r : replies) {
                Stmt st("INSERT INTO comments(post_id,phone,name,text,time,parent_id) VALUES(?,?,?,?,?,?)");
                st.bind(1, pid); st.bind(2, ""); st.bind(3, pname); st.bind(4, r);
                st.bind(5, isoAdd(t, j)); st.bind(6, cid);
                st.done();
                j++;
            }
            nComments++;
        }
    }

    int nFollows = 0;
    {
        vector<string> raw = splitLines(readFileUtf8(DATA_DIR + "/follows.txt"));
        vector<string> t;
        for (const string& ln : raw) if (!trim(ln).empty()) t.push_back(trim(ln));
        for (size_t i = 0; i + 2 < t.size(); i += 3) {
            Stmt st("INSERT OR IGNORE INTO follows(user_phone,follow_phone,follow_name) VALUES(?,?,?)");
            st.bind(1, t[i]); st.bind(2, t[i + 1]); st.bind(3, t[i + 2]);
            st.done();
            nFollows++;
        }
    }

    sqlite3_exec(g_db, "COMMIT;", nullptr, nullptr, nullptr);

    cout << "[db] 导入完成: " << nUsers << " 用户 / " << nPosts << " 帖（含 "
         << nDeleted << " 条墓碑）/ " << nComments << " 评论 / " << nFollows << " 关注\n";
}

// ---------------- 校验 ----------------
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

// ---------------- JSON ----------------
static string okJson(const string& extra = "") {
    return extra.empty() ? "{\"ok\":true}" : "{\"ok\":true," + extra + "}";
}
static string failJson(const string& msg) {
    return "{\"ok\":false,\"error\":\"" + jesc(msg) + "\"}";
}

static string jsonState() {
    ostringstream o;
    o << "{\"mode\":\"sqlite\"";

    o << ",\"users\":[";
    {
        Stmt st("SELECT phone,pwd,name FROM users ORDER BY rowid");
        bool first = true;
        while (st.row()) {
            if (!first) o << ",";
            first = false;
            o << "{\"phone\":\"" << jesc(st.txt(0)) << "\",\"pwd\":\"" << jesc(st.txt(1))
              << "\",\"name\":\"" << jesc(st.txt(2)) << "\"}";
        }
    }
    o << "]";

    o << ",\"posts\":[";
    {
        Stmt st("SELECT id,phone,name,time,content FROM posts WHERE deleted=0 ORDER BY time DESC, id DESC");
        bool first = true;
        while (st.row()) {
            if (!first) o << ",";
            first = false;
            o << "{\"id\":\"p" << st.num(0) << "\",\"phone\":\"" << jesc(st.txt(1))
              << "\",\"name\":\"" << jesc(st.txt(2)) << "\",\"time\":\"" << jesc(st.txt(3))
              << "\",\"content\":\"" << jesc(st.txt(4)) << "\"}";
        }
    }
    o << "]";

    o << ",\"comments\":[";
    {
        Stmt st("SELECT id,post_id,phone,name,text,time FROM comments "
                "WHERE deleted=0 AND parent_id IS NULL ORDER BY time, id");
        bool first = true;
        while (st.row()) {
            long long cid = st.num(0);
            if (!first) o << ",";
            first = false;
            o << "{\"id\":\"c" << cid << "\",\"postId\":\"p" << st.num(1) << "\",\"phone\":\""
              << jesc(st.txt(2)) << "\",\"name\":\"" << jesc(st.txt(3)) << "\",\"text\":\""
              << jesc(st.txt(4)) << "\",\"time\":\"" << jesc(st.txt(5)) << "\",\"replies\":[";
            Stmt rq("SELECT name,text,time FROM comments WHERE parent_id=? AND deleted=0 ORDER BY time,id");
            rq.bind(1, cid);
            bool f2 = true;
            while (rq.row()) {
                if (!f2) o << ",";
                f2 = false;
                o << "{\"name\":\"" << jesc(rq.txt(0)) << "\",\"text\":\"" << jesc(rq.txt(1))
                  << "\",\"time\":\"" << jesc(rq.txt(2)) << "\"}";
            }
            o << "]}";
        }
    }
    o << "]";

    o << ",\"follows\":[";
    {
        Stmt st("SELECT user_phone,follow_phone,follow_name FROM follows ORDER BY rowid");
        bool first = true;
        while (st.row()) {
            if (!first) o << ",";
            first = false;
            o << "{\"userphone\":\"" << jesc(st.txt(0)) << "\",\"fophone\":\"" << jesc(st.txt(1))
              << "\",\"foname\":\"" << jesc(st.txt(2)) << "\"}";
        }
    }
    o << "]";

    o << ",\"files\":{\"users\":" << scalar("SELECT COUNT(*) FROM users")
      << ",\"contents\":" << scalar("SELECT COUNT(*) FROM posts")
      << ",\"comments\":" << scalar("SELECT COUNT(*) FROM comments WHERE parent_id IS NULL AND deleted=0")
      << ",\"follows\":" << scalar("SELECT COUNT(*) FROM follows") << "}";

    o << ",\"db\":{\"path\":\"" << jesc(DB_PATH) << "\",\"engine\":\"SQLite "
      << sqlite3_libversion() << "\"}}";
    return o.str();
}

// ---------------- 业务 ----------------
static string apiLogin(const string& body) {
    string phone = trim(jget(body, "phone"));
    string pwd = jget(body, "pwd");
    if (!validPhone(phone)) return failJson("手机号格式不正确");
    Stmt st("SELECT pwd,name FROM users WHERE phone=?");
    st.bind(1, phone);
    if (!st.row()) return failJson("该手机号尚未注册");
    if (st.txt(0) != pwd) return failJson("密码不正确");
    return okJson("\"user\":{\"phone\":\"" + jesc(phone) + "\",\"name\":\"" + jesc(st.txt(1)) + "\"}");
}
static string apiRegister(const string& body) {
    string phone = trim(jget(body, "phone"));
    string pwd = jget(body, "pwd");
    string name = trim(jget(body, "name"));
    if (!validPhone(phone)) return failJson("手机号格式不正确");
    string e = pwdError(pwd);
    if (!e.empty()) return failJson(e);
    if (name.empty()) return failJson("昵称不能为空");
    if (scalar("SELECT COUNT(*) FROM users WHERE phone=?", phone) > 0) return failJson("该手机号已经注册");
    Stmt st("INSERT INTO users(phone,pwd,name) VALUES(?,?,?)");
    st.bind(1, phone); st.bind(2, pwd); st.bind(3, name);
    if (!st.done()) return failJson("写入失败");
    return okJson("\"user\":{\"phone\":\"" + jesc(phone) + "\",\"name\":\"" + jesc(name) + "\"}");
}
static string apiCreatePost(const string& body) {
    string phone = trim(jget(body, "phone"));
    string content = trim(jget(body, "content"));
    if (content.empty()) return failJson("内容不能为空");
    if (content.size() > 1200) return failJson("内容太长了");
    string name;
    {
        Stmt q("SELECT name FROM users WHERE phone=?");
        q.bind(1, phone);
        if (!q.row()) return failJson("用户不存在，请重新登录");
        name = q.txt(0);
    }
    Stmt st("INSERT INTO posts(phone,name,time,content) VALUES(?,?,?,?)");
    st.bind(1, phone); st.bind(2, name); st.bind(3, nowIso()); st.bind(4, content);
    if (!st.done()) return failJson("发布失败");
    return okJson("\"post\":{\"id\":\"p" + to_string(sqlite3_last_insert_rowid(g_db)) + "\"}");
}
static string apiDeletePost(const string& body) {
    string id = jget(body, "id");
    string phone = trim(jget(body, "phone"));
    if (id.size() < 2) return failJson("参数不正确");
    long long pid = atoll(id.c_str() + 1);
    Stmt q("SELECT phone FROM posts WHERE id=? AND deleted=0");
    q.bind(1, pid);
    if (!q.row()) return failJson("动态不存在");
    if (!phone.empty() && q.txt(0) != phone) return failJson("只能删除自己的动态");
    Stmt st("UPDATE posts SET deleted=1 WHERE id=?");
    st.bind(1, pid);
    st.done();
    return okJson();
}
static string apiCreateComment(const string& body) {
    string postId = jget(body, "postId");
    string phone = trim(jget(body, "phone"));
    string text = trim(jget(body, "text"));
    string parentId = jget(body, "parentId");
    if (text.empty()) return failJson("评论不能为空");
    if (text.size() > 600) return failJson("评论太长了");
    if (postId.size() < 2) return failJson("参数不正确");
    long long pid = atoll(postId.c_str() + 1);

    {
        Stmt q("SELECT deleted FROM posts WHERE id=?");
        q.bind(1, pid);
        if (!q.row()) return failJson("动态不存在");
        if (q.num(0)) return failJson("动态已被删除");
    }
    string name;
    {
        Stmt q("SELECT name FROM users WHERE phone=?");
        q.bind(1, phone);
        if (!q.row()) return failJson("用户不存在，请重新登录");
        name = q.txt(0);
    }
    long long parent = 0;
    if (!parentId.empty() && parentId.size() > 1) {
        parent = atoll(parentId.c_str() + 1);
        Stmt q("SELECT COUNT(*) FROM comments WHERE id=? AND deleted=0 AND post_id=?");
        q.bind(1, parent); q.bind(2, pid);
        if (!q.row() || q.num(0) == 0) return failJson("要回复的评论不存在");
    }
    Stmt st("INSERT INTO comments(post_id,phone,name,text,time,parent_id) VALUES(?,?,?,?,?,?)");
    st.bind(1, pid); st.bind(2, phone); st.bind(3, name); st.bind(4, text); st.bind(5, nowIso());
    if (parent) st.bind(6, parent); else sqlite3_bind_null(st.s, 6);
    if (!st.done()) return failJson("评论失败");
    return okJson();
}
static string apiDeleteComment(const string& body) {
    string id = jget(body, "id");
    string phone = trim(jget(body, "phone"));
    if (id.size() < 2) return failJson("参数不正确");
    long long cid = atoll(id.c_str() + 1);
    Stmt q("SELECT phone FROM comments WHERE id=? AND deleted=0");
    q.bind(1, cid);
    if (!q.row()) return failJson("评论不存在");
    if (!phone.empty() && q.txt(0) != phone) return failJson("只能删除自己的评论");
    {
        Stmt st("UPDATE comments SET deleted=1 WHERE id=?");
        st.bind(1, cid);
        st.done();
    }
    {
        Stmt st("UPDATE comments SET deleted=1 WHERE parent_id=?");
        st.bind(1, cid);
        st.done();
    }
    return okJson();
}
static string apiFollow(const string& body) {
    string phone = trim(jget(body, "phone"));
    string target = trim(jget(body, "target"));
    string action = jget(body, "action");
    if (phone.empty() || target.empty()) return failJson("参数不正确");
    if (phone == target) return failJson("不能关注自己");
    if (action == "remove") {
        Stmt st("DELETE FROM follows WHERE user_phone=? AND follow_phone=?");
        st.bind(1, phone); st.bind(2, target);
        st.done();
        return okJson();
    }
    string fname;
    {
        Stmt q("SELECT name FROM users WHERE phone=?");
        q.bind(1, target);
        if (!q.row()) return failJson("对方不存在");
        fname = q.txt(0);
    }
    Stmt st("INSERT OR IGNORE INTO follows(user_phone,follow_phone,follow_name) VALUES(?,?,?)");
    st.bind(1, phone); st.bind(2, target); st.bind(3, fname);
    st.done();
    return okJson();
}

// ---------------- HTTP ----------------
static const char* statusText(int code) {
    switch (code) {
        case 200: return "OK";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 404: return "Not Found";
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
static string serveFile(const string& rel) {
    ifstream f((WEB_ROOT + "/" + rel).c_str(), ios::binary);
    if (!f) return "";
    return string((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
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
        if (html.empty()) respond(c, 404, "text/plain; charset=utf-8", "index.html not found");
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
        if (path == "/api/login")                out = apiLogin(body);
        else if (path == "/api/register")        out = apiRegister(body);
        else if (path == "/api/post/create")     out = apiCreatePost(body);
        else if (path == "/api/post/delete")     out = apiDeletePost(body);
        else if (path == "/api/comment/create")  out = apiCreateComment(body);
        else if (path == "/api/comment/delete")  out = apiDeleteComment(body);
        else if (path == "/api/follow")          out = apiFollow(body);
        else {
            respond(c, 404, "application/json; charset=utf-8", failJson("接口不存在"));
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
    DB_PATH = DATA_DIR + "/redbook.db";

    if (!fs::exists(DATA_DIR)) fs::create_directories(DATA_DIR);

    // 数据目录里没有 txt 就从原始 Homework 目录拷一份（只读，仅作导入源）
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
            cout << "[init] 已从 " << src << " 复制四个 txt 作为导入源\n";
        }
    }

    if (sqlite3_open(DB_PATH.c_str(), &g_db) != SQLITE_OK) {
        cout << "[error] 打不开数据库 " << DB_PATH << ": " << sqlite3_errmsg(g_db) << "\n";
        return 1;
    }
    sqlite3_busy_timeout(g_db, 3000);
    ensureSchema();
    importFromTxt();

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
    cout << "  单机版小红书 · 后端（SQLite）\n";
    cout << "  http://localhost:" << port << "\n";
    cout << "  数据库: " << fs::absolute(DB_PATH).string() << "\n";
    cout << "  引擎: " << sqlite3_libversion() << "\n";
    cout << "  Ctrl+C 停止\n";
    cout << "==========================================\n";
    cout.flush();

    while (true) {
        SOCKET c = accept(srv, nullptr, nullptr);
        if (c == INVALID_SOCKET) continue;
        handleClient(c);
    }
    sqlite3_close(g_db);
    closesocket(srv);
    WSACleanup();
    return 0;
}
