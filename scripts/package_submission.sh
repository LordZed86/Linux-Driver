#!/usr/bin/env bash
# Build the source-code ZIP for the final submission (file 1 of 2; the report PDF is file 2).
# Run from anywhere in the repo, on any machine with git:
#   ./scripts/package_submission.sh
#
# Packages the last COMMIT (git archive), so build output, .DS_Store, and anything
# in .gitignore stay out. Commit your changes first.
# Leaves out the report PDF (submitted separately) and the report paste guide.
# private/ (course files, raw screenshots) is gitignored, so it is never included.

set -eu
cd "$(dirname "$0")/.."

if [ -n "$(git status --porcelain)" ]; then
  echo "WARNING: uncommitted changes are NOT included. Commit first if they matter:"
  git status --short
  echo
fi

NAME=CS370_Final_Linux_Driver
OUT=${1:-$(cd .. && pwd)/$NAME.zip}

git archive --format=zip --prefix="$NAME/" -o "$OUT" HEAD -- . \
  ':(exclude)docs/reports/final_report.pdf' \
  ':(exclude)docs/reports/final_report_paste_guide.md'

echo "Wrote $OUT"
unzip -Z1 "$OUT" | grep -v '/$'
