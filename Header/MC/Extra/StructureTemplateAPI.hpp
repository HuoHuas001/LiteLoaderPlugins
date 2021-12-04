//Extra Part For StructureTemplate.hpp
#ifdef EXTRA_INCLUDE_PART_STRUCTURETEMPLATE
// Include Headers or Declare Types Here
#include "../StructureSettings.hpp"
class CompoundTag;
class StructureTemplateData;

#else
// Add Member There
char filler[216]; // IDA StructureTemplate::StructureTemplate

public:
LIAPI StructureTemplate(std::string const& name);
LIAPI static StructureTemplate fromTag(std::string name, CompoundTag* tag);
LIAPI static StructureTemplate fromWorld(std::string name, int dimid, BlockPos p1, BlockPos p2, bool ignoreEntities=false, bool ignoreBlocks=false);
LIAPI std::unique_ptr<CompoundTag> toTag();
LIAPI bool toWorld(int dimid, BlockPos p1, Mirror mirror, Rotation rotation);
LIAPI StructureTemplateData* getData();

#endif
