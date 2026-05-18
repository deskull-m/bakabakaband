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
- MP: `csp`, `msp`, `csp_frac`
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

### `psex` の SEX_NONE

`player_sex::SEX_NONE = 4` (MAX_SEXES = 5) が追加され、`init_monster_profile()` で
モンスター生成途中の中間値として `psex = SEX_NONE` に明示初期化される。
`one_monster_placer` が `kind_flags` の MALE/FEMALE から確定値を上書きする。
`get_sex_info()` は範囲外の値を SEX_NONE にフォールバック。

`get_psex()` / `get_ppersonality()` は `CreatureEntity` の virtual アクセサ
(将来モンスター固有のオーバーライド余地)。

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

- `clang-format-15` が必要。未インストールの場合は `sudo apt-get install clang-format-15`
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
| `creature.max_plv = X` / `creature.msp = X` 書込 | `creature.set_max_plv(X)` / `creature.set_msp(X)` (提案 27) |
| `creature.exp = X` / `creature.max_exp = X` / `creature.max_max_exp = X` 書込 | `creature.set_exp(X)` / `set_max_exp(X)` / `set_max_max_exp(X)` (提案 27) |
| `creature.au [+\-/]= X` / `creature.csp [+\-]= X` の compound assignment | `creature.set_au(X)` / `add_au(X)` / `sub_au(X)` / `divide_au(X)`、`set_csp(X)` / `add_csp(X)` / `sub_csp(X)` (提案 27b)。約 110 箇所の `+=` / `-=` を移行済み |

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

