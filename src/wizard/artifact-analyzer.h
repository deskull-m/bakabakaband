#pragma once

class ItemEntity;
class ArtifactsDumpInfo;
class CreatureEntity;
ArtifactsDumpInfo object_analyze(CreatureEntity &creature, const ItemEntity &item);
ArtifactsDumpInfo random_artifact_analyze(CreatureEntity &creature, const ItemEntity &item);
