#!/usr/bin/env python3
"""
tools/post_discord_changelog.py

Posts newly generated release notes / recent merged PR changelog
to a Discord webhook thread.

Usage:
  export DISCORD_WEBHOOK_URL="https://discord.com/api/webhooks/..."
  export THREAD_ID="1542647578548772954"
  python3 tools/post_discord_changelog.py --notes /tmp/discord_notes.md --title "TortoiseBots Update" --tag "v2026-09-16"
"""

import argparse
import json
import os
import sys
import urllib.request
import urllib.error


def split_text(text, max_len=3900):
    """Split markdown text cleanly along paragraph or line boundaries."""
    if len(text) <= max_len:
        return [text]

    chunks = []
    lines = text.split("\n")
    current_chunk = []
    current_len = 0

    for line in lines:
        line_len = len(line) + 1
        if current_len + line_len > max_len and current_chunk:
            chunks.append("\n".join(current_chunk))
            current_chunk = [line]
            current_len = line_len
        else:
            current_chunk.append(line)
            current_len += line_len

    if current_chunk:
        chunks.append("\n".join(current_chunk))

    return chunks


def send_discord_message(url, payload):
    data = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(
        url,
        data=data,
        headers={
            "Content-Type": "application/json",
            "User-Agent": "TortoiseBots-Changelog-Notifier/1.0"
        },
        method="POST"
    )
    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            return resp.status in (200, 204), resp.status
    except urllib.error.HTTPError as e:
        body = e.read().decode("utf-8", errors="replace")
        print(f"Discord API error HTTP {e.code}: {body}", file=sys.stderr)
        return False, e.code
    except Exception as e:
        print(f"Network error sending to Discord: {e}", file=sys.stderr)
        return False, 0


def main():
    parser = argparse.ArgumentParser(description="Post changelog or release announcement to Discord webhook thread.")
    parser.add_argument("--notes", default="", help="Path to markdown release notes file (optional if --short).")
    parser.add_argument("--title", default="TortoiseBots Update", help="Title for the announcement.")
    parser.add_argument("--tag", default="", help="Release tag or version identifier.")
    parser.add_argument("--repo", default="Sagiroth/TortoiseBots", help="GitHub repository name.")
    parser.add_argument("--thread-id", default="", help="Target Discord thread ID.")
    parser.add_argument("--username", default="TortoiseBots Releases", help="Webhook bot display name.")
    parser.add_argument("--avatar-url", default="", help="Optional bot avatar image URL.")
    parser.add_argument("--short", action="store_true", help="Post a concise release announcement with link instead of full changelog dump.")
    args = parser.parse_args()

    webhook_url = os.environ.get("DISCORD_WEBHOOK_URL", "").strip()
    thread_id = (args.thread_id or os.environ.get("THREAD_ID", "")).strip()

    if not webhook_url:
        print("Warning: DISCORD_WEBHOOK_URL is not set. Skipping Discord notification.")
        sys.exit(0)

    # Append thread_id if provided
    if thread_id:
        sep = "&" if "?" in webhook_url else "?"
        endpoint = f"{webhook_url}{sep}thread_id={thread_id}"
    else:
        endpoint = webhook_url

    release_url = f"https://github.com/{args.repo}/releases/tag/{args.tag}" if args.tag else f"https://github.com/{args.repo}"

    notes_text = ""
    if args.notes and os.path.exists(args.notes):
        with open(args.notes, "r", encoding="utf-8") as f:
            notes_text = f.read().strip()

    if args.short:
        embed_title = f"{args.title}"
        if args.tag and args.tag not in args.title and args.tag.lstrip("v") not in args.title:
            embed_title += f" • {args.tag}"

        # Release thread: version, download and the full changelog only. The
        # list of changes itself goes to the changelog thread.
        changelog_url = f"https://github.com/{args.repo}/blob/main/CHANGELOG.md"
        desc_parts = [
            f"A new release of TortoiseBots is out: **{args.tag or args.title}**.",
            "",
            f"**Download:** {release_url}",
            f"**Full changelog:** {changelog_url}",
        ]
        embed_desc = "\n".join(desc_parts)

        embed = {
            "title": embed_title,
            "url": release_url,
            "description": embed_desc,
            "color": 0x2ECC71,  # Emerald green
            "footer": {
                "text": f"TortoiseBots 1.18.1 • {args.repo}"
            }
        }

        payload = {
            "username": args.username,
            "embeds": [embed]
        }
        if args.avatar_url:
            payload["avatar_url"] = args.avatar_url

        success, code = send_discord_message(endpoint, payload)
        if not success and thread_id and code in (400, 404):
            print(f"Warning: Discord thread {thread_id} failed with HTTP {code}. Retrying directly to channel webhook...", file=sys.stderr)
            success, _ = send_discord_message(webhook_url, payload)
        if not success:
            sys.exit(1)

        print(f"Successfully posted short release announcement to Discord thread {thread_id or 'channel'}.")
        return

    # Full changelog mode
    if not args.notes or not os.path.exists(args.notes):
        print(f"Error: Notes file not found at {args.notes}", file=sys.stderr)
        sys.exit(1)

    if not notes_text:
        print("Notes file is empty. Nothing to post to Discord.")
        sys.exit(0)

    chunks = split_text(notes_text, max_len=3900)
    total_parts = len(chunks)

    for idx, chunk in enumerate(chunks, start=1):
        embed_title = f"🐢 {args.title}"
        if args.tag and args.tag not in args.title and args.tag.lstrip("v") not in args.title:
            embed_title += f" • {args.tag}"
        if total_parts > 1:
            embed_title += f" (Part {idx}/{total_parts})"

        embed = {
            "title": embed_title,
            "url": release_url,
            "description": chunk,
            "color": 0x2ECC71,  # Emerald green
            "footer": {
                "text": f"TortoiseBots 1.18.1 • {args.repo}"
            }
        }

        payload = {
            "username": args.username,
            "embeds": [embed]
        }
        if args.avatar_url:
            payload["avatar_url"] = args.avatar_url

        success, code = send_discord_message(endpoint, payload)
        if not success and thread_id and code in (400, 404):
            print(f"Warning: Discord thread {thread_id} failed with HTTP {code}. Retrying directly to channel webhook...", file=sys.stderr)
            success, _ = send_discord_message(webhook_url, payload)
            if success:
                endpoint = webhook_url  # Use fallback for any subsequent chunks
        if not success:
            sys.exit(1)

    print(f"Successfully posted changelog ({total_parts} message(s)) to Discord thread {thread_id or 'channel'}.")


if __name__ == "__main__":
    main()
