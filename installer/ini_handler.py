#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Minimal INI handler that preserves all comments, blank lines and formatting."""
import os
import re
import shutil
from pathlib import Path


class IniFile:
    """
    Minimal INI handler that keeps the original text intact.

    Values are read into `self.values[(section, key)]`.
    On save, only the matching `key=value` lines are rewritten; every comment,
    blank line and ordering detail from the source file survives untouched.
    """

    SECTION_RE = re.compile(r'^\s*\[([^\]]+)\]\s*$')
    KV_RE = re.compile(r'^(\s*)([A-Za-z0-9_\-]+)(\s*=\s*)(.*?)(\s*)$')

    def __init__(self, path):
        self.path = Path(path)
        self.lines = self.path.read_text(encoding='utf-8', errors='replace').splitlines()
        self.values = {}
        # (section, key) -> line index
        self._index = {}

        section = None
        for i, line in enumerate(self.lines):
            m = self.SECTION_RE.match(line)
            if m:
                section = m.group(1)
                continue
            if section is None:
                continue
            stripped = line.lstrip()
            if stripped.startswith(';') or stripped.startswith('#') or not stripped:
                continue
            kv = self.KV_RE.match(line)
            if kv:
                key = kv.group(2)
                self.values[(section, key)] = kv.group(4)
                self._index[(section, key)] = i

    def get(self, section, key, default='auto'):
        return self.values.get((section, key), default)

    def set(self, section, key, value):
        self.values[(section, key)] = value

    def sections(self):
        """All section names that actually contain at least one key."""
        seen = []
        for sec, _ in self.values.keys():
            if sec not in seen:
                seen.append(sec)
        return seen

    def keys_of(self, section):
        """Keys of one section, in file order."""
        pairs = [(self._index[(s, k)], k)
                 for (s, k) in self.values.keys() if s == section]
        return [k for _, k in sorted(pairs)]

    def save(self, backup=True):
        if backup:
            bak = self.path.with_suffix(self.path.suffix + '.bak')
            shutil.copy2(self.path, bak)

        out = list(self.lines)
        appended = {}

        for (section, key), value in self.values.items():
            idx = self._index.get((section, key))
            if idx is not None:
                kv = self.KV_RE.match(out[idx])
                if kv:
                    out[idx] = f'{kv.group(1)}{kv.group(2)}{kv.group(3)}{value}'
                else:
                    out[idx] = f'{key}={value}'
            else:
                appended.setdefault(section, []).append(f'{key}={value}')

        # Keys that were not present in the original file get appended to the
        # end of their section (or a new section at EOF).
        for section, entries in appended.items():
            insert_at = None
            in_section = False
            for i, line in enumerate(out):
                m = self.SECTION_RE.match(line)
                if m:
                    if in_section:
                        insert_at = i
                        break
                    in_section = (m.group(1) == section)
            if in_section and insert_at is None:
                insert_at = len(out)
            if insert_at is None:
                out.extend(['', f'[{section}]'] + entries)
            else:
                out[insert_at:insert_at] = entries

        tmp = self.path.with_suffix(self.path.suffix + '.tmp')
        tmp.write_text('\n'.join(out) + '\n', encoding='utf-8')
        os.replace(tmp, self.path)
