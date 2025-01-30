## TODO

- [ ] Move individual section serializer code to NBTParser.h
- [ ] Make serialized sections use a global palette. (rather than palette index)
- [ ] Create code which serializes all sections (entire chunk)
- [ ] Write script which serializes entire world
- [ ] Make chunk serialization parallel with cuda


## Notes
Raw block list is gotten from: https://piston-data.mojang.com/v1/objects/0530a206839eb1e9b35ec86acbbe394b07a2d9fb/client.txt
Search for net.minecraft.world.level.block.Blocks class

