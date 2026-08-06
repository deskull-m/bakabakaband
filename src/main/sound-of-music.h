#pragma once
/*!
 * @file sound-of-music.h
 * @brief BGM及び効果音のterm出力処理ヘッダ
 */

#include <vector>

extern bool has_monster_music;

enum class SoundKind;
class CreatureEntity;
void bell();
void sound(SoundKind sk);
void play_music(int type, int num);
void select_floor_music(CreatureEntity &creature);
void select_monster_music(CreatureEntity &creature, const std::vector<short> &monster_list);
