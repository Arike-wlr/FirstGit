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
static string DATA_DIR = "data";          // 数据库（SQLite）所在目录
static string WEB_ROOT = ".";
static string DB_PATH  = "data/redbook.db";
static sqlite3* g_db = nullptr;
static const int MAX_BODY = 1 << 20;

// ---------------- 小工具 ----------------
static string trim(const string& s) {
    size_t a = 0, b = s.size();
    while (a < b && isspace((unsigned char)s[a])) a++;
    while (b > a && isspace((unsigned char)s[b - 1])) b--;
    return s.substr(a, b - a);
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
static string fmtIso(time_t t) {
    struct tm tmv;
    localtime_s(&tmv, &t);
    char buf[40];
    strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%S", &tmv);
    return buf;
}
static string nowIso() { return fmtIso(time(nullptr)); }

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

// ---------------- 建表 + 补列 ----------------
static void ensureSchema() {
    // 用 SQLite 默认的 rollback journal（DELETE 模式），不用 WAL。
    // WAL 会在 data/ 下额外生成 redbook.db-wal 和 redbook.db-shm 两个文件；
    // 本项目是单线程 accept 循环、单进程访问，用不上 WAL 的并发读写优势，
    // 一个 redbook.db 更好备份也更好拷给别人。
    dbExec("PRAGMA journal_mode=DELETE;");
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

    // 老库迁移：回复某条回复时，记下被回复人的名字
    bool hasReplyTo = false;
    {
        Stmt st("PRAGMA table_info(comments)");
        while (st.row()) if (st.txt(1) == "reply_to") hasReplyTo = true;
    }
    if (!hasReplyTo) dbExec("ALTER TABLE comments ADD COLUMN reply_to TEXT;");

    // 注销账号：软删除，帖子和评论都保留
    bool hasUserDeleted = false;
    {
        Stmt st("PRAGMA table_info(users)");
        while (st.row()) if (st.txt(1) == "deleted") hasUserDeleted = true;
    }
    if (!hasUserDeleted) dbExec("ALTER TABLE users ADD COLUMN deleted INTEGER NOT NULL DEFAULT 0;");
}

// ---------------- 校验 ----------------
static bool validPhone(const string& p) {
    if (p.size() != 11) return false;
    for (char c : p) if (!isdigit((unsigned char)c)) return false;
    if (p[0] != '1') return false;
    if (p[1] < '3' || p[1] > '9') return false;
    return true;
}
static string pwdError(const string& p) {
    if (p.size() < 8) return "密码长度至少 8 位";
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
        Stmt st("SELECT phone,pwd,name FROM users WHERE deleted=0 ORDER BY rowid");
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
            Stmt rq("SELECT id,name,text,time,reply_to,phone FROM comments WHERE parent_id=? AND deleted=0 ORDER BY time,id");
            rq.bind(1, cid);
            bool f2 = true;
            while (rq.row()) {
                if (!f2) o << ",";
                f2 = false;
                o << "{\"id\":\"c" << rq.num(0) << "\",\"name\":\"" << jesc(rq.txt(1))
                  << "\",\"text\":\"" << jesc(rq.txt(2)) << "\",\"time\":\"" << jesc(rq.txt(3))
                  << "\",\"to\":\"" << jesc(rq.txt(4)) << "\",\"phone\":\"" << jesc(rq.txt(5)) << "\"}";
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
    Stmt st("SELECT pwd,name FROM users WHERE phone=? AND deleted=0");
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
    if (scalar("SELECT COUNT(*) FROM users WHERE phone=?", phone) > 0) {
        // 已注销的号允许重新注册（旧帖子和评论会归到新账号名下）
        if (scalar("SELECT deleted FROM users WHERE phone=?", phone) == 0)
            return failJson("该手机号已经注册");
        Stmt up("UPDATE users SET pwd=?, name=?, deleted=0 WHERE phone=?");
        up.bind(1, pwd); up.bind(2, name); up.bind(3, phone);
        if (!up.done()) return failJson("写入失败");
        return okJson("\"user\":{\"phone\":\"" + jesc(phone) + "\",\"name\":\"" + jesc(name) + "\"}");
    }
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
    string to = trim(jget(body, "to"));      // 回复某条回复时，被回复人的名字
    Stmt st("INSERT INTO comments(post_id,phone,name,text,time,parent_id,reply_to) VALUES(?,?,?,?,?,?,?)");
    st.bind(1, pid); st.bind(2, phone); st.bind(3, name); st.bind(4, text); st.bind(5, nowIso());
    if (parent) st.bind(6, parent); else sqlite3_bind_null(st.s, 6);
    if (!to.empty()) st.bind(7, to); else sqlite3_bind_null(st.s, 7);
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

static string apiDeleteUser(const string& body) {
    string phone = trim(jget(body, "phone"));
    if (phone.empty()) return failJson("参数不正确");
    if (scalar("SELECT COUNT(*) FROM users WHERE phone=? AND deleted=0", phone) == 0)
        return failJson("账号不存在");
    // 只标记账号失效：posts / comments / follows 一行都不动
    Stmt st("UPDATE users SET deleted=1 WHERE phone=?");
    st.bind(1, phone);
    if (!st.done()) return failJson("注销失败");
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
        else if (path == "/api/user/delete")    out = apiDeleteUser(body);
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

    if (sqlite3_open(DB_PATH.c_str(), &g_db) != SQLITE_OK) {
        cout << "[error] 打不开数据库 " << DB_PATH << ": " << sqlite3_errmsg(g_db) << "\n";
        return 1;
    }
    sqlite3_busy_timeout(g_db, 3000);
    ensureSchema();

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
