#!/usr/bin/env python3
import json
import os
import sys
import shutil
from pathlib import Path

def merge_extensions(source_file, target_file):
    """Merge extensions.json files"""
    try:
        with open(source_file, 'r', encoding='utf-8') as f:
            source = json.load(f)
        
        if os.path.exists(target_file):
            with open(target_file, 'r', encoding='utf-8') as f:
                target = json.load(f)
        else:
            target = {}
        
        if 'recommendations' not in target:
            target['recommendations'] = []
        if 'unwantedRecommendations' not in target:
            target['unwantedRecommendations'] = []
        
        for rec in source.get('recommendations', []):
            if rec not in target['recommendations']:
                target['recommendations'].append(rec)
        
        for unwanted in source.get('unwantedRecommendations', []):
            if unwanted not in target['unwantedRecommendations']:
                target['unwantedRecommendations'].append(unwanted)
        
        with open(target_file, 'w', encoding='utf-8') as f:
            json.dump(target, f, indent=4, ensure_ascii=False)
            f.write('\n')
        
        return True
    except Exception as e:
        print(f'error merging extensions.json: {e}', file=sys.stderr)
        return False

def merge_settings(source_file, target_file):
    """Merge settings.json files"""
    try:
        with open(source_file, 'r', encoding='utf-8') as f:
            source = json.load(f)
        
        if os.path.exists(target_file):
            with open(target_file, 'r', encoding='utf-8') as f:
                target = json.load(f)
        else:
            target = {}
        
        for key, value in source.items():
            target[key] = value
        
        with open(target_file, 'w', encoding='utf-8') as f:
            json.dump(target, f, indent=4, ensure_ascii=False)
            f.write('\n')
        
        return True
    except Exception as e:
        print(f'error merging settings.json: {e}', file=sys.stderr)
        return False

def main():
    if len(sys.argv) != 2:
        print('usage: setup_clangd_config.py <workspace_path>')
        sys.exit(1)
    
    workspace = Path(sys.argv[1])
    source_dir = workspace / 'doc' / 'vscode_settings'
    vscode_dir = workspace / '.vscode'
    
    success = True
    
    clangd_source = source_dir / 'clangd'
    clangd_target = workspace / '.clangd'
    if clangd_source.exists():
        try:
            shutil.copy2(clangd_source, clangd_target)
            print('[OK] .clangd configuration copied')
        except Exception as e:
            print(f'[WARNING] failed to copy .clangd: {e}')
            success = False
    else:
        print('[WARNING] source file not found: doc/vscode_settings/clangd')
        success = False
    
    vscode_dir.mkdir(exist_ok=True)
    
    extensions_source = source_dir / 'extensions.json'
    extensions_target = vscode_dir / 'extensions.json'
    if extensions_source.exists():
        if extensions_target.exists():
            print('[INFO] merging extensions.json...')
            if merge_extensions(extensions_source, extensions_target):
                print('[OK] extensions.json merged')
            else:
                print('[WARNING] failed to merge extensions.json, using direct copy')
                try:
                    shutil.copy2(extensions_source, extensions_target)
                except Exception as e:
                    print(f'[WARNING] failed to copy extensions.json: {e}')
                    success = False
        else:
            try:
                shutil.copy2(extensions_source, extensions_target)
                print('[OK] extensions.json copied to .vscode')
            except Exception as e:
                print(f'[WARNING] failed to copy extensions.json: {e}')
                success = False
    else:
        print('[WARNING] source file not found: doc/vscode_settings/extensions.json')
        success = False
    
    settings_source = source_dir / 'settings.json'
    settings_target = vscode_dir / 'settings.json'
    if settings_source.exists():
        if settings_target.exists():
            print('[INFO] merging settings.json...')
            if merge_settings(settings_source, settings_target):
                print('[OK] settings.json merged')
            else:
                print('[WARNING] failed to merge settings.json, using direct copy')
                try:
                    shutil.copy2(settings_source, settings_target)
                except Exception as e:
                    print(f'[WARNING] failed to copy settings.json: {e}')
                    success = False
        else:
            try:
                shutil.copy2(settings_source, settings_target)
                print('[OK] settings.json copied to .vscode')
            except Exception as e:
                print(f'[WARNING] failed to copy settings.json: {e}')
                success = False
    else:
        print('[WARNING] source file not found: doc/vscode_settings/settings.json')
        success = False
    
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()

