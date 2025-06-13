import json
import uuid

def print_json_keys_values(data, uuid_set):
    if isinstance(data, dict):
        for key, value in data.items():
            if key == 'uuid':
                if value in uuid_set:
                    print("Found duplicated uuid!")
                    data['uuid'] = str(uuid.uuid4())
                    uuid_set.add(data['uuid'])
                else :
                    uuid_set.add(value)

            if isinstance(value, (dict, list)):
                print_json_keys_values(value, uuid_set)

    elif isinstance(data, list):
        for i, item in enumerate(data):
            if isinstance(item, (dict, list)):
                print_json_keys_values(item, uuid_set)
def load_json(file_path):
    with open(file_path, 'r') as file:
        return json.load(file)

def save_json(data, file_path):
    with open(file_path, 'w') as file:
        json.dump(data, file, indent=4)

def main(file_path):
    try:
        json_data = load_json(file_path)
        uuid_set = set()
        print_json_keys_values(json_data, uuid_set)
        save_json(json_data, file_path)
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 2:
        print("Usage: python script.py <path_to_json_file>")
    else:
        json_file_path = sys.argv[1]
        main(json_file_path)
