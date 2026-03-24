#include "object-use/read/read-executor-factory.h"
#include "artifact/fixed-art-types.h"
#include "object-use/read/gbh-shirt-read-executor.h"
#include "object-use/read/parchment-read-executor.h"
#include "object-use/read/ring-power-read-executor.h"
#include "object-use/read/scroll-read-executor.h"
#include "object/tval-types.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"

std::unique_ptr<ReadExecutorBase> ReadExecutorFactory::create(CreatureEntity &creature, ItemEntity *o_ptr, bool known)
{
    const auto tval = o_ptr->bi_key.tval();
    if (tval == ItemKindType::SCROLL) {
        return std::make_unique<ScrollReadExecutor>(creature, o_ptr, known);
    }

    if (o_ptr->is_specific_artifact(FixedArtifactId::GHB)) {
        return std::make_unique<GbhShirtReadExecutor>();
    }

    if (o_ptr->is_specific_artifact(FixedArtifactId::POWER)) {
        return std::make_unique<RingOfPowerReadExecutor>();
    }

    if (tval == ItemKindType::READING_MATTER) {
        return std::make_unique<ParchmentReadExecutor>(creature, o_ptr);
    }

    THROW_EXCEPTION(std::logic_error, "Invalid item is specified; this can't be read!");
}
