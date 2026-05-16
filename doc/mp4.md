<div align="center">
  <h1>💻 Machine Problem 4 - File System</h1>
  <h3>CSIE3310 - Operating Systems</h3>
  <h4>National Taiwan University</h4>
</div>

<hr />

<div align="center">
  <table>
    <tr>
      <td><strong>Total Points:</strong></td>
      <td>100</td>
      <td><strong>Release Date:</strong></td>
      <td>May 19</td>
    </tr>
    <tr>
      <td><strong>Due Date:</strong></td>
      <td>June 2, 23:59:00 (UTC+8)</td>
      <td><strong>TA Hours:</strong></td>
      <td>TBD</td>
    </tr>
  </table>
</div>

<hr />

## 📋 Table of Contents

- [📋 Table of Contents](#-table-of-contents)
- [💬 Discussion Policy](#-discussion-policy)
- [📝 Summary](#-summary)
- [🛠️ Environment Setup](#️-environment-setup)
- [💻 Part 1: Large Files (40 points)](#-part-1-large-files-40-points)
  - [Description](#description)
  - [Implementation Details](#implementation-details)
  - [Guidelines and Hints](#guidelines-and-hints)
- [💻 Part 2: Symbolic Link & Cycle Prevention (60 points)](#-part-2-symbolic-link--cycle-prevention-60-points)
  - [Description](#description-1)
  - [Implementation Details](#implementation-details-1)
  - [Guidelines and Hints](#guidelines-and-hints-1)
- [🧪 Run Public Test Cases](#-run-public-test-cases)
- [📤 Submission](#-submission)
- [📊 Grading Policy](#-grading-policy)

---

## 💬 Discussion Policy

If you have any questions about this machine problem, please post them on the corresponding NTU COOL discussion board. For special requests, you can email [ntuos@googlegroups.com](mailto:ntuos@googlegroups.com).

---

## 📝 Summary

In this MP, you will learn the fundamental knowledge of the file system by adding two features to xv6: **large files** and **symbolic links** (including cycle prevention). We strongly recommend you read xv6 book while you trace the code. This gives you a quick overview of how xv6 implements its file system.

The core data structures (`struct dinode` and `struct inode`) have been pre-modified for you in the template. Your task is to implement the logic to handle these new structures and features.

---

## 🛠️ Environment Setup

Before starting this machine problem, ensure you have completed the initial environment setup.

1. **Initialize the environment**:
   ```bash
   ./mp.sh init
   ```

2. **Launch xv6**:
   ```bash
   ./mp.sh qemu
   ```

3. **Cleanup**:
   ```bash
   ./mp.sh clean
   ```

Refer to the [Handbook](./doc/handbook.md) for more details on the `mp.sh` toolkit.

---

## 💻 Part 1: Large Files (40 points)

### Description

In this problem, you have to increase the maximum size of an xv6 file. Currently, xv6 files are limited to 268 blocks (12 direct + 1 singly-indirect). Your task is to implement **doubly-indirect** blocks to support much larger files (up to 131,338 blocks).

The template uses a layout of **10 direct blocks, 1 singly-indirect block, and 2 doubly-indirect blocks**.

### Implementation Details

You need to modify the following functions in `kernel/fs.c`:

1. **`bmap()`**:
   - Locate the logic for the "To do: bigfile" block (around line 411).
   - Implement the mapping for logical block numbers falling into the two doubly-indirect slots (`addrs[11]` and `addrs[12]`).
   - You must allocate indirect and data blocks on demand using `balloc()`.
   - Use `bread()`, `log_write()`, and `brelse()` correctly to navigate the levels of indirection.

2. **`itrunc()`**:
   - Locate the logic for the "To do: bigfile" block (around line 459).
   - Implement the logic to free all data blocks and indirect blocks associated with the two doubly-indirect slots.
   - Failure to free all blocks will cause a disk space leak, which may cause subsequent tests to fail.

### Test Specifications

| Test Case | Type | Description | Points |
| :--- | :--- | :--- | :--- |
| `public testcase 1` | Public | Small file write/read test (270 blocks). | 15 |
| `public testcase 2` | Public | Larger file write/read test (6666 blocks). | 15 |
| `private testcases` | Private | Stress tests to verify doubly-indirect logic and disk consistency. | 10 |


### Guidelines and Hints

- The layout is pre-defined in `kernel/fs.h`: `10 direct + 1 singly + 2 doubly`.
- `MAXFILE` is calculated as `10 + 256 + 2 * 256 * 256 = 131,338` blocks.
- Do not use recursion when traversing blocks to avoid kernel stack overflow.
- Always `brelse()` a buffer once you are done with it to avoid `bget: no buffers` panic.

---

## 💻 Part 2: Symbolic Link & Cycle Prevention (60 points)

### Description

In this problem, you will add symbolic links to xv6. A symbolic link is a special type of file that contains a path to another file or directory. When the system encounters a symbolic link during path resolution, it should follow the link to the target.

You must also implement **cycle prevention**. If a chain of symbolic links forms a loop or becomes excessively long, the system must return an error.

### Implementation Details

You need to modify the following locations:

1. **`sys_symlink()`** in `kernel/sysfile.c`:
   - Implement the body of the system call.
   - Create a new inode of type `T_SYMLINK`.
   - Write the target path string into the inode's data blocks using `writei()`.

2. **`sys_open()`** in `kernel/sysfile.c`:
   - Add logic to follow symbolic links when a file is opened.
   - If `O_NOFOLLOW` is specified in the flags, the symbolic link itself should be opened instead of following it.
   - Cycle detection: maintain a per-call set of inode numbers visited during this follow chain. Before following another symlink, check whether its inode is already in the set — if so, return -1. After following, add the inode to the set. A pure depth limit is not sufficient: it cannot distinguish a 25-hop non-cyclic chain (which must succeed) from a short cycle. The 100-hop cap is a safety net for pathological long chains, not a substitute for the visited-set check.

3. **`namex()`** in `kernel/fs.c`:
   - Add logic to follow directory symbolic links when they appear as intermediate components of a path (e.g., in `/a/link_to_dir/b`, `link_to_dir` must be followed).

4. **`sys_chdir()`** in `kernel/sysfile.c`:
   - Add logic to follow the symbolic link if the target of `chdir` is a link pointing to a directory.

### Test Specifications

#### Symbolic Links to Files & Directories
| Test Case | Type | Description | Points |
| :--- | :--- | :--- | :--- |
| `symlinkfile: public 1-2`| Public | Basic resolution and chained file symlinks. | 10 |
| `symlinkdir: public 1-2` | Public | Resolution via intermediate directory symlinks. | 10 |
| `private testcases` | Private | Advanced resolution scenarios . | 20 |

#### Cycle Detection
| Test Case | Type | Description | Points |
| :--- | :--- | :--- | :--- |
| `symlinkcycle: public 1-2`| Public | Detection of self-loops and 2-cycles. | 8 |
| `symlinkcycle: public 5` | Public | Detection of cycles in the middle of a chain. | 4 |
| `private testcases` | Private | Complex cycle detection and `O_NOFOLLOW` verification. | 8 |


### Guidelines and Hints

- The target of a symbolic link does not need to exist when the link is created.
- Symbolic links to directories should behave just like the directories they point to.

---

## 🧪 Run Public Test Cases

To run all automated public tests and see your score:

```bash
./mp.sh grade
```

The automated grader will run the following scripts:
- `mp4_bigfile_public.py`
- `mp4_symlinkfile_public.py`
- `mp4_symlinkdir_public.py`
- `mp4_symlinkcycle_public.py`

---

## 📤 Submission

1. Run `./mp.sh clean` before pushing. Do not commit build artifacts such as `.o`, `.d`, or `.asm` files.

2. **Push your changes**:
   Commit and push your modified `kernel/fs.c` and `kernel/sysfile.c` to your repository.

> [!IMPORTANT]
> The English letters in `<student_id>` must be **lowercase**. E.g., use `r13944062` instead of `R13944062`.
---

## 📊 Grading Policy

- **Part 1: Large Files (40%)**
  - `bigfile: public testcases` — 30%
  - `bigfile: private testcases` — 10%

| Sub-task | Points |
| :--- | ---: |
| public 1: Large files 270 blocks | 15 |
| public 2: Large files 6666 blocks | 15 |
| private 1| 10 |

- **Part 2: Symbolic Link & Cycle Prevention (60%)**
  - `symlinkfile & symlinkdir: public testcases` — 20%
  - `symlinkfile & symlinkdir: private testcases` — 20%
  - `symlinkcycle: public testcases` — 12%
  - `symlinkcycle: private testcases` — 8%

| Sub-task | Points |
| :--- | ---: |
| public 1: Symlink for file testcase 1 | 10 |
| public 2: Symlink for file testcase 2 | 10 |
| public 3: Symlink for directory testcase 1 | 10 |
| public 4: Symlink for directory testcase 2 | 10 |
| public 5: cycle prevent 1 | 2 |
| public 6: cycle prevent 2 | 2 |
| public 7: cycle prevent 3 | 2 |
| public 8: cycle prevent 4 | 3 |
| public 9: cycle prevent 5 | 3 |
| private 1 : cycle prevent 6 | 4 |
| private 2 : cycle prevent 7 | 4 |

**Penalties:**
- **Compilation Error**: 0 points.
- **Late Submission**: Score = $\max(\text{raw score} - 20 \times \lceil n \rceil,\ 0)$, where $n$ is the number of days late. No points are awarded if $n \geq 5$.
- Only the **last submission** will be graded.
