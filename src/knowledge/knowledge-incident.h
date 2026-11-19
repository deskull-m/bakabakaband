#pragma once

#include <cstdio>

class CreatureEntity;
void write_incident_report(CreatureEntity &creature, FILE *fff);
void do_cmd_knowledge_incident(CreatureEntity &creature);
