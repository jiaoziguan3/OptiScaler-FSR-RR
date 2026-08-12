#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
OptiScaler Settings Editor

GUI for editing OptiScaler.ini.
Writes changes back by patching the original lines, so all of the
documentation comments in the INI are preserved.
"""

import os
import re
import sys
import shutil
import tkinter as tk
from tkinter import ttk, messagebox, filedialog
from pathlib import Path

from settings_spec import MODULES
from ini_handler import IniFile


# --------------------------------------------------------------------------- #
# UI strings
# --------------------------------------------------------------------------- #

UI = {
    'title':        {'zh': 'OptiScaler 设置', 'en': 'OptiScaler Settings'},
    'module':       {'zh': '模块：', 'en': 'Module:'},
    'ui_language':  {'zh': '界面语言：', 'en': 'Language:'},
    'save':         {'zh': '保存', 'en': 'Save'},
    'cancel':       {'zh': '取消', 'en': 'Cancel'},
    'reset_module': {'zh': '本模块全部恢复默认', 'en': 'Reset module to auto'},
    'auto':         {'zh': '默认', 'en': 'auto'},
    'help_title':   {'zh': '选项说明', 'en': 'Option Help'},
    'success':      {'zh': '成功', 'en': 'Success'},
    'error':        {'zh': '错误', 'en': 'Error'},
    'save_ok':      {'zh': '已保存到 {0}\n\n原文件已备份为 OptiScaler.ini.bak\n\n重启游戏后生效。',
                     'en': 'Saved to {0}\n\nOriginal backed up as OptiScaler.ini.bak\n\n'
                           'Restart the game to apply.'},
    'save_fail':    {'zh': '保存失败：{0}', 'en': 'Save failed: {0}'},
    'ini_not_found': {'zh': '未找到 OptiScaler.ini\n\n请选择 OptiScaler.ini 的位置。',
                      'en': 'OptiScaler.ini not found\n\nPlease locate OptiScaler.ini.'},
    'pick_ini':     {'zh': '选择 OptiScaler.ini', 'en': 'Select OptiScaler.ini'},
    'ini_path':     {'zh': '配置文件：', 'en': 'Config file:'},
    'unsaved':      {'zh': '有未保存的修改，确定要退出吗？',
                     'en': 'You have unsaved changes. Quit anyway?'},
    'confirm':      {'zh': '确认', 'en': 'Confirm'},
    'dirty_hint':   {'zh': '● 有未保存的修改', 'en': '● Unsaved changes'},
    'saved_hint':   {'zh': '所有修改已保存', 'en': 'All changes saved'},
    'bad_number':   {'zh': '「{0}」的值 "{1}" 不是合法数字，请修正后再保存。',
                     'en': '"{1}" is not a valid number for "{0}". Fix it before saving.'},
}


class Loc:
    def __init__(self, lang='zh'):
        self.lang = lang

    def __call__(self, key):
        return UI.get(key, {}).get(self.lang, key)

    def field(self, spec, idx):
        """idx 0 = label, 1 = help text"""
        return spec[3].get(self.lang, spec[3]['en'])[idx]

    def module(self, mod):
        return mod['name'].get(self.lang, mod['name']['en'])


# --------------------------------------------------------------------------- #
# INI read / patch-in-place
# --------------------------------------------------------------------------- #
# (now imported from ini_handler.py)


# --------------------------------------------------------------------------- #
# Slider + entry + auto checkbox
# --------------------------------------------------------------------------- #

class NumericField:
    """
    A numeric row: [auto checkbox] [-------slider-------] [entry]

    While 'auto' is checked the slider and entry are disabled and the stored
    value stays the literal string 'auto'. Unchecking it switches to a real
    number that the slider and entry keep in sync.
    """

    def __init__(self, parent, spec, loc, initial, on_change):
        self.spec = spec
        self.loc = loc
        self.on_change = on_change
        lo, hi, step = spec[2]
        self.lo, self.hi, self.step = lo, hi, step
        self.is_float = isinstance(step, float) or isinstance(lo, float) or isinstance(hi, float)

        self.frame = ttk.Frame(parent)

        parsed = self._parse(initial)
        self.auto_var = tk.BooleanVar(value=parsed is None)
        self.num_var = tk.DoubleVar(value=parsed if parsed is not None else self._fallback())
        self.text_var = tk.StringVar(value=self._fmt(self.num_var.get()))

        self.auto_chk = ttk.Checkbutton(self.frame, text=loc('auto'),
                                        variable=self.auto_var, command=self._toggle_auto)
        self.auto_chk.pack(side=tk.LEFT, padx=(0, 8))

        self.slider = ttk.Scale(self.frame, from_=lo, to=hi, orient=tk.HORIZONTAL,
                                variable=self.num_var, command=self._on_slide, length=210)
        self.slider.pack(side=tk.LEFT, fill=tk.X, expand=True)

        self.entry = ttk.Entry(self.frame, textvariable=self.text_var, width=9, justify=tk.RIGHT)
        self.entry.pack(side=tk.LEFT, padx=(8, 0))
        self.entry.bind('<Return>', self._on_type)
        self.entry.bind('<FocusOut>', self._on_type)

        self._apply_state()

    # -- helpers -------------------------------------------------------- #
    def _parse(self, raw):
        try:
            return float(str(raw).strip())
        except (TypeError, ValueError):
            return None

    def _fallback(self):
        mid = self.lo + (self.hi - self.lo) / 2.0
        return mid if self.is_float else round(mid)

    def _fmt(self, v):
        if self.is_float:
            decimals = max(0, len(str(self.step).split('.')[-1])) if '.' in str(self.step) else 2
            return f'{float(v):.{decimals}f}'
        return str(int(round(float(v))))

    def _snap(self, v):
        v = max(self.lo, min(self.hi, float(v)))
        if self.step:
            v = self.lo + round((v - self.lo) / self.step) * self.step
            v = max(self.lo, min(self.hi, v))
        return v

    def _apply_state(self):
        state = 'disabled' if self.auto_var.get() else 'normal'
        self.slider.state(['disabled'] if self.auto_var.get() else ['!disabled'])
        self.entry['state'] = state

    # -- events --------------------------------------------------------- #
    def _toggle_auto(self):
        self._apply_state()
        self.on_change()

    def _on_slide(self, _=None):
        if self.auto_var.get():
            return
        snapped = self._snap(self.num_var.get())
        self.text_var.set(self._fmt(snapped))
        self.on_change()

    def _on_type(self, _=None):
        if self.auto_var.get():
            return
        parsed = self._parse(self.text_var.get())
        if parsed is None:
            self.text_var.set(self._fmt(self.num_var.get()))
            return
        snapped = self._snap(parsed)
        self.num_var.set(snapped)
        self.text_var.set(self._fmt(snapped))
        self.on_change()

    # -- value ---------------------------------------------------------- #
    def get(self):
        if self.auto_var.get():
            return 'auto'
        parsed = self._parse(self.text_var.get())
        if parsed is None:
            raise ValueError(self.text_var.get())
        return self._fmt(self._snap(parsed))

    def retranslate(self):
        self.auto_chk['text'] = self.loc('auto')


# --------------------------------------------------------------------------- #
# Main window
# --------------------------------------------------------------------------- #

class SettingsApp:
    def __init__(self, root):
        self.root = root
        self.loc = Loc('zh')
        self.dirty = False
        self.widgets = {}          # ini_key -> control
        self.current_module = MODULES[0]

        self.ini_path = self._locate_ini()
        if self.ini_path is None:
            root.destroy()
            sys.exit(1)
        self.ini = IniFile(self.ini_path)

        self._build()
        self._render_module()

    # -- ini discovery -------------------------------------------------- #
    def _locate_ini(self):
        base = Path(sys.executable).parent if getattr(sys, 'frozen', False) \
            else Path(__file__).parent.absolute()
        candidate = base / 'OptiScaler.ini'
        if candidate.exists():
            return candidate

        messagebox.showwarning(self.loc('error'), self.loc('ini_not_found'))
        picked = filedialog.askopenfilename(
            title=self.loc('pick_ini'),
            filetypes=[('OptiScaler.ini', 'OptiScaler.ini'), ('INI files', '*.ini'),
                       ('All files', '*.*')])
        return Path(picked) if picked else None

    # -- layout --------------------------------------------------------- #
    def _build(self):
        self.root.title(self.loc('title'))
        self.root.geometry('860x720')
        self.root.minsize(720, 560)
        try:
            self.root.option_add('*Font', '{Microsoft YaHei UI} 9')
        except Exception:
            pass

        # --- top bar: module selector --------------------------------- #
        top = ttk.Frame(self.root, padding=(14, 12, 14, 8))
        top.pack(fill=tk.X)

        self.module_label = ttk.Label(top, text=self.loc('module'))
        self.module_label.pack(side=tk.LEFT, padx=(0, 6))

        self.module_var = tk.StringVar()
        self.module_combo = ttk.Combobox(top, textvariable=self.module_var,
                                         state='readonly', width=34)
        self.module_combo.pack(side=tk.LEFT)
        self.module_combo.bind('<<ComboboxSelected>>', self._on_module_change)

        self.reset_btn = ttk.Button(top, text=self.loc('reset_module'),
                                    command=self._reset_module)
        self.reset_btn.pack(side=tk.RIGHT)

        self.path_label = ttk.Label(self.root, foreground='#707070',
                                    text=f"{self.loc('ini_path')}{self.ini_path}")
        self.path_label.pack(fill=tk.X, padx=14)

        ttk.Separator(self.root, orient=tk.HORIZONTAL).pack(fill=tk.X, padx=14, pady=(8, 0))

        # --- scrollable body ------------------------------------------ #
        body = ttk.Frame(self.root)
        body.pack(fill=tk.BOTH, expand=True, padx=14, pady=8)

        self.canvas = tk.Canvas(body, highlightthickness=0, borderwidth=0)
        vbar = ttk.Scrollbar(body, orient=tk.VERTICAL, command=self.canvas.yview)
        self.canvas.configure(yscrollcommand=vbar.set)
        vbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        self.inner = ttk.Frame(self.canvas)
        self.inner_id = self.canvas.create_window((0, 0), window=self.inner, anchor='nw')
        self.inner.bind('<Configure>',
                        lambda e: self.canvas.configure(scrollregion=self.canvas.bbox('all')))
        self.canvas.bind('<Configure>',
                         lambda e: self.canvas.itemconfigure(self.inner_id, width=e.width))
        self.canvas.bind_all('<MouseWheel>', self._on_wheel)

        # --- bottom bar ----------------------------------------------- #
        ttk.Separator(self.root, orient=tk.HORIZONTAL).pack(fill=tk.X, padx=14)
        bottom = ttk.Frame(self.root, padding=(14, 10))
        bottom.pack(fill=tk.X)

        self.lang_label = ttk.Label(bottom, text=self.loc('ui_language'))
        self.lang_label.pack(side=tk.LEFT, padx=(0, 6))
        self.lang_var = tk.StringVar(value='简体中文')
        lang_combo = ttk.Combobox(bottom, textvariable=self.lang_var,
                                  values=['简体中文', 'English'], state='readonly', width=12)
        lang_combo.pack(side=tk.LEFT)
        lang_combo.bind('<<ComboboxSelected>>', self._on_lang_change)

        self.cancel_btn = ttk.Button(bottom, text=self.loc('cancel'), command=self._on_close)
        self.cancel_btn.pack(side=tk.RIGHT, padx=(6, 0))
        self.save_btn = ttk.Button(bottom, text=self.loc('save'), command=self._save)
        self.save_btn.pack(side=tk.RIGHT)

        self.status = ttk.Label(bottom, text=self.loc('saved_hint'), foreground='#707070')
        self.status.pack(side=tk.RIGHT, padx=(0, 16))

        self._refresh_module_list()
        self.root.protocol('WM_DELETE_WINDOW', self._on_close)

    def _on_wheel(self, event):
        self.canvas.yview_scroll(int(-1 * (event.delta / 120)), 'units')

    def _refresh_module_list(self):
        names = [self.loc.module(m) for m in MODULES]
        self.module_combo['values'] = names
        self.module_var.set(self.loc.module(self.current_module))

    # -- rendering ------------------------------------------------------ #
    def _render_module(self):
        for child in self.inner.winfo_children():
            child.destroy()
        self.widgets.clear()

        mod = self.current_module
        section = mod['section']

        header = ttk.Label(self.inner, text=self.loc.module(mod),
                           font=('Microsoft YaHei UI', 12, 'bold'))
        header.grid(row=0, column=0, columnspan=3, sticky=tk.W, pady=(0, 10))

        self.inner.columnconfigure(1, weight=1)

        for i, spec in enumerate(mod['fields'], start=1):
            key, kind = spec[0], spec[1]
            raw = self.ini.get(section, key, 'auto')
            label_text = self.loc.field(spec, 0)

            ttk.Label(self.inner, text=label_text).grid(
                row=i, column=0, sticky=tk.W, pady=6, padx=(0, 10))

            if kind == 'combo':
                var = tk.StringVar(value=raw if raw in spec[2] else spec[2][0])
                combo = ttk.Combobox(self.inner, textvariable=var, values=spec[2],
                                     state='readonly', width=22)
                combo.grid(row=i, column=1, sticky=tk.W, pady=6)
                combo.bind('<<ComboboxSelected>>', lambda e: self._mark_dirty())
                self.widgets[key] = ('combo', var)

            elif kind == 'slider':
                field = NumericField(self.inner, spec, self.loc, raw, self._mark_dirty)
                field.frame.grid(row=i, column=1, sticky=(tk.W, tk.E), pady=6)
                self.widgets[key] = ('slider', field)

            else:  # entry
                var = tk.StringVar(value=raw)
                entry = ttk.Entry(self.inner, textvariable=var, width=32)
                entry.grid(row=i, column=1, sticky=tk.W, pady=6)
                var.trace_add('write', lambda *a: self._mark_dirty())
                self.widgets[key] = ('entry', var)

            btn = ttk.Button(self.inner, text='?', width=3,
                             command=lambda s=spec: self._show_help(s))
            btn.grid(row=i, column=2, sticky=tk.W, pady=6, padx=(8, 0))

        self.canvas.yview_moveto(0)

    def _show_help(self, spec):
        messagebox.showinfo(f"{self.loc('help_title')} - {self.loc.field(spec, 0)}",
                            f"[{spec[0]}]\n\n{self.loc.field(spec, 1)}")

    # -- state ---------------------------------------------------------- #
    def _mark_dirty(self, *_):
        if not self.dirty:
            self.dirty = True
        self.status['text'] = self.loc('dirty_hint')
        self.status['foreground'] = '#b06000'

    def _collect_current(self):
        """Push the visible controls into self.ini. Raises ValueError on bad input."""
        section = self.current_module['section']
        for key, (kind, ctrl) in self.widgets.items():
            if kind == 'slider':
                try:
                    value = ctrl.get()
                except ValueError as bad:
                    label = next(self.loc.field(s, 0)
                                 for s in self.current_module['fields'] if s[0] == key)
                    raise ValueError(self.loc('bad_number').format(label, bad.args[0]))
            else:
                value = ctrl.get().strip() or 'auto'
            self.ini.set(section, key, value)

    def _on_module_change(self, _=None):
        try:
            self._collect_current()
        except ValueError as err:
            messagebox.showerror(self.loc('error'), str(err))
            self.module_var.set(self.loc.module(self.current_module))
            return
        idx = self.module_combo.current()
        if 0 <= idx < len(MODULES):
            self.current_module = MODULES[idx]
            self._render_module()

    def _reset_module(self):
        for kind, ctrl in self.widgets.values():
            if kind == 'slider':
                ctrl.auto_var.set(True)
                ctrl._apply_state()
            else:
                ctrl.set('auto')
        self._mark_dirty()

    def _on_lang_change(self, _=None):
        try:
            self._collect_current()
        except ValueError:
            pass  # language switch shouldn't be blocked by a bad field
        self.loc.lang = 'zh' if self.lang_var.get() == '简体中文' else 'en'
        self.root.title(self.loc('title'))
        self.module_label['text'] = self.loc('module')
        self.lang_label['text'] = self.loc('ui_language')
        self.save_btn['text'] = self.loc('save')
        self.cancel_btn['text'] = self.loc('cancel')
        self.reset_btn['text'] = self.loc('reset_module')
        self.path_label['text'] = f"{self.loc('ini_path')}{self.ini_path}"
        self.status['text'] = self.loc('dirty_hint') if self.dirty else self.loc('saved_hint')
        self._refresh_module_list()
        self._render_module()

    # -- save / quit ---------------------------------------------------- #
    def _save(self):
        try:
            self._collect_current()
        except ValueError as err:
            messagebox.showerror(self.loc('error'), str(err))
            return
        try:
            self.ini.save()
        except Exception as err:
            messagebox.showerror(self.loc('error'), self.loc('save_fail').format(err))
            return
        self.dirty = False
        self.status['text'] = self.loc('saved_hint')
        self.status['foreground'] = '#707070'
        messagebox.showinfo(self.loc('success'),
                            self.loc('save_ok').format(self.ini_path.name))

    def _on_close(self):
        if self.dirty and not messagebox.askyesno(self.loc('confirm'), self.loc('unsaved')):
            return
        self.root.destroy()


def main():
    root = tk.Tk()
    SettingsApp(root)
    root.mainloop()


if __name__ == '__main__':
    main()
