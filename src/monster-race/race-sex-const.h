#pragma once

class MonraceDefinition;

enum class MonsterSex {
    NONE = 0,
    MALE = 1,
    FEMALE = 2,
    MAX = 3,
};

bool is_male(const MonsterSex sex);
bool is_male(const MonraceDefinition &monrace);
bool is_female(const MonsterSex sex);
bool is_female(const MonraceDefinition &monrace);
