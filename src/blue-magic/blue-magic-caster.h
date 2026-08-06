#pragma once
/*!
 * @file blue-magic-caster.h
 * @brief 青魔法のその他系統の呪文定義と詠唱時分岐処理ヘッダ
 */

enum class MonsterAbilityType;

class CreatureEntity;
bool cast_learned_spell(CreatureEntity &creature, MonsterAbilityType spell, const bool success);
bool cast_blue_dispel(CreatureEntity &creature);
