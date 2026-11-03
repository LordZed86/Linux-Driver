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
- [ ] **Final (Dec 3):** final report and demo integrating the second-half course concepts
- [ ] **Final:** source code zip with a working README, submitted on Canvas
- [ ] Stretch features (`ioctl`, open counter) only if the core is finished (the mutex is now core scope)

---

## 6. Last look

- [ ] `make clean` leaves no build files in the repo, and `git status` is clean
- [ ] A fresh clone builds and runs following only the README
- [ ] The repo's first screen (title, one-paragraph summary, status) reads well to someone who has never seen it