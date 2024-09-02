import json
import sys

input_file = sys.argv[1]
output_file = sys.argv[2]

def parse_perf_script(input_file):
    events = []
    with open(input_file, 'r') as file:
        for line in file:
            if not line.strip() or line.startswith("#"):
                continue

            parts = line.split()
            event = {
                'time': parts[0],
                'event': parts[1],
                'details': ' '.join(parts[2:])
            }
            events.append(event)

    return events

parsed_data = parse_perf_script(input_file)

with open(output_file, 'w') as json_file:
    json.dump(parsed_data, json_file, indent=4)
