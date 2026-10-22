#!/usr/bin/env python3
"""Every .bot command in the dispatch table must appear in the command docs.

Reads the botCommands table from commands/BotCommands.cpp and requires each
primary command name to show up as `.bot <name>` in
docs/play/commands/README.md. Aliases are not required to be documented.
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'commands' / 'BotCommands.cpp'
DOC = ROOT / 'docs' / 'play' / 'commands' / 'README.md'


def table_names():
    text = SOURCE.read_text()
    start = text.index('static BotCommandEntry const botCommands[] = {')
    end = text.index('};', start)
    return re.findall(r'^\s*\{"([a-z]+)",', text[start:end], re.M)


def main():
    names = table_names()
    if not names:
        print('verify_command_docs: no command names found in the botCommands table')
        return 1
    doc = DOC.read_text()
    missing = [name for name in names
               if not re.search(r'\.bot ' + re.escape(name) + r'\b', doc)]
    if missing:
        print('verify_command_docs: commands missing from docs/play/commands/README.md:')
        for name in missing:
            print(f'  .bot {name}')
        return 1
    print(f'verify_command_docs: all {len(names)} .bot commands documented.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
