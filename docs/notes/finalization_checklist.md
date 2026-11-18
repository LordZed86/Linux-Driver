# Finalization Checklist

Things to finish before the project is submitted and before the repo is made public (for example, to list on a resume). Check items off as they're completed.

---

## 1. Privacy and repo cleanup (do this before making the repo public)

Making a repo public publishes its **entire history**, not just the current files. Clean first, change visibility last.

### Check the history

- [ ] List every author name and email in the history and confirm none is a real personal email:

  ```bash
  git log --all --format='%an <%ae>' | sort -u
  ```

- [ ] Search the full history for my username, hostname, and secrets (replace the words with the real values):

  ```bash
  git log --all -p | grep -niE 'yourusername|yourhostname|password|token|BEGIN .*PRIVATE'
  ```

- [ ] Search the full history for IP addresses:

  ```bash
  git log --all -p | grep -nE '\b[0-9]{1,3}(\.[0-9]{1,3}){3}\b'
  ```

### Fix what the checks find

- [ ] Set the GitHub noreply email for new commits (GitHub Settings > Emails, copy the noreply address):

  ```bash
  git config --global user.email "12345+username@users.noreply.github.com"
  ```

- [ ] On GitHub, tick **"Keep my email addresses private"** and **"Block command line pushes that expose my email"**
- [ ] If old commits show a real email: back up the repo folder, then rewrite history with `git filter-repo --mailmap mailmap.txt` and force push (or start a fresh repo if the history can't be scrubbed)
- [ ] If a password, token, or private key was ever committed, **revoke or rotate it**; rewriting history alone isn't enough

### Check current files

- [ ] Scan the working tree for usernames, hostnames, IPs, and secrets:

  ```bash
  grep -rniE 'yourusername|yourhostname' . --exclude-dir=.git
  grep -rnE '\b[0-9]{1,3}(\.[0-9]{1,3}){3}\b' . --exclude-dir=.git
  grep -rniE 'password|token|BEGIN .*PRIVATE' . --exclude-dir=.git
  ```

- [ ] Review every screenshot in `docs/screenshots/` for shell prompts (`user@hostname`), IP addresses, and file paths; crop or redact
- [ ] Review everything in `docs/evidence/` (especially `dmesg` output, which can include hardware identifiers); trim to the lines that matter
- [ ] Confirm `user-data` (VM password) and anything from `~/.ssh/` are **not** in the repo
- [ ] Confirm `.gitignore` also covers `user-data`, `*.pem`, `*.key`, `.env`, and any `private/` folder
- [ ] Run `git diff --staged` and read it before each commit

### Make it public

- [ ] Only after everything above is clean: GitHub repo Settings > Danger Zone > Change visibility
- [ ] Open the public repo in a private browser window and check what a visitor sees

---

## 2. Fill in placeholders

- [ ] `README.md`: `[LICENSE]` (badge, section, and file), Ubuntu version, `[output of uname -r]`
- [ ] Add a `LICENSE` file to the repo root; for a kernel module, a GPL-compatible license matches `MODULE_LICENSE("GPL")` in the source
- [ ] `README.md` and `setup_guide.md`: replace any remaining `[...]` and `<...>` placeholders that should have real values (keep the masked ones: `<host-user>`, `<host-name>`, `<mac-ip>`, `<vm-ip>`)
- [ ] `module/mychardev.c`: update `MODULE_DESCRIPTION` and `MODULE_VERSION` to match the finished module
- [ ] Remove or resolve every `TODO` comment in the source files

---

## 3. Documentation accuracy

- [ ] Fix the date on the dev log entries written on Oct 12 (they were drafted as 2026-10-08)
- [ ] Update the setup guide status note once the VM steps have all been confirmed, and correct any step that differed in practice (including the cloud image filename)
- [ ] Update the README **Status** checklist to match what actually works
- [ ] Fill in the README **Testing** results table with real pass/fail results
- [ ] Add real screenshots to the README **Screenshots** section
- [ ] Fix the Documentation table links (proposal, midterm report, evidence, logs) so none are broken
- [ ] Add `docs/setup_guide.md` and this checklist to the Documentation table
- [ ] Re-run the README build and run instructions from scratch on a clean VM to confirm they work as written
- [ ] Update the README's CS 370 concepts table with the second-half topics applied to the driver

---

## 4. Honesty and attribution

- [ ] Make the README **GenAI Attribution** section match how AI was actually used, and follow the course AI policy
- [ ] Keep `docs/notes/ai_use_log.md` up to date, with a row for each significant use
- [ ] Be able to explain every line of the module and test program in the demo

---

## 5. Course deliverables

- [ ] **Midterm (Oct 21):** report PDF with the nine required sections, 3-5 pages, plus technical evidence and screenshots
- [ ] **Midterm:** source code and `README.md` with compile and run instructions submitted on Canvas
- [ ] **Midterm:** in-class demo prepared and rehearsed
- [ ] Stretch features (`ioctl`, open counter) only if the core is finished (the mutex is now core scope)

### Final project (guidelines PDF: `private/final_project_guidelines.pdf`, not in Git)

Guidelines: submission due **Tue Dec 1**, Zoom presentations Dec 1 / 3 / 8. My slot: **Mon Nov 30** (confirm). Target: everything done by **Wed Nov 25**, so Thanksgiving is buffer.

Submit **exactly two files**: one ZIP (source + supporting files) and one PDF (the updated midterm report). Do not upload the slides.

**Phase 1: make it work (Nov 18–19, in the VM)**

- [x] VM has 2+ vCPUs; take a VM snapshot first
- [x] Commit, then run `./scripts/collect_stage4.sh`; every mode prints PASS (race with `use_lock=0` is *expected* to FAIL)
- [x] Run `./scripts/collect_lldb.sh` (registers, stack frame, user vs. kernel addresses)
- [x] Read every file in `docs/evidence/stage4/` for usernames, hostnames, and IPs; then commit
- [ ] Screenshots into `docs/screenshots/stage4/`: test_driver rw, big, fault, race lock on vs. off, dmesg, lldb registers
- [x] Fix anything that fails, re-run, log it in the dev log (it's material for "Challenges and Solutions")

**Phase 2: docs (Nov 20–21)**

- [x] README: Status checklist, Testing table, status badge
- [ ] README: stage 4 screenshot in the Screenshots section
- [x] Dev log entry for the test run
- [ ] Bump `MODULE_VERSION` to `1.0` once tests pass

**Phase 3: report PDF (Nov 20–23)**: keep all midterm content, add final work, cover all nine sections

- [ ] 1 Project introduction (title, problem, objectives, scope; note the Nov 3 mutex scope change)
- [ ] 2 Requirements: functional (open/read/write/release, 1024 B buffer, ENOSPC, EFAULT, mutex) and non-functional (no kernel crash, VM-only loading, kernel 7.0, C99 test program)
- [ ] 3 System design: architecture diagram (user program → syscall → VFS → file_operations → kbuf + mutex), memory design (user vs. kernel address space, 1024 B buffer, `kbuf_len`)
- [ ] 4 Implementation: module walkthrough, test program modes, module parameters, concepts table
- [ ] 5 Project output: stage 3 + stage 4 screenshots with captions
- [ ] 6 Testing and results: full test table with real results, race lock on/off numbers, strace excerpt
- [ ] 7 Challenges and solutions: midterm list + stage 4 (errno bug, race test design, mutex vs. spinlock, anything from the VM run)
- [ ] 8 Conclusion and future scope: ioctl, open counter, per-open buffers, kernel debugging in QEMU
- [ ] 9 References: LKMPG, kernel docs (`copy_to_user`, `mutex`, `cdev`), man pages, course materials
- [ ] Export as one PDF (`docs/reports/final_report.pdf`; the packaging script leaves it out of the ZIP)

**Phase 4: slides (Nov 22–24)**: exactly 8 content slides plus a title slide and a thank-you slide

- [ ] 1 Intro, problem, objectives, solution · 2 Midterm progress and planned final scope
- [ ] 3 Final overview and features · 4 Architecture and implementation · 5 How the major functions work
- [ ] 6 Output screenshots and results · 7 Testing, challenges, solutions · 8 Conclusion and future scope
- [ ] Decide where the live demo goes (after slide 5 or as part of slide 6)

**Phase 5: package and rehearse (Nov 24–29)**

- [ ] `make clean`, commit, then `./scripts/package_submission.sh`; unzip it somewhere and confirm the README instructions build it
- [ ] Two full timed run-throughs (12–15 min) over Zoom screen share, using `docs/presentations/final/demo.sh`
- [ ] Screen recording of a clean demo run as a backup
- [ ] Night before: VM boots, module builds, `sudo -v` works, terminal font large, `export PS1='$ '`
- [ ] Upload ZIP + PDF to Canvas (by Dec 1, ideally the same day as the presentation)

---

## 6. Last look

- [ ] `make clean` leaves no build files in the repo, and `git status` is clean
- [ ] A fresh clone builds and runs following only the README
- [ ] The repo's first screen (title, one-paragraph summary, status) reads well to someone who has never seen it