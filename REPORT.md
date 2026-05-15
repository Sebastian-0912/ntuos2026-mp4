# MP4 Template Design Report

> 本份 report 說明在 `ntuos2026-mp4` template 中，我（助教）一共挖了哪些空格 (`/* To do: ... */`)、每個空格分別對應到作業的哪一個 part，以及為何挑這幾個點當作學生的 hand-in 位置。

## 0. 設計總原則

整份 mp4 的目標分成兩個 part：

| Part | 主題 | 分數 | 對應 To do 空格數 |
| :--- | :--- | :--: | :--: |
| Part 1 | Large file（doubly indirect pointer） | 40% | 5 |
| Part 2 | Symbolic link（file / dir / cycle prevent） | 60% | 4 |
| **合計** | | **100%** | **9** |

挖空格時遵守兩個原則：

1. **「最少必要修改」**：只挖學生「非寫不可」的地方，不挖週邊的 boilerplate（例如 `usys.pl`、`user.h`、`syscall.c`、`syscall.h` 都已先把 `sys_symlink` 的 syscall number、user-side stub、kernel dispatch table 接好），讓學生專注在 file system 與 path lookup 的邏輯，不要把時間花在「忘記在哪裡註冊 syscall」這種瑣事上。
2. **「指出位置、不教做法」**：每個空格只放 `/* To do: <task> */` 多行區塊，body 一律留空。學生要怎麼做請看 handbook 與課程講義；template 只負責告訴他「該動的地方在這裡」。

下面依 part 逐一說明。

---

## 1. Part 1：Large File（40%）

> 目標：理解 inode 與 disk data block 的對應關係，並把 xv6 原本「12 direct + 1 single-indirect」的 layout 擴充到能裝下 `bigfile` 測資的 6666 個 block。
>
> **本份解答採用的 layout 是「10 direct + 1 single-indirect + 2 doubly-indirect」**，即 `addrs[]` 從 `NDIRECT+1 = 13` 改成 `NDIRECT+3 = 13`（NDIRECT 從 12 降到 10），上限 = `10 + 256 + 2*256*256 = 131,338` 個 block。理論上「11 direct + 1 doubly」也夠用（`11 + 256 + 65536 = 65,803`），但我採用 2 個 doubly-slot 的版本是為了多一倍餘裕，未來測資加大也不用再改 layout。

這個 part 我一共挖了 **5 個空格**，分成「on-disk / in-memory 結構」與「實際存取邏輯」兩類。

### 1.1 結構類：3 個空格

| # | 檔案 | 行號 | 空格 | 為何挖在這 |
| :- | :--- | :--: | :--- | :--- |
| ① | [kernel/fs.h](kernel/fs.h#L27) | 27 | `NDIRECT / NINDIRECT / MAXFILE` 這組常數定義上方 | 加 doubly-indirect 一定要重新定義 `NDIRECT`、新增 `NDINDIRECT`、並把 `MAXFILE` 改大。挖在這裡是為了強迫學生想清楚「整體 block 上限」這個對外契約。 |
| ② | [kernel/fs.h](kernel/fs.h#L42) | 42 | `struct dinode` 的 `addrs[NDIRECT+1]` 上方 | on-disk inode 的 `addrs[]` 長度要從 `NDIRECT+1` 改大。這個改動會直接影響 `mkfs` 產生的 disk image，學生改完之後必須 `make clean`、重新跑 `mkfs`。 |
| ③ | [kernel/file.h](kernel/file.h#L29) | 29 | `struct inode`（in-memory）的 `addrs[]` 上方 | in-memory inode 必須跟 on-disk dinode 對得起來，否則 `iget` / `ilock` 從 disk 載回 inode 時會 layout 對不上。挖兩個（②③）是為了讓學生親眼看到「on-disk vs in-memory」這條對應線。 |

#### 解答 ①：`kernel/fs.h:27`

```c
#define NDIRECT 10
#define NINDIRECT (BSIZE / sizeof(uint))
#define NDINDIRECT (NINDIRECT * NINDIRECT)
#define MAXFILE (NDIRECT + NINDIRECT + 2*NDINDIRECT)
```

#### 解答 ②：`kernel/fs.h:42`（`struct dinode`）

```c
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

#### 解答 ③：`kernel/file.h:29`（`struct inode`）

```c
uint addrs[NDIRECT+3];
```

> 三個空格放在一起的教學意義：學生光是改完這 3 個地方，編譯就會 OK，但 `bmap` 還不會用新欄位 → 自然推進到下一組空格。

### 1.2 邏輯類：2 個空格

| # | 檔案 | 行號 | 空格 | 為何挖在這 |
| :- | :--- | :--: | :--- | :--- |
| ④ | [kernel/fs.c](kernel/fs.c#L411) | 411 | `bmap()` 開頭 | `bmap(ip, bn)` 是「邏輯 block number → 實體 disk block address」的唯一入口，所有 read / write / append 都會走到這裡。原版只處理 `bn < NDIRECT` 與 `bn < NINDIRECT` 兩段，第三段（doubly-indirect）就是 Part 1 的重點。把 To do 放在原本兩段判斷的「正前方」，讓學生在 read 完整段邏輯後，再自然加入第三段。 |
| ⑤ | [kernel/fs.c](kernel/fs.c#L459) | 459 | `itrunc()` 開頭 | 寫入路徑要記得 free 掉，否則檔案 unlink 之後 doubly-indirect 子樹會 leak block，跑完 `bigfile` 測資會把 `FSSIZE=200000` 的 disk 慢慢吃光。挖在這裡是要求學生「allocate 與 free 必須對稱」。 |

#### 解答 ④：`kernel/fs.c` 的 `bmap()`（在原本兩段判斷之後追加第三段）

```c
bn -= NINDIRECT;

if(bn < 2 * NDINDIRECT){
  int slot = NDIRECT + 1 + bn / NDINDIRECT;
  uint sub = bn % NDINDIRECT;
  uint i1 = sub / NINDIRECT;   // 第一層 index
  uint i2 = sub % NINDIRECT;   // 第二層 index

  // 第 0 層：addrs[slot] 指向第一層 indirect block
  if((addr = ip->addrs[slot]) == 0){
    addr = balloc(ip->dev);
    if(addr == 0) return 0;
    ip->addrs[slot] = addr;
  }
  bp = bread(ip->dev, addr);
  a = (uint*)bp->data;

  // 第 1 層：a[i1] 指向第二層 indirect block
  if((addr = a[i1]) == 0){
    addr = balloc(ip->dev);
    if(addr == 0){ brelse(bp); return 0; }
    a[i1] = addr;
    log_write(bp);
  }
  brelse(bp);

  // 第 2 層：b[i2] 才是真正的 data block
  bp = bread(ip->dev, addr);
  a = (uint*)bp->data;
  if((addr = a[i2]) == 0){
    addr = balloc(ip->dev);
    if(addr){
      a[i2] = addr;
      log_write(bp);
    }
  }
  brelse(bp);
  return addr;
}
```

#### 解答 ⑤：`kernel/fs.c` 的 `itrunc()`（在原本 singly-indirect 釋放之後追加）

```c
for(k = 0; k < 2; k++){               // 兩個 doubly-indirect slot
  int slot = NDIRECT + 1 + k;
  if(ip->addrs[slot] == 0)
    continue;
  bp = bread(ip->dev, ip->addrs[slot]);
  a = (uint*)bp->data;
  for(j = 0; j < NINDIRECT; j++){     // 第一層
    if(a[j] == 0) continue;
    bp2 = bread(ip->dev, a[j]);
    b = (uint*)bp2->data;
    for(m = 0; m < NINDIRECT; m++){   // 第二層 → data block
      if(b[m]) bfree(ip->dev, b[m]);
    }
    brelse(bp2);
    bfree(ip->dev, a[j]);
  }
  brelse(bp);
  bfree(ip->dev, ip->addrs[slot]);
  ip->addrs[slot] = 0;
}
```

> 別忘了在 `itrunc` 開頭把 local 變數加上 `int i, j, k, m;` 與 `struct buf *bp, *bp2; uint *a, *b;`。

### 1.3 配套的提示（不算 To do，但故意露出來給學生）

- [kernel/param.h](kernel/param.h#L12)：`FSSIZE` 已經被改成 `200000` 並加註解 `// largefile: file system needs 200000 blocks to fit 6666-block test`，告訴學生「你的 disk 一定要夠大」。
- [user/bigfile.c](user/bigfile.c)：直接給出 270 / 6666 兩個 target，讓學生可以本機快速驗證。

---

## 2. Part 2：Symbolic Link & Cycle Prevent（60%）

> 目標：實作 `int symlink(const char *target, const char *path)` syscall，並且把 path lookup 改成「遇到 `T_SYMLINK` 會跟著走、但有 cycle 時要安全失敗」。

這個 part 我挖了 **4 個空格**，分別對應到三個 sub-task。`sys_symlink` 的 syscall number（`SYS_symlink = 22`）、user-side stub（`usys.pl: entry("symlink")`）、kernel dispatch entry（`[SYS_symlink] sys_symlink`）、以及 `O_NOFOLLOW`（`fcntl.h`）和 `T_SYMLINK`（`stat.h`）這些「腳手架」都已經接好，學生不必碰。

### 2.1 Sub-task A：`symlink()` for file（20%）

| # | 檔案 | 行號 | 空格 | 為何挖在這 |
| :- | :--- | :--: | :--- | :--- |
| ⑥ | [kernel/sysfile.c](kernel/sysfile.c#L521) | 521 | `sys_symlink()` 整個 function body | 這是 `symlink(target, path)` syscall 的本體：要 `argstr` 取兩個參數、`begin_op/end_op` 包 transaction、`create(path, T_SYMLINK, 0, 0)` 建一個新的 symlink inode，再把 `target` 字串寫進該 inode 的 data block。挖整個 body（只留 `panic("sys_symlink not implemented");`）是因為這部分是學生第一次自己加一個完整的 syscall，需要練習「從 0 寫一個 sys_xxx」的流程。 |
| ⑦ | [kernel/sysfile.c](kernel/sysfile.c#L337) | 337 | `sys_open()` 裡 `ilock(ip)` 之後、`ip->type == T_DEVICE` 檢查之前 | symlink 建好之後，`open("link")` 要會自動 follow。挖在這個位置是因為：(a) `ilock(ip)` 已經拿到，可以讀 inode；(b) 還沒進入 `f->type` / `fd` 的分配，跳轉 inode 不會洩漏 fd；(c) 學生要在這裡處理 `O_NOFOLLOW`（不 follow）以及「multi-hop symlink chain」的迭代。 |

> 在「open 的哪一行 follow」是這題最常見的 bug 點：太早 follow 會錯過 `O_NOFOLLOW`、太晚 follow 會把 symlink 當成普通檔案打開。把空格放在這個夾縫，等於替學生畫好邊界。

#### 解答 ⑥：`kernel/sysfile.c` 的 `sys_symlink()`

```c
uint64
sys_symlink(void)
{
  char target[MAXPATH], path[MAXPATH];
  struct inode *ip;

  memset(target, 0, sizeof(target));
  memset(path, 0, sizeof(path));
  if(argstr(0, target, MAXPATH) < 0 || argstr(1, path, MAXPATH) < 0)
    return -1;

  begin_op();
  ip = create(path, T_SYMLINK, 0, 0);   // create 會回傳已 ilock 的 inode
  if(ip == 0){
    end_op();
    return -1;
  }
  if(writei(ip, 0, (uint64)target, 0, MAXPATH) != MAXPATH){
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}
```

#### 解答 ⑦：`kernel/sysfile.c` 的 `sys_open()` follow 段（cycle prevent 也在這）

```c
if(ip->type == T_SYMLINK && !(omode & O_NOFOLLOW)){
  uint visited[100];
  int n = 0;
  char tgt[MAXPATH];
  while(ip->type == T_SYMLINK){
    for(int i = 0; i < n; i++){
      if(visited[i] == ip->inum){       // 偵測到已經拜訪過 → cycle
        iunlockput(ip); end_op();
        return -1;
      }
    }
    if(n >= 100){                       // 100 hop 上限保險
      iunlockput(ip); end_op();
      return -1;
    }
    visited[n++] = ip->inum;
    memset(tgt, 0, sizeof(tgt));
    if(readi(ip, 0, (uint64)tgt, 0, MAXPATH) <= 0){
      iunlockput(ip); end_op();
      return -1;
    }
    iunlockput(ip);
    if((ip = namei(tgt)) == 0){ end_op(); return -1; }
    ilock(ip);
  }
  if(ip->type == T_DIR && omode != O_RDONLY){   // follow 完若是 dir，需要 readonly
    iunlockput(ip); end_op();
    return -1;
  }
}
```

### 2.2 Sub-task B：`symlink()` for directory（20%）

| # | 檔案 | 行號 | 空格 | 為何挖在這 |
| :- | :--- | :--: | :--- | :--- |
| ⑧ | [kernel/fs.c](kernel/fs.c#L710) | 710 | `namex()` 內，`ip = next;` 之後、`while` 繼續下一輪 path component 之前 | path lookup 的核心。當路徑中間段（如 `/testsymlink3/q/p`）的 `q` 是 directory symlink 時，必須在這裡 follow 才能讓後面的 `p` 接著被解析。挖在 `ip = next` 之後是因為這時 `ip` 已經是下一段的 inode、且尚未 `ilock`，符合 follow 後重新 `iget` 的流程。 |
| ⑨ | [kernel/sysfile.c](kernel/sysfile.c#L428) | 428 | `sys_chdir()` 裡 `ilock(ip)` 之後、`ip->type != T_DIR` 檢查之前 | `chdir("link_to_dir")` 應該要能進入 symlink 指到的真正 directory，否則 `T_DIR` check 會把 symlink 直接判失敗。挖在 `ilock` 之後、`T_DIR` 之前，是 follow 邏輯該插入的唯一合法位置。 |

> 把空格 ⑧ 放在 `namex` 而不是 `open` 是刻意的：directory symlink 與 file symlink 在 lookup 時機點不同，前者要在「path 還沒解析完」就 follow，後者是「path 解析完最後一段」才 follow。⑦ 對應後者、⑧ 對應前者，兩個空格逼學生分清楚這條界線。

#### 解答 ⑧：`kernel/fs.c` 的 `namex()` 在 `ip = next;` 之後

```c
if(*path != '\0'){                      // 只 follow path 還沒走完的中間段
  uint visited[100];
  int n = 0;
  char tgt[MAXPATH];
  ilock(ip);
  while(ip->type == T_SYMLINK){
    for(int i = 0; i < n; i++){
      if(visited[i] == ip->inum){ iunlockput(ip); return 0; }
    }
    if(n >= 100){ iunlockput(ip); return 0; }
    visited[n++] = ip->inum;
    memset(tgt, 0, sizeof(tgt));
    if(readi(ip, 0, (uint64)tgt, 0, MAXPATH) <= 0){
      iunlockput(ip); return 0;
    }
    iunlockput(ip);
    if((ip = namei(tgt)) == 0) return 0;
    ilock(ip);
  }
  iunlock(ip);
}
```

#### 解答 ⑨：`kernel/sysfile.c` 的 `sys_chdir()` 在 `ilock(ip)` 之後

```c
{
  uint visited[100];
  int n = 0;
  char tgt[MAXPATH];
  while(ip->type == T_SYMLINK){
    for(int i = 0; i < n; i++){
      if(visited[i] == ip->inum){ iunlockput(ip); end_op(); return -1; }
    }
    if(n >= 100){ iunlockput(ip); end_op(); return -1; }
    visited[n++] = ip->inum;
    memset(tgt, 0, sizeof(tgt));
    if(readi(ip, 0, (uint64)tgt, 0, MAXPATH) <= 0){
      iunlockput(ip); end_op(); return -1;
    }
    iunlockput(ip);
    if((ip = namei(tgt)) == 0){ end_op(); return -1; }
    ilock(ip);
  }
}
```

### 2.3 Sub-task C：Cycle Prevent（20%）

Cycle prevent 我**沒有再額外挖一個專屬的空格**，原因是：

- self-loop（`a → a`）、2-cycle（`a → b → a`）、n-cycle 全部都是「重複拜訪同一個 inode」的特例，自然會落在空格 ⑦⑧⑨ 的 follow loop 裡。
- 測資（[user/symlinkcycle.c](user/symlinkcycle.c)）要求：cycle 必須 `open(...)` 失敗回 `-1`，但 25-hop **非** cyclic chain 必須成功。
- 所以學生實作 ⑦⑧⑨ 的 follow loop 時，必須在每次 hop 前檢查「這個 inode number 是不是已經訪問過」，並把當前 hop 的 `ip->inum` 加進 `visited[]` 集合。25-hop 直鏈會經過 25 個**不同**的 inum，所以不會被誤殺；任何 cycle 在繞回時 inum 必然重複，被立刻擋下。

> **設計決策說明：visited-set vs depth-limit**
>
> 比較早期的版本（commit `66d68e8` 之前）是用「hop 上限 = 25」這種 depth limit 的做法，但這在 `public4` 的 25-hop 直鏈剛好踩到邊界。後來改成 visited-inode set（`adc45ae feat(symlink): cycle detection via visited inode set (replaces depth limit)`）：
>
> - **正確性**：visited-set 直接命中「cycle = 回到曾經訪問過的點」的數學定義，不會被「鏈長 = 上限 ± 1」這種臨界值問題搞死。
> - **空間**：`uint visited[100]` 只佔 stack 400 bytes，遠小於 `MAXPATH * 100`。
> - **時間**：每次 hop 做 O(n) 線性掃描，n ≤ 100，總成本 O(n²) ≈ 10000 比較，相對於一次 disk I/O 完全可以忽略。
>
> 同時保留 `n >= 100` 的硬上限，是為了在「使用者真的串了非常長的非 cyclic chain」時也能優雅 fail，避免 kernel stack 被打爆。

換句話說：**Cycle Prevent 沒有自己的空格，是因為它必須是 follow loop 的一部分；如果幫學生獨立挖一個，反而會誤導他們去寫多餘的 graph traversal。**

---

## 3. 對照表：9 個空格與課程目標

| 空格 # | 檔案:行 | 對應 Part | 對應 sub-task | 必要性 |
| :--: | :--- | :--: | :--- | :--- |
| ① | fs.h:27 | 1 | doubly-indirect 常數 | 改 layout 的入口 |
| ② | fs.h:42 | 1 | dinode addrs | on-disk 結構 |
| ③ | file.h:29 | 1 | inode addrs | in-memory 結構 |
| ④ | fs.c:411 | 1 | `bmap` 第三段 | 讀寫核心 |
| ⑤ | fs.c:459 | 1 | `itrunc` 第三段 | 防 block leak |
| ⑥ | sysfile.c:521 | 2 | `sys_symlink` body | syscall 本體 |
| ⑦ | sysfile.c:337 | 2 | `sys_open` follow | file symlink + 順便擋 cycle |
| ⑧ | fs.c:710 | 2 | `namex` follow | directory symlink + 順便擋 cycle |
| ⑨ | sysfile.c:428 | 2 | `sys_chdir` follow | directory symlink |

合計：Part 1 共 5 個空格（結構 3 + 邏輯 2），Part 2 共 4 個空格（syscall 1 + follow 3）。

---

## 4. 為什麼這樣切，不那樣切？

最後補充幾個 design decision，讓助教 / 同學知道哪些地方是「故意不挖」：

1. **`mkfs/mkfs.c` 不挖**：mkfs 會根據 `fs.h` 的 `NDIRECT/NINDIRECT` 自動算出 layout，學生只要把 `fs.h` 改對、`make clean` 重做 image 就會自動生效。挖 `mkfs.c` 反而會讓學生誤以為要動 host-side 工具。
2. **`syscall.c` / `syscall.h` / `usys.pl` / `user.h` 不挖**：這些都是「註冊 syscall」的 boilerplate。讓學生練習這個並沒有教學價值，反而是常見的「忘了註冊→ usertests 直接掛」失分點。template 先幫他們接好。
3. **`fcntl.h` 的 `O_NOFOLLOW = 0x004` 不挖**：這是 spec 級的常數，挖了會讓學生在 bit value 上各寫各的，造成 grading 環境不一致。
4. **`stat.h` 的 `T_SYMLINK = 4` 不挖**：同上，且 user 端的 test（`symlinkfile.c`）會直接用這個常數比對 `st.type`，挖了會破壞測資。
5. **沒有為 cycle 多挖一格**：見 §2.3，cycle 是 follow loop 的副產品而非獨立 algorithm。

---

## 5. TL;DR

整份 mp4 我挖了 **9 個 `To do` 空格**：
- **Part 1（large file，40%）佔 5 格**：3 格放在 `fs.h` / `file.h` 的結構定義（採 `NDIRECT=10` + 1 singly + 2 doubly 的 layout，`addrs[NDIRECT+3]`）、2 格放在 `fs.c` 的 `bmap` 與 `itrunc`，逼學生把「on-disk layout ↔ in-memory layout ↔ 存取邏輯 ↔ 釋放邏輯」四件事一起想。
- **Part 2（symlink + cycle prevent，60%）佔 4 格**：1 格在 `sys_symlink` 本體（`create(path, T_SYMLINK) + writei(target)`）、1 格在 `sys_open` 處理 file symlink、1 格在 `namex` 處理 directory 中間段、1 格在 `sys_chdir` 處理最後一段是 dir symlink 的情況；**cycle prevent 故意「不獨立挖格」**，要求學生在 follow loop 裡用 `uint visited[100]` 的 inode-set 自然解決，比 depth-limit 的做法更穩。

所有空格在 template 釋出時只放 `/* To do: <task> ... */` 多行區塊、body 一律留空，是為了「指位置、不洩漏實作」；本份 REPORT 把每一格對應的標準解答附在後面，是給助教 / 自己日後 regrade 時對照用，學生版的 template 並不會看到這些程式碼。
