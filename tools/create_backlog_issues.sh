#!/usr/bin/env bash
# Creates one GitHub issue per user story in docs/BACKLOG.md, in this repository's GitHub remote.
# Requires the GitHub CLI, logged in as the backlog owner. Pass --dry-run to print the issues
# without contacting GitHub.
set -euo pipefail

OWNER_LOGIN="YMYanakiev24"
ROLE_LABELS=("Backend:backend" "Frontend:frontend" "QA:qa" "Scrum Trainer:scrum-trainer")

cd "$(dirname "$0")/.."
workDir="$(mktemp -d)"
trap 'rm -rf "$workDir"' EXIT
mkdir "$workDir/stories"

# Writes one file per story: the issue title on the first line, the issue body after it.
splitBacklog() {
    awk -v dir="$workDir/stories" '
        function finish() { if (file != "") close(file); file = "" }
        { sub(/\r$/, "") }
        /^## Epic/ { finish(); epic = substr($0, 4); next }
        /^\*\*US-/ { finish(); file = sprintf("%s/%02d", dir, ++count); title = ""; inTitle = 1 }
        file == "" { next }
        inTitle {
            title = title (title == "" ? "" : " ") $0
            if (title ~ /\*\([0-9]+ pts?\)\*/) {
                points = title
                sub(/^.*\*\(/, "", points)
                sub(/ pts?\).*$/, "", points)
                sub(/ *\*\(.*$/, "", title)
                sub(/^\*\*/, "", title)
                sub(/\*\* /, ": ", title)
                print title > file
                print "**" epic "** | Story points: " points "\n" > file
                inTitle = 0
            }
            next
        }
        /^- \[/ { print > file; next }
        { finish() }
    ' docs/BACKLOG.md
}

# Prints the comma-separated role labels for the tasks in story file $1.
labelsFor() {
    local labels="" entry
    for entry in "${ROLE_LABELS[@]}"; do
        if grep -qF "[${entry%%:*}]" "$1"; then
            labels="${labels:+$labels,}${entry##*:}"
        fi
    done
    echo "$labels"
}

requireOwnerLogin() {
    local login
    login="$(gh api user --jq .login)"
    if [ "${login,,}" != "${OWNER_LOGIN,,}" ]; then
        echo "Logged in to GitHub as $login, expected $OWNER_LOGIN. Run: gh auth login" >&2
        exit 1
    fi
}

createLabels() {
    local entry
    for entry in "${ROLE_LABELS[@]}"; do
        gh label create "${entry##*:}" --description "${entry%%:*} task" --force
    done
}

splitBacklog

if [ "${1:-}" != "--dry-run" ]; then
    requireOwnerLogin
    createLabels
fi

for story in "$workDir"/stories/*; do
    title="$(head -n 1 "$story")"
    labels="$(labelsFor "$story")"
    tail -n +2 "$story" > "$workDir/body"

    if [ "${1:-}" = "--dry-run" ]; then
        printf '=== %s\n[labels: %s]\n' "$title" "$labels"
        cat "$workDir/body"
        continue
    fi

    # Every story in the backlog is already delivered, so its issue is closed as completed.
    url="$(gh issue create --title "$title" --body-file "$workDir/body" --label "$labels")"
    gh issue close "$url" --reason completed
    echo "$url  $title"
done
