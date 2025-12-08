#!/usr/bin/env python3
import sys
import os
import re
import subprocess
from pathlib import Path

if len(sys.argv) < 4:
    print(f"usage: {sys.argv[0]} [COMMIT1/BRANCH1] [COMMIT2/BRANCH2] [out-dir]")
    sys.exit(1)

COMMIT1 = sys.argv[1]
COMMIT2 = sys.argv[2]
OUTDIR = Path(sys.argv[3])
OUTDIR.mkdir(parents=True, exist_ok=True)
OUTCSV = OUTDIR / f"{Path(COMMIT2).name}.csv"

def run_git(*args, cwd=None):
    p = subprocess.run(("git",) + args, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if p.returncode != 0:
        return ""
    return p.stdout

def git_changed_files(commit):
    out = run_git("diff", "--name-only", f"{commit}~..{commit}")
    return out.splitlines()

def git_is_merge_commit(commit):
    out = run_git("rev-list", "--parents", "-n", "1", commit).strip()
    return len(out.split()) == 3

def git_show_field(commit, fmt, date_short=False):
    args = ["show", "-s", f"--pretty=format:{fmt}", commit]
    if date_short:
        args.insert(1, "--date=short")
    return run_git(*args).strip()

def git_show_stat_last_line(commit):
    out = run_git("show", "--stat", commit)
    if not out:
        return ""
    lines = [l for l in out.splitlines() if l.strip()]
    return lines[-1] if lines else ""

PATTERNS = [
    (r"mysqld_safe\.sh", "mysqld_safe"),
    (r"mysql-test-run\.pl", "mysql-test-run"),
    (r"\.gitignore", "gitignore"),
    (r"\.pem", "cert"),
    (r"\.cmake", "cmake"),
    (r"CMakeLists\.txt", "cmakelists"),
    (r"innobase", "innodb"),
    (r"ndb", "ndb"),
    (r"valgrind", "valgrind"),
    (r"jemalloc", "jemalloc"),
    (r"clang", "clang"),
    (r"client/", "client"),
    (r"components/", "components"),
    (r"Docs/", "doc"),
    (r"include/", "include"),
    (r"libmysql/", "libmysql"),
    (r"man/", "man"),
    (r"mysys/", "mysys"),
    (r"mysql-test/suite/clone/", "clone"),
    (r"mysql-test/suite/privacy/", "privacy"),
    (r"mysql-test/suite/thread_pool/", "thread_pool"),
    (r"percona-xtradb-cluster-tests", "pxc-tests"),
    (r"packaging/", "packaging"),
    (r"plugin/", "plugin"),
    (r"plugin/clone", "plugin_clone"),
    (r"plugin/group_replication", "group_replication"),
    (r"policy/", "policy"),
    (r"router/", "router"),
    (r"scripts/", "scripts"),
    (r"sql/", "optimizer"),
    (r"sql_optimizer\.cc", "query optimizer"),
    (r"sql_executor\.cc", "query executor"),
    (r"sql_lex\.cc", "parser"),
    (r"sql_yacc\.yy", "parser"),
    (r"binlog", "binlog"),
    (r"rpl_", "replication"),
    (r"sql-common/", "sql-common"),
    (r"storage/archive ", "archive"),
    (r"storage/blackhole", "blackhole"),
    (r"storage/federated", "federated"),
    (r"storage/heap", "heap"),
    (r"storage/myisam", "myisam"),
    (r"storage/perfschema", "perfschema"),
    (r"storage/temptable", "temptable"),
    (r"strings/", "strings"),
    (r"support-files/", "support-files"),
    (r"mysql-test/", "mtr"),
    (r"vio/", "vio"),
    (r"unittest/", "unittest"),
    (r"extra/", "third-party-libraries"),
]

HEADER = "Date;Author;Commit;Commit title;Components;Modifications;Investigation Owner;Was it in upstream 5.7?;Fix Owner;Additional Info;Status"

with OUTCSV.open("w", encoding="utf-8") as f:
    f.write(HEADER + "\n")

rev_list_args = ["rev-list", "--topo-order", "--reverse", COMMIT2, f"^{COMMIT1}"]
commits_out = run_git(*rev_list_args)
commits = [c for c in commits_out.splitlines() if c.strip()]

for commit in commits:
    tags = []

    changed = git_changed_files(commit)

    for pattern, tag in PATTERNS:
        try:
            prog = re.compile(pattern)
        except re.error:
            prog = re.compile(re.escape(pattern))
        matched = any(prog.search(fn) for fn in changed)
        if matched:
            tags.append(tag)

    if git_is_merge_commit(commit):
        tags.append("merge_commit")

    if len(tags) == 1 and tags[0] in ("ndb", "doc", "third-party-libraries"):
        tags.append("to be omitted")
    tags_str = ", ".join(tags) if tags else ""


    date_author = git_show_field(commit, "%cd;%an", date_short=True)
    commit_short = git_show_field(commit, "%h")
    link = f"https://github.com/mysql/mysql-server/commit/{commit_short}"

    title = git_show_field(commit, "%s").replace(";", "")
    statline = git_show_stat_last_line(commit)

    # parse stats: "<N> files changed, X insertions(+), Y deletions(-)"
    files_changed = 0
    insertions = 0
    deletions = 0
    if statline:
        m_files = re.search(r"(\d+)\s+files?\s+changed", statline)
        if m_files:
            files_changed = int(m_files.group(1))
        m_ins = re.search(r"(\d+)\s+insertions?\(\+\)", statline)
        if m_ins:
            insertions = int(m_ins.group(1))
        m_del = re.search(r"(\d+)\s+deletions?\(-\)", statline)
        if m_del:
            deletions = int(m_del.group(1))
        # Sometimes git shows "X insertions(+), Y deletions(-)" without "files changed"
        if files_changed == 0:
            m_files2 = re.search(r"(\d+)\s+file\s+changed", statline)
            if m_files2:
                files_changed = int(m_files2.group(1))

    modifications = f"{files_changed} files {insertions}+ {deletions}-"

    # Build CSV line (semicolon-separated)
    hyperlink = f'=HYPERLINK("{link}","{commit_short}")'
    line = f"{date_author};{hyperlink};{title};{tags_str};{modifications};\n"

    # append to overall CSV
    with OUTCSV.open("a", encoding="utf-8") as f:
        f.write(line)

    # component-wise CSV files
    if tags:
        for t in tags:
            t_clean = t.strip()
            if not t_clean:
                continue
            tagfile = OUTDIR / f"{t_clean}.csv"
            # ensure tag file has header if newly created
            if not tagfile.exists():
                with tagfile.open("w", encoding="utf-8") as tf:
                    tf.write(HEADER + "\n")
            with tagfile.open("a", encoding="utf-8") as tf:
                tf.write(line)
                
