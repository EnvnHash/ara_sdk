import json
import uuid

def ensure_json_uuids(data, uuid_set):
    if isinstance(data, dict):
        # 1. Ensure the current dict has a valid, unique 'uuid'
        current_uuid = data.get('uuid')
        if not current_uuid or current_uuid in uuid_set:
            if current_uuid in uuid_set:
                print(f"Found duplicated uuid: {current_uuid}")
            new_uuid = str(uuid.uuid4())
            data['uuid'] = new_uuid
            uuid_set.add(new_uuid)
        else:
            uuid_set.add(current_uuid)

        # 2. Recursively check all nested values
        for value in list(data.values()):
            if isinstance(value, (dict, list)):
                ensure_json_uuids(value, uuid_set)

    elif isinstance(data, list):
        # Iterate through list elements directly (not enumerate tuples)
        for item in data:
            if isinstance(item, (dict, list)):
                ensure_json_uuids(item, uuid_set)

def load_json(file_path):
    with open(file_path, 'r', encoding='utf-8') as file:
        return json.load(file)

def save_json(data, file_path):
    with open(file_path, 'w', encoding='utf-8') as file:
        json.dump(data, file, indent=4)

def main(file_path):
    try:
        json_data = load_json(file_path)
        uuid_set = set()
        ensure_json_uuids(json_data, uuid_set)
        save_json(json_data, file_path)
        print("JSON UUIDs checked and updated successfully.")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 2:
        print("Usage: python check_json_uuids.py <path_to_json_file>")
    else:
        json_file_path = sys.argv[1]
        main(json_file_path)