#pragma once

#include <cstdio>

class CreatureEntity;
void write_alliance_impression_summary(CreatureEntity &creature, FILE *fff);
void do_cmd_knowledge_alliance(CreatureEntity &creature, bool detail);
