import re
import json

# Read the contents of the file
with open("raw_block_list.txt", "r") as file:
    data = file.read()

# Regex pattern to extract block names
pattern = r"Block (\w+)"

# Extract block names
block_names = re.findall(pattern, data)

# Convert the block names to JSON
json_data = json.dumps(block_names, indent=4)

# Save the block names to a JSON file
with open("block_list.json", "w") as json_file:
    json_file.write(json_data)

# Print the JSON output
print(json_data)
