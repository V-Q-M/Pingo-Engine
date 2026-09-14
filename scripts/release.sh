#!/usr/bin/env bash
#
# Creates a version commit on the production branch.
#
# production contains exactly one commit per version. Each of these commits
# holds the complete state of the source branch at the time of the release,
# all commits since the previous version squashed into one. The detailed
# history stays on dev and main.
#
# The commit is built directly from the tree of the source branch, so
# production never needs merges and never runs into conflicts. Nothing is
# checked out, the working directory stays untouched.
#
# production is published to its own public remote, also called production,
# as its main branch. The first version has no parent, so the private history
# of dev and main never reaches the public repository.
#
# Usage:
#   scripts/release.sh "Alpha 0.0.2" [source-branch]
#
# source-branch defaults to main. The version has to match the one in
# src/Engine/Version.h on that branch, so the splash screen shows it too.
#
# Repository-only changes like .gitignore, README or documentation do not
# need a new version: commit them on dev/main as usual and copy them onto
# production with git cherry-pick.

set -euo pipefail

version="${1:-}"
source_branch="${2:-main}"

if [[ -z "$version" ]]; then
    echo "Usage: scripts/release.sh \"Alpha 0.0.2\" [source-branch]" >&2
    exit 1
fi

cd "$(git rev-parse --show-toplevel)"

if ! git rev-parse --verify --quiet "$source_branch^{commit}" > /dev/null; then
    echo "Branch \"$source_branch\" does not exist." >&2
    exit 1
fi

if [[ "$(git branch --show-current)" == "production" ]]; then
    echo "Please switch away from production first, e.g. git switch dev." >&2
    exit 1
fi

# The engine shows the version on the splash screen, both have to agree
if ! git show "$source_branch:src/Engine/Version.h" 2> /dev/null | grep -qF "\"$version\""; then
    echo "src/Engine/Version.h on $source_branch does not contain \"$version\"." >&2
    echo "Bump PINGO_ENGINE_VERSION and commit it first." >&2
    exit 1
fi

tree="$(git rev-parse "$source_branch^{tree}")"
source_commit="$(git rev-parse --short "$source_branch")"
message="$version"$'\n\n'"Squashed from $source_branch at $source_commit."

if git rev-parse --verify --quiet "refs/heads/production" > /dev/null; then
    if [[ "$(git rev-parse "production^{tree}")" == "$tree" ]]; then
        echo "production already has exactly the state of $source_branch, nothing to release." >&2
        exit 1
    fi

    commit="$(git commit-tree "$tree" -p production -m "$message")"
else
    # The first version starts the branch without a parent
    commit="$(git commit-tree "$tree" -m "$message")"
fi

git branch -f production "$commit"

echo "production is now at $(git rev-parse --short production): $version"
echo "Publish it with: git push production production:main"
