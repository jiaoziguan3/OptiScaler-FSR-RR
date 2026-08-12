import importlib.util
import re
import shutil
import tempfile
import unittest
from pathlib import Path


INSTALLER_DIR = Path(__file__).parent
ROOT_DIR = INSTALLER_DIR.parent
INI_PATH = ROOT_DIR / 'OptiScaler.ini'
REQUIRED_FILES = (
    'optiscaler_settings.py',
    'settings_spec.py',
    'ini_handler.py',
    'OptiScalerSettings.spec',
)


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def ini_fields(path):
    fields = {}
    section = None
    for line in path.read_text(encoding='utf-8', errors='replace').splitlines():
        section_match = re.match(r'^\s*\[([^]]+)\]\s*$', line)
        if section_match:
            section = section_match.group(1)
            fields.setdefault(section, [])
            continue
        key_match = re.match(r'^\s*([A-Za-z0-9_-]+)\s*=', line)
        if section and key_match:
            fields[section].append(key_match.group(1))
    return fields


class SettingsPortTests(unittest.TestCase):
    def test_only_four_required_editor_files_are_ported(self):
        for name in REQUIRED_FILES:
            self.assertTrue((INSTALLER_DIR / name).is_file(), name)
        self.assertFalse((INSTALLER_DIR / 'optiscaler_installer.py').exists())
        self.assertFalse((INSTALLER_DIR / 'OptiScalerInstaller.spec').exists())

    def test_settings_spec_matches_target_ini(self):
        settings_spec = load_module('settings_spec', INSTALLER_DIR / 'settings_spec.py')
        actual = ini_fields(INI_PATH)
        declared = {module['section']: [field[0] for field in module['fields']]
                    for module in settings_spec.MODULES}
        self.assertEqual(actual, declared)

    def test_ini_round_trip_preserves_unknown_content(self):
        ini_handler = load_module('ini_handler', INSTALLER_DIR / 'ini_handler.py')
        temp_dir = Path(tempfile.mkdtemp(prefix='optiscaler_settings_', dir=INSTALLER_DIR))
        self.addCleanup(shutil.rmtree, temp_dir, True)
        test_ini = temp_dir / 'OptiScaler.ini'
        original = '; header\n[Menu]\nScale = auto\nUnknownTargetKey=keep\n\n[Custom]\nValue=untouched\n'
        test_ini.write_text(original, encoding='utf-8')
        ini = ini_handler.IniFile(test_ini)
        ini.set('Menu', 'Scale', '1.30')
        ini.save(backup=False)
        saved = test_ini.read_text(encoding='utf-8')
        reparsed = ini_handler.IniFile(test_ini)
        self.assertIn('; header', saved)
        self.assertIn('UnknownTargetKey=keep', saved)
        self.assertIn('[Custom]\nValue=untouched', saved)
        self.assertEqual('1.30', reparsed.get('Menu', 'Scale'))


if __name__ == '__main__':
    unittest.main()
