/*!
 * @brief 突然変異の一覧を出力する
 * @date 2020/04/24
 * @author Hourier
 */

#include "io/mutations-dump.h"
#include "mutation/mutation-flag-types.h"
#include "player/player-status-flags.h"
#include "system/creature-entity.h"
#include <fmt/format.h>

/*!
 * @brief ファイルポインタを通じて突然変異の一覧を出力する
 * @param out_file 出力先ファイルポインタ
 */
void dump_mutations(CreatureEntity &creature, FILE *out_file)
{
    if (!out_file) {
        return;
    }

    if (creature.get_mutations().any() || has_good_luck(creature) || has_pervert_attraction(creature)) {
        if (creature.get_mutations().has(PlayerMutationType::SPIT_ACID)) {
            fmt::println(out_file, _(" あなたは酸を吹きかけることができる。(ダメージ レベルX1)", " You can spit acid (dam lvl)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::BR_FIRE)) {
            fmt::println(out_file, _(" あなたは炎のブレスを吐くことができる。(ダメージ レベルX2)", " You can breathe fire (dam lvl * 2)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::HYPN_GAZE)) {
            fmt::println(out_file, _(" あなたの睨みは催眠効果をもつ。", " Your gaze is hypnotic."));
        }

        if (creature.get_mutations().has(PlayerMutationType::TELEKINES)) {
            fmt::println(out_file, _(" あなたは念動力をもっている。", " You are telekinetic."));
        }

        if (creature.get_mutations().has(PlayerMutationType::VTELEPORT)) {
            fmt::println(out_file, _(" あなたは自分の意思でテレポートできる。", " You can teleport at will."));
        }

        if (creature.get_mutations().has(PlayerMutationType::MIND_BLST)) {
            fmt::println(out_file, _(" あなたは敵を精神攻撃できる。", " You can Mind Blast your enemies."));
        }

        if (creature.get_mutations().has(PlayerMutationType::RADIATION)) {
            fmt::println(out_file, _(" あなたは自分の意思で放射能を発生することができる。", " You can emit hard radiation at will."));
        }

        if (creature.get_mutations().has(PlayerMutationType::VAMPIRISM)) {
            fmt::println(out_file, _(" あなたは吸血鬼のように敵から生命力を吸収することができる。", " You can drain life from a foe like a vampire."));
        }

        if (creature.get_mutations().has(PlayerMutationType::SMELL_MET)) {
            fmt::println(out_file, _(" あなたは近くにある貴金属をかぎ分けることができる。", " You can smell nearby precious metal."));
        }

        if (creature.get_mutations().has(PlayerMutationType::SMELL_MON)) {
            fmt::println(out_file, _(" あなたは近くのモンスターの存在をかぎ分けることができる。", " You can smell nearby monsters."));
        }

        if (creature.get_mutations().has(PlayerMutationType::BLINK)) {
            fmt::println(out_file, _(" あなたは短い距離をテレポートできる。", " You can teleport yourself short distances."));
        }

        if (creature.get_mutations().has(PlayerMutationType::EAT_ROCK)) {
            fmt::println(out_file, _(" あなたは硬い岩を食べることができる。", " You can consume solid rock."));
        }

        if (creature.get_mutations().has(PlayerMutationType::SWAP_POS)) {
            fmt::println(out_file, _(" あなたは他の者と場所を入れ替わることができる。", " You can switch locations with another being."));
        }

        if (creature.get_mutations().has(PlayerMutationType::SHRIEK)) {
            fmt::println(out_file, _(" あなたは身の毛もよだつ叫び声を発することができる。", " You can emit a horrible shriek."));
        }

        if (creature.get_mutations().has(PlayerMutationType::ILLUMINE)) {
            fmt::println(out_file, _(" あなたは明るい光を放つことができる。", " You can emit bright light."));
        }

        if (creature.get_mutations().has(PlayerMutationType::DET_CURSE)) {
            fmt::println(out_file, _(" あなたは邪悪な魔法の危険を感じとることができる。", " You can feel the danger of evil magic."));
        }

        if (creature.get_mutations().has(PlayerMutationType::BERSERK)) {
            fmt::println(out_file, _(" あなたは自分の意思で狂乱戦闘状態になることができる。", " You can drive yourself into a berserk frenzy."));
        }

        if (creature.get_mutations().has(PlayerMutationType::POLYMORPH)) {
            fmt::println(out_file, _(" あなたは自分の意志で変化できる。", " You can polymorph yourself at will."));
        }

        if (creature.get_mutations().has(PlayerMutationType::MIDAS_TCH)) {
            fmt::println(out_file, _(" あなたは通常アイテムを金に変えることができる。", " You can turn ordinary items to gold."));
        }

        if (creature.get_mutations().has(PlayerMutationType::GROW_MOLD)) {
            fmt::println(out_file, _(" あなたは周囲にキノコを生やすことができる。", " You can cause mold to grow near you."));
        }

        if (creature.get_mutations().has(PlayerMutationType::RESIST)) {
            fmt::println(out_file, _(" あなたは元素の攻撃に対して身を硬くすることができる。", " You can harden yourself to the ravages of the elements."));
        }

        if (creature.get_mutations().has(PlayerMutationType::EARTHQUAKE)) {
            fmt::println(out_file, _(" あなたは周囲のダンジョンを崩壊させることができる。", " You can bring down the dungeon around your ears."));
        }

        if (creature.get_mutations().has(PlayerMutationType::EAT_MAGIC)) {
            fmt::println(out_file, _(" あなたは魔法のエネルギーを自分の物として使用できる。", " You can consume magic energy for your own use."));
        }

        if (creature.get_mutations().has(PlayerMutationType::WEIGH_MAG)) {
            fmt::println(out_file, _(" あなたは自分に影響を与える魔法の力を感じることができる。", " You can feel the strength of the magics affecting you."));
        }

        if (creature.get_mutations().has(PlayerMutationType::STERILITY)) {
            fmt::println(out_file, _(" あなたは集団的生殖不能を起こすことができる。", " You can cause mass impotence."));
        }

        if (creature.get_mutations().has(PlayerMutationType::HIT_AND_AWAY)) {
            fmt::println(out_file, _(" あなたは攻撃した後身を守るため逃げることができる。", " You can run for your life after hitting something."));
        }

        if (creature.get_mutations().has(PlayerMutationType::DAZZLE)) {
            fmt::println(out_file, _(" あなたは混乱と盲目を引き起こす放射能を発生することができる。 ", " You can emit confusing, blinding radiation."));
        }

        if (creature.get_mutations().has(PlayerMutationType::LASER_EYE)) {
            fmt::println(out_file, _(" あなたは目からレーザー光線を発射することができる。", " Your eyes can fire laser beams."));
        }

        if (creature.get_mutations().has(PlayerMutationType::RECALL)) {
            fmt::println(out_file, _(" あなたは街とダンジョンの間を行き来することができる。", " You can travel between town and the depths."));
        }

        if (creature.get_mutations().has(PlayerMutationType::BANISH)) {
            fmt::println(out_file, _(" あなたは邪悪なモンスターを地獄に落とすことができる。", " You can send evil creatures directly to Hell."));
        }

        if (creature.get_mutations().has(PlayerMutationType::COLD_TOUCH)) {
            fmt::println(out_file, _(" あなたは物を触って凍らせることができる。", " You can freeze things with a touch."));
        }

        if (creature.get_mutations().has(PlayerMutationType::LAUNCHER)) {
            fmt::println(out_file, _(" あなたはアイテムを力強く投げることができる。", " You can hurl objects with great force."));
        }

        if (creature.get_mutations().has(PlayerMutationType::BERS_RAGE)) {
            fmt::println(out_file, _(" あなたは狂戦士化の発作を起こす。", " You are subject to berserker fits."));
        }

        if (creature.get_mutations().has(PlayerMutationType::COWARDICE)) {
            fmt::println(out_file, _(" あなたは時々臆病になる。", " You are subject to cowardice."));
        }

        if (creature.get_mutations().has(PlayerMutationType::RTELEPORT)) {
            fmt::println(out_file, _(" あなたはランダムにテレポートする。", " You sometimes randomly teleport."));
        }

        if (creature.get_mutations().has(PlayerMutationType::ALCOHOL)) {
            fmt::println(out_file, _(" あなたの体はアルコールを分泌する。", " Your body produces alcohol."));
        }

        if (creature.get_mutations().has(PlayerMutationType::HALLU)) {
            fmt::println(out_file, _(" あなたは幻覚を引き起こす精神錯乱に侵されている。", " You have a hallucinatory insanity."));
        }

        if (creature.get_mutations().has(PlayerMutationType::FLATULENT)) {
            fmt::println(out_file, _(" あなたは制御できない強烈な屁をこく。", " You are subject to uncontrollable flatulence."));
        }

        if (creature.get_mutations().has(PlayerMutationType::IKISUGI)) {
            fmt::println(out_file, _(" あなたは時々イキすぎる。", " You sometimes climax too much."));
        }

        if (creature.get_mutations().has(PlayerMutationType::ATT_NASTY)) {
            fmt::println(out_file, _(" あなたはクッソ汚い輩を引きつける。", " You attract nasty creatures."));
        }

        if (has_pervert_attraction(creature)) {
            fmt::println(out_file, _(" あなたは変質者を引きつける。", " You attract perverts."));
        }

        if (creature.get_mutations().has(PlayerMutationType::PROD_MANA)) {
            fmt::println(out_file, _(" あなたは制御不能な魔法のエネルギーを発している。", " You produce magical energy uncontrollably."));
        }

        if (creature.get_mutations().has(PlayerMutationType::ATT_DEMON)) {
            fmt::println(out_file, _(" あなたはデーモンを引きつける。", " You attract demons."));
        }

        if (creature.get_mutations().has(PlayerMutationType::SCOR_TAIL)) {
            fmt::println(out_file, _(" あなたはサソリの尻尾が生えている。(毒、ダメージ 3d7)", " You have a scorpion tail (poison, 3d7)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::HORNS)) {
            fmt::println(out_file, _(" あなたは角が生えている。(ダメージ 2d6)", " You have horns (dam. 2d6)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::BEAK)) {
            fmt::println(out_file, _(" あなたはクチバシが生えている。(ダメージ 2d4)", " You have a beak (dam. 2d4)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::SPEED_FLUX)) {
            fmt::println(out_file, _(" あなたはランダムに早く動いたり遅く動いたりする。", " You move faster or slower randomly."));
        }

        if (creature.get_mutations().has(PlayerMutationType::BANISH_ALL)) {
            fmt::println(out_file, _(" あなたは時々近くのモンスターを消滅させる。", " You sometimes cause nearby creatures to vanish."));
        }

        if (creature.get_mutations().has(PlayerMutationType::EAT_LIGHT)) {
            fmt::println(out_file, _(" あなたは時々周囲の光を吸収して栄養にする。", " You sometimes feed off of the light around you."));
        }

        if (creature.get_mutations().has(PlayerMutationType::TRUNK)) {
            fmt::println(out_file, _(" あなたは象のような鼻を持っている。(ダメージ 1d4)", " You have an elephantine trunk (dam 1d4)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::ATT_ANIMAL)) {
            fmt::println(out_file, _(" あなたは動物を引きつける。", " You attract animals."));
        }

        if (creature.get_mutations().has(PlayerMutationType::TENTACLES)) {
            fmt::println(out_file, _(" あなたは邪悪な触手を持っている。(ダメージ 2d5)", " You have evil looking tentacles (dam 2d5)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::RAW_CHAOS)) {
            fmt::println(out_file, _(" あなたはしばしば純カオスに包まれる。", " You occasionally are surrounded with raw chaos."));
        }

        if (creature.get_mutations().has(PlayerMutationType::NORMALITY)) {
            fmt::println(out_file, _(" あなたは変異していたが、回復してきている。", " You may be mutated, but you're recovering."));
        }

        if (creature.get_mutations().has(PlayerMutationType::WRAITH)) {
            fmt::println(out_file, _(" あなたの肉体は幽体化したり実体化したりする。", " You fade in and out of physical reality."));
        }

        if (creature.get_mutations().has(PlayerMutationType::POLY_WOUND)) {
            fmt::println(out_file, _(" あなたの健康はカオスの力に影響を受ける。", " Your health is subject to chaotic forces."));
        }

        if (creature.get_mutations().has(PlayerMutationType::WASTING)) {
            fmt::println(out_file, _(" あなたは衰弱する恐ろしい病気にかかっている。", " You have a horrible wasting disease."));
        }

        if (creature.get_mutations().has(PlayerMutationType::ATT_DRAGON)) {
            fmt::println(out_file, _(" あなたはドラゴンを引きつける。", " You attract dragons."));
        }

        if (creature.get_mutations().has(PlayerMutationType::WEIRD_MIND)) {
            fmt::println(out_file, _(" あなたの精神はランダムに拡大したり縮小したりしている。", " Your mind randomly expands and contracts."));
        }

        if (creature.get_mutations().has(PlayerMutationType::NAUSEA)) {
            fmt::println(out_file, _(" あなたの胃は非常に落ち着きがない。", " You have a seriously upset stomach."));
        }

        if (creature.get_mutations().has(PlayerMutationType::CHAOS_GIFT)) {
            fmt::println(out_file, _(" あなたはカオスの守護悪魔から褒美をうけとる。", " Chaos deities give you gifts."));
        }

        if (creature.get_mutations().has(PlayerMutationType::WALK_SHAD)) {
            fmt::println(out_file, _(" あなたはしばしば他の「影」に迷い込む。", " You occasionally stumble into other shadows."));
        }

        if (creature.get_mutations().has(PlayerMutationType::WARNING)) {
            fmt::println(out_file, _(" あなたは敵に関する警告を感じる。", " You receive warnings about your foes."));
        }

        if (creature.get_mutations().has(PlayerMutationType::INVULN)) {
            fmt::println(out_file, _(" あなたは時々負け知らずな気分になる。", " You occasionally feel invincible."));
        }

        if (creature.get_mutations().has(PlayerMutationType::SP_TO_HP)) {
            fmt::println(out_file, _(" あなたは時々血が筋肉にどっと流れる。", " Your blood sometimes rushes to your muscles."));
        }

        if (creature.get_mutations().has(PlayerMutationType::HP_TO_SP)) {
            fmt::println(out_file, _(" あなたは時々頭に血がどっと流れる。", " Your blood sometimes rushes to your head."));
        }

        if (creature.get_mutations().has(PlayerMutationType::DISARM)) {
            fmt::println(out_file, _(" あなたはよくつまづいて物を落とす。", " You occasionally stumble and drop things."));
        }

        if (creature.get_mutations().has(PlayerMutationType::HYPER_STR)) {
            fmt::println(out_file, _(" あなたは超人的に強い。(腕力+4)", " You are superhumanly strong (+4 STR)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::PUNY)) {
            fmt::println(out_file, _(" あなたは虚弱だ。(腕力-4)", " You are puny (-4 STR)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::HYPER_INT)) {
            fmt::println(out_file, _(" あなたの脳は生体コンピュータだ。(知能＆賢さ+4)", " Your brain is a living computer (+4 INT/WIS)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::MORONIC)) {
            fmt::println(out_file, _(" あなたは精神薄弱だ。(知能＆賢さ-4)", " You are moronic (-4 INT/WIS)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::RESILIENT)) {
            fmt::println(out_file, _(" あなたの体は弾力性に富んでいる。(耐久+4)", " You are very resilient (+4 CON)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::XTRA_FAT)) {
            fmt::println(out_file, _(" あなたは極端に太っている。(耐久+2,スピード-2)", " You are extremely fat (+2 CON, -2 speed)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::ALBINO)) {
            fmt::println(out_file, _(" あなたはアルビノだ。(耐久-4)", " You are albino (-4 CON)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::FLESH_ROT)) {
            fmt::println(out_file, _(" あなたの肉体は腐敗している。(耐久-2,魅力-1)", " Your flesh is rotting (-2 CON, -1 CHR)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::SILLY_VOI)) {
            fmt::println(out_file, _(" あなたの声は間抜けなキーキー声だ。(魅力-4)", " Your voice is a silly squeak (-4 CHR)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::BLANK_FAC)) {
            fmt::println(out_file, _(" あなたはのっぺらぼうだ。(魅力-1)", " Your face is featureless (-1 CHR)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::ILL_NORM)) {
            fmt::println(out_file, _(" あなたは幻影に覆われている。", " Your appearance is masked with illusion."));
        }

        if (creature.get_mutations().has(PlayerMutationType::XTRA_EYES)) {
            fmt::println(out_file, _(" あなたは余分に二つの目を持っている。(探索+15)", " You have an extra pair of eyes (+15 search)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::MAGIC_RES)) {
            fmt::println(out_file, _(" あなたは魔法への耐性をもっている。", " You are resistant to magic."));
        }

        if (creature.get_mutations().has(PlayerMutationType::XTRA_NOIS)) {
            fmt::println(out_file, _(" あなたは変な音を発している。(隠密-3)", " You make a lot of strange noise (-3 stealth)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::INFRAVIS)) {
            fmt::println(out_file, _(" あなたは素晴らしい赤外線視力を持っている。(+3)", " You have remarkable infravision (+3)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::XTRA_LEGS)) {
            fmt::println(out_file, _(" あなたは余分に二本の足が生えている。(加速+3)", " You have an extra pair of legs (+3 speed)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::SHORT_LEG)) {
            fmt::println(out_file, _(" あなたの足は短い突起だ。(加速-3)", " Your legs are short stubs (-3 speed)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::WEAK_LOWER_BODY)) {
            fmt::println(out_file, _(" あなたは上半身に比べて下半身が貧弱すぎる。(腕力+2, 加速-2)", " Your lower body is too weak compared to your upper body (+2 STR, -2 speed)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::ELEC_TOUC)) {
            fmt::println(out_file, _(" あなたの血管には電流が流れている。", " Electricity runs through your veins."));
        }

        if (creature.get_mutations().has(PlayerMutationType::FIRE_BODY)) {
            fmt::println(out_file, _(" あなたの体は炎につつまれている。", " Your body is enveloped in flames."));
        }

        if (creature.get_mutations().has(PlayerMutationType::WART_SKIN)) {
            fmt::println(out_file, _(" あなたの肌はイボに被われている。(魅力-2, AC+5)", " Your skin is covered with warts (-2 CHR, +5 AC)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::SCALES)) {
            fmt::println(out_file, _(" あなたの肌は鱗になっている。(魅力-1, AC+10)", " Your skin has turned into scales (-1 CHR, +10 AC)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::IRON_SKIN)) {
            fmt::println(out_file, _(" あなたの肌は鉄でできている。(器用-1, AC+25)", " Your skin is made of steel (-1 DEX, +25 AC)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::WINGS)) {
            fmt::println(out_file, _(" あなたは羽を持っている。", " You have wings."));
        }

        if (creature.get_mutations().has(PlayerMutationType::FEARLESS)) {
            fmt::println(out_file, _(" あなたは全く恐怖を感じない。", " You are completely fearless."));
        }

        if (creature.get_mutations().has(PlayerMutationType::REGEN)) {
            fmt::println(out_file, _(" あなたは急速に回復する。", " You are regenerating."));
        }

        if (creature.get_mutations().has(PlayerMutationType::ESP)) {
            fmt::println(out_file, _(" あなたはテレパシーを持っている。", " You are telepathic."));
        }

        if (creature.get_mutations().has(PlayerMutationType::LIMBER)) {
            fmt::println(out_file, _(" あなたの体は非常にしなやかだ。(器用+3)", " Your body is very limber (+3 DEX)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::ARTHRITIS)) {
            fmt::println(out_file, _(" あなたはいつも関節に痛みを感じている。(器用-3)", " Your joints ache constantly (-3 DEX)."));
        }

        if (creature.get_mutations().has(PlayerMutationType::VULN_ELEM)) {
            fmt::println(out_file, _(" あなたは元素の攻撃に弱い。", " You are susceptible to damage from the elements."));
        }

        if (creature.get_mutations().has(PlayerMutationType::MOTION)) {
            fmt::println(out_file, _(" あなたの動作は正確で力強い。(隠密+1)", " Your movements are precise and forceful (+1 STL)."));
        }

        if (has_good_luck(creature)) {
            fmt::println(out_file, _(" あなたは白いオーラにつつまれている。", " There is a white aura surrounding you."));
        }

        if (creature.get_mutations().has(PlayerMutationType::BAD_LUCK)) {
            fmt::println(out_file, _(" あなたは黒いオーラにつつまれている。", " There is a black aura surrounding you."));
        }

        if (creature.get_mutations().has(PlayerMutationType::DEFECATION)) {
            fmt::print(out_file, _("あなたは脱糞を制御できない。", "You are subject to uncontrollable defecation."));
        }

        if (creature.get_mutations().has(PlayerMutationType::ZEERO_VIRUS)) {
            fmt::print(out_file, _("あなたはゼEROウイルスに感染している。SEXDA!", "You are infected with ZEERO virus. Let's SEX!"));
        }

        if (creature.get_mutations().has(PlayerMutationType::HOMO_SEXUAL)) {
            fmt::print(out_file, _("あなたは同性愛者だ。", "You are Homosexual."));
        }

        if (creature.get_mutations().has(PlayerMutationType::BI_SEXUAL)) {
            fmt::print(out_file, _("あなたは両性愛者だ。", "You are Homosexual."));
        }
    }
}
