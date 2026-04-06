/*!
 * @file sound-of-music.cpp
 * @brief BGM及び効果音のterm出力処理実装
 */

#include "main/sound-of-music.h"
#include "game-option/disturbance-options.h"
#include "game-option/special-options.h"
#include "main/scene-table.h"
#include "system/creature-entity.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/enum-converter.h"

// モンスターBGMの設定有無。設定なし時に関連処理をスキップする。
bool has_monster_music = false;

/*
 * Flush the screen, make a noise
 */
void bell(void)
{
    term_fresh();
    if (ring_bell) {
        term_xtra(TERM_XTRA_NOISE, 0);
    }

    flush();
}

/*!
 * @brief 音を鳴らす
 * @todo intをsound_typeに差し替える
 */
void sound(SoundKind sk)
{
    if (!use_sound) {
        return;
    }

    term_xtra(TERM_XTRA_SOUND, enum2i(sk));
}

/*!
 * @brief Hack -- Play a music
 */
void play_music(int type, int val)
{
    if (!use_music) {
        return;
    }

    interrupt_scene(type, val);
    term_xtra(type, val);
}

/*!
 * @brief シチュエーションに合ったBGM選曲
 * @param creature クリーチャーへの参照
 * @details 設定がない場合はミュートする。
 */
void select_floor_music(CreatureEntity &creature)
{
    if (!use_music) {
        return;
    }

    refresh_scene_table(creature);
    term_xtra(TERM_XTRA_SCENE, 0);
}

/*!
 * @brief モンスターBGM選曲
 * @param creature クリーチャーへの参照
 * @param monster_list モンスターリスト
 */
void select_monster_music(CreatureEntity &creature, const std::vector<short> &monster_list)
{
    if (!use_music) {
        return;
    }

    refresh_scene_table(creature, monster_list);
    term_xtra(TERM_XTRA_SCENE, 0);
}
