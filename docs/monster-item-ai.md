# モンスター AI アイテム使用ロジック

bakabakaband のモンスター AI は毎ターン `inventory[]` の消耗品を状況判定で
自動使用する。本書は実装場所と判定優先度をまとめたリファレンス。

実装ファイル: `src/monster/monster-processor.cpp`

呼出箇所: `process_monster()` 内、`process_stalking()` 直前。
スタン (50% で打ち切り) の後、通常行動に移る前に実行される。

## 実行順序

毎ターン以下の 3 つのヘルパが順に呼ばれる:

1. `monster_quaff_potion(creature, monster)` — ポーション
2. `monster_read_scroll(creature, monster, m_idx)` — 巻物
3. `monster_use_wand_or_rod(creature, monster, m_idx)` — 杖/ロッド

各ヘルパは `inventory[]` を 1 回走査し、状況にマッチする中で priority の
最も小さい (= 最優先) アイテムを 1 個消費する。マッチなしなら何もしない。

## 状況フラグ

```cpp
const bool low_hp_severe   = monster.hp < monster.maxhp / 4;  // 25% 未満
const bool low_hp_mid      = monster.hp < monster.maxhp / 2;  // 50% 未満
const bool poisoned_heavy  = POISON > 100;
const bool fearful         = monster.is_fearful();
const bool not_fast        = ACCELERATION == 0;
const bool not_resistant   = OPPOSE_FIRE == 0 && OPPOSE_COLD == 0;
const bool not_buffed      = HERO == 0 && BERSERK == 0;
const bool fighting_context = monster.is_hostile() && monster.is_visible_on_map();
const bool can_target_player = fighting_context && projectable(...);
const bool has_status      = fearful || confused || stunned || POISON > 0;
```

## ポーション (`monster_quaff_potion`)

| Priority | sval | 発動条件 | 効果 |
|---|---|---|---|
| 0 | STAR_HEALING / LIFE | `low_hp_severe` | HP+1200, 状態異常リセット, FEAR 解除 |
| 1 | HEALING | `low_hp_severe` | HP+300, CUT/STUN/POISON リセット |
| 2 | INVULNERABILITY | `low_hp_severe` | INVULNERABILITY 8〜16 ターン |
| 3 | CURE_CRITICAL | `low_hp_mid` | HP+6d8, CUT/STUN/POISON リセット |
| 4 | CURE_SERIOUS | `low_hp_mid` | HP+4d8, CUT 半減 |
| 5 | CURE_LIGHT | `low_hp_mid` | HP+2d8, CUT 微減 |
| 6 | CURE_POISON | `poisoned_heavy` | POISON リセット |
| 7 | BOLDNESS | `fearful` | FEAR リセット |
| 8 | BESERK_STRENGTH | `fearful` or (`fighting_context && not_buffed`) | FEAR/STUN リセット, BERSERK 25〜50, HP+30 |
| 9 | HEROISM | `fearful` or (`fighting_context && not_buffed`) | FEAR リセット, HERO 25〜50, HP+10 |
| 10 | SPEED | `fighting_context && not_fast` | ACCELERATION 16〜40 |
| 11 | RESISTANCE | `fighting_context && not_resistant` | OPPOSE_ACID/ELEC/FIRE/COLD/POIS 21〜40 |

## 巻物 (`monster_read_scroll`)

| Priority | sval | 発動条件 | 効果 |
|---|---|---|---|
| 0 | TELEPORT_LEVEL | `low_hp_severe` | フロア間離脱 |
| 1 | TELEPORT | `low_hp_severe` | 200 距離テレポート |
| 2 | PHASE_DOOR | `low_hp_severe` | 10 距離テレポート |
| 7 | SUMMON_KIN | `fighting_context` | 同族 3 体召喚 (試行) |
| 8 | SUMMON_MONSTER / SUMMON_UNDEAD | `fighting_context` | 汎用/アンデッド 3 体召喚 (試行) |

## 杖/ロッド (`monster_use_wand_or_rod`)

### 自己標的

| Priority | sval | 種別 | 発動条件 | 効果 |
|---|---|---|---|---|
| 0 | ROD_HEALING | rod | `low_hp_mid` | HP+500 |
| 1 | WAND_HEAL_MONSTER | wand | `low_hp_mid` | HP+20 |
| 4 | ROD_SPEED | rod | `fighting_context && not_fast` | ACCELERATION 16〜40 |
| 5 | WAND_HASTE_MONSTER | wand | `fighting_context && not_fast` | ACCELERATION 100〜200 |
| 6 | ROD_CURING | rod | `has_status` | FEAR/CONF/STUN/POISON リセット |
| 9 | WAND_CLONE_MONSTER | wand | `fighting_context` | 自身分裂 (`multiply_monster`) |

### プレイヤー標的 (攻撃)

priority 10 (wand) / 11 (rod), 発動条件は `can_target_player`

| sval | tval | dam |
|---|---|---|
| MAGIC_MISSILE | wand | MISSILE 3d4 |
| ACID_BOLT | wand | ACID 10d8 |
| FIRE_BOLT | wand | FIRE 9d8 |
| COLD_BOLT | wand | COLD 6d8 |
| HYPODYNAMIA | wand | HYPODYNAMIA 80 |
| STINKING_CLOUD | wand | POIS 12 ball rad 2 |
| ACID_BALL/ELEC_BALL/FIRE_BALL/COLD_BALL | wand | 60/32/72/48 ball rad 2 |
| DRAGON_FIRE/COLD/BREATH | wand | 100/80/120 ball rad 3 |
| ROD_ACID_BOLT | rod | ACID 12d8 |
| ROD_ELEC_BOLT | rod | ELEC 6d6 |
| ROD_FIRE_BOLT | rod | FIRE 16d8 |
| ROD_COLD_BOLT | rod | COLD 10d8 |
| ROD_HYPODYNAMIA | rod | HYPODYNAMIA 75 |
| ROD_ACID_BALL/ELEC_BALL/FIRE_BALL/COLD_BALL | rod | 60/32/72/48 ball rad 2 |

### プレイヤー標的 (防衛)

| Priority | sval | 種別 | 発動条件 | 効果 |
|---|---|---|---|---|
| 3 | WAND_TELEPORT_AWAY | wand | `can_target_player && low_hp_mid` | プレイヤーを 100 距離テレポート |

## 消費メカニクス

- **ポーション**: `number > 1` ならデクリメント、`= 1` なら `wipe()` + `inven_cnt--`
- **巻物**: 同上
- **杖**: `pval--` (charges 1 消費)
- **ロッド**: `timeout += base_pval` (再充填時間設定)
  - 単一所持で `timeout > 0` ならガード、複数所持で `timeout > base_pval * (number-1)` ならガード

## ペット運用との関係

`fighting_context` の判定は `ai_is_in_fighting_context()` ヘルパに集約され、
以下の 2 つの状況で true を返す:

- 敵対モンスター: 視認可能 (`is_visible_on_map()`) かつ `is_hostile()`
- ペット: 視認可能で、視認可能な hostile monster が同フロアに存在

これにより、ペットも以下が発動するようになっている:

- ポーション: SPEED, HEROISM, BERSERK, RESISTANCE 等の戦闘バフ
- 杖/ロッド: HEAL_MONSTER, HASTE_MONSTER, CLONE_MONSTER (自身分裂で増援)
- 攻撃 wand/rod: 最寄りの hostile monster (projectable) を標的として発射

ただし以下は `monster_read_scroll` 内の独自 `fighting_context` (hostile only) で
ガードされており、ペットは使わない:

- SCROLL_SUMMON_MONSTER / UNDEAD / KIN (敵を増やす行動)

`monster_use_wand_or_rod` 内で攻撃 wand の標的決定は:

- 敵対モンスター: target = プレイヤー位置
- ペット: target = `is_pet() && best_dist` で選定された最寄り hostile monster の位置

ペットの WAND_CLONE_MONSTER は分身が `multiply_monster(..., PM_NO_PET)` で
生成されるため、ペット自身が分裂したものは敵性ではないが pet flag は
継承されない (NO_PET 強制) 仕様。これは「ペットが一時的に増援を呼ぶ」
意図に合っている (バランス上 ペットが無限増殖しないように)。

## メッセージ

すべて `is_seen(creature, monster)` で視認可能な場合のみ表示:

- ポーション: `%s^ quaffs %s.` / 「%s^ は %s を飲んだ。」
- 巻物: `%s^ reads %s.` / 「%s^ は %s を読んだ。」
- 杖: `%s^ aims %s.` / 「%s^ は %s を振った。」(対象指向)
- ロッド: `%s^ zaps %s.` / 「%s^ は %s を振った。」

## 関連 commit (実装履歴)

- 080d4dbf3: 回復ポーション 5 種
- 21e3215fd: ポーション 12 種に拡張 (バフ系)
- dbcabb762: 巻物 (TELEPORT/PHASE_DOOR)
- fa873535f: TELEPORT_LEVEL 巻物 + WAND_TELEPORT_AWAY
- 92032a3f5: 自己標的 wand/rod (HEAL/HASTE/HEALING/SPEED/CURING)
- e9cebf58a: 攻撃 wand/rod 17 種
- 49a7fa33b: 召喚巻物 + WAND_CLONE_MONSTER
