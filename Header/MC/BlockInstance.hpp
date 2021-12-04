#pragma once
#include "../Global.h"
class BlockSource;
class Block;
class ItemStack;

class BlockInstance
{
    Block* block;
    BlockPos pos;
    int dim;

public:
    BlockInstance(Block* block, BlockPos pos, int dimid)
        :block(block), pos(pos), dim(dimid)
    { }
    LIAPI bool operator==(BlockInstance const& bli);
    LIAPI Block* getBlock();

    LIAPI bool breakNaturally();
    LIAPI bool breakNaturally(ItemStack* tool);
    LIAPI ItemStack& getBlockDrops();
    LIAPI bool isNull();

    LIAPI const static BlockInstance Null;
};