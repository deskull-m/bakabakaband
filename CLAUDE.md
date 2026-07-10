# CLAUDE.md — bakabakaband 開発ガイド

## プロジェクト概要

bakabakaband は Hengband をベースにした日本語ローグライクゲーム（C++）。
Hengband 派生プロジェクトとして、プレイヤー（`PlayerType`）とモンスター
(かつての `MonsterEntity`) を共通の `CreatureEntity` スーパークラスで
扱えるようにする **CreatureEntity 統合リファクタリング** が長期方針として
進行しており、現在は概ね完了している（詳細はロードマップ節参照）。

---

## CreatureEntity 統合リファクタリング

### 目的

プレイヤー（`PlayerType`）とモンスター（旧 `MonsterEntity`）を共通の
`CreatureEntity` スーパークラスで扱えるようにし、ダメージ処理・状態効果・
行動 AI などのロジックを両者で共通化する。

**最終目標:** `MonsterEntity` を `CreatureEntity` に完全吸収し、
モンスター固有データは `MonsterProfile` 構造体に分離する。

→ 現状 `MonsterEntity` クラスは既に削除済み。モンスターは
`CreatureEntity` インスタンスとして `monster_profile`
(`tl::optional<MonsterProfile>`) を伴って存在する。

### クラス階層（現状）

```
CreatureEntity  (基底クラス)  src/system/creature-entity.h
└── PlayerType               src/system/player-type-definition.h
```

モンスターは `CreatureEntity` を直接インスタンス化し、
`CreatureEntity::monster_profile` に `MonsterProfile`
（`src/system/monster-profile.h`）を詰めて使用する。

### 純粋仮想メソッド（必ず両クラスで実装）

| メソッド | 説明 |
|---|---|
| `get_current_hp()` | 現在 HP |
| `get_max_hp()` | 最大 HP |
| `is_valid()` | 生存中かどうか |
| `is_dead()` | 死亡しているか |
| `is_player()` | プレイヤーなら true |

### CreatureEntity が持つ主なフィールド（移動済み）

- 座標: `x`, `y`, `oldpx`, `oldpy`
- HP: `hp`, `maxhp`, `max_maxhp`, `hp_frac`
- MP: `current_mp`, `max_mp`, `current_mp_frac` (旧 `csp` / `msp` / `csp_frac`、提案 6 で改名)
- 能力値: `stat_cur[]`, `stat_max[]`, `stat_use[]` 等
- スキル: `skill_dis`, `skill_dev`, `skill_sav` 等
- タイムドエフェクト: `invuln`, `hero`, `oppose_fire` 等多数
- 経験値: `exp`, `max_exp`, `max_max_exp`, `exp_frac`
- 速度・エネルギー: `speed`, `energy_need`
- AC: `ac`, `to_a`
- レベル: `level`
- 名前: `name`

---

## コーディング規約

### 関数シグネチャ

```cpp
// NG: 古い形式（プレイヤーのみ）
void some_function(PlayerType *player_ptr);

// OK: 新しい形式（プレイヤー・モンスター共用）
void some_function(CreatureEntity &creature);
```

新規関数、または既存関数を修正する際は可能な限り `CreatureEntity &` を使うこと。

### 型キャスト・型分岐

型の分岐が必要な場合は `is_player()` で判定する：

```cpp
if (creature.is_player()) {
    auto &player = static_cast<PlayerType &>(creature);
    // プレイヤー固有の処理（class_specific_data 等にアクセスしたい場合など）
} else {
    // モンスター固有の処理
    auto &profile = creature.get_monster_profile(); // MonsterProfile
    // ...
}
```

`MonsterEntity` クラスは既に廃止済みのため、モンスター側は
`creature.get_monster_profile()` 経由で固有データ (`MonsterProfile`) に
アクセスする。`CreatureEntity` 基底からキャストする必要はない。

キャストは最小限に。可能な限り virtual メソッド (`is_confused()` /
`get_timed_effect()` / `is_pet()` 等) で処理すること。

### GCC ビルド注意事項

ヘッダファイルで `uint8_t` 等の固定幅整数型を使う場合は、`<cstdint>` を明示的にインクルードすること。
GCC は MSVC と異なり、他のヘッダ経由での暗黙インクルードに依存できない。

```cpp
// NG: <cstdint> なしで uint8_t を使うとGCCエラー
tl::optional<MonraceId> select_pit_nest_monrace_id(CreatureEntity &creature, uint8_t &sub_align, int boost);

// OK: <cstdint> を明示的にインクルード
#include <cstdint>
tl::optional<MonraceId> select_pit_nest_monrace_id(CreatureEntity &creature, uint8_t &sub_align, int boost);
```

---

## 統合ロードマップ進捗

### Phase 1: 関数シグネチャの統一 ✅ 完了

`PlayerType *player_ptr` を引数に取る関数を `CreatureEntity &creature` に
変更する作業は完了済み。

- `PlayerType *player_ptr` 引数は `PlayerType::get_instance_ptr()` の
  静的アクセサ以外には存在しない
- `player_ptr` / `p_ptr` 識別子はローカル変数・クラスメンバ・doxygen
  コメントまで含めて全て `creature` / `creature_ptr` に統一済み

新規コードでは引き続き `CreatureEntity &creature` を基本形とすること。

### Phase 2: 状態チェックの仮想化 ✅ 完了

`is_confused()`, `is_stunned()`, `is_fearful()`, `is_invulnerable()`,
`is_blind()`, `is_paralyzed()`, `is_fast()`, `is_accelerated()`,
`is_decelerated()`, `is_blessed()`, `is_hero()`, `is_shero()`,
`is_asleep()` 等はすべて `CreatureEntity` の virtual メソッドとして
実装済み（`src/system/creature-entity.h`）。

旧 `is_hero(player_ptr)` / `is_blessed(creature)` 等の自由関数は削除済み
(13 個)。全ての呼出は `creature.is_hero()` 形式に移行済み。

### Phase 3: タイムドエフェクトの統一 ✅ 完了

全てのタイムドエフェクトは `CreatureEntity::timed_effects_map`
（`std::map<CreatureTimedEffect, TIME_EFFECT>`）に**単一管理**されている。

- 共通列挙型: `CreatureTimedEffect`（`src/system/creature-timed-effect-types.h`）
- 共通 API: `get_timed_effect(effect)` / `set_timed_effect(effect, value)`
  (virtual, プレイヤー・モンスター完全同一経路)
- プレイヤーの旧 41 個の直接 `TIME_EFFECT` フィールド (`hero` / `invuln` 等)
  および `word_recall` / `alter_reality` は削除済み
- モンスター側の旧 `MonsterProfile::mtimed` も削除済み
- 旧 `TimedEffects` オブジェクトおよび配下 9 簡易ヘルパクラス
  (`PlayerAcceleration` / `PlayerBlindness` / `PlayerConfusion` /
  `PlayerDeceleration` / `PlayerFear` / `PlayerHallucination` /
  `PlayerParalysis` / `PlayerPoison` / `PlayerProtection`) は
  **提案 5 完了で削除済み**。残る `PlayerStun` / `PlayerCut` は
  stateless static utility (`get_rank(short value)` 等) に変換済み
- 全セーブ/ロード・ステータスセッター等が統一 API 経由で動作

### Phase 4: ダメージ処理の完全統一 ✅ 完了

共通基盤は既に整備済み:

- `CreatureEntity::apply_raw_damage(damage)` が HP 減算と `on_take_hit()`
  呼出を行う共通プリミティブとして実装済み。`take_hit()` (プレイヤー経路)
  と `MonsterDamageProcessor::mon_take_hit()` (モンスター経路) の両方から
  呼ばれる
- `on_take_hit(damage)` / `on_death(cause)` は `CreatureEntity` の
  virtual フックとして実装済み。`PlayerType` が自身のセーブ・ゲームオーバー
  処理等をこれで行う（`src/player/player-damage.cpp`）

呼出経路は依然プレイヤー専用 (`take_hit`) とモンスター専用
(`MonsterDamageProcessor`) に分かれているが、これは両者の副次処理
（プレイヤー: ダメージタイプ別 autosave / インベントリ破壊 / モンスター:
死亡ドロップ / 経験値 / アライアンス更新）が本質的に異なるためで、
単一ディスパッチャにまとめても call site は型を分けて使い分ける必要がある。

**当初 `src/combat/damage-dispatcher.{h,cpp}` に薄いディスパッチャ関数
`apply_damage_to_creature()` を用意していたが、どの call site でも
自分の被害者型を既に把握しており利用されなかったため削除した。**
新規コードで被害者型が実行時に不定となるケース（稀）が発生した場合は、
`is_player()` で分岐して `take_hit()` / `MonsterDamageProcessor` を
呼び分けるのが現状の方針。

### Phase 5: AC・防御の統一 ✅ 完了

`get_ac()` は `CreatureEntity` の virtual メソッドとして既に実装済み
（`src/system/creature-entity.{h,cpp}`）。`ac + to_a` をベースに
モンスターの `NAKED` フラグ等も考慮する。

### Phase 6: フロアポインタの整理 ✅ 完了

`current_floor_ptr` は `CreatureEntity` の protected メンバに整理され、
アクセスは全て `get_floor()` / `set_floor()` virtual メソッド経由。
直接参照は残っていない。

---

## モンスター固有フィールドの扱い方針

モンスター固有フィールドは `MonsterProfile`
(`src/system/monster-profile.h`) に集約する方針。

### Phase 7: 汎用化できるフィールドを CreatureEntity に移動 ✅ 完了

| フィールド | 状況 |
|---|---|
| `cdis` | 既にテンポラリ化 (`Grid::calc_distance(...)` 経由のローカル変数) |
| `target_y`, `target_x` | `CreatureEntity::target`（`Pos2D`）として統合済み |
| `mflag` (ペット/フレンドリー等) | `is_pet()` / `is_friendly()` / `is_hostile()` を
  `CreatureEntity` の virtual メソッドとして提供、実体は `MonsterProfile::mflag2` |

### Phase 8: MonsterEntity の完全吸収 ✅ 完了

`MonsterEntity` クラスは削除済み。モンスターは `CreatureEntity` インスタンス
として生成され、固有データは `CreatureEntity::monster_profile`
(`tl::optional<MonsterProfile>`) に詰める。

```
// 現状
CreatureEntity
├── 共通フィールド（HP, 座標, 速度, 状態, timed_effects_map, ...）
├── std::shared_ptr<TimedEffects> timed_effects
│       (プレイヤー用 TimedEffects オブジェクト。stun / confusion 等の
│        高機能タイマー。モンスターでは nullptr)
└── tl::optional<MonsterProfile> monster_profile
        (モンスター固有データ: alliance_idx / mflag / smart /
         transform_* 等。提案 21 で全メンバ private 化済)
```

### Phase 9-32b: MonsterProfile + CreatureEntity アクセス API の整備 ✅ 完了

提案 9 系列 (read-side virtual)、提案 14-22 系列 (write-side virtual /
共通走査 API / 一括操作 / class 化)、提案 24-27b 系列 (CreatureEntity
直下フィールドの setter / 派生情報の自動計算化 / compound assignment
の OO 化)、提案 28-29 系列 (read getter 整備と完全 private 化) により、
モンスター状態への外部アクセスはほぼ全て `CreatureEntity` の virtual
API 経由に統一された。一部フィールド (r_idx / ap_r_idx / riding) は
完全 private 化されている。

- **提案 14 / 14b**: `find_nearest_creature` / `has_visible_creature` /
  `collect_creatures` で m_list 走査を集約
- **提案 15 / 15b / 18 / 20**: `mflag2` の has/set/reset/assign/clear/
  get_all/set_all を virtual 化
- **提案 16 / 20**: `mflag` (一時フラグ) を virtual 化
- **提案 9b / 17 / 19 / 22**: `alliance_idx`/`sub_align`/`parent_m_idx`/
  `smart`/`inventory`/`transform_*`/`death_count`/`r_idx`/`ap_r_idx`/
  `riding` の write-side virtual
- **提案 21**: `MonsterProfile` を class 化、全メンバ private 化、
  `friend` は `CreatureEntity` / `MonsterLoader50` / `MonsterWriter`
  の 3 つに限定
- **提案 24**: `age` / `ht` / `wt` / `prestige` 等の setter virtual
- **提案 25**: `inven_cnt` / `equip_cnt` フィールドを廃止し
  inventory[] から自動計算 (`get_inven_cnt()` / `get_equip_cnt()`)
- **提案 26**: `ambush_flag` / `food` / `town_num` / `level` の setter
  virtual
- **提案 27**: `max_plv` / `msp` / 経験値系 (exp / max_exp /
  max_max_exp) の setter virtual
- **提案 27b**: `au` (所持金) / `csp` (現在 MP) の set/add/sub/divide
  virtual。約 110 箇所の compound assignment (`+=` / `-=`) を OO 化
- **提案 28 / 28b**: `r_idx` / `ap_r_idx` / `riding` の getter virtual
  整備と全 read site (約 290 箇所、参照とポインタ経由両方) の getter
  経由化
- **提案 29**: `r_idx` / `ap_r_idx` / `riding` を CreatureEntity の
  private 化。CreatureEntity 直下フィールドの完全 private 化に成功
  した最初の例。これらフィールドへのアクセスは get_*() / set_*() /
  polymorph_to() / ride_monster() の virtual API 経由でのみ可能
- **提案 30**: 戦闘ボーナス (to_h_b / to_h_m / to_d_m / to_a) と
  能力値配列 (stat_max / stat_cur / stat_max_max / stat_use / stat_top /
  stat_add / stat_index) の setter virtual 整備、約 55 箇所 migration
- **提案 31**: 残り plain field 14 種 (au / csp / food / town_num /
  age / ht / wt / prestige / max_plv / msp / exp / max_exp /
  max_max_exp / ambush_flag) の getter virtual 整備と read site 約
  350 箇所の migration
- **提案 31b**: 能力値配列 `stat_*[]` (7 種) と戦闘ボーナス
  (`to_h_b` / `to_h_m` / `to_d_m` / `to_a` / `to_h[hand]` /
  `to_d[hand]`) の getter virtual 整備と read site 約 270 箇所の
  migration
- **提案 31c**: `level` read site 628 箇所を `get_level()` virtual
  経由に統一 (175 ファイル migration)
- **提案 32**: 安全な 7 フィールド (ambush_flag / prestige /
  max_max_exp / max_plv / to_h_b / to_h_m / to_d_m) を CreatureEntity
  の private 化。他クラス (ItemEntity / MonraceDefinition /
  ArtifactType / 等) と名前が衝突しないものに限定した縮小スコープ
- **提案 32b**: 残り 14 個の plain field (age / ht / wt / stat_*[] /
  max_exp / exp / msp / csp / town_num / level / au / food /
  to_h[] / to_d[]) を個別 fix で完全 private 化。**合計 37 フィールド
  が完全 private 化**に到達し、CreatureEntity のフィールドカプセル化
  は事実上最終形
- **提案 33**: ESP 系 15 個 + 装備集計系 19 個 + special_attack の合計
  35 個の BIT_FLAGS フィールドを private 化。`set_X(BIT_FLAGS)` setter,
  `get_X_flags()` getter (差分検出キャッシュ用), `add_special_attack(flag)`
  / `remove_special_attack(flag)` 等の compound assignment 用 virtual を
  整備し、約 70 箇所の write/read site を migration。**合計 private 化
  フィールド数: 37 → 72** (約 2 倍に到達)
- **提案 34**: 表示用既知値 5 個 (dis_to_h[2] / dis_to_h_b / dis_to_d[2]
  / dis_to_a / dis_ac) を private 化。`get_dis_X()` / `set_dis_X()`
  virtual を整備し、19 箇所の access site を migration。提案 30
  (to_h/d/a 本体) と対称な仕上げタスク。**合計 private 化フィールド数:
  72 → 77**
- **提案 35**: `is_player()` 分岐の削減を試みたが、コードベース調査
  の結果、機械的削減可能な冗長ガードは 1 サイトのみと判明 (現コード
  ベースの 83 サイトのうち約 63 は型契約を表す早期 return ガード、
  残りは真の振る舞い分岐)。`monster-attack-monster.cpp:365` の
  `if (creature.is_player() && ...) disturb(...)` を `disturb` 内部
  ガードに委ねる形に簡素化。さらなる削減は signature 変更
  (`bool foo(PlayerType &)`) が必要で別提案として扱う方針。
- **提案 41**: 呪文マスク 6 個 (spell_learned1/2 / spell_worked1/2 /
  spell_forgotten1/2) を private 化。realm_idx (0/1) と spell_id (0..31)
  ベースの virtual API 12 個を整備し、`is_realm1 ? *_1 : *_2` の
  三項演算子パターン 24 箇所を意図明示形 (`has_X_spell(realm, idx)`)
  に簡素化。savefile load/save の API も統一。**合計 private 化
  フィールド数: 77 → 83**
- **提案 39**: 装備派生キャッシュフィールド 11 個 (num_blow[2] / num_fire /
  to_m_chance / cur_lite / cumber_armor / cumber_glove / heavy_wield[2] /
  icky_wield[2] / icky_riding_wield[2] / riding_ryoute / monlite) を private
  化。get/set virtual 23 個を整備し、31 ファイルの read/write site を移行。
  メソッド名衝突回避のため `is_icky_wield[2]` → `icky_wield[2]`,
  `is_icky_riding_wield[2]` → `icky_riding_wield[2]` にフィールドをリネーム。
  `ac` は書込のみ `set_ac()` virtual に統一し、フィールドは public のまま
  残置 (io-dump で raw 値が JSON 出力されるため意味が異なる)。
  **合計 private 化フィールド数: 83 → 94**
- **提案 44**: 突然変異/呪い系フラグ 5 個 (`muta` / `trait` /
  `cursed` / `cursed_special` / `patron`) を private 化。23 個の virtual
  API (`has_mutation` / `add_mutation` / `remove_mutation` /
  `clear_mutations` / `set_mutations`、同様に trait / curse /
  curse_special) を整備し、29 ファイルで約 97 サイト
  (`muta` 52 + `trait` 2 + `cursed` 27 + `cursed_special` 4 +
  `patron` 12) の read/write を migration。`get_X_flags()` 系の
  const ref getter は savefile save 用に残置。**合計 private 化
  フィールド数: 94 → 99**
- **提案 40**: ペット関連フィールド 4 個 (`pet_extra_flags` /
  `pet_follow_distance` / `pet_t_m_idx` / `riding_t_m_idx`) を private
  化。11 個の virtual API (`has_pet_extra_flag` / `add_pet_extra_flag` /
  `remove_pet_extra_flag` / `get_pet_extra_flags` /
  `set_pet_extra_flags`、scalar 系の get/set) を整備し、19 ファイル
  約 102 サイトを migration。`(flags & MASK) != MASK` の全ビットセット
  セマンティクスは個別 `has_X()` 2 連検査に分解。さらにデッド
  フィールド `health_who` (宣言以外で未使用) を削除。
  **合計 private 化フィールド数: 99 → 103**
- **提案 42**: 旧差分検出キャッシュ `old_*` 系 12 個 (`old_lite` /
  `old_race1/2` / `old_realm` / `old_spells` / `old_cumber_armor/glove` /
  `old_heavy_wield[2]` / `old_heavy_shoot` / `old_icky_wield[2]` /
  `old_riding_wield[2]` / `old_riding_ryoute` / `old_monlite`) を private
  化。25 個の virtual API (`get_old_X()` / `set_old_X()`、bool 系は意図
  明示のため `was_X()` / `set_was_X()` 命名) を整備し、11 ファイル
  約 46 サイトを migration。さらにデッドフィールド `old_food_aux`
  (宣言以外で未使用) を削除。**合計 private 化フィールド数: 103 → 115**
- **提案 43**: 行動状態フラグ 14 個 (`action` / `running` / `resting` /
  `fired` (旧 `is_fired`) / `level_up_message` / `timewalk` / `now_damaged` /
  `playing` / `leaving` / `monk_notify_aux` / `teleport_town` / `yoiyami` /
  `sutemi` / `fishing_dir`) を private 化。28 個の virtual API (scalar 系は
  `get_X()` / `set_X()`、bool 系は意図明示のため `is_X()` / `has_X()` /
  `set_X()` 命名) を整備し、73 ファイル約 180 サイトを migration。
  メソッド名衝突回避のため `is_fired` フィールドを `fired` にリネーム。
  `.action` / `.running` / `.leaving` の他構造体での同名フィールドは慎重に
  除外。**合計 private 化フィールド数: 115 → 129**
- **提案 47**: その他小規模フィールド 6 個 (`dealt_damage` /
  `run_py` / `run_px` / `vanish_stairs_flag` / `suppress_multi_reward` /
  `tracking_bi_id`) を private 化。15 個の virtual API (scalar/bool 系の
  get/set、`dealt_damage` には `+=` パターン用 `add_dealt_damage(delta)`)
  を整備し、10 ファイル約 24 サイトを migration。さらにデッドフィールド
  `tval_xtra` (Unused、宣言以外で未使用) を削除。**合計 private 化
  フィールド数: 129 → 135**
- **提案 48**: 追加の小規模フィールド 6 個 (`tval_ammo` / `dtrap` /
  `autopick_autoregister` / `recall_dungeon` / `enchant_energy_need` /
  `energy_use`) を private 化。18 個の virtual API を整備し (ENERGY 系
  は compound assignment 用に `add_X/sub_X/mul_X/div_X` も整備)、25
  ファイル約 61 サイトを migration。`PlayerEnergy` ラッパークラスの
  内部実装も virtual API 経由に切替え、フィールド直接アクセスを完全
  排除。**合計 private 化フィールド数: 135 → 141**
- **提案 1/2**: プレイヤー専用フィールドのモンスター運用化基盤完了。
  種族 (`prace`) / 職業 (`pclass`) / 魔法領域 (`realm1` / `realm2` /
  `element_realm`) / パトロン (`patron`) / 変身形態 (`mimic_form`)
  の get/set virtual を追加。表示系の主要 access site 約 24 箇所を
  `creature.get_X()` 形式に migration。残約 177 直接 access は player
  専用 path で型契約済として残置 (機械的 sed の構造的価値が低いため)。
  モンスター個別運用 (種族や職業の付与) を実装する将来提案でこの
  基盤を利用可能。提案 2 (能力問合せ系 virtual の共通化) も提案 10/36
  / 30 / 31b / 32b / 33 等で実質完了。

今後の残作業としては、現在 `CreatureEntity` 直下に残存するプレイヤー
固有フィールド群（種族・職業・熟練度等）を、モンスターにも
共通で持たせて運用できる形に整備していく方針。プレイヤー専用構造体
（`PlayerProfile` のようなもの）に切り出してモンスターから隔離する
方向は取らない。

**残タスク詳細は [`docs/creature-entity-refactoring-roadmap.md`](docs/creature-entity-refactoring-roadmap.md) 参照。**
Phase 1-32b 完了後の継続提案（プレイヤー専用フィールドのクリーチャー
共通化、プレイヤー専用仮想メソッドの共通化、TimedEffects 二重管理
解消、戦闘ボーナス系の compound assignment 移行、その他フィールドの
read 側アクセサ化と private 化等）を同書で管理する。
新規の統合作業に着手する際は先に同書を参照し、作業完了後は同書と
本ファイルの両方に進捗を反映すること。

**移行しない（モンスター種族定義側に残す）フィールド:**

| フィールド | 残す場所 |
|---|---|
| `sub_align` | モンスター種族定義（`MonraceDefinition`）に近い概念 |
| `mtimed` | `MonsterProfile` 内 |
| `mflag`, `mflag2` | `MonsterProfile` 内 |
| `hold_o_idx_list` | `MonsterProfile` 内 |
| `alliance_idx` | `MonsterProfile` 内 |
| `smart` | `MonsterProfile` 内 |
| `parent_m_idx` | `MonsterProfile` 内 |
| `transform_r_idx` 等 | `MonsterProfile` 内 |

---

## bakabakaband 独自仕様 (CreatureEntity 統合派生)

### モンスター能力値補正 (`stat_modifiers`)

`MonraceDefinition` に `std::array<tl::optional<int>, A_MAX> stat_modifiers` を持ち、
JSON `lib/edit/MonraceDefinitions.jsonc` で個別モンスターの 6 能力値補正値を指定できる。

```jsonc
"stat_modifiers": { "STR": -3, "INT": -5, "WIS": -2, "DEX": -4, "CON": -1, "CHR": -2 }
```

各値は表示単位 (1 = +1.0 = 内部 10 単位)、`-40〜+40` の範囲。値が指定されない能力値は
`get_stats()` のロール結果をそのまま使う。指定されている能力値は `place_monster_one()`
内で chameleon 判定後の `new_monrace.stat_modifiers` を加算し、`stat_max/stat_cur/stat_use`
を更新（`stat_max_max` 必要に応じて拡張）、最終的に `[30, 400]` にクランプする。

街の `WILD_TOWN` フラグ付き `t` 系人間モンスター 44 体には負の補正値が一括付与済み
(`fewer monsters... but tougher` ではなく flavor 重視のバランス)。

### UNIQUE モンスターの個体名

UNIQUE フラグ付きモンスターは生成時に `creature.name = monrace.name.string()` で
種族名を個体名フィールドに自動セット。これにより:

- `CreatureEntity::name` が ペットのニックネームと UNIQUE の個体名の双方で同形管理
- `creature.is_named()` が UNIQUE でも true を返す (floor-leaver の保存判定で活用)
- `monster_desc()` は UNIQUE 表示が「X「X」」とならないよう、`monster.name == monrace.name`
  なら "called" 表記を抑止

### HP/MP 自然回復計算の統一

`compute_regen_amount(CreatureEntity &)` (`src/hpmp/hp-mp-regenerator.cpp`) が
プレイヤー基準の自然回復係数を算出する共通ヘルパ:

- `PY_REGEN_NORMAL` を起点に、満腹度・スタンス・呪い・ミュータント体質などは
  `is_player()` ガードで適用
- 再生種族フラグ・毒・切り傷・行動・地形衛生はプレイヤー/モンスター共通

`process_player_hp_mp` (10 ターン毎) と `regenerate_monsters` (100 ターン毎、内部で 10
スケールアップ) の両方で同じ計算式を使用。さらに `c` ステータス画面 (`display_player`)
の `ENTRY_HP_REGEN`/`ENTRY_MP_REGEN` 表示もモンスター inspect 時に同じ式で表示される。

### 最大HP算出の統一 (`maxhp` / `max_maxhp` の意味統一)

プレイヤーとモンスターの最大HP算出を観点ごとに `CreatureEntity` の共通メソッドへ
集約している。各補正は **プレイヤー (`update_max_hitpoints`) とモンスター生成
(`place_monster_one`) の両経路から同一メソッドを呼ぶ**ことで一致させる。

- `calc_max_hp_con_bonus()`: CON(耐久)補正 `(adj_con_mhp[idx] - 128) * level / 4`。
  索引は `stat_value_to_table_index()` で配列範囲にクランプ (モンスターは
  `stat_index[]` を計算しないため `stat_use` から都度算出)。
- `calc_min_max_hp()`: 最大HP下限 `level + 1`。
- `calc_max_hp_status_bonus()`: 一時状態補正 (英雄化/狂戦士化/つよし/呪術)。
  HEX はプレイヤー専用だが `SpellHex(monster)` は安全に常時 false。

**`maxhp` / `max_maxhp` の意味は以下に統一済み (プレイヤー・モンスター共通):**

| フィールド | 意味 |
|---|---|
| `max_maxhp` | 本来の最大HP (一時減少前のキャップ)。経験値計算・dealt_damage 上限・捕獲判定など一時減少に左右されるべきでない処理が参照。getter は `get_max_maxhp()` |
| `maxhp` | 現在の最大HP (一時減少を反映した実効値)。HP回復上限・被ダメージ・表示はこちら。getter は `get_max_hp()` |

旧来プレイヤーの `max_maxhp` は未設定 (0) だったが、本統一で
`update_max_hitpoints` が `set_max_hp(mhp)` を呼び **プレイヤーでも `max_maxhp` を
本来の最大HPとして維持**するようになった (プレイヤー専用 `on_take_hit` を持つため
従来も実害は無かったが、意味が一貫した)。

**拡張点 (将来の「最大HP一時減少」ステータス異常):**

- `get_maxhp_reduction()` (virtual, 既定 0): 一時的な最大HP減少量を返す拡張シーム。
- `set_max_hp(full)`: `max_maxhp = full` を確定し `refresh_max_hp()` を呼ぶ唯一の窓口。
- `refresh_max_hp()`: `maxhp = max(1, max_maxhp - get_maxhp_reduction())` を反映し、
  現在HPが超過していれば切り詰める。減少量が変化したら呼ぶ。

将来「最大HP一時減少」異常を追加する際は、減少量を `get_maxhp_reduction()` が
返すようにし (時限効果等から算出)、変化時に `refresh_max_hp()` を呼ぶだけで
プレイヤー・モンスター双方に反映される。`maxhp = max_maxhp` の直接代入
(polymorph/floor 変更/捕獲復元等の全回復リセット系) は現状 `reduction == 0` で
等価のため残置しているが、動的減少を導入する場合はこれらも `refresh_max_hp()`
経由に切り替えること。

#### レベル別HPテーブル (`hp_table[]`) と敵モンスターHPのテーブル式

プレイヤーとモンスターの基礎最大HPは `CreatureEntity::hp_table[PY_MAX_LEVEL]`
(旧 `player_hp`、レベル別累積HPテーブル) を共通の土台とする。

- `roll_hp_table()`: **プレイヤー (および `player_birth_as_monster` のモンスター運用)**
  用のローラー。Lv1 = `hit_dice.maxroll()` + 3 ロール、以降は各レベルで `hit_dice`
  を 1 回ずつ累積する前方加重型。`hit_dice` は職業・種族由来の per-level ダイス。
  最終レベルHPが期待値の 75〜125% に収まるまで振り直す。`roll_hitdice()` (spell)
  から呼ばれ、プレイヤー固有の UI 更新 (体力ランク / 再描画) はそちらに残す。
- `roll_monster_hp_table(force_max)`: **敵モンスター生成 (`place_monster_one`)** 用の
  ローラー。モンスター種族の `hit_dice` は「全HPの単発ロール」スケールなので、その
  per-level ダイス (1 レベルあたりHPダイス) を L レベル分累積して
  `hp_table[0..L-1]` を埋め `hp_table[L-1]` を返す。`L = get_level() =
  monrace.level/2`、`[1, PY_MAX_LEVEL]` にクランプ。FORCE_MAXHP は各レベル
  最大値で累積する。プレイヤー用 `roll_hp_table()` の前方加重 (Lv1 maxroll +
  3 ロール) は敵HPのスケールを膨らませるため**用いない**。

  per-level ダイスの決定 (提案: 段階移行 第 2 段):
  - **JSON 明示指定優先**: `MonraceDefinition::hit_dice_per_level`
    (JSON キー `"hit_point_per_level"`, 形式 `"ndY"`, 任意) が指定されていれば
    それをそのまま per-level ダイスに使う。アンバランスな個体の手動調節用。
  - **未指定時の既定値**: 旧 HP ダイス `hit_dice` (= `XdY`) から算出する。
    **面数は旧 `Y` を維持**し、ダイス数を期待値保存で較正する:
    `n = round(X / L)` (最低 `1dY`)。`E[n d Y] * L = X*(Y+1)/2` (旧単発ロール
    期待値) に一致させる狙い。

  - サイズ補正 (`is_huge()`/`is_large()` 等)・CON 補正 (`calc_max_hp_con_bonus()`)・
    状態補正 (`calc_max_hp_status_bonus()`)・nightmare 倍化・`MONSTER_MAXHP` クランプ
    は従来通りこの基礎HPの**後段で**乗算/加算する (一切変更なし)。
  - `calc_min_max_hp()` (= `level+1`) の下限クランプも従来通り後段で効く。

  **段階移行ステータス:**
  - **第 1 段 (完了)**: スケール保存=バランス不変で算出経路をテーブル式に統一。
    per-level ダイスは `1d s` (`s = round(X*(Y+1)/L) - 1`) を内部算出していた
    (全パイプライン検証で new/old 最終HP比 平均 1.00 / 中央値 1.00 /
    99.4% が ±10% 以内 / 1.3 倍超ゼロ)。
  - **第 2 段 (完了, 当節)**: per-level ダイスを JSON データ駆動化。既定値の
    面数を旧 `Y` に統一 (`n d Y`, `n = round(X/L)`) し、`"hit_point_per_level"`
    で個別上書き可能にした。**この既定式は第 1 段の `1d s` とは分布が異なり、
    面数 `Y` を維持する都合上、高 `Y`・低 `X` の個体 (例: `3d800` のドラゴン) は
    HP が膨張する** (全 2,300 種中 152 体が 1.5 倍超、最大 8 倍)。これらの
    アンバランス個体は `"hit_point_per_level"` で手動調節する方針。
  - **第 3 段 (完了, groundwork)**: モンスターのレベルアップに伴う HP 成長機構。
    `grow_hp_table_to_level(new_level)` が `hp_table[]` を生成時の添字より上へ
    (旧レベル→新レベルまで) per-level ダイスで累積し、基礎HP増分と CON 補正増分を
    現在の最大HPに**加算**する (生成時 1 回限りのサイズ補正乱数倍率は成長分には
    乗じない加算的成長)。per-level ダイスは `make_monster_per_level_die()` に共通化し、
    既定値の較正レベルを種族本来のレベル (`monrace.level/2`) で固定して反復成長でも
    ダイスが変動しないようにした。`set_level()` で実効レベルも更新する。
    **実際のレベルアップ発火は wizard デバッグコマンド `L`
    (`wiz_level_up_target_monster`) のみ**で、通常プレイのゲームバランスは不変。
    あわせて進化 (`monster_gain_exp`) 時の最大HP再計算も `roll_monster_hp_table()`
    のテーブル式に統一した (従来は単発ロールのままだった整合性修正)。
    モンスターの `hp_table[]` 自体は savefile 非保存だが、成長後の `max_maxhp` /
    `level` は creature-common で保存されるため成長結果は永続する。
  - **第 4 段 (完了, 実ゲーム発火・バランス変更)**: モンスターが戦闘で得た経験値で
    実際にレベルアップ (HP成長) するようにした。`monster_gain_exp()`
    (モンスターが他クリーチャーを撃破した時に発火) を拡張:
    - 従来 `next_exp == 0` (進化先なし) のモンスターは経験値を一切得なかったが、
      このゲートを進化処理の直前へ移し、**全モンスターが経験値を蓄積**するようにした。
    - 経験値加算後に `try_monster_level_up()` を呼ぶ。種族本来のレベル
      `base = monrace.level/2` を基準に、蓄積経験値が自種族の殺害時経験値
      `mexp` の整数倍に達するごとに 1 レベル成長し、`grow_hp_table_to_level()` で
      HP を伸ばす。**成長量は `base` レベル分まで** (実効レベル・HPは最大で約 2 倍)
      に制限して暴走を防ぐ。カメレオンは従来通り除外。
    - 進化を持つモンスターは進化前 (`exp < next_exp`) の間レベルアップし、進化時に
      `set_level(0)` で実効レベルを進化後種族の基準へリセットする (exp も 0 リセット)。
    - **ペース** (実データ約 2,200 種): 地下で同格を撃破し続けた場合、中央値で
      約 2 体ごとに +1 レベル、上限 (レベル 2 倍) 到達まで中央値約 17 体。主に
      ピット/増殖/召喚での同士討ち時に顕在化する有界な強化。
    - **チューニング**: 成長ペースは `try_monster_level_up()` の `exp_unit`
      (既定 `max(1, mexp)`) で調整可能。成長上限は `base_level` の係数で調整可能。
  - **今後の段**: モンスター `hp_table[]` の savefile 保存等。**注:** 当初
    「進行中の exp は非保存」としていたが、creature-common シリアライザ統合 (v52)
    により `exp` / `max_exp` / `max_max_exp` / `exp_frac` / `level` / `max_maxhp` は
    **既に保存済**（`creature-common-writer.cpp` 経由）。未保存はレベル別累積 HP の
    ロール履歴 `hp_table[]` のみで、`max_maxhp` / `level` が保存されるため成長結果
    自体は永続する（ロード後の再レベルアップは HP を再ロールするが最終値は同等）。

### `psex` の SEX_NONE

`player_sex::SEX_NONE = 4` (MAX_SEXES = 5) が追加され、`init_monster_profile()` で
モンスター生成途中の中間値として `psex = SEX_NONE` に明示初期化される。
`one_monster_placer` が `kind_flags` の MALE/FEMALE から確定値を上書きする。
`get_sex_info()` は範囲外の値を SEX_NONE にフォールバック。

`get_psex()` / `get_ppersonality()` は `CreatureEntity` の virtual アクセサ
(将来モンスター固有のオーバーライド余地)。

### モンスターの性格指定 (`personality`)

`MonraceDefinition` に `player_personality_type personality`
(`src/system/monrace/monrace-definition.h`) を持ち、JSON
`lib/edit/MonraceDefinitions.jsonc` で個別モンスターの性格を固定指定できる。

```jsonc
"personality": "LUCKY"
```

指定可能なトークンは `r_info_personality`
(`src/info-reader/race-info-tokens-table.cpp`) を参照:
`ORDINARY` / `MIGHTY` / `SHREWD` / `PIOUS` / `NIMBLE` / `FEARLESS` /
`COMBAT` / `LAZY` / `SEXY` / `LUCKY` / `PATIENT` / `MUNCHKIN` /
`CHARGEMAN` / `TOUGH` / `SUSHI_EATER` / `MESUGAKI`。

- 未指定 (キー省略 = `PERSONALITY_NONE`) の場合は従来通り。モンスター生成時は
  `CreatureEntity::assign_random_personality()` でランダム (いかさま除外)、
  プレイヤー運用開始時は対話選択。
- 指定がある場合は**常にその性格が適用**される。モンスター生成
  (`assign_random_personality()` 内の固定指定チェック) でも、プレイヤー運用
  開始 (`apply_monrace_personality()` で対話選択をスキップ) でも同一。
- 性格適用は `CreatureEntity::set_personality(player_personality_type)` に集約。

### モンスターの種族・職業指定 (`player_race` / `player_class`) — 提案 C1

`MonraceDefinition` に `PlayerRaceType player_race` / `PlayerClassType player_class`
(`src/system/monrace/monrace-definition.h`) を持ち、JSON
`lib/edit/MonraceDefinitions.jsonc` で個別モンスターにプレイヤー種族・職業を
固定指定できる。CreatureEntity 統合の C トラック（モンスターへのプレイヤー機能
付与）の第 1 弾で、**JSON オプトイン方式**（既定バランス不変・指定個体のみ）。

```jsonc
"player_race": "HIGH_ELF",
"player_class": "MAGE"
```

指定可能なトークンは `r_info_player_race` / `r_info_player_class`
(`src/info-reader/race-info-tokens-table.cpp`) を参照（`PlayerRaceType` /
`PlayerClassType` の enum 名がそのままトークン。例: 種族 `HUMAN` / `HIGH_ELF` /
`DRACONIAN`…、職業 `WARRIOR` / `MAGE` / `PRIEST`…）。

- 未指定 (キー省略 = `NONE`) の場合は従来通り `prace`/`pclass` は NONE のまま。
- 指定時はモンスター生成 (`place_monster_one`) 内で
  `CreatureEntity::assign_fixed_player_race_and_class()` が `prace`/`pclass` に付与する
  (chameleon 判定後の実効 monrace を参照)。
- **第1弾は効果未反映**（フィールド付与のみ。種族耐性・職業特典等の戦闘効果は
  反映しない）。効果反映は C トラックのバランス判断のもと段階導入する方針。
- スキーマ `schema/MonraceDefinitions.schema.json` に `player_race`/`player_class`
  を登録済（CI の JSON 検証を通す）。

### モンスターの種族属性耐性の反映 (`applies_player_race_resistances`) — 提案 C1第2弾

`MonraceDefinition` に `bool applies_player_race_resistances`
(`src/system/monrace/monrace-definition.h`) を持ち、JSON
`lib/edit/MonraceDefinitions.jsonc` で「付与された `player_race`（C1）の属性耐性を
被ダメージへ反映する」を有効化できる。C1 第1弾（種族付与・効果なし）に対し、
**耐性という最初の戦闘効果**を JSON オプトイン方式（既定 `false`=無効でバランス不変）で
導入したもの。

```jsonc
"player_race": "HIGH_ELF",
"applies_player_race_resistances": true
```

- **反映対象（第2弾: 基本 5 属性）:** 火 `TR_RES_FIRE` / 冷 `TR_RES_COLD` /
  電 `TR_RES_ELEC` / 酸 `TR_RES_ACID` / 毒 `TR_RES_POIS`。付与種族の
  `CreatureRace::tr_flags()` に当該耐性があれば、`effect-monster-resist-hurt.cpp` の
  各属性ハンドラで被ダメージを**約 1/3 に軽減**（プレイヤーの部分耐性と同水準）。
- **反映対象（第3弾: 二次属性 7 種）:** 地獄 `TR_RES_NETHER` / 混沌 `TR_RES_CHAOS` /
  破片 `TR_RES_SHARDS` / 轟音 `TR_RES_SOUND` / 混乱 `TR_RES_CONF` /
  劣化 `TR_RES_DISEN` / 因果 `TR_RES_NEXUS`。各ハンドラの**ネイティブ耐性経路へ
  OR-in** する形で反映（種族耐性でネイティブと同じ軽減・副作用抑止を発火）。
  轟音は `do_stun`、混乱は `do_conf`、混沌は polymorph/混乱の**状態異常も抑止**する。
  種族由来耐性では monrace ネイティブ耐性の思い出フラグ (`r_resistance_flags`) を
  記録しない（`native_resist` ガード）。付随攻撃（rocket/icee/void/abyss 等の
  複合・特殊ロジック）は対象外（将来拡張余地）。
- **配線箇所:** `target_race_resists_element(em_ptr, tr_type)` /
  `apply_monster_race_resistance(em_ptr)`（基本 5 属性用の 1/3 軽減）（
  `effect-monster-resist-hurt.cpp` の匿名 namespace）。基本属性は免疫 (`IMMUNE_*`)
  優先・弱点 (`HURT_*`) と排他 (`else if`)。毒は D7 の DoT 蓄積 (`dam/2`) より前に
  軽減を適用するため継続毒も減る。二次属性は各ハンドラのネイティブ resist 条件へ
  `|| target_race_resists_element(...)` を OR-in。
- **安全性:** `prace == NONE` は `false` を返すため、耐性反映は「有効な種族を持つ
  個体」に限定され OOB 等は起きない。既定 `false` のため誰にも反映されず
  **既定バランス完全不変**。
- **未反映:** 職業特典・種族の非耐性特典（ESP・赤外線視等）は対象外（将来段階）。
  軽減率（現状 1/3）はハンドラの `apply_monster_race_resistance` で調整可能。
- スキーマに `applies_player_race_resistances` を登録済。

### モンスターの能力値成長 (`grows_stats`) — 提案 C2

`MonraceDefinition` に `bool grows_stats`
(`src/system/monrace/monrace-definition.h`) を持ち、JSON
`lib/edit/MonraceDefinitions.jsonc` で個別モンスターのレベルアップ時能力値成長を
有効化できる。C トラック第 2 弾で、**JSON オプトイン方式**（既定 `false`=無効で
バランス不変）。

```jsonc
"grows_stats": true
```

- モンスターのレベルアップ（`try_monster_level_up()`、戦闘で経験値蓄積時）に
  `grows_stats` が立つ個体のみ、獲得レベル数に応じて
  `CreatureEntity::grow_stats_by_levels()` が 6 能力値を成長させる。
- 成長量は保守的な既定値 `stat_growth_per_level = 2`（内部 10 単位 = 表示 0.1/レベル、
  `src/system/creature-entity.cpp`）。獲得レベル数は基準レベル上限で有界のため成長も
  有界。バランス調整はこの定数で行う。
- HP 成長（第 4 段）は従来通り全モンスター共通（`grows_stats` 非依存）。本フラグは
  **能力値成長のみ**を制御する。
- **現状の能力値のゲーム効果は限定的**（生成時の CON→最大HP 補正・一部判定）。
  能力値を戦闘へ広く反映する拡張は将来提案で段階導入する。
- スキーマに `grows_stats` を登録済。

### モンスターの MP 消費詠唱 (`consumes_mp`) — 提案 C4

`MonraceDefinition` に `bool consumes_mp`
(`src/system/monrace/monrace-definition.h`) を持ち、JSON
`lib/edit/MonraceDefinitions.jsonc` で個別モンスターの呪文詠唱に MP 消費を
課せる。C トラック第 3 弾で、**JSON オプトイン方式**（既定 `false`=無効で
バランス不変）。

```jsonc
"consumes_mp": true
```

- `make_attack_spell()`（`src/mspell/mspell-attack.cpp`）で、`consumes_mp` が立つ
  個体は詠唱前に **レベル比例の保守的コスト** `max(1, rlev / mp_cost_divisor)`
  （既定 `mp_cost_divisor = 10`）を要求する。
- **MP 不足時はその手番の詠唱をスキップ**して近接等にフォールする（全呪文一律
  コストのため、支払えなければどの呪文も使えない）。
- 詠唱が成立した場合のみ `sub_current_mp()` で消費する。モンスターは生成時に
  満タンの MP（`calc_creature_mana()`）と自然回復を持つため、MP は詠唱を無制限に
  許さない緩やかな制限として働く。
- バランス調整は `mp_cost_divisor` 定数で行う（小さいほど高コスト）。
- 既定 OFF のため実データ・既定バランスは不変。スキーマに `consumes_mp` を登録済。

### モンスターの突然変異 (`mutations`) — 提案 C5

`MonraceDefinition` に `EnumClassFlagGroup<PlayerMutationType> mutations`
(`src/system/monrace/monrace-definition.h`) を持ち、JSON
`lib/edit/MonraceDefinitions.jsonc` で個別モンスターに突然変異を付与できる。
C トラック第 4 弾で、**JSON オプトイン方式**（既定=なし=バランス不変）。

```jsonc
"mutations": [ "BERSERK", "REGEN" ]
```

指定可能なトークンは `r_info_mutation`
(`src/info-reader/race-info-tokens-table.cpp`、`PlayerMutationType` の enum 名) を参照。

- 生成時に `CreatureEntity::assign_fixed_mutations()`（`place_monster_one`）が
  `monrace.mutations` を `add_mutation` で付与する。
- per-turn 処理は**プレイヤー用 `process_world_aux_mutation()` とは分離した専用関数
  `process_monster_mutation(player, monster)`**（`src/mutation/mutation-processor.cpp`）
  が担う。巨大なプレイヤー版（UI プロンプト・脱糞・`BadStatusSetter` の徳/構え副作用等
  深いプレイヤー結合）は改造せず、モンスターに意味のある能動変異のみ curated 実装:
  **BERS_RAGE**（激怒→恐怖解除+加速）/ **COWARDICE**（恐怖）/ **RTELEPORT**
  （テレポート）/ **SPEED_FLUX**（速度変動）。効果は `set_monster_monfear/fast/slow` /
  `teleport_away` 等モンスター安全なプリミティブで適用し、メッセージは視認時のみ。
- 発火は `process_world()`（`TURNS_PER_TICK`=10 ゲームターン周期）のプレイヤー変異処理
  直後に変異持ちモンスターを走査して行うため、発動確率はプレイヤー版と同一周期で整合。
- **未対応の変異は付与しても per-turn では発火しない**（上記 4 種以外）。受動変異
  （耐性・ESP 等）の反映は将来拡張。スキーマに `mutations` を登録済。

### モンスターの魔法領域詠唱 (`realm_abilities`) — 提案 C6

`MonraceDefinition` に `RealmType realm_abilities`
(`src/system/monrace/monrace-definition.h`) を持ち、JSON
`lib/edit/MonraceDefinitions.jsonc` で個別モンスターに魔法領域由来の詠唱能力を
付与できる。C トラック第 5 弾で、**JSON オプトイン方式**（既定 `NONE`=無効で
バランス不変）。

```jsonc
"realm_abilities": "CHAOS"
```

指定可能なトークンは `r_info_realm`（`RealmType` の enum 名。LIFE / SORCERY /
NATURE / CHAOS / DEATH / TRUMP / ARCANE / CRAFT / DAEMON / CRUSADE / MUSIC /
HISSATSU / HEX）を参照。

- **橋渡し方式（メンテナ選択: 案A realm→ability マッピング）:** プレイヤーの
  `exe_spell()` は `get_aim_dir` 等の UI プロンプトを直呼びしヘッドレス不可のため、
  realm 呪文を対応する `MonsterAbilityType` に写像し、**既存 mspell 経路**
  （自動ターゲット・MP消費(C4)・耐性・smart AI）で詠唱させる。
- **非破壊・race-level 尊重:** モンスター能力は race 単位 (`monrace.ability_flags`)
  で mspell に読まれるため、恒久的に monrace を書き換えず、詠唱文脈
  (`msa_type` 構築時) の `ability_flags` にのみ realm 由来能力を **OR-in** する
  (`src/mspell/mspell-attack-util.cpp` の `add_realm_granted_abilities()`)。
- 写像表は保守的な初期セット（10 魔法領域。MUSIC/HISSATSU/HEX は未マッピング）で、
  バランス調整・拡張はこの表で行う。
- 実際に撃たせるには当該 monrace に `freq_spell > 0`（詠唱頻度）が必要。
- 既定 `NONE` のため実データ・既定バランスは不変。スキーマに `realm_abilities` を登録済。

### モンスターの継続毒 (`suffers_poison_dot`) — 提案 D7

`MonraceDefinition` に `bool suffers_poison_dot`
(`src/system/monrace/monrace-definition.h`) を持ち、JSON
`lib/edit/MonraceDefinitions.jsonc` で個別モンスターが毒攻撃で継続毒 (POISON DoT) を
受けるようにできる。D トラック（処理同化）で判明した「モンスターは POISON を持てても
per-turn で発火しない（切り傷・毒の inflict 経路が無い）」ギャップを、**JSON オプトイン
方式**（既定 `false`=無効でバランス不変）の小機能として埋めたもの。

```jsonc
"suffers_poison_dot": true
```

- **inflict:** `effect_monster_pois`（`src/effect/effect-monster-resist-hurt.cpp`）で、
  毒免疫でなく `suffers_poison_dot` が立つ個体は毒攻撃を受けると `POISON` タイマーを
  `max(1, dam/2)` 蓄積する。
- **tick:** `process_world()`（`TURNS_PER_TICK`=10 ゲームターン周期、プレイヤーの毒 DoT
  と同一）で `POISON>0` のモンスターに 1/ターンの毒ダメージを `MonsterDamageProcessor`
  (`AttributeType::POIS`) で与え、`POISON` を 1 減らす。無敵中はスキップ。死亡時は
  ドロップ・経験値も正規経路で処理。
- 既定 OFF のため誰にも POISON が蓄積せず**既定バランス不変**。蓄積量 (`dam/2`)・
  tick 量 (1) で調整可能。cut DoT は近接由来で inflict 経路が別のため今回は poison のみ。
- スキーマに `suffers_poison_dot` を登録済。

### モンスターのレベル別HPダイス指定 (`hit_point_per_level`)

`MonraceDefinition` に `Dice hit_dice_per_level`
(`src/system/monrace/monrace-definition.h`) を持ち、JSON
`lib/edit/MonraceDefinitions.jsonc` で「1 レベルあたりのHPダイス」を任意指定できる。

```jsonc
"hit_point": "3d800",        // 旧来の全HP単発ロール用ダイス (必須)
"hit_point_per_level": "1d40" // レベル別HPテーブル用 1 レベルあたりダイス (任意)
```

- 敵モンスターの最大HPは `roll_monster_hp_table()` がこの per-level ダイスを
  実効レベル `L`(= `monrace.level/2`) 分累積して算出する (詳細は「最大HP算出の
  統一」節)。
- **未指定時の既定値**は `hit_point` (`XdY`) から自動算出する: 面数は旧 `Y` を
  維持し、ダイス数を `n = round(X/L)` (最低 `1dY`) と較正する。
- この既定式は面数 `Y` を維持する都合上、**高 `Y`・低 `X` の個体 (例: `3d800`)
  ではHPが膨張する** (約 152 体が 1.5 倍超)。バランス調整が必要な個体に
  `hit_point_per_level` を明示指定して上書きする。

### セーブ/ロードの統合 (CreatureEntity 共通シリアライズ) — フェーズ 1〜4

旧 PlayerType / 旧 MonsterEntity に分かれていたセーブ/ロード処理を、
`CreatureEntity` 基底フィールド単位で 1 箇所に統合していく方針。
**セーブデータバージョンは 54** に更新済み (フェーズ1=50 / 2=51 / 3=52 / 4=53 /
E3=54)。v54 で `prace` / `pclass` を共通ブロック (`wr_creature_common`) へ集約
(従来プレイヤー writer=byte / モンスター writer=s16b の二重記述を s16b に統一。
旧バージョンは各固有経路の `older_than(54)` ガードで読む)。

- **フェーズ4 で全時限効果 (`timed_effects_map`) を共通化済み**:
  `wr_creature_common()` は `CreatureTimedEffect` を列挙順に全 (約 56) ダンプし、
  `rd_creature_common()` は v53 以降この全ダンプを読む (v52 以前は共通 7 種のみ)。
  これにより旧フォーマットに存在した時限効果の順序不整合
  (writer/reader で TIM_RES_* 等の順序が食い違い、バイト数だけ整合して値が
  ずれていたバグ) を v53 で解消。プレイヤー固有経路 (`wr_player` /
  `rd_bad_status` / `rd_status` / `set_timed_effects` / `rd_player_status` の
  インライン / `rd_special_attack` / `rd_special_defense`) の約 49 個の個別
  時限効果書込/読込を削除・`older_than(53)` ガード化し、時限効果と交互配置
  されていた非時限フィールド (food / recall_dungeon / infravision / mimic_form
  / special_attack / special_defense 等) のみ残置。
  **注意: `CreatureTimedEffect` enum に値を追加するとダンプ件数が変わるため、
  必ずセーブデータバージョンを更新すること** (列挙順依存のため中間挿入は避け、
  末尾 `MAX` 直前への追加が無難)。

- 共通基底シリアライザ:
  - 書込: `wr_creature_common(const CreatureEntity &)` (`src/save/creature-common-writer.{h,cpp}`)
  - 読込: `rd_creature_common(CreatureEntity &)` (`src/load/creature-common-loader.{h,cpp}`)
  - 対象フィールド (v50/v51 基本): `name` / 座標 (`y`,`x`) / `hp`/`maxhp`/`max_maxhp` /
    `dealt_damage` / `speed` / `energy_need` / `ac` / `exp` / `au` /
    `ht`/`wt` / `target` / 共通時限効果 7 種 (SLEEP_OR_PARALYSIS /
    ACCELERATION / DECELERATION / STUN / CONFUSION / FEAR /
    INVULNERABILITY) / `materials` (材質)。プレイヤー様式の明示フォーマット。
  - 対象フィールド (v52 拡張): `level` / `age` / `hp_frac` / `msp` / `csp` /
    `csp_frac` / `max_exp` / `max_max_exp` / `exp_frac` / 能力値配列
    (`stat_max`/`stat_max_max`/`stat_cur` 各 6)。
  - 対象フィールド (v54 拡張): `prace` / `pclass` (符号付き `s16b`。C1 で
    モンスターにも付与可能になった共通フィールド。従来プレイヤー=byte /
    モンスター=s16b の二重記述を共通ブロックへ集約)。
  - **`rd_creature_common()` は内部でバージョン分岐する**: v52 拡張分は
    `loading_savefile_version_is_older_than(52)`、v54 拡張分 (`prace`/`pclass`) は
    `older_than(54)` で囲み、旧セーブでは読み飛ばす。これにより**呼び出し側
    (モンスター reader 等) は変更不要**で v50〜v54 を自動的に正しく読める
    (拡張時はこの 1 箇所だけ直せばよい)。
- **モンスター経路は統合済み**: `MonsterWriter::write_to_savedata()` は
  `wr_creature_common()` + モンスター固有フィールド (r_idx / ap_r_idx /
  alliance / sub_align / smart / mflag2 / parent / transform / prace /
  pclass / インベントリ) を書く。旧ビットマスク方式
  (`SaveDataMonsterFlagType` / `write_monster_flags` / `write_monster_info`)
  は廃止。
- **プレイヤー経路も統合済み (フェーズ2)**: `wr_player()` は先頭で
  `wr_creature_common()` を呼び、共通基底フィールドを集約する。以降の
  プレイヤー固有フィールドは共通フィールドを除いて従来の相対順序を保つ。
  ロード側 (`player-info-loader.cpp`) は v51 以降で `rd_base_info()` 先頭の
  `rd_creature_common()` で共通基底を読み、各共通フィールドの旧読込箇所
  (name / ht / wt / au / exp / maxhp / hp / dealt_damage / energy_need /
  CONFUSION / ACCELERATION / DECELERATION / FEAR / STUN / INVULNERABILITY の
  15 箇所) を `loading_savefile_version_is_older_than(51)` でガードして
  v50 以前のみ読む。プレイヤー固有フィールドの相対順序は不変。
  ロード側の v52 拡張フィールドの旧個別読込箇所 (age / stats / max_exp /
  max_max_exp / exp_frac / level / hp_frac / msp / csp / csp_frac) は
  `loading_savefile_version_is_older_than(52)` でガードして v51 以前のみ読む。
- **モンスターフォーマットは v50/v51/v52 を `rd_monster_v50()` 1 本で読む**。
  共通ブロックの拡張は `rd_creature_common()` 内部のバージョン分岐で吸収される
  ため、`rd_monster()` の分岐は `older_than(50)` のままでよい。モンスターも
  v52 から能力値配列・MP 等を保存・復元するようになった (従来は未保存)。
- **後方互換**: v49 以前のモンスターは `rd_monster_legacy()`、v50 以降は
  `rd_monster_v50()`。プレイヤーは v50 以前が旧個別読込、v51 以降が
  `rd_creature_common()` + ガード済み個別読込。
- 書込/読込は完全対称が前提。共通ブロックのフィールド追加・順序変更時は
  writer/loader 双方 (および利用する全経路) を同時更新し、必要なら
  バージョンを再度更新すること。`prace`/`pclass` は `NONE` (-1) を取り得る
  ため符号付き `s16b` で保存する (byte 保存は -1 が 255 になり復元不能)。
  共通ブロック拡張時は `wr_creature_common()` に末尾追加し、
  `rd_creature_common()` に対応するバージョンガード付き読込を追加するだけで
  全経路 (プレイヤー・モンスター) に反映される。
- **残タスク (後続フェーズ)**: 共通化の対象はほぼ完了。残るプレイヤー固有
  シリアライズ (スキル経験値・職業固有データ・徳・インシデント・領域・
  突然変異/特性のフラグ群等) は本質的にプレイヤー専用であり、共通基底には
  含めない方針。今後モンスターにも持たせたいフィールドが出てきた場合に
  個別に `wr_creature_common()` へ移行する。

---

## 開発フロー

### PR の粒度

1 PR = 1 つの論理的な変更。例：
- `「do_cmd_xxx 系関数の引数を CreatureEntity & に変更」`
- `「is_confused() を CreatureEntity の仮想メソッド化」`

### ブランチ命名

`refactor/` プレフィックスを使用：
- `refactor/creature-entity-status-check`
- `refactor/creature-entity-timed-effects`
- `refactor/creature-entity-damage-unified`

### ビルド確認

ローグライクの性質上、自動テストは困難なため変更後はゲームを起動して基本動作を確認すること。

CI と同等のチェックをローカルで実行するためのスクリプトを `.github/scripts/` に用意している。
スクリプトは Linux 系シェル（`sh`）で実行すること。

#### clang-format 整形チェック

GitHub Actions の `check_format` ジョブと同等の確認を行う。

```bash
sh .github/scripts/ci-check-format.sh
```

- `clang-format-18` が必要。未インストールの場合は `sudo apt-get install clang-format-18`
  （上流 PR #4215 で clang-format-15 から 18 へ移行。スタイル定義はリポジトリ
  ルートの `.clang-format` を参照する）
- 整形が必要なファイルがある場合は差分を表示して終了コード 1 を返す

#### autotools ビルドテスト

GitHub Actions の `build_test_japanese` ジョブと同等のビルドを行う。

```bash
# デフォルト（g++、-Werror -Wall -Wextra、--disable-pch）
sh .github/scripts/ci-build-test.sh

# コンパイラ・フラグを変更する場合
sh .github/scripts/ci-build-test.sh --cxx clang++-15 --cxxflags "-pipe -O3 -Werror -Wall -Wextra -stdlib=libc++"

# 英語版ビルド
sh .github/scripts/ci-build-test.sh --configure-opts "--disable-pch --disable-japanese"
```

- 依存パッケージ: `sudo apt-get install libncursesw5-dev libcurl4-openssl-dev nkf libc++-15-dev libc++abi-15-dev`
- 内部で `./bootstrap` → `./configure` → `make -j$(nproc)` を順に実行する

#### Windows (MSVC) ビルド確認

MSVC ビルドは `.github/workflows/build-test-with-msvc.yml` を参照。
ローカルでは VisualStudio ソリューションから Debug ビルドを実行して確認する：

```powershell
MSBuild -warnAsError .\VisualStudio\Bakabakaband.sln /t:Rebuild /p:Configuration=Debug
```

---

## 変愚蛮怒（上流）からのマージ指針

### 背景

bakabakaband は変愚蛮怒（Hengband, https://github.com/hengband/hengband ）をベースにした派生作品であり、
歴史的に上流の修正・機能追加を手動チェリーピックで取り込んできた。しかし近年の
CreatureEntity 統合リファクタリング（`PlayerType *` → `CreatureEntity &`、`p_ptr` 廃止、
`MonsterEntity` 吸収、時限効果の enum 統一等）により **直接のチェリーピックは衝突が
多発し不可能** となっている。

本節では、変愚蛮怒の更新を継続的に取り込むための標準手順を定義する。

### セットアップ：上流リモートの追加

変愚蛮怒を `hengband` という名前のリモートとして追加する（初回のみ）。

```bash
git remote add hengband https://github.com/hengband/hengband.git
git remote set-url --push hengband DISABLED  # 誤 push 防止
git fetch hengband
```

確認:

```bash
git remote -v
# origin   https://github.com/deskull-m/bakabakaband.git (fetch)
# origin   https://github.com/deskull-m/bakabakaband.git (push)
# hengband https://github.com/hengband/hengband.git (fetch)
# hengband DISABLED (push)
```

### 定期取得

```bash
git fetch hengband
```

主要ブランチは `hengband/develop`。リリース済みバージョンは `hengband/master` を参照。

### マージ対象の特定

マージ対象となる ISSUE は、タイトルに「変愚「...」のマージ」を含むものが中心。
GitHub 検索で列挙できる：

```
is:issue is:open 変愚 マージ in:title
```

各 ISSUE 本文に上流 PR 番号（例: `#5323`）が記載されているので、それを基に
対応する上流コミットを特定する。

```bash
# 上流 PR に含まれるコミット範囲を取得
git log hengband/develop --grep="#5323" --oneline
# または上流 PR のマージコミットから辿る
git log hengband/develop --oneline --first-parent | grep 'Merge pull request #5323'
```

### マージ手順

#### Step 1: 上流コミット範囲の特定

上流 PR のマージコミットを見つけ、そのマージコミットに含まれる実コミット群を確認する。

```bash
# マージコミットを特定
MERGE_SHA=$(git log hengband/develop --first-parent --grep="pull request #5323" --format=%H | head -1)
# マージに含まれるコミット一覧
git log $MERGE_SHA^..$MERGE_SHA^2 --oneline
```

#### Step 2: 作業ブランチの作成

マージ対象ごとに専用ブランチを切る。命名規則は `merge/hengband-<PR番号>-<要約>`。

```bash
git checkout -b merge/hengband-5323-terrain-flag-rename
```

#### Step 3: チェリーピック試行（競合前提）

コミットを 1 つずつチェリーピックする。衝突は前提なので `--no-commit` で停止させて
差分を精査する方針が望ましい。

```bash
git cherry-pick --no-commit <commit-sha>
# 衝突発生 → 手動で差分吸収（下記 Step 4）
```

競合なくピックできる低リスクな変更（jsonc データファイル、リソース、ドキュメント等）は
そのまま通常のチェリーピックでよい：

```bash
git cherry-pick <commit-sha>
```

#### Step 4: 差分吸収（リファクタリング由来の衝突解決）

bakabakaband 側と上流側でよくある構造的差異を以下のルールで機械的に置換する。
この作業は Claude Code に以下を指示して代行させられる：

**指示例:**
> 以下のパッチを bakabakaband の構造に合わせて適応してください。
> `PlayerType *player_ptr` → `CreatureEntity &creature`、`p_ptr` → `PlayerType::get_instance()`、
> `player_ptr->hero` → `creature.get_timed_effect(CreatureTimedEffect::HERO)` 等の置換を
> 実施し、最終的にコンパイル可能な形にしてください。

**主要な変換マッピング**:

| 上流（変愚蛮怒） | bakabakaband |
|---|---|
| `PlayerType *player_ptr` 引数 | `CreatureEntity &creature` |
| `player_ptr->field` | `creature.field` |
| `p_ptr->field` | `PlayerType::get_instance().field` |
| `p_ptr` / `*p_ptr` 引数 | `PlayerType::get_instance()` |
| `MonsterEntity *m_ptr = &floor.m_list[idx]` | `auto &monster = floor.m_list[idx]`（`CreatureEntity &`） |
| `m_ptr->field` | `monster.field` |
| `MonsterEntity` の直接使用 | `CreatureEntity`（大抵の場面で互換） |
| `player_ptr->current_floor_ptr` | `creature.get_floor()` |
| `player_ptr->hero`, `player_ptr->blessed`, `player_ptr->invuln` 等の直接 TIME_EFFECT フィールド | `creature.get_timed_effect(CreatureTimedEffect::HERO)` 等 |
| `player_ptr->hero = v` 等の代入 | `creature.set_timed_effect(CreatureTimedEffect::HERO, v)` |
| `MonsterTimedEffect::SLEEP` | `CreatureTimedEffect::SLEEP_OR_PARALYSIS` |
| `MonsterTimedEffect::FAST` | `CreatureTimedEffect::ACCELERATION` |
| `MonsterTimedEffect::SLOW` | `CreatureTimedEffect::DECELERATION` |
| `MonsterTimedEffect::STUN/CONFUSION/FEAR/INVULNERABILITY` | 同名 `CreatureTimedEffect::XXX`（`MonsterTimedEffect` 自体は削除済み） |
| `MONSTER_TIMED_EFFECT_RANGE` | `MONSTER_TIMED_EFFECT_LIST` |
| `player_ptr->mimic_form` | `creature.get_mimic_form()` / `set_mimic_form()` |
| `is_hero(player_ptr)` 等の古い自由関数 | `creature.is_hero()` 等の仮想メソッド |
| `take_hit(player_ptr, ...)` | `take_hit(creature, ...)` |
| `mon_take_hit_mon(...)` | `MonsterDamageProcessor(creature, ...).mon_take_hit(...)` |
| `has_resist_fire(creature)` 等の自由関数呼出 | `creature.has_resist_fire()` 等の virtual メンバ |
| `has_immune_fire(creature)` / `has_vuln_fire(creature)` | `creature.has_immune_fire()` / `creature.has_vuln_fire()` |
| `has_pass_wall(creature)` / `has_two_handed_weapons(creature)` 等の装備集計 | `creature.has_pass_wall()` / `creature.has_two_handed_weapons()` 等 |
| `has_reflect(creature)` / `has_sh_fire(creature)` / `has_down_saving(creature)` 等 | `creature.has_reflect()` / `creature.has_sh_fire()` / `creature.has_down_saving()` 等 |
| `creature.muta.has(X)` / `creature.cursed.has(X)` 等の構造体メンバ直接 | `creature.get_mutations().has(X)` / `creature.get_cursed_flags().has(X)` 等の virtual アクセサ |
| `creature.race->X` / `(*creature.personality).X` / `(*creature.pclass_ref).X` | `creature.get_race_info()->X` / `(*creature.get_personality_info()).X` / `(*creature.get_class_info()).X` |
| `creature.skill_sav` / `creature.skill_dis` 等の直接読み取り | `creature.get_skill_save()` / `creature.get_skill_disarm()` 等 |
| `creature.see_infra` の直接読み取り | `creature.get_infravision()` |
| `creature.telepathy` / `creature.esp_*` の直接読み取り | `creature.has_telepathy()` / `creature.has_esp_*()` (代入は直接フィールドのまま) |
| `creature.see_inv` / `creature.can_swim` / `creature.levitation` の直接読み取り | `creature.can_see_invisible()` / `creature.has_can_swim()` / `creature.has_levitation()` |
| `creature.free_act` / `creature.anti_magic` / `creature.anti_tele` 等の BIT_FLAGS 読み取り | `creature.has_free_act()` / `creature.has_anti_magic()` / `creature.has_anti_tele()` 等 |
| `creature.regenerate` / `creature.hold_exp` / `creature.slow_digest` 等 | `creature.has_regen_flag()` / `creature.has_hold_exp()` / `creature.has_slow_digest_flag()` 等 (フィールド値読取り、`_flag` 等のサフィックス注意) |
| `creature.lite` / `creature.warning` / `creature.impact` / `creature.earthquake` の読み取り | `creature.has_lite_flag()` / `creature.has_warning_flag()` / `creature.has_impact_flag()` / `creature.has_earthquake_flag()` (FLAG_CAUSE_* ビットマスクとして使う場合は直接フィールドのまま残置) |
| `creature.see_nocto` / `creature.dec_mana` / `creature.easy_spell` 等 | `creature.has_see_nocto()` / `creature.has_dec_mana()` / `creature.has_easy_spell()` 等 |
| `creature.special_attack & ATTACK_XXX` | `creature.has_special_attack(ATTACK_XXX)` (代入は直接フィールドのまま) |
| `creature.special_defense & DEFENSE_XXX` | `creature.has_special_defense(DEFENSE_XXX)` |
| `creature.effects()->stun().current()` 等の値読取り | `creature.get_timed_effect(CreatureTimedEffect::STUN)` 等 (HALLUCINATION/CUT/POISON も enum に追加済み) |
| `creature.effects()->stun().reset()` の値クリア | `creature.set_timed_effect(CreatureTimedEffect::STUN, 0)` |
| `creature.effects()->cut().is_cut()` / `effects()->poison().is_poisoned()` | `creature.is_cut()` / `creature.is_poisoned()` (Phase 2 系の virtual) |
| `PlayerType::get_timed_effect()` / `set_timed_effect()` のオーバーライド | bakabakaband 側では基底クラスに分岐ロジック統一済みのため不要 (PlayerType オーバーライドは廃止) |
| `DungeonFeatureType::BIG` | `DungeonFeatureType::LARGEST` (上流 PR #5360 で改名済) |
| `small_levels` / `empty_levels` / `always_small_levels` / `ironman_small_levels` / `ironman_empty_levels` | `allow_smallest_floor` / `allow_arena_floor` / `always_small_floor` / `ironman_smallest_floor` / `ironman_force_arena_floor` (上流 PR #5360 91177e87f) |
| `GameOption` の旧 `(set, bits)` 引数 | `GameOption` の `GameOptionType::XXX` 列挙 (上流 PR #5363/#5364 で導入。bakabakaband 独自 `MONSTER_TOMBSTONES` (66) / `IRONMAN_ALLIANCE_HOSTILITY` (211) 追加済) |
| `misc_to_attr[256]` / `misc_to_char[256]` 個別配列 | `misc_to_display_symbol[256]` (DisplaySymbol) → 後に `ds_bolt[256]` に改名 (上流 PR #5352) |
| `ItemEntity *ref_item(...)` | `std::shared_ptr<ItemEntity> ref_item(...)` (上流 PR #5339) |
| `ItemEntity *choose_object(...)` | `std::pair<std::shared_ptr<ItemEntity>, short> choose_item(...)` (上流 PR #5339; 関数名も改名) |
| `wield_slot(creature, ItemEntity *)` | `wield_slot(creature, const ItemEntity &)` (上流 PR #5339) |
| `ae_type::o_ptr` (ItemEntity *) | `ae_type::item` (`std::shared_ptr<ItemEntity>`) + コンストラクタ + `decide_activation_level()` メソッド (上流 PR #5339) |
| `cosmic_cast_off(creature, ItemEntity **)` | `cosmic_cast_off(creature, const ItemEntity &)` 戻り値 `std::shared_ptr<ItemEntity>` (上流 PR #5342) |
| `curse_weapon_object(creature, bool, ItemEntity *)` / `enchant_equipment(ItemEntity *, ...)` | 引数を `ItemEntity &` 参照に変更 (上流 PR #5342) |
| `switch_activation(..., ItemEntity **, ...)` 戻り値 `bool` | `switch_activation(..., const ItemEntity &, ...)` 戻り値 `std::pair<bool, std::shared_ptr<ItemEntity>>` (上流 PR #5342) |
| `activate_artifact(creature, ItemEntity *)` 戻り値あり | `activate_artifact(creature, std::shared_ptr<ItemEntity> &)` 戻り値廃止 (上流 PR #5342) |
| `concptr ANGBAND_SYS/KEYBOARD/GRAF` | `std::string_view ANGBAND_SYS/KEYBOARD/GRAF` (上流 PR #5354) |
| `monster.get_monster_profile().ml` | `monster.is_visible_on_map()` (読取り) / `monster.set_visible_on_map(bool)` (書込み) (フェーズ 7) |
| `creature.get_monster_profile().alliance_idx` / `.sub_align` / `.parent_m_idx` / `.smart` / `.hold_o_idx_list` 読取り | `creature.get_alliance_idx()` / `get_sub_align()` / `get_parent_m_idx()` / `get_smart_flags()` (read-only) (フェーズ 9) |
| `creature.spell_exp[idx]` / `creature.skill_exp[key]` / `creature.weapon_exp[tval][sval]` 読取り | `creature.get_spell_exp(idx)` / `get_skill_exp(skill)` / `get_weapon_exp(tval, sval)` (フェーズ 10) |
| `MonsterProfile::hold_o_idx_list` / `ItemEntity::held_m_idx` 経由のモンスター所持 | `monster.inventory[INVEN_TOTAL]` 直接、`monster.store_item(item)` / `monster.acquire_item(item)` / `monster.drop_all_inventory(dropper)` (フェーズ A 完了で廃止) |
| `monster_drop_carried_objects(creature, monster)` | `monster.drop_all_inventory(creature)` (提案 17 で free function ラッパ削除) |
| `store_item_to_inventory(creature, item_ptr)` | `creature.store_item(item)` (提案 17 で OO 形式に統一)。free function は inventory-object.cpp 内部のみ残置 |
| `check_store_item_to_inventory(creature, item_ptr)` | `creature.can_store_item(item)` (提案 17) |
| `creature.effects()->protection().current()` | `creature.get_timed_effect(CreatureTimedEffect::PROTECTION)` (PlayerProtection も統一 API 経由) |
| 個別ターゲット選定の m_list 走査ループ | `creature.find_nearest_creature(predicate, require_projectable)` / `creature.has_visible_creature(predicate)` / `creature.collect_creatures(predicate)` (提案 14 / 14b) |
| `monster.get_monster_profile().mflag2.has(MonsterConstantFlagType::X)` 読取り | `monster.has_constant_flag(...)` あるいは個別 virtual `is_kage()` / `is_chameleon()` / `is_huge()` / `is_large()` / `is_small()` / `is_fat()` / `is_gaunt()` / `is_lightweight()` / `is_naked()` / `is_zombified()` / `is_illegal_modified()` / `is_santa()` / `is_angered()` / `is_waifuized()` / `is_quylthlug_born()` / `is_defecated()` / `is_vomited()` / `has_noflow()` / `is_nogeno()` 等 (提案 15 / 15b) |
| `monster.get_monster_profile().mflag2.set(...)` / `.reset(...)` 書込 | `monster.set_constant_flag(flag)` / `monster.reset_constant_flag(flag)` / 初期化リスト版 `set_constant_flags({...})` / `reset_constant_flags({...})` (提案 18) |
| `monster.get_monster_profile().mflag2 = bits` 一括代入 | `monster.set_all_constant_flags(bits)` / 取得は `get_all_constant_flags()` (提案 20)。savefile 復元での bool 代入は `assign_constant_flag(flag, bool)` |
| `monster.get_monster_profile().mflag.has(...)` 読取 / `.set(...)` 書込 | `monster.has_temporary_flag(flag)` / `set_temporary_flag(flag)` / `reset_temporary_flag(flag)`、個別 virtual `is_in_view()` / `is_marked_for_los()` / `is_sensed_by_esp()` / `was_present_at_turn_start()` / `has_prevent_magic()` / `has_sanity_blast()` (提案 16) |
| `monster.get_monster_profile().mflag.clear()` / `mflag2.clear()` 一括クリア | `clear_temporary_flags()` / `clear_constant_flags()` (提案 20) |
| `monster.get_monster_profile().alliance_idx = X` / `.sub_align = X` / `.parent_m_idx = X` 書込 | `set_alliance_idx(X)` / `set_sub_align(X)` / `add_sub_align(mask)` / `set_parent_m_idx(X)` (提案 9b) |
| `monster.get_monster_profile().smart.set(...)` / `.clear()` | `monster.add_smart_flag(flag)` / `monster.clear_smart_flags()` (提案 9b) |
| `monster.get_monster_profile().transform_r_idx = X` / `.transform_hp_threshold = X` / `.has_transformed = X` / `.death_count = X` | `set_transform_r_idx(X)` / `set_transform_hp_threshold(X)` / `set_has_transformed(X)` / `set_death_count(X)`、減算は `decrement_death_count()`、読取は `get_transform_r_idx()` / `get_transform_hp_threshold()` / `has_transformed()` / `get_death_count()` (提案 19) |
| `monster.r_idx = X` / `monster.ap_r_idx = X` 書込 | `monster.set_r_idx(X)` / `monster.set_ap_r_idx(X)`、両方同期は `monster.polymorph_to(X)` (提案 22) |
| `monster.r_idx == X` / `monster.ap_r_idx` 読取り (`m_ptr->r_idx` 含む) | `monster.get_r_idx() == X` / `monster.get_ap_r_idx()` / `m_ptr->get_r_idx()` 等の getter 経由 (提案 28 / 28b) |
| `creature.riding = X` 書込 (compaction/floor 切替等の付替え) | `creature.set_riding(X)` (提案 22)。通常の騎乗開始/終了は `creature.ride_monster(X)` |
| `creature.riding == 0` / `creature.riding` 読取り (`m_ptr->riding` 含む) | `creature.get_riding() == 0` / `creature.get_riding()` 経由 (提案 28 / 28b) |
| `MonsterProfile` 直接アクセス (`get_monster_profile().X`) | **提案 21 でメンバ private 化済み。** CreatureEntity の virtual API 経由でのみアクセス可能。例外は `friend` 宣言された `CreatureEntity` 自身、`MonsterLoader50`、`MonsterWriter` |
| `creature.age = X` / `creature.ht = X` / `creature.wt = X` / `creature.prestige = X` 書込 | `creature.set_age(X)` / `set_ht(X)` / `set_wt(X)` / `set_prestige(X)`、加算は `add_age(d)` / `add_prestige(d)`、`prestige /= N` は `divide_prestige(N)` (提案 24) |
| `creature.inven_cnt` / `creature.equip_cnt` フィールド | **提案 25 でフィールド廃止。** `creature.get_inven_cnt()` / `creature.get_equip_cnt()` を呼ぶ。インベントリ変更時の cnt 同期は不要 (inventory[] から自動計算) |
| `creature.ambush_flag = X` / `creature.food = X` / `creature.town_num = X` / `creature.level = X` 書込 | `creature.set_ambush_flag(X)` / `set_food(X)` / `set_town_num(X)` / `set_level(X)` (提案 26) |
| `creature.max_plv = X` / `creature.msp = X` 書込 | `creature.set_max_plv(X)` / `creature.set_max_mp(X)` (提案 27、msp は提案 6 で `max_mp` に改名) |
| `creature.exp = X` / `creature.max_exp = X` / `creature.max_max_exp = X` 書込 | `creature.set_exp(X)` / `set_max_exp(X)` / `set_max_max_exp(X)` (提案 27) |
| `creature.au [+\-/]= X` / `creature.csp [+\-]= X` の compound assignment | `creature.set_au(X)` / `add_au(X)` / `sub_au(X)` / `divide_au(X)`、`set_current_mp(X)` / `add_current_mp(X)` / `sub_current_mp(X)` (提案 27b。csp は提案 6 で `current_mp` に改名)。約 110 箇所の `+=` / `-=` を移行済み |
| `creature.csp` / `creature.msp` / `creature.csp_frac` 読取り (上流の MP フィールド) | `creature.get_current_mp()` / `creature.get_max_mp()` / `current_mp_frac` (提案 6 でフィールドを `current_mp` / `max_mp` / `current_mp_frac` に改名。get/set/add/sub も `*_current_mp` / `*_max_mp` 系へ。**上流マージ時は `csp`→`current_mp`、`msp`→`max_mp` の置換が必要**) |
| `creature.num_blow[hand]` / `creature.num_fire` / `creature.to_m_chance` / `creature.cur_lite` 読取り | `creature.get_num_blow(hand)` / `get_num_fire()` / `get_to_m_chance()` / `get_cur_lite()` (提案 39) |
| `creature.num_blow[hand] = X` / `num_fire = X` 等の書込 | `creature.set_num_blow(hand, X)` / `set_num_fire(X)` / `set_to_m_chance(X)` / `set_cur_lite(X)` (提案 39) |
| `creature.cumber_armor` / `cumber_glove` 読取り | `creature.is_cumber_armor()` / `is_cumber_glove()` (提案 39) |
| `creature.heavy_wield[hand]` / `is_icky_wield[hand]` / `is_icky_riding_wield[hand]` 読取り | `creature.is_heavy_wield(hand)` / `is_icky_wield(hand)` / `is_icky_riding_wield(hand)` (提案 39) |
| `creature.riding_ryoute` / `creature.monlite` 読取り | `creature.is_riding_ryoute()` / `creature.is_monlite()` (提案 39) |
| `creature.is_icky_wield[hand]` / `is_icky_riding_wield[hand]` フィールド | **提案 39 でフィールド名を `icky_wield[hand]` / `icky_riding_wield[hand]` にリネーム。** メソッドからの読取りは `is_X(hand)` 経由のみ |
| `creature.ac = X` / `m_ptr->ac = X` 書込 | `creature.set_ac(X)` (提案 39)。`ac` フィールドの直接読取りは public 残置 (io-dump 用) |
| `creature.muta.has(X)` / `creature.trait.has(X)` 読取 | `creature.has_mutation(X)` / `creature.has_trait(X)` (提案 44) |
| `creature.muta.set(X)` / `creature.muta.reset(X)` / `creature.muta.clear()` 書込 | `creature.add_mutation(X)` / `creature.remove_mutation(X)` / `creature.clear_mutations()` (提案 44)。trait 系も同様 (`add_trait` / `remove_trait` / `clear_traits`) |
| `creature.muta = flags` / `creature.trait = flags` 一括代入 | `creature.set_mutations(flags)` / `creature.set_traits(flags)` (提案 44。savefile load 用) |
| `creature.cursed.has(X)` / `creature.cursed_special.has(X)` 読取 | `creature.has_curse(X)` / `creature.has_curse_special(X)` (提案 44) |
| `creature.cursed.set(CurseTraitType::X)` 単一フラグ | `creature.add_curse(CurseTraitType::X)` (提案 44) |
| `creature.cursed.set(obj_curse_flags)` 一括 OR-in | `creature.add_curses(obj_curse_flags)` (提案 44。bulk OR-in 専用) |
| `creature.cursed.reset(X)` / `creature.cursed.clear()` 書込 | `creature.remove_curse(X)` / `creature.clear_curses()` (提案 44)。cursed_special 系も同様 |
| `creature.cursed = flags` 一括代入 | `creature.set_curses(flags)` (提案 44。savefile load 用) |
| `creature.patron` 読取 / `creature.patron = X` 書込 | `creature.get_patron()` / `creature.set_patron(X)` (提案 44。Birther 等の他構造体の `patron` フィールドは別物で migration 対象外) |
| `creature.pet_extra_flags & PF_X` / `any_bits(creature.pet_extra_flags, PF_X)` 読取 | `creature.has_pet_extra_flag(PF_X)` (提案 40) |
| `creature.pet_extra_flags \|= PF_X` 書込 | `creature.add_pet_extra_flag(PF_X)` (提案 40) |
| `creature.pet_extra_flags &= ~PF_X` 書込 | `creature.remove_pet_extra_flag(PF_X)` (提案 40) |
| `creature.pet_extra_flags = value` 一括代入 | `creature.set_pet_extra_flags(value)` (提案 40。savefile load 用) |
| `creature.pet_extra_flags` 全体読取 | `creature.get_pet_extra_flags()` (提案 40。savefile save 用) |
| `(creature.pet_extra_flags & MASK) != MASK` (全ビットセット判定) | `!creature.has_pet_extra_flag(A) \|\| !creature.has_pet_extra_flag(B)` 等の個別判定に分解 (提案 40) |
| `creature.pet_follow_distance` / `creature.pet_t_m_idx` / `creature.riding_t_m_idx` 読取 | `creature.get_pet_follow_distance()` / `get_pet_t_m_idx()` / `get_riding_t_m_idx()` (提案 40) |
| 同上書込 | `creature.set_pet_follow_distance(X)` / `set_pet_t_m_idx(X)` / `set_riding_t_m_idx(X)` (提案 40) |
| `creature.health_who` | **提案 40 でフィールド削除** (デッドフィールドだった) |
| `creature.old_lite` / `old_race1/2` / `old_realm` / `old_spells` 読取 | `creature.get_old_lite()` / `get_old_race_flags1/2()` / `get_old_realm()` / `get_old_spells()` (提案 42) |
| 同上書込 | `creature.set_old_lite(X)` / `set_old_race_flags1/2(X)` / `set_old_realm(X)` / `set_old_spells(X)` (提案 42) |
| `creature.old_cumber_armor` / `old_cumber_glove` / `old_heavy_shoot` / `old_riding_ryoute` / `old_monlite` 読取 | `creature.was_cumber_armor()` / `was_cumber_glove()` / `was_heavy_shoot()` / `was_riding_ryoute()` / `was_monlite()` (提案 42) |
| `creature.old_heavy_wield[hand]` / `old_icky_wield[hand]` / `old_riding_wield[hand]` 読取 | `creature.was_heavy_wield(hand)` / `was_icky_wield(hand)` / `was_icky_riding_wield(hand)` (提案 42) |
| 同上書込 | `creature.set_was_heavy_wield(hand, X)` / `set_was_icky_wield(hand, X)` / `set_was_icky_riding_wield(hand, X)` (提案 42)、scalar bool は `set_was_X(value)` |
| `creature.old_realm \|= 1U << X` / `old_race1 \|= ...` 単一ビット追加 | `creature.set_old_realm(creature.get_old_realm() \| (1U << X))` (提案 42。意味的な単純化のため raw 操作を保持) |
| `creature.old_food_aux` | **提案 42 でフィールド削除** (デッドフィールドだった) |
| `creature.action` 読取 / `creature.action = X` 書込 | `creature.get_action()` / `creature.set_action(X)` (提案 43)。`.action` を持つ他構造体 (TerrainCharacteristics / autopick / cmd / DungeonDefinition 等) は別物で migration 対象外 |
| `creature.running` / `creature.resting` 読取 / 書込 | `creature.get_running()` / `set_running(X)` / `get_resting()` / `set_resting(X)` (提案 43)。`++` / `--` は `set_X(get_X() ± 1)` 形式に展開 |
| `creature.is_fired` 読取 / 書込 | `creature.is_fired()` / `creature.set_is_fired(X)` (提案 43。**フィールド名を `is_fired` → `fired` にリネーム**) |
| `creature.timewalk` / `creature.now_damaged` / `creature.playing` / `creature.leaving` / `creature.teleport_town` / `creature.sutemi` 読取 | `creature.is_timewalking()` / `is_now_damaged()` / `is_playing()` / `is_leaving()` / `is_teleport_town()` / `is_sutemi()` (提案 43) |
| 同上書込 | `creature.set_timewalking(X)` / `set_now_damaged(X)` / `set_playing(X)` / `set_leaving(X)` / `set_teleport_town(X)` / `set_sutemi(X)` (提案 43) |
| `creature.level_up_message` 読取 / 書込 | `creature.has_level_up_message()` / `set_level_up_message(X)` (提案 43) |
| `creature.monk_notify_aux` / `creature.fishing_dir` / `creature.yoiyami` 読取 / 書込 | `creature.get_monk_notify_aux()` / `get_fishing_dir()` / `get_yoiyami()` / `set_X(value)` (提案 43)。yoiyami の `\|=` は `set_yoiyami(get_yoiyami() \| X)` |

**GCC 固有の注意**:
上流は MSVC 前提のことが多く、`<cstdint>` 等のインクルード漏れがあれば追加する。

#### Step 5: ビルド・フォーマット確認

```bash
sh .github/scripts/ci-check-format.sh
sh .github/scripts/ci-build-test.sh
```

エラーが出たら Step 4 に戻り、変換漏れを補修する。

#### Step 6: コミット

チェリーピック元のコミットメッセージを保ちつつ、bakabakaband 側の改変内容を明記する。

```bash
git commit -m "$(cat <<'EOF'
merge: [上流タイトル]（hengband#5323 相当）

変愚蛮怒 PR #5323 の内容を bakabakaband 構造に適応してマージ。

上流コミット: abc1234 (hengband/develop)
主な型変換:
- PlayerType *player_ptr → CreatureEntity &creature
- TIME_EFFECT 直接フィールド → get/set_timed_effect()
EOF
)"
```

#### Step 7: ISSUE への報告・クローズ

対応する ISSUE（例: #8166）にコメントして PR を作成、マージ後に ISSUE をクローズする。

### よくある落とし穴

1. **`MonsterEntity` 残存**: 上流では `MonsterEntity` クラスが存在するが bakabakaband では廃止済み。
   フィールド数や継承関係に差があるため、`CreatureEntity` への置換時にフィールド名・
   アクセス方法が変わっていないか要確認。

2. **`get_floor()` と `current_floor_ptr` の混在**: 上流は直接メンバ参照、bakabakaband は
   アクセサ必須。`.current_floor_ptr` を `.get_floor()` に、`*creature.current_floor_ptr` を
   `*creature.get_floor()` に変換する。

3. **直接 TIME_EFFECT フィールドの廃止**: 上流の
   `creature.hero` / `creature.invuln` / `creature.oppose_fire` 等の
   直接フィールド、および `creature.word_recall` / `creature.alter_reality`
   は bakabakaband では削除済み。全て `creature.get_timed_effect(...)` /
   `creature.set_timed_effect(...)` 経由で扱うこと。ストレージは
   `CreatureEntity::timed_effects_map` に集約されている。

4. **`MonsterTimedEffect` enum および `MonsterProfile::mtimed` の廃止**:
   上流にはこれらがあるが bakabakaband では削除済み。`CreatureTimedEffect`
   に統合されており、プレイヤー・モンスターとも `timed_effects_map`
   を共有する。

5. **グローバル `p_ptr` の廃止**: bakabakaband では `extern PlayerType *p_ptr` が存在しない。
   `PlayerType::get_instance()` に置換する必要がある。

6. **状態チェック自由関数の virtual メンバ化**: `has_resist_*(creature)` /
   `has_immune_*(creature)` / `has_vuln_*(creature)` / `has_pass_wall(creature)` 等の
   引数 1 個の状態チェック自由関数は bakabakaband では `creature.has_resist_X()` の
   ような virtual メンバに変換済み。自由関数自体は player-status-flags.cpp に残存し
   委譲に使われているため動作はするが、新規 call site は virtual メンバ形式で書くこと。
   モンスター時は MonraceDefinition の resistance_flags / feature_flags / misc_flags /
   brightness_flags 由来の値を返すため、プレイヤーと同じ呼出形でモンスター情報も取れる。

7. **構造体メンバへの直接アクセスから virtual アクセサへ**:
   - `creature.muta.has(X)` → `creature.get_mutations().has(X)`
   - `creature.cursed.has(X)` → `creature.get_cursed_flags().has(X)`
   - `creature.race->X` → `creature.get_race_info()->X`
   - `(*creature.personality).X` → `(*creature.get_personality_info()).X`
   - `(*creature.pclass_ref).X` → `(*creature.get_class_info()).X`

   読み取り箇所は virtual アクセサ経由が標準。書き込み (`creature.muta.set(X)` 等) は
   フィールド直接のままで OK。

8. **スキル・赤外線視・テレパシー・特殊攻撃防御の読取りは virtual 経由**:
   `creature.skill_sav` / `creature.see_infra` / `creature.telepathy` /
   `creature.special_attack & FLAG` 等の読取りは
   `creature.get_skill_save()` / `creature.get_infravision()` /
   `creature.has_telepathy()` / `creature.has_special_attack(FLAG)` に変換済み。
   **提案 33 完了で telepathy / esp_* / 装備集計 BIT_FLAGS / special_attack
   の書き込みも `set_X(BIT_FLAGS)` / `add_special_attack(flag)` /
   `remove_special_attack(flag)` virtual 経由に統一済み。**
   フィールド直接アクセスは private 化されており不可。

9. **PlayerType::get_timed_effect() オーバーライドの廃止**: 上流では PlayerType が
   この virtual を override して TimedEffects オブジェクト経由でアクセスしているが、
   bakabakaband では基底クラス CreatureEntity::get_timed_effect() に分岐ロジック
   (TimedEffects オブジェクト優先 / フォールバック map) を集約済みで、PlayerType
   オーバーライドは削除されている。上流からこのオーバーライドを取り込もうとする
   差分は無視するか基底クラス側で吸収すること。

10. **HALLUCINATION/CUT/POISON が CreatureTimedEffect に追加済み**:
    上流ではこれらは TimedEffects オブジェクト経由のみだが、bakabakaband では
    `CreatureTimedEffect` enum に追加されており、`get/set_timed_effect()` 統一 API
    でも扱える。マージ時は `effects()->cut().current()` 等の単純呼出は
    `get_timed_effect(CreatureTimedEffect::CUT)` に置換可能。

### 差分吸収を Claude Code に委譲する場合のプロンプト雛形

```
以下の変愚蛮怒パッチを bakabakaband にマージしてください:
<パッチ内容または cherry-pick -n 後の git diff>

変換ルール（CLAUDE.md の「変愚蛮怒（上流）からのマージ指針」節を参照）:
- PlayerType * → CreatureEntity &
- p_ptr → PlayerType::get_instance()
- TIME_EFFECT 直接フィールド → get/set_timed_effect(CreatureTimedEffect::XXX)
- current_floor_ptr → get_floor() / set_floor()
- MonsterTimedEffect → CreatureTimedEffect
- mimic_form → get_mimic_form() / set_mimic_form()
- has_resist_X(creature) → creature.has_resist_X() (耐性/免疫/弱点系全 31 種)
- has_pass_wall/has_two_handed_weapons/has_reflect/has_sh_*/has_down_saving 等の
  装備集計系 → creature.has_X() (10 種)
- creature.muta.has(X)/.cursed.has(X) → creature.get_mutations().has(X) /
  get_cursed_flags().has(X) など (読取りのみ)
- creature.race->X / (*creature.personality).X → creature.get_race_info()->X /
  (*creature.get_personality_info()).X
- creature.skill_X / creature.see_infra → creature.get_skill_X() /
  creature.get_infravision()
- creature.telepathy/esp_X/see_inv/can_swim/levitation 等の読取り →
  creature.has_telepathy()/has_esp_X()/can_see_invisible()/has_can_swim()/
  has_levitation() 等。**書込みは creature.set_telepathy(BIT_FLAGS) /
  set_esp_X(BIT_FLAGS) / set_can_swim(bool) / set_levitation(BIT_FLAGS)
  virtual 経由 (提案 33 完了)。** 差分検出キャッシュ用 read は
  `creature.get_telepathy_flags()` / `get_esp_X_flags()` / `get_see_inv_flags()`
  / `get_mighty_throw_flags()` / `get_impact_flags()` /
  `get_earthquake_flags()` 経由
- creature.special_attack & FLAG → creature.has_special_attack(FLAG)
  / 書込みは `add_special_attack(flag)` / `remove_special_attack(flag)`
  / `set_special_attack_flags(BIT_FLAGS)` / `get_special_attack_flags()`
  経由 (提案 33 完了)。`creature.special_attack |= ATTACK_X` は
  `creature.add_special_attack(ATTACK_X)` に変換
- creature.effects()->X().current()/set/reset → creature.get/set_timed_effect(...)
  (提案 5 完了で effects() API 自体が削除済み)
- creature.effects()->stun().get_rank() / get_magic_chance_penalty 等 →
  PlayerStun::get_rank(creature.get_timed_effect(STUN)) / get_magic_chance_penalty(...)
- creature.effects()->cut().get_accumulation/get_damage/get_expr →
  PlayerCut::get_accumulation/get_damage/get_expr (static)
- HALLUCINATION/CUT/POISON も CreatureTimedEffect 経由 OK

作業後:
1. 変更ファイルをコンパイルチェック
2. clang-format 適用
3. 完全ビルド確認（sh .github/scripts/ci-build-test.sh）
4. 変換内容を要約した上でコミット
```

### マージ対象の優先順位

変愚マージ関連 ISSUE が多数溜まっているので、以下の順で優先する：

1. **バグ修正**: `[Fix]` プレフィックスの ISSUE。小さい差分で影響が限定的。
2. **データファイル更新**: jsonc、効果音、タイル等。ロジック変更を伴わないもの。
3. **小さな機能追加**: モンスター追加、アイテム追加等。
4. **リファクタリング**: `[Refactor]` プレフィックス。bakabakaband 側の差異吸収コストが高い。
5. **大規模機能**: HTTP 通信、X11 BGM 等。依存が多いものは最後。

### マージ作業前の適用可否判断

ISSUE の中には、以下のような理由で**マージ作業が不要**または**困難**なものが含まれている。
Claude Code が ISSUE を処理する際は、着手前に必ず以下の観点で可否を判定し、
該当する場合は**作業を止めてユーザーに指示を仰ぐこと**。

#### 作業不要と判断すべきケース

1. **既に修正済み**: 上流 PR の内容が別経路（独自実装、別マージ、リファクタリング時の
   副次的対応）で既に bakabakaband に反映されている。
   - 判定方法: 上流 PR の主要変更点が bakabakaband 現行コードに存在するか grep で確認。

2. **対象機能が bakabakaband に存在しない**: 上流が持つ機能を bakabakaband は実装していない、
   または既に削除している（例: 特定のプレイヤークラス、特定の UI モード）。
   - 判定方法: 変更対象ファイル/シンボルが bakabakaband に存在するか確認。

3. **bakabakaband の方針と矛盾する**: 上流の仕様変更が bakabakaband 独自の設計方針と
   衝突する（例: バランス調整、ゲームシステム変更）。
   - 判定方法: ISSUE の議論履歴を確認し、過去に方針差があった分野かをチェック。

4. **時代遅れ / 意味を失った**: 修正対象のコード自体が bakabakaband のリファクタリングで
   大幅に書き換えられており、上流パッチがそもそも適用対象外。
   - 判定方法: 上流 PR の変更ファイル・関数が bakabakaband に存在するか確認。

#### 作業困難と判断すべきケース

1. **差分吸収が大規模**: 上流 PR の変更点が bakabakaband の構造と大きく乖離し、
   機械的な変換マッピングでは対応できない（例: 根本的なクラス階層差異）。

2. **依存する上流変更が未マージ**: 対象 PR が別の未マージ PR に依存している場合、
   先にそちらを処理する必要があるか、まとめて対応する必要がある。

3. **判断が必要な設計選択**: 上流が複数の実装選択肢から 1 つを選んでいる場合、
   bakabakaband でも同じ選択で良いか人間判断が必要。

#### 判定時の報告フォーマット

ユーザーに指示を仰ぐ際は、以下の情報を整理して提示すること：

```markdown
## ISSUE #XXXX 判定結果: [作業不要 / 作業困難 / 要判断]

### 上流 PR 概要
- リポジトリ: hengband/hengband#YYYY
- 変更ファイル数: N
- 主な変更点: ...

### bakabakaband 側の現状
- 該当コードの存在: [ある / ない / 大幅に変更済み]
- 既存の対応: ...

### 判定理由
...

### 推奨アクション
- [ ] ISSUE クローズ（作業不要のため）
- [ ] 別 ISSUE に統合
- [ ] 保留（上流側の議論待ち等）
- [ ] ユーザー判断による作業継続
```

#### ISSUE 処理フロー全体

```
1. ISSUE 本文から上流 PR 番号を特定
   ↓
2. git log hengband/develop で該当コミット群を取得
   ↓
3. 適用可否を判定（上記基準）
   ├─ 作業不要/困難 → ユーザーに報告・指示待ち
   └─ 適用可能 → 次へ
   ↓
4. 作業ブランチ作成
   ↓
5. cherry-pick --no-commit で試行
   ↓
6. 衝突解決（変換マッピング適用）
   ↓
7. ビルド確認 → コミット → プッシュ
   ↓
8. ISSUE にコメント・クローズ
```

