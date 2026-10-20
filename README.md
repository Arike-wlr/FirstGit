# 单机版小红书

把 C++ 课设的「单机版小红书」重做成能看的网页 + 安卓 App。

原程序是一个纯文本菜单项目，用四个 `.txt` 文件存数据（`users.txt` / `contents.txt` / `comments.txt` / `follows.txt`）。
这个项目保留了原程序的功能与数据结构，换掉了两样东西：

- **存储**：四个 txt → 一个 SQLite 库（`data/redbook.db`）
- **界面**：黑窗口菜单 → 网页界面，左下角保留一个「原始版本 · 控制台」入口，可以进去敲两下回味

---

## 快速开始

### 网页版

```
双击 启动后端.bat
```

脚本会用最小化窗口启动 `server.exe 8787`，然后自动打开 <http://localhost:8787>。

想手动启动也行：

```
server.exe 8787
```

## 项目结构

```
redbook/
├── index.html          网页全部内容（HTML + CSS + JS 单文件，约 2079 行）
├── server.cpp          C++ 后端（约 731 行），零第三方框架，只依赖 SQLite + Winsock
├── server.exe          编译好的后端（.gitignore 已忽略）
├── 启动后端.bat        一键启动脚本
├── data/
│   └── redbook.db      SQLite 数据库：四张表全在这一个文件里（首次启动自动创建）
├── sqlite3.c/.h/.o     SQLite 3.54.1  amalgamation，直接编进后端
├── apk/
│   └── 单机版小红书-离线版.apk
├── tools/
│   ├── make_icon.py     生成 app.ico（桌面快捷方式图标）
│   ├── app.ico
│   └── app-icon.png     256px 预览图
├── android-app/        安卓工程（Java + WebView）
└── .gitignore
```

---

## 三种运行模式

`index.html` 根据打开方式自动切换数据来源，顶栏左角会显示当前状态：

| 打开方式 | 状态标识 | 数据来源 |
|---|---|---|
| `http://localhost:8787` | C++ 后端 · 端口 8787 | `data/redbook.db`（经后端读写） |
| 双击 `index.html` | 纯前端 · localStorage | 浏览器 localStorage |
| 安卓 App 内打开 | 纯前端 · localStorage | 手机 WebView 的 localStorage |

判定逻辑在 `index.html` 顶部：

```js
const IN_APP = location.hostname === 'appassets.androidplatform.net';
const API = (!IN_APP && /^https?:$/.test(location.protocol)) ? 'api' : null;
```

> 网页端和 App 端目前**不互通**，各自独立。要做手机↔电脑同步得另外设计（服务端 CORS 已经开好了，`Access-Control-Allow-Origin: *`）。

---

## 数据模型

### 只有一个 `.db` 文件

`data/` 下就 `redbook.db` 一个文件，四张表 + 两个索引全在里头。

会不会多出伴生文件，取决于 SQLite 的日志模式：

| 模式 | 目录里有什么 | 本项目 |
|---|---|---|
| `DELETE`（默认，rollback journal） | 只有 `redbook.db`。事务进行中会临时冒出一个 `redbook.db-journal`，提交即删 | ✅ 当前使用 |
| `WAL`（write-ahead log） | 常驻 `redbook.db-wal` + `redbook.db-shm`，这就是「看起来像三个数据库」的原因 | ❌ 已弃用 |

之前用的是 WAL，后来换回默认模式，原因很实际：

- 后端是单线程 `accept` 循环，一次只处理一个请求（另有 `sqlite3_busy_timeout` 3 秒兜底），**没有并发读写需要照顾**，WAL 的并发优势用不上；
- 单机课设场景几乎不会断电崩盘，WAL 的抗崩溃收益也不明显；
- 代价却是目录里多两个文件，备份要拷三个，拷给别人还得解释哪个是哪个。

切模式只要在服务停掉后跑一次，SQLite 会把 `-wal` 里的内容 checkpoint 回主库、再删掉两个文件，数据一行不少：

```
sqlite3 data/redbook.db "PRAGMA wal_checkpoint(TRUNCATE); PRAGMA journal_mode=DELETE;"
```

> 有个坑：只 checkpoint 不切模式没用，下次启动又会重新生成那两个文件——因为 `server.cpp` 的 `ensureSchema()` 里写死了 `PRAGMA journal_mode=WAL`。那行已经一并改成 `DELETE` 了。

对应代码（`server.cpp`）：

```cpp
DB_PATH = DATA_DIR + "/redbook.db";
sqlite3_open(DB_PATH.c_str(), &g_db);
dbExec("PRAGMA journal_mode=DELETE;");   // 单文件模式，不产生 -wal / -shm
```

所以**备份就是拷 `data/redbook.db` 一个文件**，没有“只拷主库会丢最新数据”这回事了。`-journal` 是事务期间的临时文件，提交后自动删除，也不用管（`.gitignore` 里顺手忽略了 `*.db-journal`）。

### 四张表

对应原来的四个 txt：

| 原文件 | 表 | 字段 |
|---|---|---|
| `users.txt` | `users` | phone / pwd / name / deleted |
| `contents.txt` | `posts` | id / phone / name / time / content / deleted |
| `comments.txt` | `comments` | id / post_id / phone / name / text / time / parent_id / deleted / reply_to |
| `follows.txt` | `follows` | user_phone / follow_phone / follow_name |

另外建了两个索引：`idx_comments_post ON comments(post_id)`、`idx_posts_time ON posts(time)`。

**现在库里装了什么**（2026-10-20 统计，都是测试时随手发的）：

| 表 | 行数 | 说明 |
|---|---|---|
| `users` | 17 | 其中 4 个已注销（`deleted=1`，数据留档，手机号可重新注册） |
| `posts` | 27 | 其中 11 条已删（软删除，`/api/state` 不再返回） |
| `comments` | 70 | 其中 3 条已删；`parent_id` 非空的就是「回复的回复」 |
| `follows` | 26 | 取关是**真删行**（`DELETE FROM follows`），所以这张表没有 `deleted` 列 |

一句话概括各表职责：`users` 是账号（手机号当主键），`posts` 是动态正文，`comments` 用 `post_id` 挂在动态下、用 `parent_id` 串回复链，`follows` 是两个手机号组成的关注关系（复合主键，重复关注会被 `INSERT OR IGNORE` 挡掉）。

两处和原程序不同的地方，都是刻意改的：

- **软删除**：删动态 / 删评论 / 注销账号走 `deleted` 标记，不物理删除，所以动态和评论能保留。删用户后该手机号可重新注册。
- **回复**：`comments.parent_id` 支持「回复某条回复」，原程序只在正文里拼 `博主回复：` 前缀，现在拆成结构化字段。`reply_to` 记下被回复人的名字。

> `reply_to` 是后加的列——`ensureSchema()` 会检查 `PRAGMA table_info`，老库里没有就 `ALTER TABLE` 补上，所以旧数据库不用重建。

---

## HTTP 接口

后端只做一件事：读写 SQLite。全部接口在 `server.cpp` 的 `handleClient()` 里分发。

**GET**

| 路径 | 说明 |
|---|---|
| `/` `/index.html` | 返回 `index.html` |
| `/api/state` | 一次性拉取全量状态（users / posts / comments / follows） |

**POST**（body 为 JSON）

| 路径 | 说明 |
|---|---|
| `/api/login` | 登录 |
| `/api/register` | 注册 |
| `/api/post/create` | 发动态 |
| `/api/post/delete` | 删动态（软删除） |
| `/api/comment/create` | 评论 / 回复 |
| `/api/comment/delete` | 删评论 |
| `/api/follow` | 关注 / 取关 |
| `/api/user/delete` | 注销账号（软删除） |

返回统一是 `{ ok: true, ... }` 或 `{ ok: false, error: "..." }`。

---

## 从源码重新编译

### 后端（MinGW / MSYS2）

`sqlite3.o` 已经编好了，直接链接即可：

```
g++ -O2 -std=c++17 server.cpp sqlite3.o -o server.exe -lws2_32
```

如果 `sqlite3.o` 也丢了，先编 SQLite：

```
gcc -O2 -c sqlite3.c -o sqlite3.o
```

### 安卓 App

需要 JDK 17、Gradle 8.11.1、Android SDK（compileSdk 35）。

```
cd android-app
gradle assembleRelease
```

产物在 `android-app/app/build/outputs/apk/release/app-release.apk`。

**改了 `index.html` 后**，记得同步一份到 App 资源目录，否则 App 里还是旧页面：

```
cp index.html android-app/app/src/main/assets/web/index.html
```

App 通过 `WebViewAssetLoader` 以 `https://appassets.androidplatform.net` 虚拟域名加载这份 HTML，好处是同源策略正常，`localStorage` 能用。

---

## 已知问题 / 待办

- [ ] 网页端与 App 端数据不同步
- [x] ~~`data/redbook.db-wal` / `data/redbook.db-shm` 被 git 跟踪~~ —— 已切回 `DELETE` 日志模式，两个文件不存在了，并且已 `git rm --cached` + 写进 `.gitignore`
- [ ] `android-app/app/build.gradle` 里 `versionName` 目前是调试残留值（`"1.我已经半分钟没有更新了"`），发布前要改回正常版本号
- [ ] release 包用的是 debug 签名，正式发布需要换成自己的签名

---

## 桌面快捷方式（带图标）

在 PowerShell 里执行（会先删掉旧的符号链接）：

```powershell
$sh = New-Object -ComObject WScript.Shell
$lnk = $sh.CreateShortcut("C:\Users\HUAWEI\Desktop\单机版小红书.lnk")
$lnk.TargetPath       = "D:\2025\FirstGit-cpp\redbook\启动后端.bat"
$lnk.WorkingDirectory = "D:\2025\FirstGit-cpp\redbook"
$lnk.IconLocation     = "D:\2025\FirstGit-cpp\redbook\tools\app.ico,0"
$lnk.Description      = "单机版小红书 · 启动后端"
$lnk.Save()
```

图标文件 `tools/app.ico` 由 `tools/make_icon.py` 生成（图案与网页 favicon 一致：红底白色书本）：

```
python tools/make_icon.py
```

> 图标改版只需重跑脚本，然后**删掉桌面快捷方式重新执行上面的 PowerShell**（或直接刷新图标缓存），旧快捷方式不会自动更新。

---

## 一点说明

- 左下角「原始版本 · 控制台」是**用 JavaScript 复刻**原程序菜单的交互外壳，不是在跑老 C++ 代码。但它读写的数据是真的——每条命令都走 `call()` 打到 `server.cpp`，和网页共用同一个 SQLite 库。
- 数据早已不依赖 txt 文件。原先有一段「从 txt 导入」的启动逻辑和 txt 文件本体，都已删除；`server.cpp` 里现在没有任何 txt 读写。