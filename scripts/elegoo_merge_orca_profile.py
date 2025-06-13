import json

with open('../resources/profiles/OrcaFilamentLibrary.json', 'r', encoding='utf-8') as f:
    data = json.load(f)

def should_keep(item):
    if item['sub_path'].endswith('@System.json'):
        return item['name'].startswith('Generic')
    return True

data['filament_list'] = [item for item in data['filament_list'] if should_keep(item)]

with open('../resources/profiles/OrcaFilamentLibrary.json', 'w', encoding='utf-8') as f:
    json.dump(data, f, ensure_ascii=False, indent=4)