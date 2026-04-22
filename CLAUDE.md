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
（`std::map<CreatureTimedEffect, TIME_EFFECT>`）に集約済み。

- 共通列挙型: `CreatureTimedEffect`（`src/system/creature-timed-effect-types.h`）
- 共通 API: `get_timed_effect(effect)` / `set_timed_effect(effect, value)`
  (virtual, プレイヤー側は特定効果のみ `TimedEffects` オブジェクト経由)
- プレイヤーの旧 41 個の直接 `TIME_EFFECT` フィールド (`hero` / `invuln` 等)
  および `word_recall` / `alter_reality` は削除済み
- モンスター側の旧 `MonsterProfile::mtimed` も削除済み
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
         hold_o_idx_list 等)
```

今後の残作業としては、プレイヤー固有データを明示的に `PlayerProfile`
構造体に分離し `CreatureEntity::player_profile` として保持させる形に
するかを検討（現状はプレイヤー固有フィールドも `CreatureEntity` 直下に残存）。

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

