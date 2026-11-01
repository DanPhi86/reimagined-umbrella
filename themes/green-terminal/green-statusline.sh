#!/usr/bin/env bash
# Grüne Terminal-Statuszeile für Claude Code.
# Claude Code übergibt Sitzungsdaten als JSON auf stdin.
input=$(cat)
model=$(printf '%s' "$input" | sed -n 's/.*"display_name"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' | head -1)
dir=$(printf '%s' "$input" | sed -n 's/.*"current_dir"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' | head -1)
branch=$(git -C "${dir:-.}" branch --show-current 2>/dev/null)
G=$'\e[38;2;51;255;51m'; D=$'\e[38;2;26;92;26m'; R=$'\e[0m'
printf '%s>%s %s%s%s %s::%s %s%s%s' "$D" "$R" "$G" "${dir##*/}" "$R" "$D" "$R" "$G" "${model:-claude}" "$R"
[ -n "$branch" ] && printf ' %s[%s]%s' "$D" "$branch" "$R"
printf ' %s_%s\n' "$G" "$R"
