# CreatureEntity 統合リファクタリング 残タスク・ロードマップ

本書は `CLAUDE.md` の「CreatureEntity 統合リファクタリング」節の続編として、
Phase 1-8 完了後に残存している統合作業項目を整理したもの。
新規の統合作業を行う際の指針として参照すること。

作業着手時は該当提案の Issue を立てるか、既存の変愚マージ ISSUE と
区別可能なタイトル（例: `refactor: PlayerProfile 抽出`）で PR を作成する。

---

## 提案 1: `PlayerProfile` 抽出（最優先・最大の残タスク）

### 背景

Phase 8 で `MonsterEntity` 吸収が完了し、モンスター固有データは
`CreatureEntity::monster_profile` (`tl::optional<MonsterProfile>`) に
集約された。一方でプレイヤー固有フィールドは依然 `CreatureEntity`
基底クラス直下に 50+ 残存しており、モンスターインスタンスにとっては
無意味な領域を消費している。

また、`prace = PlayerRaceType::HUMAN` / `pclass = PlayerClassType::WARRIOR`
等のデフォルト値がモンスターにも付与され、意味不明な状態になっている。

### 方針

`MonsterProfile` と対称な `PlayerProfile` 構造体を新設し、
`CreatureEntity::player_profile` (`tl::optional<PlayerProfile>`) として保持する。

```
CreatureEntity
├── 共通フィールド (HP, 座標, 速度, timed_effects_map, ...)
├── std::shared_ptr<TimedEffects> timed_effects
├── tl::optional<MonsterProfile> monster_profile  (モンスター時のみ)
└── tl::optional<PlayerProfile> player_profile    (プレイヤー時のみ) ← 新規
```

### 移動候補フィールド

| カテゴリ | フィールド |
|---|---|
| 種族/職業/性格 | `prace`, `pclass`, `ppersonality`, `psex`, `race`/`personality`/`pclass_ref` ポインタ |
| 熟練度 | `spell_exp[]`, `weapon_exp[][]`, `skill_exp[]` |
| 魔法領域 | `realm1`, `realm2`, `element_realm` |
| 突然変異 | `muta`, `trait`, `patron` |
| キャラクタ履歴 | `old_race1/2`, `old_realm`, `history[4][60]`, `player_hp[PY_MAX_LEVEL]` |
| ESP/特殊能力 BIT_FLAGS | `telepathy`, `esp_*`, `cursed`, `special_defense`, `special_attack`, `dec_mana`, `easy_spell` 等 |
| 休息/旅行 | `resting`, `running`, `action` |
| その他 | `class_specific_data`, 各種 `old_*` 差分検出キャッシュ |

### 想定される影響範囲

- `PlayerType` 自身のフィールドアクセスは `this->player_profile->xxx` に変更
- 既存の `creature.prace` 等の直接参照は `creature.get_player_profile().prace` 等の経由に
- セーブ/ロード処理の大規模改修（`PlayerType` の load/save は `PlayerProfile` 経由に）
- 巨大 diff になるため段階的 PR 推奨（種族/職業系 → 熟練度 → BIT_FLAGS → ...）

### 期待効果

- モンスターインスタンスのメモリ削減
- 意味不明なデフォルト値解消
- `is_player()` 判定で安全に `player_profile` アクセス可能に
- コード可読性向上（プレイヤー専用か否かが型レベルで明確に）

---

## 提案 2: プレイヤー専用仮想メソッドの追加による安全化

### 背景

Phase 2 で基本的な状態チェックは virtual 化済みだが、モンスターから
呼ぶとクラッシュ／不定値を返す関数が残存している可能性がある。

特に Proposal 1 の PlayerProfile 抽出と併せて、アクセサを virtual 化して
モンスター側では安全なデフォルト値（`std::nullopt` / `0` / `false`）を
返すようにする必要がある。

### 候補メソッド

- HP/MP 自動回復判定関数群
- 祝福/呪い関連判定 (`is_cursed_item_used()` 等)
- ESP 判定 (`has_esp()`, `has_esp_evil()`, `has_esp_undead()` 等)
- `get_race()`, `get_class()`, `get_personality()` (既存があれば null-safe 化)
- `has_special_attack(flag)`, `has_special_defense(flag)`

### 作業方針

1. `CreatureEntity` に virtual 版を追加、デフォルト実装で安全値を返す
2. `PlayerType` でオーバーライドして既存ロジック
3. 呼出側の `is_player()` チェックを徐々に削減

---

## 提案 3: 残存する `PlayerType::get_instance()` 多用箇所の削減

### 背景

Phase 1 で `player_ptr` 引数は排除済みだが、関数内部で直接
`PlayerType::get_instance()` を呼んでいる箇所が多数残存している。
これは実質的なグローバル参照であり、統合目標（引数で creature を
受け渡す）と矛盾する。

### 検出コマンド

```bash
git grep -n "PlayerType::get_instance()" -- 'src/**/*.cpp' | wc -l
```

### 作業方針

- 呼出側スタックに `CreatureEntity &creature` 引数が既にある関数から優先的に置換
- ない場合は引数追加 PR を挟む
- グローバル参照を完全排除することでテスト容易性も向上

---

## 提案 4: 未統合の状態チェック関数の仮想化

### 背景

Phase 2 で主要な `is_xxx()` 系は仮想化済みだが、以下はまだ自由関数
または `PlayerType` 固有のまま残存している可能性がある。

### 候補

- `is_time_limit_esp()`, `is_time_limit_stealth()`
- `has_resist_*()` 系（耐性チェック）
- `has_immune_*()` 系
- `has_lite()`, `has_dark_source()` 系
- 視界/光源関連判定

### 作業方針

モンスターにも意味がある判定については `CreatureEntity` の virtual メソッド化。
プレイヤー専用の判定は `PlayerType` 側に維持しつつ、呼出側を
`is_player()` 分岐で保護するか、デフォルト false を返す virtual に。

---

## 提案 5: TimedEffects オブジェクトとの二重管理解消

### 背景

現状、プレイヤーの一部効果 (スタン / 混乱 / 恐怖 / 加速 / 減速 / 麻痺 / 盲目)
は `timed_effects` (`TimedEffects` オブジェクト) と `timed_effects_map`
の両方で管理されている。`PlayerType::get_timed_effect()` は前者を
優先参照する特殊分岐を持つ。一方モンスターは `timed_effects_map` のみ使用。

この二重管理はバグ誘発要因（同期漏れ）であり、理想的には一本化したい。

### 選択肢

- **(a) 全 effect を `timed_effects_map` に寄せる**
  - `TimedEffects` オブジェクト (stun/confusion 等の高機能タイマー) を廃止
  - メッセージ処理・ティック処理は別途共通関数に分離
  - モンスターとの完全統一が可能
  - diff 規模大

- **(b) モンスターも `TimedEffects` オブジェクトを持つ**
  - 全クリーチャーに shared_ptr が付与される（メモリコスト増）
  - 分岐ロジックが消える
  - `TimedEffects` 内のメッセージ処理等をプレイヤー専用部分から分離する必要あり

### 推奨: (a)

`timed_effects_map` 側が既に全 enum 対応済みで共通基盤。
`TimedEffects` の各効果クラス（`PlayerStun` 等）のロジックを
共通関数（例: `CreatureEntity::process_stun_tick()`）として切り出し、
マップ側に統合する方針。

---

## 提案 6: フィールド名の命名統一

### 背景

モンスター / プレイヤー兼用になったフィールドで、プレイヤー時代の
命名を引きずっているものがある。統合が進んだ今、汎用的な名前に
改名することで可読性が向上する。

### 候補

| 現名称 | 候補名 | 備考 |
|---|---|---|
| `csp` / `msp` | `current_mp` / `max_mp` | モンスターが MP 使用する設計拡張時に有効 |
| `exp` / `max_exp` / `max_max_exp` | モンスターは未使用 | PlayerProfile 送り候補でもある |
| `hp_frac` / `csp_frac` | `hp_fraction` / `mp_fraction` | 可読性のみ |

### 注意

改名 PR は diff が巨大化し、変愚マージ時の衝突を増やす。
**提案 1-5 が全て完了してから着手する**のが安全。
改名時は一括 sed 実行 + ビルド確認を自動化したスクリプトを用意すること。

---

## 推奨実施順序

1. **提案 1** - PlayerProfile 抽出（段階的 PR、最大規模）
2. **提案 2** - プレイヤー専用 virtual メソッド追加（提案 1 と並行可）
3. **提案 5** - TimedEffects 二重管理解消
4. **提案 4** - 残存状態チェック関数の仮想化
5. **提案 3** - `PlayerType::get_instance()` 削減
6. **提案 6** - フィールド命名統一（最後）

各提案は独立した PR として進めること。提案 1 は段階的 PR 必須
（一度に全フィールドを移すと diff が追えなくなる）。

---

## 作業時の共通留意事項

- 新規提案を完了したら本書と `CLAUDE.md` の両方を更新し、進捗を反映する
- 提案番号は維持し、完了したものには `✅ 完了` マーカーを付与
- 提案を統合・分割する場合は変更履歴をコメントで残す
- ビルド確認は必ず `sh .github/scripts/ci-build-test.sh` で実施
- 変愚マージ作業との競合に注意（`CLAUDE.md` の「変愚蛮怒（上流）からのマージ指針」節参照）
