import re
import json

#
# Raw block list is gotten from: https://piston-data.mojang.com/v1/objects/0530a206839eb1e9b35ec86acbbe394b07a2d9fb/client.txt
# Search for net.minecraft.world.level.block.Blocks class and paste into raw_block_list.txt
#

# Read the contents of the file
with open("raw_block_list.txt", "r") as file:
    data = file.read()

# Regex pattern to extract block names
pattern = r"Block (\w+)"

# Extract block names
block_names_unprocessed = re.findall(pattern, data)

# Make lower case and prepend
block_names = ["minecraft:" + s.lower() for s in block_names_unprocessed]

with open('block_list.txt', 'w') as f:
    for name in block_names:
        f.write(f"{name}\n")