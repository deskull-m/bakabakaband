# CLAUDE.md — bakabakaband 開発ガイド

## プロジェクト概要

bakabakaband は Hengband をベースにした日本語ローグライクゲーム（C++）。
現在の最重要課題は **PlayerType / MonsterEntity の CreatureEntity への統合リファクタリング**。

---

## CreatureEntity 統合リファクタリング

### 目的

プレイヤー（`PlayerType`）とモンスター（`MonsterEntity`）を共通の `CreatureEntity` スーパークラスで扱えるようにし、ダメージ処理・状態効果・行動 AI などのロジックを両者で共通化する。

**最終目標:** `MonsterEntity` を `CreatureEntity` に完全吸収し、モンスター固有データは別の軽量構造体（`MonsterProfile` 的な位置づけ）に分離する。将来的にはモンスターも `CreatureEntity` 単体で表現できる状態を目指す。

### クラス階層

```
CreatureEntity  (基底クラス)  src/system/creature-entity.h
├── PlayerType               src/system/player-type-definition.h
└── MonsterEntity            src/system/monster-entity.h
```

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

### 型キャスト

型の分岐が必要な場合は `is_player()` で判定してからキャストする：

```cpp
if (creature.is_player()) {
    auto &player = static_cast<PlayerType &>(creature);
    // プレイヤー固有の処理
} else {
    auto &monster = static_cast<MonsterEntity &>(creature);
    // モンスター固有の処理
}
```

キャストは最小限に。可能な限りバーチャルメソッドで処理すること。

---

## 残タスク ロードマップ

### Phase 1: 関数シグネチャの統一（継続中）

`PlayerType *player_ptr` を引数に取る関数を `CreatureEntity &creature` に変更する。

**手順:**
1. `grep -r "PlayerType \*" src/` で対象を列挙
2. 関数の呼び出し元を確認
3. `PlayerType *player_ptr` → `CreatureEntity &creature` に変更
4. 内部の `player_ptr->` アクセスを `creature.` に変更
5. `PlayerType` 固有機能が必要な箇所のみキャストを使用
6. PR は機能単位（例：`do_cmd_xxx` 系、`spell_xxx` 系）でまとめる

### Phase 2: 状態チェックの仮想化

`MonsterEntity` には `is_confused()`, `is_stunned()`, `is_fearful()`, `is_invulnerable()` 等が実装済み。
これらを `CreatureEntity` の仮想メソッドとして定義し、`PlayerType` にも実装することで両者で共通に扱えるようにする。

```cpp
// CreatureEntity に追加予定
virtual bool is_stunned() const = 0;
virtual bool is_confused() const = 0;
virtual bool is_fearful() const = 0;
virtual bool is_invulnerable() const = 0;
```

### Phase 3: タイムドエフェクトの統一

**現状の不一致:**
- `PlayerType` のタイムドエフェクトは `CreatureEntity` の直接フィールド（`hero`, `invuln` 等）
- `MonsterEntity` は `std::map<MonsterTimedEffect, short> mtimed` で管理

**方針（検討中）:**
- 共通インターフェースとして `get_timed_effect(type)` / `set_timed_effect(type, value)` 仮想メソッドを定義
- または統一列挙型 `CreatureTimedEffect` を作成して両者を同一 map で管理

### Phase 4: ダメージ処理の完全統一

**現状:**
- プレイヤー: `take_hit(CreatureEntity &, int damage_type, int damage, ...)` in `player-damage.cpp`
- モンスター: `MonsterDamageProcessor(CreatureEntity &).mon_take_hit(...)` in `monster-damage.cpp`

**目標:** 単一のエントリポイントから両者を処理できるようにする。
死亡処理・落とすアイテム・メッセージ等の副作用は仮想メソッドに委譲：

```cpp
// CreatureEntity に追加予定
virtual void on_death(std::string_view cause) = 0;
virtual void on_take_hit(int damage) {}
```

### Phase 5: AC・防御の統一

`MonsterEntity::get_ac()` が実装済み。`CreatureEntity` の仮想メソッドとして昇格させる。

```cpp
// CreatureEntity に追加予定
virtual int get_ac() const = 0;
```

### Phase 6: フロアポインタの整理

`current_floor_ptr` が `MonsterEntity` の直接フィールドとして残存。
`get_floor()` 仮想メソッドは既に `CreatureEntity` に存在するので、各サブクラスでの実装を整理する。

---

## MonsterEntity 固有フィールドの扱い方針

モンスター固有フィールドは最終的に以下の方針で整理する。

### Phase 7: 汎用化できるフィールドを CreatureEntity に移動

| フィールド | 移動方針 |
|---|---|
| `cdis` | テンポラリ変数。`get_floor()` 経由で計算に変更し削除 |
| `target_y`, `target_x` | `CreatureEntity` に汎用 `target` 座標として移動 |
| `mflag` (一部) | `CreatureEntity` の汎用フラグ（ペット・フレンドリー等）として移動 |

### Phase 8: MonsterEntity の完全吸収

最終目標として `MonsterEntity` クラスそのものを廃止し、モンスター固有データを別構造体に切り出す。

**方針:**
- `MonsterEntity` の残存フィールドを `MonsterSpecificData` 的な構造体にまとめ、`CreatureEntity` の `optional<MonsterSpecificData>` として保持する
- または `MonsterProfile` として `MonraceDefinition` 側に置き、`CreatureEntity` から `MonraceId` で参照する

```
// 最終形イメージ
CreatureEntity
├── 共通フィールド（HP, 座標, 速度, 状態...）
├── optional<PlayerProfile>   // プレイヤー固有データ
└── optional<MonsterProfile>  // モンスター固有データ（alliance, smart 等）
```

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
