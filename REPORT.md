# MP4 Template Design Report

> 本份 report 說明在 `ntuos2026-mp4` template 中，我（助教）一共挖了哪些 `/* To do: ... */` 空格、每個空格對應到作業的哪一部分、要學生實作什麼、以及為何挑這幾個點當作 hand-in 位置。
>
> 文件目標讀者：助教自己（給未來 regrade 用）＋ 寫 spec 的人（spec 的 “Implementation requirements” 章節可以直接抄 §1.2 / §2.x）。

## 0. 設計總原則

| Part | 主題 | 分數 | 空格數 |
| :--- | :--- | :--: | :--: |
| Part 1 | Large file（doubly indirect pointer） | 40% | 2 |
| Part 2 | Symbolic link（file / dir / cycle prevent） | 60% | 4 |
| **合計** | | **100%** | **6** |

兩個原則：

1. **「最少必要修改」**：只挖學生「非寫不可」的地方。`syscall.h / syscall.c / usys.pl / user.h / fcntl.h / stat.h` 這些 boilerplate / spec 常數通通預先接好；連 `struct dinode` 與 `struct inode` 的新 `addrs[NDIRECT+3]` layout 我也直接補在 template，**不要學生改 struct definition**，避免學生在「on-disk vs in-memory 對齊」這種無教學價值的坑卡住。學生面對的只剩 **fs.c 的兩個 function** 與 **symlink 相關的 4 個 hook 點**。
2. **「指出位置、不教做法」**：每個空格只留 `/* To do: <task> */` 多行區塊、body 一律留空。Template 只負責告訴學生「該動的地方在這裡」；做法請看 spec 與 lecture。

### 預先補好的結構（學生不用碰，但要知道）

`kernel/fs.h`：

```c
#define NDIRECT 10
#define NINDIRECT (BSIZE / sizeof(uint))
#define NDINDIRECT (NINDIRECT * NINDIRECT)
#define MAXFILE (NDIRECT + NINDIRECT + 2*NDINDIRECT)

// addrs layout: [0..NDIRECT-1] direct
//               [NDIRECT]      singly indirect
//               [NDIRECT+1..2] two doubly indirect
struct dinode {
  short type;
  short major;
  short minor;
  short nlink;
  uint size;
  uint addrs[NDIRECT+3];
};
```

`kernel/file.h` 的 `struct inode` 對應改成 `uint addrs[NDIRECT+3];`。

也就是說 layout 是 **10 direct + 1 singly + 2 doubly**，上限 = `10 + 256 + 2×256×256 = 131,338` block，足夠 `bigfile` 測資（≤ 6666 block）的兩倍餘裕。學生實作 `bmap` / `itrunc` 時要對上這個 layout。

---

## 1. Part 1：Large File（40%）

> 目標：學生看懂上面 `addrs[NDIRECT+3]` 這個 layout 後，把 `bmap`（讀寫路徑）與 `itrunc`（unlink 路徑）兩個函式都擴充到能正確處理「2 個 doubly-indirect slot」。

### 1.1 空格清單

| # | 檔案 | 行號 | 空格 | 為何挖在這 |
| :- | :--- | :--: | :--- | :--- |
| ① | [kernel/fs.c](kernel/fs.c#L411) | 411 | `bmap()` 開頭 | `bmap(ip, bn)` 是「邏輯 block number → 實體 disk block address」的唯一入口；所有 `readi/writei` 都會走到這裡。原版只處理 `bn < NDIRECT` 與 `bn < NINDIRECT` 兩段，第三段（doubly-indirect）就是這題重點。 |
| ② | [kernel/fs.c](kernel/fs.c#L459) | 459 | `itrunc()` 開頭 | unlink 時 `itrunc` 必須對稱地把所有配過的 block 釋放，否則 `bigfile` 跑完會 leak 出十萬個 block，後續測試把 `FSSIZE=200000` 的 disk 慢慢吃光。 |

### 1.2 學生必須實作的內容（可直接拿去寫 spec）

#### ① `bmap()` ── doubly-indirect 讀寫

擴充 `bmap()`，在原本兩段 (direct / singly indirect) 之**後**加入第三段：

- 輸入：inode pointer `ip` 與邏輯 block number `bn`，`bn ∈ [0, MAXFILE)`。
- 對於 `bn ∈ [NDIRECT + NINDIRECT, NDIRECT + NINDIRECT + 2*NDINDIRECT)`：
  - 推算出該 block 落在 `addrs[NDIRECT+1]` 或 `addrs[NDIRECT+2]` 哪一個 doubly slot（`slot = NDIRECT + 1 + sub_index / NDINDIRECT`）。
  - 推算第一層 indirect block 中的 index（`i1`）與第二層 indirect block 中的 index（`i2`）。
  - 若 `ip->addrs[slot]` 為 0，呼叫 `balloc` 配新 block。
  - 透過 `bread/bwrite/log_write/brelse` 一路走兩層 indirect block。第一層、第二層的 entry 若為 0 同樣要 `balloc`。
  - 回傳最終的 data block address；任何 `balloc` 失敗時應該回 0（不要 panic）。
- 不可使用 recursion，必要時 `brelse` 上一層 buffer 後再 `bread` 下一層，避免一次拿太多 buffer 觸發 `bget: no buffers` panic。

#### ② `itrunc()` ── doubly-indirect 釋放

擴充 `itrunc()`，在原本釋放 direct + singly-indirect 之**後**加入第三段：

- 對 `slot ∈ {NDIRECT+1, NDIRECT+2}`：
  - 若 `ip->addrs[slot] == 0` 跳過。
  - `bread` 第一層 indirect block，對其中每個非 0 entry：
    - `bread` 第二層 indirect block，對其中每個非 0 entry 呼叫 `bfree` 釋放 data block。
    - `brelse` 第二層，`bfree` 該第一層 entry。
  - `brelse` 第一層，`bfree` `ip->addrs[slot]`，並把 `ip->addrs[slot] = 0`。
- 結束後 `ip->size = 0; iupdate(ip);` 已由原本程式碼處理，不要重複。

> 額外提示：`itrunc` 原本宣告了 `int i, j; struct buf *bp; uint *a;`，doubly-indirect 段需要再多兩個 local 變數（多一層迴圈 index + 多一個 buffer pointer + 多一個 entry array pointer）。

### 1.3 為什麼 layout 預先寫死

把 `NDIRECT=10`、`addrs[NDIRECT+3]` 直接放進 template，是因為這個決定**沒有教學自由度**：

- 若放給學生自選，會出現 11/1、10/2、9/3 等變體，grading 環境 disk image 對不上。
- `mkfs/mkfs.c` 會根據這個 layout 算出 inode 大小，不一致時 disk image 整個爛掉。
- 課程目標是「理解 indirect pointer 的階層存取」，而不是「決定要幾層 indirect」。

預先寫死 layout 後，學生改完 fs.c 即可。`make clean && make qemu` 會自動 rebuild `fs.img`。

---

## 2. Part 2：Symbolic Link & Cycle Prevent（60%）

> 目標：實作 `int symlink(const char *target, const char *path)` syscall，並把 path lookup 改成「遇到 `T_SYMLINK` 會跟著走、但有 cycle 時要安全失敗」。
>
> 先講結論：**cycle prevention 不是獨立空格，而是被 inline 寫進 ④⑤⑥ 三個 follow loop 內部**（見 §2.5）。

`SYS_symlink = 22` 的 syscall number、`usys.pl` 的 stub、`user.h` 的 prototype、kernel `syscalls[]` table 的 dispatch entry、`fcntl.h` 的 `O_NOFOLLOW = 0x004`、`stat.h` 的 `T_SYMLINK = 4`，這些都已經接好。學生只要動下表 4 個位置。

### 2.1 空格總覽

| # | 檔案 | 行號 | sub-task |
| :- | :--- | :--: | :--- |
| ③ | [kernel/sysfile.c](kernel/sysfile.c#L521) | 521 | `sys_symlink()` 整個 body（symlink for file 的「建立」部分） |
| ④ | [kernel/sysfile.c](kernel/sysfile.c#L337) | 337 | `sys_open()` 在 `ilock(ip)` 之後加 follow 邏輯（symlink for file 的「讀取」部分 + cycle） |
| ⑤ | [kernel/fs.c](kernel/fs.c#L710) | 710 | `namex()` 在 `ip = next;` 之後加 follow 邏輯（symlink for directory 的「path 中間段」+ cycle） |
| ⑥ | [kernel/sysfile.c](kernel/sysfile.c#L428) | 428 | `sys_chdir()` 在 `ilock(ip)` 之後加 follow 邏輯（symlink for directory 的「path 末段」+ cycle） |

### 2.2 ③ `sys_symlink(target, path)`（Symlink for file，10/20 分基礎）

學生要從 0 寫一個 syscall body：

- 用 `argstr(0, target, MAXPATH)` 與 `argstr(1, path, MAXPATH)` 取兩個字串參數，失敗回 -1。**建議在呼叫 `argstr` 前先把 buffer 用 `memset` 清零**，避免 `writei` 寫到未初始化記憶體。
- 用 `begin_op() / end_op()` 包成一個 log transaction。
- 呼叫 `create(path, T_SYMLINK, 0, 0)` 建立一個新的 symlink inode；`create` 回傳的 inode 已經是 `ilock` 狀態（這點和 `sys_open` 的 `O_CREATE` 一致）。`create` 失敗（檔名已存在等）就 `end_op()` + return -1。
- 把 `target` 字串透過 `writei(ip, 0, (uint64)target, 0, MAXPATH)` 寫進該 symlink inode 的 data block。建議寫滿 `MAXPATH` bytes（含結尾的 zero padding），讓未來 `readi` 拿回 string 不需要單獨記 length。寫入失敗（< MAXPATH）必須 `iunlockput(ip) + end_op() + return -1`。
- 最後 `iunlockput(ip); end_op(); return 0;`。

> 重點 invariant：**符號連結存在 inode data block 裡的就是純粹的 path 字串**。lookup 端（④⑤⑥）就是把這串 path 餵回 `namei` 重新解析。

### 2.3 ④ `sys_open()` follow loop（Symlink for file，10/20 分 + cycle 共用）

在 `sys_open()` 既有的 `ilock(ip)` 之後、`ip->type == T_DEVICE` 檢查之前，加入：

- 進入條件：`ip->type == T_SYMLINK && !(omode & O_NOFOLLOW)`。
  - 如果使用者顯式給 `O_NOFOLLOW`，要把 symlink 本身當作 fd 開出去（不要 follow），這樣 `stat_slink` 之類的 test 才能讀到 `T_SYMLINK`。
- 進到 loop 後重複以下動作直到 `ip->type != T_SYMLINK`：
  - **cycle check**：如果 `ip->inum` 已經在 `visited[]` 集合裡 → `iunlockput(ip); end_op(); return -1`。
  - **safety check**：如果 `visited` 數量達上限（建議 100）→ 一樣失敗，避免 kernel stack 被超長 chain 打爆。
  - 把當前 `ip->inum` 加進 `visited[]`。
  - 用 `readi(ip, 0, (uint64)tgt, 0, MAXPATH)` 把 target path 讀出來；失敗即回 -1。
  - `iunlockput(ip);`，呼叫 `namei(tgt)` 拿到新的 inode；`namei` 失敗即回 -1（注意：`namei` 不會 lock，所以下一輪要先 `ilock` 才能讀 type）。
- Loop 結束後，若 follow 完得到的是 `T_DIR` 但 `omode != O_RDONLY`，要 `iunlockput + end_op + return -1`（symlink 不能讓 dir 偷偷被 O_RDWR 開啟）。

### 2.4 ⑤ `namex()` follow loop（Symlink for directory ─ path 中間段，10/20 分 + cycle 共用）

當路徑像 `/a/b/c`，而 `b` 本身是個 directory symlink 時，必須在 path lookup **解析到 `b` 之後、繼續往下找 `c` 之前**先 follow。挖空位置在 `namex()` 內 `ip = next;` 之後。

學生要做的：

- 進入條件：`*path != '\0'`（還有未解析的 component；末段 component 留給 caller 決定要不要 follow，這是 dir-symlink 與 file-symlink 行為差異的關鍵）。
- 在進入 loop 前先 `ilock(ip)`（前一行 `iunlockput` 已把鎖放開）。
- Loop 邏輯與 §2.3 完全一致：
  - cycle check（`ip->inum` 已在 `visited[]`）→ `iunlockput; return 0;`（`namex` 失敗用 `return 0` 而非 -1）
  - 大於 100 hop → 同樣 `return 0`
  - `readi` target → `iunlockput` → `namei(tgt)` → 失敗 `return 0`，成功 `ilock` 後續迴圈
- Loop 結束（`ip->type != T_SYMLINK`）後 `iunlock(ip);` 讓外層迴圈繼續使用 `ip`。

> 注意：`namex` 是 read-only path resolution，**不要**在這裡呼叫 `begin_op/end_op`；caller（`sys_open` / `sys_unlink` 等）已經包好了 transaction。

### 2.5 ⑥ `sys_chdir()` follow loop（Symlink for directory ─ path 末段，10/20 分 + cycle 共用）

`chdir("/link_to_dir")` 時，`namei` 拿到的會是 symlink inode 本身（因為末段不會被 `namex` 自動 follow，見 §2.4）。`sys_chdir` 必須自己再 follow 一次才能拿到真正的 directory。

在 `ilock(ip)` 之後、`ip->type != T_DIR` 檢查之前加入：

- 不需要進入條件判斷（不管 `ip->type` 是不是 symlink，都進 loop 試一次；loop 條件 `ip->type == T_SYMLINK` 自然會在非 symlink 時直接跳出）。
- Loop 邏輯與 §2.3 完全一致：visited-set cycle check、100 hop 上限、`readi` → `iunlockput` → `namei` → `ilock`。失敗一律 `end_op + return -1`。
- Loop 結束後讓現有的 `ip->type != T_DIR` 檢查接手判斷。

### 2.6 Cycle Prevent（20%）── 它放在哪裡？

> **重點回答（user 問的）：cycle prevention 沒有專屬的空格；它是 §2.3 / §2.4 / §2.5 三個 follow loop 內部「visited inode set」這 4 行的副作用。**
>
> 即是說，學生實作 ④⑤⑥ 三個 follow loop 時，**每一個 loop 都必須自己內含一份 cycle check**。沒有獨立挖第七個空格。

具體機制（在 ④⑤⑥ 每個 follow loop 裡都要重複一遍）：

```c
uint visited[100];
int n = 0;
while (ip->type == T_SYMLINK) {
    for (int i = 0; i < n; i++)
        if (visited[i] == ip->inum)
            /* cycle! 釋放並回失敗 */;
    if (n >= 100) /* 太長，回失敗 */;
    visited[n++] = ip->inum;
    /* ...readi target → iunlockput → namei → ilock... */
}
```

#### 為什麼用 visited-set，不用 depth-limit？

- 早期想法是「hop 數 ≥ 上限 → 失敗」，但測資 `mp4_symlinkcycle_public.py` 的 public4 是 25-hop **非** cyclic 直鏈（`a → b → c → … → y → z`），這對 depth-limit 是死亡測資（25 剛好踩邊界）。
- visited-set 直接命中「cycle = 重複造訪同一個 inode」的數學定義：25-hop 直鏈會經過 25 個**不同**的 inum，過得了；任何 cycle 在繞回時 inum 必然重複，立刻擋下。
- 空間 `uint visited[100]` 只佔 stack 400 bytes；時間 O(n²) 在 n ≤ 100 時 ≈ 10000 次整數比較，相對一次 disk read 完全可忽略。

#### 為什麼不挖獨立空格給 cycle prevent？

- cycle 偵測**必須**和 follow loop 共享同一份 hop 計數器與 inum 紀錄，不然 follow 完一輪後 visited[] 就被丟掉，第二個 syscall 進來又從 0 開始。
- 若獨立挖一個空格，學生會誤以為要寫一個 `int has_cycle(struct inode *ip)` 的 graph traversal function，這跟 Linux `ELOOP` 的實作方式完全不符。
- 所以選擇「綁在 follow loop 內部」這個更貼近真實 Unix kernel 的設計。

---

## 3. 對照表：6 個空格與課程目標

| # | 檔案:行 | 對應 Part | 學生要實作什麼 | 含 cycle? |
| :--: | :--- | :--: | :--- | :--: |
| ① | fs.c:411 | 1 (40%) | `bmap()` 加入 2 個 doubly-indirect slot 的查找 / 配置 | – |
| ② | fs.c:459 | 1 (40%) | `itrunc()` 加入 2 個 doubly-indirect slot 的釋放（防 block leak） | – |
| ③ | sysfile.c:521 | 2 (10%) | `sys_symlink` body：create T_SYMLINK inode + writei target | – |
| ④ | sysfile.c:337 | 2 (10%) + cycle | `sys_open` follow loop：O_NOFOLLOW、follow chain、若末端為 T_DIR 拒絕非 RDONLY | ✓ |
| ⑤ | fs.c:710 | 2 (10%) + cycle | `namex` follow loop：path 中間段（`*path != '\0'`）才 follow | ✓ |
| ⑥ | sysfile.c:428 | 2 (10%) + cycle | `sys_chdir` follow loop：末段是 dir symlink 時 follow 後再做 T_DIR 檢查 | ✓ |

合計：Part 1 共 2 個空格、Part 2 共 4 個空格，cycle prevent 被 ④⑤⑥ **共同**承擔 20 分。

---

## 4. 不挖在哪裡（避免誤導）

| 不挖的地方 | 原因 |
| :--- | :--- |
| `kernel/fs.h` 的 `NDIRECT / NDINDIRECT / MAXFILE` 與 `struct dinode.addrs[]` | layout 沒有教學自由度，由 mkfs 與 inode 大小強制決定；給學生自選只會 disk image 對不上。**已在 template 直接補成解答版**。 |
| `kernel/file.h` 的 `struct inode.addrs[]` | 必須跟 dinode 對齊，連動上一條。**已在 template 直接補成解答版**。 |
| `kernel/syscall.c` / `syscall.h` / `usys.pl` / `user.h` | 純 boilerplate（註冊 syscall），無教學價值，挖了只會讓學生在「忘了註冊 → usertests 直接掛」失分。 |
| `kernel/fcntl.h` 的 `O_NOFOLLOW = 0x004` | spec 級常數，挖了會讓學生各寫各的 bit value，造成 grading 不一致。 |
| `kernel/stat.h` 的 `T_SYMLINK = 4` | 同上，且 user-side test (`symlinkfile.c`) 會直接用這個常數比對 `st.type`。 |
| `kernel/param.h` 的 `FSSIZE = 200000` | 已預先放大並加註解，避免學生 disk 不夠 `bigfile` 跑。 |
| Cycle prevent 獨立空格 | 見 §2.6，必須是 follow loop 的一部分。 |

---

## 5.

整份 mp4 一共 **6 個 `/* To do */` 空格**：

- **Part 1（large file，40%）** ── 2 格全在 [kernel/fs.c](kernel/fs.c)：
  - ① `bmap()` 第三段（doubly-indirect 讀寫，2 個 slot）
  - ② `itrunc()` 第三段（doubly-indirect 釋放，防 block leak）
  - layout（`NDIRECT=10` + 1 singly + 2 doubly，`addrs[NDIRECT+3]`）**已在 template 預先補上**，學生不用碰結構定義。

- **Part 2（symlink + cycle prevent，60%）** ── 4 格分散在 sysfile.c / fs.c：
  - ③ `sys_symlink` body — `create(path, T_SYMLINK) + writei(target, MAXPATH)`
  - ④ `sys_open` follow loop — 處理 `O_NOFOLLOW` 與 file symlink
  - ⑤ `namex` follow loop — 處理 directory symlink 的 path 中間段
  - ⑥ `sys_chdir` follow loop — 處理 path 末段是 directory symlink
  - **cycle prevent 沒有獨立空格，被 inline 在 ④⑤⑥ 三個 follow loop 裡的 `visited[100]` inode-set 內部處理**（見 §2.5 / §2.6）。25-hop 非 cyclic 直鏈也能正確通過。
