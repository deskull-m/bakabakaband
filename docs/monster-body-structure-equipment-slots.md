# モンスター体構造別装備スロット設計

**ステータス**: ✅ 実装完了 (Phase 1-2.7 全段階)
**最終更新**: 2026-05-14
**作成日**: 2026-05-14
**関連提案**: 提案 12 (モンスター装備有効化) の延長

---

## 実装サマリ (2026-05-14 完了)

| Phase | 内容 | 状況 |
|---|---|:---:|
| Step 1 | BodyStructureType enum + BodySlotPolicy + JSON 受入 | ✅ |
| Step 2 | CreatureEntity::can_equip_to() + wield_slot ガード + c コマンド表示 | ✅ |
| Step 3 | 全 2347 体への自動分類 (シンボルベース) | ✅ |
| Phase 2 | 拡張スロット基盤 (ExtendedSlotType / extended_inventory / save/load) | ✅ |
| Phase 2.5 | 拡張スロットへの自動装備 (INVEN_EXTENDED_BASE 経由) | ✅ |
| Phase 2.6 | AC / 耐性集計の extended_inventory 反映 (FLAG_CAUSE_INVEN_EXTENDED) | ✅ |
| Phase 2.7 | extended_equipment_slots の JSON 個別上書き | ✅ |
| 表示 | r recall に体構造タグ表示、装備時メッセージ | ✅ |
| 表示 | c ステータス 1 ページ目に自分の体構造を表示 (2026-09-01) | ✅ |
| 分類調整 | D → DRACONIC、n → HUMANOID リファインメント | ✅ |

**現状分類分布**:
- HUMANOID:    1326 (フル装備可能)
- DRACONIC:      76 (フル装備 + TAIL_RING + WING_L + WING_R)
- QUADRUPED:    366 (首/胴体/頭のみ)
- INCORPOREAL:  350 (装備不可)
- AMORPHOUS:    114 (リング 2 個のみ)
- SERPENTINE:    63 (首/胴体 + TAIL_RING)
- BIPEDAL:       52 (首/光源/胴体/頭/脚)

**実装ファイル一覧**:
- `src/system/monrace/body-structure-types.h` (新規)
- `src/system/monrace/body-structure-policy.{h,cpp}` (新規)
- `src/system/monrace/extended-slot.{h,cpp}` (新規)
- `src/system/monrace/monrace-definition.h` (body_structure + extended_slots_override)
- `src/system/creature-entity.{h,cpp}` (can_equip_to, get_extended_slot_*, init_extended_inventory)
- `src/object/object-info.cpp` (wield_slot 拡張)
- `src/info-reader/race-reader.cpp` + race-info-tokens-table (JSON パーサー)
- `src/inventory/inventory-slot-types.h` (INVEN_EXTENDED_BASE)
- `src/player/player-status-flags.{h,cpp}` (FLAG_CAUSE_INVEN_EXTENDED)
- `src/save/monster-writer.cpp` + `src/load/old/monster-loader-savefile50.cpp` (save/load)
- `src/view/display-player-inventory-page.{h,cpp}` (拡張部位セクション)
- `src/view/display-lore.cpp` (体構造タグ)
- `src/view/display-player.cpp` + `src/view/display-util.cpp` + `src/view/status-first-page.h` (c ステータスの体構造行)
- `src/test/system/monrace/test-body-structure-policy.cpp` (ポリシー・表示名のユニットテスト)
- `src/monster-floor/monster-object.cpp` (装備時メッセージ)
- `lib/edit/MonraceDefinitions.jsonc` (全モンスターの body_structure 設定)

---

## 背景

提案 11/12/13 でモンスターも `inventory[INVEN_TOTAL]` を持つようになり、
プレイヤーと同じ装備スロット (INVEN_MAIN_HAND..INVEN_ASSHOLE の 13 部位)
を利用可能になった。しかし現状はすべてのモンスターが
プレイヤーと同一の身体構造を仮定しており、以下の課題がある:

- **意味的不整合**: ヘビが手袋を装備、スライムが王冠を被る、などの
  非現実的な装備が可能
- **拡張性欠如**: ドラゴンの複数の首にアミュレットを掛ける、
  ヒドラの多頭に兜を被らせる、といった種族固有のスロット拡張が不可
- **AI 判断困難**: モンスター AI が「自分の体型で装備可能か」を
  判定する仕組みがなく、無意味な装備選択を行う

本書は、モンスター毎に身体構造 (`body_structure`) を定義し、
装備可能スロットを動的に決定する仕組みを設計する。

---

## 設計目標

1. **JSON で宣言的に指定**: `MonraceDefinitions.jsonc` の各モンスター
   定義に `body_structure` フィールドを追加し、外部データだけで
   装備構成を変更可能とする
2. **HUMANOID をデフォルト**: 既存モンスター約 1500 体は明示指定
   不要で従来通り全装備可能
3. **減算と加算の両対応**: 既存スロットの一部を「無効化」する
   形と、新規スロットを「追加」する形の両方をサポート
4. **後方互換**: セーブファイル形式は不変
   (`inventory[INVEN_TOTAL]` の配列サイズは固定維持)
5. **段階的移行**: まず減算 (slot 無効化) を実装、後から加算
   (新スロット追加) を別フェーズで実装

---

## 用語

- **body structure (体構造)**: モンスターの身体構成を表現する
  カテゴリ値。例: HUMANOID, SERPENTINE, AMORPHOUS, DRACONIC
- **slot mask (スロットマスク)**: 各 body structure が「どの
  プレイヤースロットを許可するか」を表すビットフラグ集合
- **derived slot (派生スロット)**: 既存 `INVEN_*` 列挙の範疇に
  収まる装備部位
- **extended slot (拡張スロット)**: 既存列挙外の追加部位
  (例: SECOND_NECK, TAIL, WING) — Phase 2 で扱う

---

## アーキテクチャ概要

```
┌─────────────────────────────────────────┐
│ MonraceDefinitions.jsonc                │
│   "body_structure": "SERPENTINE"        │
└──────────────────┬──────────────────────┘
                   │ race-reader でパース
                   ▼
┌─────────────────────────────────────────┐
│ MonraceDefinition::body_structure       │
│   BodyStructureType (enum)              │
└──────────────────┬──────────────────────┘
                   │ 参照
                   ▼
┌─────────────────────────────────────────┐
│ body_structure_table[]                  │
│   各 BodyStructureType → SlotPolicy     │
│   (slot_mask + extended_slots)          │
└──────────────────┬──────────────────────┘
                   │ 問合せ
                   ▼
┌─────────────────────────────────────────┐
│ CreatureEntity::can_equip_to(slot)      │
│ CreatureEntity::wield_slot(item)        │
│   既存ロジックが body_structure を       │
│   経由してスロット可否を判定             │
└─────────────────────────────────────────┘
```

---

## Phase 1: 体構造による既存スロットの減算

### 1.1 BodyStructureType enum

新規ファイル: `src/system/monrace/body-structure-types.h`

```cpp
enum class BodyStructureType : uint8_t {
    HUMANOID = 0,        //!< 二足歩行・両手・頭・胴体: 全スロット有効 (デフォルト)
    BIPEDAL = 1,         //!< 鳥型・恐竜型: 手の代わりに翼/前肢、武器装備不可
    QUADRUPED = 2,       //!< 四足獣: 手なし、首・胴体・脚装備のみ
    SERPENTINE = 3,      //!< ヘビ・うなぎ型: 首と胴体のみ
    AMORPHOUS = 4,       //!< スライム・ジェル: 装備不可 (リング以外?)
    INCORPOREAL = 5,     //!< 幽霊・ベクター: 装備一切不可
    DRACONIC = 6,        //!< ドラゴン: HUMANOID + 翼/尾 (Phase 2)
    INSECTOID = 7,       //!< 昆虫型: 多腕、頭、胴体、脚 (Phase 2 で四腕拡張)
    AVIAN = 8,           //!< 鳥型: 翼、頭、胴体、脚
    AQUATIC = 9,         //!< 魚型: 頭、胴体、尾 (Phase 2)
    MAX,
};
```

初期実装では HUMANOID/BIPEDAL/QUADRUPED/SERPENTINE/AMORPHOUS/
INCORPOREAL を対象とし、後 4 種は Phase 2 (extended slots) で
追加する。

### 1.2 スロット可否ポリシー

新規ファイル: `src/system/monrace/body-structure-policy.h/.cpp`

```cpp
struct BodySlotPolicy {
    std::bitset<INVEN_TOTAL - INVEN_MAIN_HAND> allowed_slots;
    // 拡張用 (Phase 2): std::vector<ExtendedSlotDef> extended_slots;
};

const BodySlotPolicy &get_body_slot_policy(BodyStructureType type);
```

各タイプのデフォルト定義例:

| BodyStructure | MAIN_HAND | SUB_HAND | BOW | RING×2 | NECK | LITE | BODY | OUTER | HEAD | ARMS | FEET |
|---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| HUMANOID | ○ | ○ | ○ | ○ | ○ | ○ | ○ | ○ | ○ | ○ | ○ |
| BIPEDAL | × | × | × | × | ○ | ○ | ○ | × | ○ | × | ○ |
| QUADRUPED | × | × | × | × | ○ | × | ○ | × | ○ | × | × |
| SERPENTINE | × | × | × | × | ○ | × | ○ | × | × | × | × |
| AMORPHOUS | × | × | × | ○ | × | × | × | × | × | × | × |
| INCORPOREAL | × | × | × | × | × | × | × | × | × | × | × |

`ASSHOLE` スロット (尻穴) は HUMANOID 以外は基本的に無効
(独自仕様だが SERPENTINE は example として有効でもよい)。

### 1.3 MonraceDefinition への追加

```cpp
// src/system/monrace/monrace-definition.h
class MonraceDefinition {
    ...
    BodyStructureType body_structure{ BodyStructureType::HUMANOID };
    ...
};
```

### 1.4 JSON パーサー拡張

`src/info-reader/race-reader.cpp` の `parse_monrace()` に `body_structure`
キーを追加:

```cpp
if (mon_data.contains("body_structure")) {
    std::string body_str;
    if (auto err = info_set_string(mon_data["body_structure"], body_str)) {
        return err;
    }
    auto type = parse_body_structure_type(body_str);
    if (!type) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    monrace.body_structure = *type;
}
// 未指定の場合は HUMANOID のまま (デフォルト初期値)
```

`info-reader/race-info-tokens-table.cpp` に文字列 → enum 変換表
`body_structure_token_table` を追加。

### 1.5 CreatureEntity への問合せ API

```cpp
// src/system/creature-entity.h
class CreatureEntity {
public:
    /*!
     * @brief 指定スロットに装備可能か判定する
     * @param slot inventory_slot_type
     * @return 体構造的に装備可能なら true
     * @details プレイヤーは常に true (HUMANOID 想定)。モンスターは
     *          MonraceDefinition::body_structure を参照する。
     */
    virtual bool can_equip_to(int slot) const;
};
```

実装例 (`creature-entity.cpp`):

```cpp
bool CreatureEntity::can_equip_to(int slot) const
{
    if (slot < INVEN_MAIN_HAND || slot >= INVEN_TOTAL) {
        return false;
    }
    if (this->is_player()) {
        return true;  // プレイヤーは HUMANOID 固定で常に許可
    }

    const auto &monrace = this->get_monrace();
    const auto &policy = get_body_slot_policy(monrace.body_structure);
    return policy.allowed_slots.test(slot - INVEN_MAIN_HAND);
}
```

### 1.6 既存ロジックへの組み込み

#### wield_slot() の調整

`src/object/object-info.cpp:wield_slot()` で、選択されたスロットが
`can_equip_to()` で許可されない場合はフォールバック (例: パック行き)
にする。

```cpp
short wield_slot(CreatureEntity &creature, const ItemEntity &item)
{
    auto desired = wield_slot_internal(item);  // 既存の type→slot 表
    if (desired >= INVEN_MAIN_HAND && !creature.can_equip_to(desired)) {
        return -1;  // 装備不可
    }
    return desired;
}
```

#### モンスター AI の調整

`src/monster-floor/one-monster-placer.cpp` で初期装備をモンスター
種族 (`monrace.drop_kind`) から生成する際、`can_equip_to()` で
弾かれたアイテムは床に置く / 持物スロットに格納する。

提案 13 で実装したモンスター AI アイテム使用ロジック
(`src/mind/` 配下) でも装備スロット参照箇所を `can_equip_to()`
ガードで補強する。

### 1.7 表示への反映

提案 14 の c コマンド「装備＆所持品」ページ (mode 5) で:

- 装備スロット表示時、`can_equip_to(slot) == false` のスロットは
  `(なし)` / `(empty)` ではなく `(該当なし)` / `(no slot)` に変更
  または非表示

```cpp
// src/view/display-player-inventory-page.cpp
for (auto slot = INVEN_MAIN_HAND; slot < INVEN_TOTAL; ++slot) {
    if (!creature.can_equip_to(slot)) {
        c_put_str(TERM_L_DARK, mention_use(creature, slot), row, col);
        c_put_str(TERM_L_DARK, _("該当なし", "(no slot)"), row, col + 14);
        continue;
    }
    // 既存表示...
}
```

---

## Phase 2: 拡張スロットの追加

### 2.1 課題

既存 `inventory[INVEN_TOTAL]` 配列は 37 スロットで固定であり、
新規スロット (例: SECOND_NECK, TAIL, WING_L, WING_R) を追加すると:

- セーブファイル後方互換性を破る
- 既存の `INVEN_*` 列挙範囲外となり、`wield_slot()` 等の
  switch ハンドラに大量の改修が必要
- 全モンスターのメモリフットプリント増加

### 2.2 推奨設計 (案 A): inventory を可変長 vector に変更

```cpp
class CreatureEntity {
    // 旧: std::array<std::shared_ptr<ItemEntity>, INVEN_TOTAL> inventory;
    // 新: std::vector<std::shared_ptr<ItemEntity>> inventory;
    //     (サイズはコンストラクト時に固定)
};
```

メリット:
- 拡張スロット数を体構造ごとに変えられる
- セーブ時にサイズも記録すれば後方互換可能

デメリット:
- 既存コード全体が `INVEN_TOTAL` をマジック定数として使っており
  影響範囲が広い
- アクセス時の境界チェック追加が必要

### 2.3 推奨設計 (案 B): MonsterProfile に extended_inventory を追加

`MonsterProfile` に追加スロットを別配列として持つ:

```cpp
class MonsterProfile {
    ...
    std::vector<std::shared_ptr<ItemEntity>> extended_inventory;
    // body_structure ごとの ExtendedSlotDef と対応
};
```

ExtendedSlot 参照は `creature.get_extended_inventory(slot_id)`
のような新 API を追加。

メリット:
- 既存 `inventory[INVEN_TOTAL]` は不変、後方互換完全
- プレイヤーは extended_inventory を持たない (メモリ節約)

デメリット:
- 装備関連処理が 2 系統 (inventory + extended_inventory) に分岐
- AI ロジックや c コマンド表示で両方を扱う必要

### 2.4 推奨

**案 B を推奨**。既存の `INVEN_TOTAL = 37` 前提コードを破壊せず、
拡張スロットを段階的に追加可能。実装時には:

1. `MonsterProfile::extended_inventory` を追加
2. `ExtendedSlotDef`: `{ name_ja, name_en, item_kind_filter, equippy_char }`
3. `body_structure_policy` に `extended_slots` を持たせる
4. c コマンド表示で extended_inventory を併記
5. AI / wield_slot に対応

---

## JSON スキーマ例

```jsonc
{
  "id": 100,
  "name": { "ja": "ハイドラ", "en": "Hydra" },
  "body_structure": "DRACONIC",  // Phase 1 で減算ポリシー、Phase 2 で extended slot
  "extended_equipment_slots": [   // Phase 2 のみ。Phase 1 では未使用
    { "name": "SECOND_NECK", "item_kind": "AMULET" },
    { "name": "TAIL_TIP", "item_kind": "WEAPON" }
  ]
}
```

最も単純なケース (現状互換):

```jsonc
{
  "id": 1,
  "name": { "ja": "汚いおっさん", "en": "Filthy street urchin" },
  // body_structure 省略 → HUMANOID デフォルト
}
```

蛇の例:

```jsonc
{
  "id": 200,
  "name": { "ja": "コブラ", "en": "Cobra" },
  "body_structure": "SERPENTINE"
  // 自動的に MAIN_HAND/SUB_HAND/BOW/RING×2/LITE/OUTER/HEAD/ARMS/FEET/ASSHOLE が無効
}
```

---

## 段階的実装計画

### Step 1: 列挙と表のみ追加 (~1 PR)

- `BodyStructureType` enum 定義
- `body_structure_token_table`
- `body_structure_policy_table` (HUMANOID/BIPEDAL/QUADRUPED/SERPENTINE/
  AMORPHOUS/INCORPOREAL)
- `MonraceDefinition::body_structure` フィールド (default HUMANOID)
- JSON パーサー対応
- ビルド確認のみ。動作は変わらない (全モンスター HUMANOID 扱い)

### Step 2: can_equip_to() の実装と組込み (~1 PR)

- `CreatureEntity::can_equip_to()` virtual
- `wield_slot()` ガード
- モンスター生成時の初期装備フィルタ
- c コマンド表示の `(該当なし)` 対応
- いくつかの代表モンスター (ヘビ系/犬系/スライム系) に
  `body_structure` を設定して動作確認

### Step 3: 既存モンスターへの一括適用 (~1-2 PR)

- 既存 1500 体超のモンスターを kind_flags / symbol で
  自動分類 (例: `f` 猫類 → QUADRUPED, スライム系シンボル → AMORPHOUS)
- 例外的なモンスターは個別に JSON 修正
- ゲームバランス調整 (装備不可になることで難易度低下する場合は
  能力値を補正)

### Step 4: 拡張スロット (Phase 2) — 要件次第

- 案 B (extended_inventory) で実装
- 提案 12 のフェーズ B 装備処理 (AC・耐性・攻撃ボーナス) を
  extended slot にも適用
- c コマンド表示・savefile load/save 対応

---

## 影響範囲

### Step 1-2 で影響する主なファイル

| ファイル | 変更内容 |
|---|---|
| `src/system/monrace/body-structure-types.h` | 新規 |
| `src/system/monrace/body-structure-policy.{h,cpp}` | 新規 |
| `src/system/monrace/monrace-definition.{h,cpp}` | フィールド追加 |
| `src/info-reader/race-reader.cpp` | パーサー追加 |
| `src/info-reader/race-info-tokens-table.{h,cpp}` | token table 追加 |
| `src/system/creature-entity.{h,cpp}` | `can_equip_to()` virtual |
| `src/object/object-info.cpp` | `wield_slot()` ガード |
| `src/monster-floor/one-monster-placer.cpp` | 初期装備フィルタ |
| `src/view/display-player-inventory-page.cpp` | 非表示処理 |
| `lib/edit/MonraceDefinitions.jsonc` | 代表モンスター指定 |

### Step 3 で影響する主なファイル

| ファイル | 変更内容 |
|---|---|
| `lib/edit/MonraceDefinitions.jsonc` | 全モンスター分類 (~1500 体) |
| `src/system/monrace/body-structure-policy.cpp` | ポリシー微調整 |

### Step 4 で影響する主なファイル (Phase 2)

| ファイル | 変更内容 |
|---|---|
| `src/system/monster-profile.{h,cpp}` | `extended_inventory` 追加 |
| `src/save/monster-writer.cpp`, `src/load/monster-loader-savefile50.cpp` | 拡張スロットの save/load |
| `src/object/object-info.cpp` | 拡張スロット への wield 対応 |
| `src/view/display-player-inventory-page.cpp` | 拡張スロット表示 |
| `src/mind/` 配下 (AI item 使用) | 提案 13 系の AI を拡張対応 |

---

## 既存仕様との関係

### 提案 11/12/13 (モンスター inventory / 装備 / AI 使用) との関係

- 提案 11 でモンスターも `inventory[INVEN_TOTAL]` を持つようになった
  ことが前提
- 提案 12 のフェーズ B (AC/耐性/攻撃ボーナス装備集計) は
  `body_structure` で無効化されたスロットでは無視される
  (`!can_equip_to(slot)` ならスキップ)
- 提案 13 のモンスター AI アイテム使用は対象に影響なし
  (装備ではなく消費アイテム)

### CLAUDE.md の方針との整合

- 提案 1 (プレイヤー専用フィールドのクリーチャー共通化) の延長線上
- 既存 `INVEN_*` 列挙を変更しないので上流マージ衝突を最小化
- `body_structure` はモンスター種族定義に近い概念なので
  `MonraceDefinition` に格納 (`MonsterProfile` 内ではなく
  `MonraceDefinition` 側の所属が自然)

---

## 未決事項 / 検討項目

1. **AMORPHOUS のリング装備可否**: スライムは「擬足にリングをはめる」
   と考えると面白いが、不自然との意見もあり。最終決定は実装時。
2. **プレイヤーが装備不可能なボディタイプを持つ可能性**:
   → **一部実現済み**。`player_birth_as_monster` でモンスターを選ぶと
   `r_idx != MonraceId::PLAYER` となり、プレイヤーもその種族の
   `body_structure` に従う (人型以外になり装備部位が減る)。判定は
   `CreatureEntity::get_body_structure()` に集約し、`can_equip_to()` /
   拡張スロット / c コマンドの体構造表示が同じ窓口を共有する。
   通常のプレイヤー (`r_idx == MonraceId::PLAYER`) は従来どおり
   HUMANOID 固定。`mimic_form` 連動は未対応 (変身中も体構造は変わらない)。
3. **ペット / 召喚モンスターの装備引き継ぎ**: ペット化したモンスター
   に装備を渡す際の `can_equip_to()` チェック。提案 13 のフェーズ C
   で実装した「ペットへの装備譲渡」コマンドの拡張。
4. **savefile 互換性**: `body_structure` は `MonraceDefinition` 側
   なので savefile には基本含まれない (種族 ID で参照する)。ただし
   セーブ後に jsonc を編集して body_structure を変えた場合の
   既存個体の挙動を要検討 (再装備チェックなど)。
5. **`mflag2` フラグとの関係**: `MonsterConstantFlagType::KAGE` 等の
   既存変容フラグと body_structure の優先関係。シャドウ系は
   HUMANOID 化 (実体無視) するなどのルール検討。

---

## 参考 / 将来拡張

- D&D 5e の「creature size」と「body plan」概念
- Angband 派生 (ToME, Posband 等) のモンスター装備仕様
- Wesnoth の `[advancefrom]` / `[traits]` システム

---

## 履歴

| 日付 | 内容 | 担当 |
|---|---|---|
| 2026-05-14 | 初版設計提案 | Claude Code (claude/monster-stealth-perception-e6cuk) |
| 2026-09-01 | c ステータス 1 ページ目に体構造を表示。表示名/表示色を `body_structure_name()` / `body_structure_color()` に集約し r recall と共用。`CreatureEntity::get_body_structure()` を新設 | Claude Code (claude/creature-entity-integration-zzgibe) |
