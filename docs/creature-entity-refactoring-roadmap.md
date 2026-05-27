# CreatureEntity 統合リファクタリング 残タスク・ロードマップ

本書は `CLAUDE.md` の「CreatureEntity 統合リファクタリング」節の続編として、
Phase 1-8 完了後に残存している統合作業項目を整理したもの。
新規の統合作業を行う際の指針として参照すること。

作業着手時は該当提案の Issue を立てるか、既存の変愚マージ ISSUE と
区別可能なタイトル（例: `refactor: TimedEffects 一本化`）で PR を作成する。

---

## 提案一覧 (索引)

提案は番号順ではなく作業着手順 (≒提案追加順) で本書に並んでいるため、
特定の提案を探す際は本表から該当節へジャンプすること。
ステータスの ✅ は完了、🚧 は進行中/未着手を示す。

| 番号 | 提案 | ステータス | 対象 |
|---|---|---|---|
| [1](#提案-1-プレイヤー専用フィールドのクリーチャー共通化--基盤完了) | プレイヤー専用フィールドのクリーチャー共通化 | ✅ 基盤完了 | prace / pclass / patron / mimic_form 等の get/set virtual |
| [2](#提案-2-プレイヤー専用仮想メソッドのクリーチャー共通化--完了) | プレイヤー専用 virtual メソッドの共通化 | ✅ 完了 | has_X / get_X 系の virtual |
| [3](#提案-3-残存する-playertypeget_instance-多用箇所の削減--完了) | `PlayerType::get_instance()` 多用箇所の削減 | ✅ 完了 | グローバルアクセスの削減 |
| [4](#提案-4-未統合の状態チェック関数の仮想化--完了) | 未統合の状態チェック関数の仮想化 | ✅ 完了 | has_resist_X 等の自由関数 |
| [5](#提案-5-timedeffects-オブジェクトとの二重管理解消--完了) | TimedEffects 二重管理解消 | ✅ 完了 | timed_effects_map 統一 |
| [6](#提案-6-フィールド名の命名統一) | フィールド名の命名統一 | 🚧 | csp/msp → current_mp/max_mp 等 (大規模 diff、後回し) |
| [7](#提案-7-モンスター可視判定-ml-の-virtual-アクセサ化--完了) | モンスター可視判定 (`ml`) の virtual 化 | ✅ 完了 | is_visible_on_map() |
| [8](#提案-8-hpmp-自然回復計算の-virtual-hook-化--完了) | HP/MP 自然回復計算の virtual hook 化 | ✅ 完了 | get_base_natural_regen_amount() |
| [9](#提案-9-monsterprofile-フィールド-virtual-アクセサ化--一部完了) | MonsterProfile read-side virtual | ✅ 一部完了 | get_alliance_idx / get_smart_flags 等 |
| [9b](#提案-9b-monsterprofile-setter-の-virtual-集約--完了) | MonsterProfile setter virtual | ✅ 完了 | set_alliance_idx / set_sub_align 等 |
| [10/36](#提案-1036-熟練度配列の-virtual-アクセサ--完了) | 熟練度配列の virtual アクセサ | ✅ 完了 | get_spell_exp / get_weapon_exp 等 |
| [11](#提案-11-モンスターアイテム所持の-inventory-統一--完了-フェーズ-a) | モンスター inventory[] 統一 | ✅ 完了 | hold_o_idx_list 廃止 |
| [12](#提案-12-モンスター装備の有効化--完了-フェーズ-b) | モンスター装備の有効化 | ✅ 完了 | AC/耐性/攻撃/二刀流 |
| [13](#提案-13-モンスター-ai-のアイテム能動使用--完了-フェーズ-c) | モンスター AI アイテム能動使用 | ✅ 完了 | ポーション/巻物/杖/ロッド |
| [14](#提案-14-ai-ターゲット選定ロジックの共通化--完了) | AI ターゲット選定の共通 API 化 | ✅ 完了 | find_nearest_creature / collect_creatures |
| [15](#提案-15-mflag2-の-virtual-アクセサ集約--完了) | mflag2 read-side virtual | ✅ 完了 | is_kage / is_chameleon 等 |
| [16](#提案-16-mflag-monstertemporaryflagtype-の-virtual-化--完了) | mflag (一時フラグ) virtual 化 | ✅ 完了 | is_in_view / has_prevent_magic 等 |
| [17](#提案-17-モンスター-inventory-操作の-oo-統合--完了) | モンスター inventory 操作の OO 化 | ✅ 完了 | store_item / acquire_item / drop_all_inventory |
| [18](#提案-18-mflag2-書込パターンの-virtual-集約--完了) | mflag2 write-side virtual | ✅ 完了 | set_constant_flag / reset_constant_flag |
| [19](#提案-19-変身自壊カウンタの-virtual-化--完了) | 変身/自壊カウンタ virtual 化 | ✅ 完了 | transform_* / death_count |
| [20](#提案-20-残り-mflagmflag2-一括操作の-virtual-化--完了) | mflag/mflag2 一括操作 virtual | ✅ 完了 | get_all_constant_flags / clear_* |
| [21](#提案-21-monsterprofile-を-class-化して-private-化--完了) | MonsterProfile を class 化して private 化 | ✅ 完了 | 12 個 private、friend 制限 |
| [22](#提案-22-モンスター-identity-setter-の-virtual-化--完了) | モンスター identity setter virtual | ✅ 完了 | set_r_idx / polymorph_to |
| [23](#提案-23-ロードマップと-claudemd-の更新--完了) | ロードマップ更新 (継続) | ✅ 完了 | |
| [24](#提案-24-creatureentity-直下生フィールドの-setter-整備--完了) | 生フィールド setter 整備 | ✅ 完了 | set_age / set_ht / set_wt 等 |
| [25](#提案-25-inven_cnt--equip_cnt-の-inventory-からの自動計算化--完了) | inven_cnt / equip_cnt 自動計算化 | ✅ 完了 | フィールド廃止 |
| [26](#提案-26-ambushfoodtown_numlevel-に-setter-virtual-追加--完了) | ambush/food/town_num/level setter virtual | ✅ 完了 | |
| [27](#提案-27-max_plv--msp--経験値系の-setter-virtual--完了) | max_plv / msp / 経験値系 setter | ✅ 完了 | |
| [27b](#提案-27b-au--csp-の-addsub-系-virtual-と-compound-assignment-移行--完了) | au / csp の add/sub virtual | ✅ 完了 | 110 箇所 compound assignment 移行 |
| [28](#提案-28-r_idx--ap_r_idx--riding-に-getter-virtual-を追加--完了) | r_idx / ap_r_idx / riding getter (参照形式) | ✅ 完了 | 210 箇所移行 |
| [28b](#提案-28b-ポインタ経由の-r_idx--ap_r_idx--riding-を-getter-へ移行--完了) | 同 (ポインタ経由) | ✅ 完了 | 80 箇所移行 |
| [29](#提案-29-r_idx--ap_r_idx--riding-を-private-化--完了) | r_idx / ap_r_idx / riding private 化 | ✅ 完了 | **3 個** |
| [30](#提案-30-戦闘ボーナスと-stat-配列に-setter-virtual-を追加--完了) | 戦闘ボーナス / stat 配列 setter virtual | ✅ 完了 | 55 箇所移行 |
| [31](#提案-31-残り-plain-field-の-getter-virtual-整備と-read-側移行--完了) | plain field getter virtual と read 移行 | ✅ 完了 | 14 種 350 箇所 |
| [31b](#提案-31b-stat_-と-to_hto_dto_a-の-getter-virtual-整備と-read-移行--完了) | stat_*[] と to_h/d/a の getter virtual | ✅ 完了 | 270 箇所 |
| [31c](#提案-31c-level-read-site-を-get_level-経由に移行--完了) | level read site を get_level() に統一 | ✅ 完了 | 628 箇所 / 175 ファイル |
| [32](#提案-32-安全な-7-フィールドを-private-化--完了-縮小スコープ) | 安全な 7 フィールド private 化 | ✅ 完了 | **+7 = 22 個** |
| [32b](#提案-32b-残り-14-個の-plain-field-を-private-化--完了) | 残り 14 個の plain field private 化 | ✅ 完了 | **+14 = 37 個** (37 個達成) |
| [33](#提案-33-esp--装備集計-bit_flags-の-private-化--完了) | ESP / 装備集計 BIT_FLAGS private 化 | ✅ 完了 | **+35 = 72 個** |
| [34](#提案-34-表示用既知値-dis_to_hdaac-の-private-化--完了) | 表示用既知値 (dis_*) private 化 | ✅ 完了 | **+5 = 77 個** |
| [35](#提案-35-is_player-分岐の縮減--完了-調査ベース縮小スコープ) | is_player() 分岐縮減 | ✅ 完了 (縮小) | 1 サイトのみ |
| [39](#提案-39-装備派生キャッシュフィールドの-private-化--完了) | 装備派生キャッシュフィールド private 化 | ✅ 完了 | **+11 = 94 個** (num_blow/cumber/icky 等) |
| [40](#提案-40-ペット関連フィールドの-private-化--完了) | ペット関連フィールド private 化 | ✅ 完了 | **+4 = 103 個** (pet_extra_flags / pet_follow_distance 等、health_who 削除) |
| [41](#提案-41-呪文マスク-spell_learnedworkedforgotten-の集約--完了) | 呪文マスク集約 (spell_learned/worked/forgotten) | ✅ 完了 | **+6 = 83 個**、realm_idx ベース API |
| [42](#提案-42-旧差分検出キャッシュ-old_-の-private-化) | 旧差分検出キャッシュ (`old_*`) private 化 | 🚧 | old_cumber_* / old_heavy_* 等 |
| [43](#提案-43-行動状態フラグの-private-化) | 行動状態フラグ private 化 | 🚧 | resting / running / action 等 |
| [44](#提案-44-突然変異呪い系フラグの-private-化--完了) | 突然変異/呪い系フラグ private 化 | ✅ 完了 | **+5 = 99 個** (muta / cursed / patron 等) |
| [45](#提案-45-pet_t_m_idx--riding_t_m_idx-系を-target-pos2d-に統合) | pet_t_m_idx / riding_t_m_idx 統合 | 🚧 | Pos2D ベース |
| [46](#提案-46-esp--装備集計の差分検出を-update_creature-内-raw-access-に閉じる) | ESP / 装備集計の差分検出を内部に閉じる | 🚧 | get_X_flags() 公開撤廃検討 |

**累計 private 化フィールド数 (主要マイルストーン):**
- 提案 29 (3 個) → 32 (10 個) → 32b (37 個) → 33 (72 個) → 34 (77 個) →
  41 (83 個) → 39 (94 個) → 44 (99 個) → **40 (103 個)**

未完了の提案 6 / 40 / 42 / 43 / 45 / 46 を全完了で **130+ 個** に到達見込み。

---

## 設計方針: プレイヤー固有フィールドをモンスターにも開放する

本プロジェクトの統合方針として、現状プレイヤー固有となっている
パラメータ（種族・職業・性格・熟練度・魔法領域・突然変異・ESP 等）は
**将来モンスターにも積極的に持たせて運用する**ことを想定する。
したがって `PlayerProfile` のようにプレイヤー専用構造体へ切り出して
モンスターから隔離する方向は取らない。

言い換えると、これらのフィールドは `CreatureEntity` 直下に保持したまま、
モンスター側もこれらを活用できる形に発展させていく。モンスターの
デフォルト値（`prace = HUMAN` 等）は意味不明ではなく「未設定時の
便宜的な初期値」として扱い、必要なモンスターに対しては明示的に
設定していく運用とする。

この方針を前提に、以下の提案は「プレイヤー/モンスターで共通活用
できるようにする」方向の作業のみを列挙する。

---

## 提案 1: プレイヤー専用フィールドのクリーチャー共通化 ✅ 基盤完了

### 背景

現状 `CreatureEntity` 直下には、プレイヤー向けに設計された 50+ の
フィールド（種族 / 職業 / 性格 / 熟練度 / 魔法領域 / 突然変異 /
ESP 系 BIT_FLAGS 等）が残存している。モンスターでも将来運用する
ためには、これらを単に残すだけでなく「モンスターからも読み書き
可能で意味を持つ」形に整理する必要がある。

### 対象フィールド

| カテゴリ | フィールド | モンスター運用の想定 |
|---|---|---|
| 種族/職業/性格 | `prace`, `pclass`, `ppersonality`, `psex` | 特殊個体（ユニーク / 召喚された英雄系）の種族職業付与 |
| 熟練度 | `spell_exp[]`, `weapon_exp[][]`, `skill_exp[]` | 経験を積むモンスター・成長するボス (提案 10/36 で virtual 化済) |
| 魔法領域 | `realm1`, `realm2`, `element_realm` | 魔法使い系モンスターのスペル選択根拠 |
| 突然変異 | `muta`, `trait`, `patron` | 変異個体・カオスパトロン配下 |
| キャラクタ履歴 | `old_race1/2`, `old_realm`, `history[4][60]`, `player_hp[PY_MAX_LEVEL]` | モンスターのレベル成長履歴 |
| ESP/特殊能力 BIT_FLAGS | `telepathy`, `esp_*`, `cursed`, `special_defense`, `special_attack`, `dec_mana`, `easy_spell` 等 | モンスターの ESP / 呪い装備 / 特殊攻撃防御 (提案 33 で virtual 化済) |
| 休息/旅行 | `resting`, `running`, `action` | モンスターの待機・徘徊行動状態 |
| その他 | `class_specific_data`, `old_*` 差分検出キャッシュ | 状態キャッシュはそのままクリーチャー共通で利用 |

### 作業方針

1. **フィールドを移動しない。** モンスターからのアクセスが可能な
   前提を整備する。具体的には以下:
   - 初期値の妥当性を再検討（`prace = HUMAN` を `NONE` / `UNDEFINED`
     相当に変更するなど、モンスターデフォルト時に無意味な挙動を
     引き起こさない値にする）
   - アクセサを `CreatureEntity` に virtual で統一提供し、
     モンスターでも安全に呼べる状態にする
2. モンスター専用のデフォルト動作（「prace 未設定なら種族ベースの
   効果は発動しない」等）をフィールド読取り関数にガードとして
   組み込み、今後のモンスター運用でオプトインできる形にする

### 期待効果

- プレイヤー / モンスターの区別なくフィールドが利用できる
- モンスターに種族・職業・熟練度等の概念を導入する下地が整う
- コード分岐（`if (creature.is_player()) ...`）の削減

### 完了内容

- ✅ モンスター生成時に `prace` / `pclass` / `realm1` / `realm2` /
  `element_realm` を NONE に明示初期化（`init_monster_profile()`）
- ✅ `ppersonality` の NONE 値 (`PERSONALITY_NONE`) を `init_monster_profile()`
  で初期化
- ✅ `player_sex` enum に `SEX_NONE = 4` を追加 (sex_info に「未設定」エントリ
  も追加)。`init_monster_profile()` で `psex = SEX_NONE` に初期化し、
  `get_sex_info()` は範囲外の値を SEX_NONE にフォールバック
- ✅ virtual アクセサ `get_psex()` / `get_ppersonality()` / `get_prace()` /
  `get_pclass()` / `get_realm1()` / `get_realm2()` / `get_element_realm()` /
  `get_patron()` / `get_mimic_form()` を `CreatureEntity` に整備
- ✅ 対応する setter virtual (`set_psex` / `set_ppersonality` / `set_prace` /
  `set_pclass` / `set_realm1` / `set_realm2` / `set_element_realm` /
  `set_patron` / `set_mimic_form`) も整備
- ✅ ESP/装備集計 BIT_FLAGS 群は提案 33 で virtual 化済 (合計 35 フィールド)
- ✅ 熟練度 (spell_exp / weapon_exp / skill_exp) は提案 10/36 で
  virtual 化済 (read 31 + write 13 ＋ adder 3 virtuals)
- ✅ 表示系の主要 access site (display-player / window /
  io-dump 系) 約 24 箇所を `creature.get_X()` 形式に migration

### 設計判断: 残約 177 直接 access site は player 経路で残置

`birth/` `wizard/` `cmd-action/cmd-spell` `info-reader` `save/` `load/`
等のプレイヤー専用 code path に残る `creature.prace` / `creature.pclass`
等の直接 access (約 177 箇所) は意図的に残置する。これらは:

- 呼出側が必ずプレイヤー (引数で型契約済) であり virtual 化価値が低い
- 機械的 sed migration は可能だが提案 35 同様、構造的改善に乏しい
- フィールドは将来 private 化候補だが、`class_specific_data` /
  `incident` / `name` 等とともに別提案として扱う

提案 1 は **「virtual アクセサ整備 + 主要 display path の migration」**
を基盤完了として確定する。モンスター個別運用 (種族や職業の付与) を
実装する将来の提案ではこの基盤を利用できる。

### 残作業

- プレイヤー専用 path の直接 access 177 箇所の機械的 migration
  (低価値・別提案推奨)
- モンスター override の実装 (具体的な enemy 機能拡張時に行う)

---

## 提案 2: プレイヤー専用仮想メソッドのクリーチャー共通化 ✅ 完了

### 背景

Phase 2 で基本的な状態チェックは virtual 化済みだが、能力問合せ系
（HP/MP 回復判定・呪い判定・ESP 判定・種族/職業/性格取得・特殊攻撃
防御判定等）は依然 `PlayerType` 固有のまま残存している。提案 1 の
共通化方針に沿い、これらもモンスター側でも意味ある応答を返せる形に
整備する。

### 候補メソッド

- HP/MP 自動回復判定関数群
- 祝福/呪い関連判定 (`is_cursed_item_used()` 等)
- ESP 判定 (`has_esp()`, `has_esp_evil()`, `has_esp_undead()` 等)
- `get_race()`, `get_class()`, `get_personality()`
- `has_special_attack(flag)`, `has_special_defense(flag)`

### 作業方針

モンスターを「安全なデフォルト値で済ます対象」とするのではなく、
プレイヤーと同等の応答を返せる主体として扱う。具体的には:

1. `CreatureEntity` に virtual 版を追加する
2. 対応する実体データ（`prace` / `pclass` / `muta` / `special_attack`
   等、提案 1 で共通化済み）を参照する共通実装を基底クラスに置く
3. プレイヤー固有の追加考慮事項（装備品の呪い状態・職業特典等）は
   `PlayerType` でオーバーライドして補強
4. モンスター固有の追加考慮事項（種族フラグ由来の耐性・ESP 等）は
   `MonsterProfile` 参照ロジックとして `CreatureEntity` の共通実装
   または別 virtual 分岐に組み込む
5. 呼出側の `is_player()` 分岐を削減し、クリーチャー共通 API として
   利用できるようにする

### 期待効果

- モンスターも ESP・呪い・特殊攻撃防御等の能力問合せ API を持てる
- プレイヤー / モンスターの分岐コードが削減される
- 将来モンスターに職業・種族・突然変異を運用する際の API が揃う

### 進捗

- ✅ ESP 判定（`has_telepathy()` / `has_esp_*()`）の virtual 化と
  読み取り side の call site 移行
- ✅ 視認・水泳・浮遊（`can_see_invisible()` / `has_can_swim()` /
  `has_levitation()`）の virtual 化と call site 移行
- ✅ 追加 BIT_FLAGS の virtual 化: `has_free_act()`, `has_anti_magic()`,
  `has_anti_tele()`, `has_regen_flag()`, `has_hold_exp()`,
  `has_slow_digest_flag()`, `has_see_nocto()`, `has_special_attack(flag)`,
  `has_special_defense(flag)`, `has_lite_flag()`, `has_warning_flag()`,
  `has_impact_flag()`, `has_earthquake_flag()`, `has_dec_mana()`,
  `has_easy_spell()`, `has_hard_spell()`, `has_mighty_throw()`,
  `has_xtra_might()` と該当 call site の移行
- ✅ EnumClassFlagGroup 構造体メンバの const 参照返し virtual アクセサ:
  `get_mutations()` / `get_traits()` / `get_cursed_flags()` /
  `get_cursed_special_flags()` と読取り用途 call site 427 箇所の移行
- ✅ プレイヤー情報ポインタ (race / personality / pclass_ref) に
  `get_race_info()` / `get_personality_info()` / `get_class_info()`
  virtual アクセサ追加・call site 移行
- ✅ スキル系・赤外線視（`get_skill_save()` / `get_skill_disarm()` /
  `get_skill_device()` / `get_skill_stealth()` / `get_skill_search()` /
  `get_skill_perception()` / `get_skill_to_hit_melee()` /
  `get_skill_to_hit_bow()` / `get_skill_dig()` / `get_infravision()`）
  の virtual 化と読取り 80 箇所の移行
- ✅ 配列系フィールド (`spell_exp[]` / `weapon_exp[][]` / `skill_exp[]`)
  は提案 10/36 で read/write virtual 整備済
- ✅ `stat_cur[]` / `stat_max[]` / `to_h[]` / `to_d[]` 等は提案
  30/31b/32b で virtual 化と private 化完了
- ✅ 種族 / 職業 / 性格 / 性別 / 魔法領域 / パトロン / 変身形態の
  getter/setter virtual 整備は提案 1 で完了

提案 2 は実質完了。残るプレイヤー専用 virtual の整備は具体的な
モンスター機能拡張時に逐次追加する方針。

---

## 提案 3: 残存する `PlayerType::get_instance()` 多用箇所の削減 ✅ 完了

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

### 進捗

- ✅ 同一関数内で複数回呼んでいた箇所をローカル束縛に整理（52 → 32 箇所）
- ✅ 残り 32 箇所はエントリポイント・シグナルハンドラ・UI
  コールバックの「プレイヤー参照の根」で正当な使用
- これ以上の削減は UI 層に `CreatureEntity &` 引数を渡す大規模変更が必要で、
  費用対効果が低いため現状でクローズとする

---

## 提案 4: 未統合の状態チェック関数の仮想化 ✅ 完了

### 背景

Phase 2 で主要な `is_xxx()` 系は仮想化済みだが、以下はまだ自由関数
または `PlayerType` 固有のまま残存していた。

### 候補

- `is_time_limit_esp()`, `is_time_limit_stealth()`
- `has_resist_*()` 系（耐性チェック）
- `has_immune_*()` 系
- `has_lite()`, `has_dark_source()` 系
- 視界/光源関連判定

### 作業方針

提案 1・2 と同様、モンスター側でも意味ある応答を返す主体として
扱う。耐性・免疫・光源判定等はモンスター種族フラグ (`MonraceDefinition`
側の flags) から導出できるケースが多いので、`CreatureEntity` 共通
実装で `MonsterProfile` 経由の参照と `PlayerType` 経由の参照を
統合する。

### 完了内容

- ✅ `is_time_limit_esp()` / `is_time_limit_stealth()` は初期から virtual 化済み
- ✅ 耐性/免疫/弱点系 30 種 (has_resist_fire/cold/elec/acid/pois/conf/
  sound/lite/dark/chaos/disen/shard/blind/neth/time/water/fear/curse,
  has_vuln_curse/acid/elec/fire/cold/lite, has_immune_fire/cold/acid/
  elec/dark/lite) を virtual メソッド化。自由関数 (player-status-flags)
  に委譲する形でデフォルト実装を提供
- ✅ call site 263 箇所を `creature.has_resist_X()` 形式に一括移行
  (38 ファイル)
- ✅ 装備集計系 10 種 (has_pass_wall / has_kill_wall / has_reflect /
  has_two_handed_weapons / has_sh_fire / has_sh_elec / has_sh_cold /
  has_down_saving / has_no_ac / has_easy2_weapon) を virtual 化し
  call site 移行 (22 ファイル、約 50 箇所)
- ✅ モンスター側 override を実装: 耐性/免疫/弱点系 30 virtual に
  `MonraceDefinition::resistance_flags` (RESIST_X / IMMUNE_X /
  HURT_X / NO_FEAR / NO_CONF) 経路を追加。has_pass_wall /
  has_kill_wall / has_can_swim / has_levitation に `feature_flags`
  (PASS_WALL / KILL_WALL / CAN_SWIM / CAN_FLY) 経路、
  has_reflect / has_regen_flag に `misc_flags` (REFLECTING /
  REGENERATE) 経路を追加。これでモンスターから耐性・特殊能力の
  問合せを呼ぶと種族フラグ由来の意味ある応答が返る。
- ✅ 残存していた外部自由関数呼出 13 箇所を virtual API 経由に移行
  (specific-object/chest, object-use/quaff/quaff-effects,
   object-use/read/scroll-read-executor,
   monster-floor/monster-sweep-grid, player/player-status-resist,
   player/permanent-resistances)

### 設計判断: 自由関数 → virtual への完全本体移植は実施しない

`player-status-flags.cpp` 内の自由関数本体 (約 65 関数) は以下のため
**現状の delegation pattern を維持**する:

1. **内部相互依存**: `has_resist_fire()` 内で `has_immune_fire()` を
   呼ぶなど、自由関数が互いを building block として利用している
2. **`get_player_flags()` 巨大 switch**: tr_type → 自由関数の集中
   ディスパッチが存在し、自由関数を utility primitive として保持
   する設計が合理的
3. **virtual は public API、自由関数は internal helper の役割分担**:
   外部 (アプリ層) は virtual 経由、内部 (player-status-flags の
   建築物) は自由関数経由という二層構造が機能的に明瞭

完全本体移植は工数大で構造的価値は限定的であり、提案 4 は
**「外部 call site の virtual 移行と monster_profile override 整備」
を完了形** として確定する。

### 残作業

なし。新規 call site では引き続き `creature.has_resist_X()` 形式の
virtual を使用すること (自由関数は internal helper 扱い)。

---

## 提案 5: TimedEffects オブジェクトとの二重管理解消 ✅ 完了

### 背景

かつてプレイヤーの一部効果 (スタン / 混乱 / 恐怖 / 加速 / 減速 / 麻痺 / 盲目 /
幻覚 / 切り傷 / 毒 / 対邪悪結界) は `timed_effects` (`TimedEffects` オブジェクト)
と `timed_effects_map` の両方で管理されており、`get_timed_effect()` は
TimedEffects 優先の特殊分岐を持っていた。一方モンスターは map のみ使用。

提案 5 完了で、選択肢 **(a) 全 effect を `timed_effects_map` に寄せる**
を採用した。

### 完了内容

- ✅ `TimedEffects` クラスおよび配下 9 クラス
  (`PlayerAcceleration` / `PlayerBlindness` / `PlayerConfusion` /
  `PlayerDeceleration` / `PlayerFear` / `PlayerHallucination` /
  `PlayerParalysis` / `PlayerPoison` / `PlayerProtection`) を**完全削除**
- ✅ `PlayerStun` / `PlayerCut` は **stateless な static utility class**
  に変換。高機能 API (`get_rank` / `get_magic_chance_penalty` /
  `get_accumulation` / `get_expr` / `get_damage` / `is_knocked_out` 等)
  は全て `static T method(short value)` の形に統一
- ✅ `CreatureEntity::effects()` と `timed_effects` shared_ptr を削除
- ✅ `get_timed_effect()` / `set_timed_effect()` を **map 単一管理に簡略化**
  (`SLEEP_OR_PARALYSIS` のみ `PARALYSIS` への正規化分岐を残置)
- ✅ 全外部 callsite (~30 箇所、約 25 ファイル) を migration
  - `effects()->X().is_X()` → `creature.is_X()` (virtual)
  - `effects()->X().current()` → `creature.get_timed_effect(...)`
  - `effects()->X().set(v)` → `creature.set_timed_effect(..., v)`
  - `effects()->X().reset()` → `creature.set_timed_effect(..., 0)`
  - `effects()->stun().get_rank()` → `PlayerStun::get_rank(creature.get_timed_effect(STUN))` 等
- ✅ `Makefile.am` から 10 ファイル分のエントリ削除
- ✅ `#include "timed-effect/timed-effects.h"` を 60 ファイル超から一括削除
- ✅ 切断された transitive include 経路 (TERM_COLOR 型等) を必要箇所に
  `#include "term/term-color-types.h"` で再導入

### 効果

- **二重管理を完全解消**: 全タイムドエフェクトの単一ストレージは
  `CreatureEntity::timed_effects_map` の 1 箇所のみ
- **クリーチャー共通**: プレイヤー・モンスターで完全に同一の API 経路、
  同一のストレージ機構を使用
- **メモリ削減**: 全 CreatureEntity から shared_ptr<TimedEffects>
  ヘッダオーバーヘッドが消失。プレイヤーで参照していた 11 個の
  `short` フィールドも消失 (map エントリで代替)
- **保守性向上**: PlayerStun / PlayerCut は static utility に集約され、
  値と計算ロジックが疎結合化

### 残作業

無し (提案 5 完了)。なお `PlayerStun` / `PlayerCut` は名前空間として
`Stun` / `Cut` に改名する余地があるが、上流変愚との衝突回避のため
現状の `Player*` プレフィックスを維持する。

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
| `exp` / `max_exp` / `max_max_exp` | そのままクリーチャー共通で | モンスター側も経験値概念を将来運用 |
| `hp_frac` / `csp_frac` | `hp_fraction` / `mp_fraction` | 可読性のみ |

### 注意

改名 PR は diff が巨大化し、変愚マージ時の衝突を増やす。
**提案 1-5 が全て完了してから着手する**のが安全。
改名時は一括 sed 実行 + ビルド確認を自動化したスクリプトを用意すること。

---

## 提案 10/36: 熟練度配列の virtual アクセサ ✅ 完了

### 背景

`spell_exp[64]` (固定配列) / `weapon_exp` / `weapon_exp_max` /
`skill_exp` (`std::map`) はプレイヤーの熟練度システム用フィールドだが
`CreatureEntity` 直下に存在するため、モンスターからもアクセス可能な
構造になっている。

### 作業内容

#### read 側 (提案 10 前半、既完了)

3 つの read-only virtual インデックサ:

| 仮想関数 | 役割 |
|---|---|
| `get_spell_exp(int idx)` | 呪文熟練度を返す |
| `get_skill_exp(PlayerSkillKindType)` | スキル熟練度を返す (map に無ければ 0) |
| `get_weapon_exp(ItemKindType, int sval)` | 武器熟練度を返す (map に無ければ 0) |

#### write 側 (提案 36、新規完了)

7 つの write virtual を追加:

| 仮想関数 | 役割 |
|---|---|
| `set_spell_exp(int idx, SUB_EXP value)` | 呪文熟練度を設定 |
| `add_spell_exp(int idx, SUB_EXP delta)` | 呪文熟練度に加算 |
| `set_skill_exp(PlayerSkillKindType, SUB_EXP)` | スキル熟練度を設定 |
| `add_skill_exp(PlayerSkillKindType, SUB_EXP delta)` | スキル熟練度に加算 |
| `set_weapon_exp(ItemKindType, int sval, SUB_EXP)` | 武器熟練度を設定 |
| `add_weapon_exp(ItemKindType, int sval, SUB_EXP delta)` | 武器熟練度に加算 |
| `set_weapon_exp_max(ItemKindType, int sval, SUB_EXP)` | 武器熟練度上限を設定 |

加えて `get_weapon_exp_max(tval, sval)` virtual も新規追加。

### 移行結果

- 読取り 31 箇所 (spell_exp 4, skill_exp 22, weapon_exp 5) を get_X_exp() 経由に置換 (提案 10)
- 書込み 13 箇所を新 virtual 経由に置換 (提案 36)
  - wizard-special-process.cpp: 7 site
  - load/player-info-loader.cpp: 3 site
  - cmd-action/cmd-spell / monster-attack-player /
    quaff-effects / birth-stat: 各 1-2 site
- `weapon_exp_max[tval][sval]` 読取り 4 箇所を `get_weapon_exp_max()` に置換
  (object-hook/hook-weapon, knowledge-experiences)

### 残存パターン (private 化できない理由)

以下のパターンは API 抽象化が困難なため direct access 残置:

- `auto &exp = creature.weapon_exp[tval][sval]` のリファレンス取得
  (player-skill.cpp の `gain_attack_skill_exp` が `SUB_EXP &` を受ける)
- `for (auto &[type, exp] : creature.skill_exp)` の構造化束縛
- `for (auto &exp : creature.weapon_exp[tval])` の配列走査
- `std::span(creature.spell_exp)` の span 構築
- `creature.weapon_exp = class_skills_info[pclass].w_start` の map 全体代入
- `for (auto sval = 0U; sval < creature.weapon_exp[tval].size(); ++sval)` の
  size() 経由イテレーション

これらの抽象化には functor 受け取り API (e.g.
`apply_to_weapon_exp(tval, [&](SUB_EXP &exp){ ... })`) や
配列スパン返却 virtual (e.g. `get_weapon_exp_span(tval)`) が
必要だが、コール側の書き換えコストが大きく抽象化価値が低いため見送り。

### 効果

- 単純な write/read パターンはプレイヤー・モンスター共通の API を通る
- 将来モンスター熟練度導入時の override 点を確保
- 「set_X_exp で書込みすると整合性チェックが走る」等の拡張余地

---

## 提案 9: MonsterProfile フィールド virtual アクセサ化 ✅ 一部完了

### 背景

`MonsterProfile` の主要フィールド (alliance_idx / sub_align /
parent_m_idx / smart / hold_o_idx_list) はそれぞれ十数件以上の
直アクセスを持ち、プレイヤー側 (MonsterProfile を持たない) から
呼ぶと tl::optional dereference でクラッシュする問題を抱えていた。
提案 7 (ml) と同様の方針で virtual アクセサに集約する。

### 作業内容

`CreatureEntity` に 5 つの virtual アクセサを追加 (read-only):

| 仮想関数 | 役割 | プレイヤー側デフォルト |
|---|---|---|
| `get_alliance_idx()` | アライアンス所属 | `AllianceType::NONE` |
| `get_sub_align()` | サブアライメント | `SUB_ALIGN_NEUTRAL` |
| `get_parent_m_idx()` | 召喚親モンスター | `0` |
| `get_smart_flags()` | smart_learn フラグ群 (const ref) | 空フラグ集合 |
| `get_held_objects()` | 保持アイテム ObjectIndexList (const ref) | 空リスト |

### 移行結果

- 読取り 54 箇所を新 virtual 経由に置換
- 書込み・破壊的メソッド呼出 (set/reset/clear/add/pop_front 等) 58 箇所は
  従来通り `monster.get_monster_profile().X` 形式で残置
  (将来 setter virtual を整備する余地)

### 効果

- プレイヤー側で誤って呼んだ際の null dereference リスクを排除
- `if (creature.has_monster_profile())` ガード忘れによるバグを防止
- 提案 7 と同パターンで一貫性のある拡張

### 残作業

- mflag (227 箇所) の virtual 化は粒度を細かく (is_chameleon / is_kage 等
  個別 virtual) して別バッチで進めると衝突を抑えられる
- 書込み側 setter virtual の整備は call site が多様な mutation
  (set/reset/clear/pop/push) を行うため、virtual 経由で全て表現すると
  抽象コストが大きい。優先度低として保留

---

## 提案 8: HP/MP 自然回復計算の virtual hook 化 ✅ 完了

### 背景

`compute_regen_amount(CreatureEntity &)` 内に `if (creature.is_player())`
分岐が 4 箇所あり、満腹度・スタンス・呪い・ミュータント体質のプレイヤー
固有処理が直書きされていた。モンスター回復計算 `regenerate_monsters()`
からも同関数が呼ばれており、is_player() ガードで適切にスキップされていたが、
将来モンスターに同等の概念を持たせる際の拡張点が定義されていなかった。

### 作業内容

`CreatureEntity` に 4 つの virtual hook を追加:

| 仮想関数 | 役割 | デフォルト | PlayerType override |
|---|---|---|---|
| `should_skip_natural_regen()` | 完全停止判定 | false | KOUKIJIN 構え or HAYAGAKE 行動なら true |
| `get_base_natural_regen_amount()` | ベース量取得 | `PY_REGEN_NORMAL` | 満腹度に応じて NORMAL/WEAK/FAINT/0 |
| `apply_state_regen_modifier(int)` | 構え・呪い補正 | identity | 僧/侍構え `/2`、SLOW_REGEN 呪い `/5` |
| `apply_creature_specific_regen_modifier(int)` | 最終固有補正 | identity | `mutant_regenerate_mod` 適用 |

### 効果

- `compute_regen_amount()` から `is_player()` 分岐 4 箇所を排除
- プレイヤー固有処理が `PlayerType` 内に集約され、モンスターの hook 上書きで
  個体差・種族別回復補正を導入できる下地が整備
- 関数本体はベース量算出 → 状態補正 → 行動/地形補正 → 最終補正の流れが
  読み取りやすくなる

### 計算結果の同等性

各 virtual のデフォルト・override 順序は元の is_player ガード順を保持しているため、
プレイヤー・モンスターの数値結果は完全一致 (回帰なし)。

---

## 提案 7: モンスター可視判定 (`ml`) の virtual アクセサ化 ✅ 完了

### 背景

`MonsterProfile::ml` (bool) はプレイヤーから見えるかを保持する一時変数で、
全コードベース 119 箇所で `monster.get_monster_profile().ml` の形で
直接アクセスされていた。MonsterProfile を持たないクリーチャー
(=プレイヤー) でアクセスすると `tl::optional` の dereference でクラッシュ
するため、慣用句的に呼び側で `has_monster_profile()` ガードや
`ternary` で `false` フォールバックを書く必要があった。

### 作業内容

- `CreatureEntity` に virtual `is_visible_on_map()` を追加
  (デフォルト実装: `monster_profile ? ml : false`)
- 同 virtual `set_visible_on_map(bool)` を追加
  (デフォルト実装: monster_profile があれば書込み、なければ no-op)
- 既存呼び側を sed 一括移行: 読取り 113 箇所、書込み 6 箇所、
  43 ファイルに渡って `is_visible_on_map()` / `set_visible_on_map(...)`
  形式へ置換
- `is_seen()` (geometry.cpp) と `is_original_ap_and_seen()`
  (monster-info.cpp) の `has_monster_profile()` ガードは
  virtual 内部で吸収されるようになったため削除

### 効果

- 119 箇所の直接アクセスを virtual 経由に集約
- ガード忘れ・null dereference リスクの恒久的排除
- 将来モンスターに「視認状態」概念を追加するときの拡張ポイント確保

---

## 提案 11: モンスターアイテム所持の inventory[] 統一 ✅ 完了 (フェーズ A)

### 背景

bakabakaband ではプレイヤーとモンスター双方が `CreatureEntity` インスタンスとして
`inventory[INVEN_TOTAL]` を持つ統一的なデータ構造を整備していたが、モンスターの
アイテム所持は引き続きレガシーの `MonsterProfile::hold_o_idx_list` (floor.o_list
上に held_m_idx を立てて表現する index リスト) で管理されていた。これにより:

- プレイヤーとモンスターでアイテム所持の経路が二重化
- floor.o_list 上に「held_m_idx 付きアイテム」が orphan として残存しやすい
- monster.inventory[] フィールドは存在するも実際には使われていない

### 作業内容 (5 サブフェーズ)

- **A-1**: monster pickup を inventory[] 並走書込み (5710abdd8)
- **A-2**: monster drop を inventory[] 経路へ切替 (ee2012106)
- **A-3**: shoot/eating の取得経路を inventory[] 並走 (8358fc90f)
- **A-4a**: hold_o_idx_list の参照経路を inventory[] へ移行 (dbce1c971)
  - polymorph / attack-chaos-effect / target-describer / monster-compaction
- **A-4b**: hold_o_idx_list / held_m_idx 経路完全廃止 + save/load マイグレーション (004d3fac2)
  - `MonsterProfile::hold_o_idx_list` field 削除
  - `CreatureEntity::get_held_objects()` virtual 削除
  - load 時に held_m_idx 付き orphan アイテムを wipe で破棄
- **A-5**: store_item / drop_all_inventory を `CreatureEntity` メンバに整備 (3d8579323)

### 効果

- モンスター所持アイテムは `CreatureEntity::inventory[INVEN_TOTAL]` に一本化
- floor.o_list 上の orphan が完全消滅
- フェーズ B/C で「装備」「アイテム使用」を構築する基盤が整備
- save format は backward compat (held_m_idx flag は load 時 wipe で吸収)

---

## 提案 12: モンスター装備の有効化 ✅ 完了 (フェーズ B)

### 背景

提案 11 でモンスターも inventory[] を実際に使うようになったが、装備スロット
(INVEN_MAIN_HAND..INVEN_TOTAL) は単なる格納場所でしかなく、装備品の効果が
モンスターのステータスに反映されていなかった。

### 作業内容

- **B-1**: pickup 時の自動装備 (4031a2962)
  - `wield_slot()` で空きスロット判定 → `acquire_item()` で直接装着
- **B-2**: AC / 耐性の装備反映 (f0535d0aa)
  - `get_ac()`: モンスター時 inventory 走査で `item.ac + item.to_a` 加算
  - `has_resist_X()` 等 30 virtual: `::has_resist_X(*this)` を OR して
    装備品 + ULTIMATE_RESISTANCE 等を反映
- **B-2b**: 近接攻撃のダメージボーナス (7a9cda1ec)
  - HIT/PUNCH/SLASH/STING 打撃で `weapon.damage_dice.roll() + weapon.to_d` 加算
- **B-2c**: 近接攻撃の命中ボーナス (11058a278)
  - `to_h` を `check_hit_from_monster_to_player` の power に加算
- **B-4**: look 表示で装備品/パック品を区別 (145f1fb56)
  - 「装備している」vs「持っている」分離表示
- **二刀流対応** (57339aa0c)
  - MAIN/SUB 両方に melee weapon があれば blow index 偶奇で交互使用

### B-3 (装備フラグキャッシュ) は DEFERRED

- on-the-fly O(14) iter / call で性能十分
- 実 profiling で hot spot 判明したら再検討

### 効果

- 装備武器を持つ TAKE_ITEM 行動モンスターは 命中 + ダメージ + AC + 耐性 すべて強化
- ペットに与えた装備が発動して戦闘力 UP
- 「two-handed orc with flaming sword and shield」のような構成が機能する

---

## 提案 13: モンスター AI のアイテム能動使用 ✅ 完了 (フェーズ C)

### 背景

提案 12 でモンスター装備は機能するようになったが、消耗品 (ポーション/巻物/杖/ロッド)
は inventory[] にあるだけで使われない状態だった。プレイヤーと同等のアイテム
活用 AI を実装することで、戦術的なモンスター戦闘を実現する。

### 作業内容

- **C-1**: モンスター/ペットの自己使用 AI (`monster-processor.cpp`)
  - 回復系ポーション 5 種 (080d4dbf3)
  - 拡張: バフ/解毒/恐怖解除/加速/耐性/無敵 全 12 ポーション (21e3215fd)
  - 巻物 PHASE_DOOR/TELEPORT (dbcabb762) → TELEPORT_LEVEL 追加 (fa873535f)
  - 巻物 SUMMON_MONSTER/UNDEAD/KIN 追加 (49a7fa33b)
  - 杖 HEAL_MONSTER/HASTE_MONSTER + ロッド HEALING/SPEED/CURING (92032a3f5)
  - 攻撃用 wand/rod 17 種 (e9cebf58a) — MAGIC_MISSILE/各種 BOLT/BALL/HYPODYNAMIA/DRAGON_*
  - WAND_TELEPORT_AWAY (防衛離脱) (fa873535f)
  - WAND_CLONE_MONSTER (自身分裂) (49a7fa33b)
- **C-2**: 永続的な窃取 + 自動装備 (98976a44b)
  - 窃取後 `acquire_item()` 経由で装備可能なら即装備
- **C-3**: ペット装備指定 UI (`cmd-pet.cpp`)
  - 所持品確認コマンド (0f8589d4b)
  - give/take プレイヤー↔ペット 転送 (a6fbfeae4)
  - take は装備/パック両方から letter 選択 (6487e97de)
  - 複数ペット選択メニュー (6951daaf1)

### AI 状態判定の優先度設計

1. HP < 25%: 大回復系 + INVULNERABILITY + TELEPORT_LEVEL
2. HP < 50%: 中回復系 + WAND_TELEPORT_AWAY (防衛距離取り)
3. 重毒/恐怖: 解毒/Boldness
4. 戦闘中 + 未バフ: HEROISM/BERSERK
5. 戦闘中 + 未加速: SPEED
6. 戦闘中 + 耐性なし: RESISTANCE
7. 戦闘中 + 直射可能: 攻撃 wand/rod
8. 戦闘中: 召喚巻物 + WAND_CLONE_MONSTER

各ポーション/巻物/杖/ロッドに priority (0=最優先) を割り当て、最も状況に
適合した 1 個を毎ターン消費する。

### 効果

- TAKE_ITEM モンスターは戦闘中の生存性・脅威度が大幅に増加
- ペットも同じ AI で自己保護＋援護 (give した回復系を使う)
- 既存スペル AI (`mspell-attack`) と並列に動く副次行動として実装

### 残存検討対象 (今後の拡張余地)

- ペット AI で攻撃 wand/rod を hostile monster に向けて発動
  (現状 `is_hostile()` ガードのため敵対モンスターのみ攻撃 wand 使用)
  → **完了**: ペット AI 改善で対応済み (commit `25393af1f` 周辺)
- 戦略的脱出 (SCROLL_DESTRUCTION/GENOCIDE)
- POTION_NEO_TSUYOSHI 等の特殊効果対応
- 装備耐久 (`apply_artifact` に近い仕組みでモンスター装備も損耗)

---

## 提案 14: AI ターゲット選定ロジックの共通化 ✅ 完了

### 背景

`for (MONSTER_IDX i = 1; i < floor.m_max; i++)` で `m_list` を直接走査して
近接ターゲットを選ぶパターンが検出系・抹殺系・モンスター AI に多数散在し、
重複コードと細かい条件漏れの温床になっていた。これを `CreatureEntity` 上の
共通 API にまとめ、走査と is_valid 判定を基底クラスに集約する。

### 作業内容

- `using CreaturePredicate = std::function<bool(const CreatureEntity &)>;`
  を導入
- 共通 API 3 種を `CreatureEntity` に追加:
  - `find_nearest_creature(predicate, require_projectable)` - 述語に
    マッチする最近接モンスター (オプションで projectable() 必須)
  - `has_visible_creature(predicate)` - 任意一致判定
  - `collect_creatures(predicate)` - 該当 idx を vector で返す
- 適用先 (commit `d5fa40fb2`):
  - `monster-processor`: ai_is_in_fighting_context、wand/rod 発動先選択
  - `cmd-pet`: ペット選択
  - `target-preparation`: 表示候補絞り込み (一部)

### 提案 14b: 残り走査箇所への横展開 ✅ 完了 (commit `5392b40c4`)

- `spell-kind/spells-detection`: detect_monsters_normal/invis/evil/
  nonliving/mind/string (6 箇所)
- `spell-kind/spells-pet`: discharge_minion (2 箇所)
- `spell-kind/spells-genocide`: mass_genocide / mass_genocide_undead
- `wizard/wizard-special-process`: wiz_zap_surrounding/floor_monsters
- `monster/monster-update`: update_monsters
- `monster/monster-compaction`: 前段ループ (後段の dead-excise 逆順
  ループはアルゴリズム特性上スキップ)

### 効果

- m_list 直接走査は `find_nearest_creature` / `collect_creatures` /
  `has_visible_creature` の 3 つの共通 API に集約
- ターゲット選定ロジックは predicate を渡すだけで宣言的に書ける
- `is_valid()` チェック漏れが構造的に防止される

---

## 提案 15: mflag2 の virtual アクセサ集約 ✅ 完了

### 背景

`monster.get_monster_profile().mflag2.has(MonsterConstantFlagType::X)`
パターンが 70+ 箇所に散在し、`MonsterProfile` への直接アクセスを助長
していた。CreatureEntity 上の virtual で一元化することで API の統一感
向上と将来のフィールド変更時の影響範囲削減を狙う。

### 作業内容

- `has_constant_flag(flag)` 共通ヘルパを追加
- 高頻度フラグの個別 virtual を実装 (commit `d4fe8ad4c`):
  - `is_kage` / `is_frenzied` / `is_chameleon` / `is_cloned` /
    `is_nopet`
- 関連サイトの読取りを virtual 経由に変更

### 提案 15b: 残り mflag2 フラグの virtual 化 ✅ 完了 (commit `12e6da091`)

- 体格修飾子 (9 個): `is_huge` / `is_large` / `is_small` / `is_fat` /
  `is_gaunt` / `is_lightweight` / `is_naked` / `is_zombified` /
  `is_illegal_modified`
- フレーバー個体 (4 個): `is_santa` / `is_angered` / `is_waifuized` /
  `is_quylthlug_born`
- 行動状態 (4 個): `is_defecated` / `is_vomited` / `has_noflow` /
  `is_nogeno`
- 13 ファイルで 50+ 箇所の `mflag2.has(...)` を virtual に変換

### 効果

- モンスターサイズ・特殊状態の判定が `monster.is_huge()` 等の自然な
  読み方に統一
- `MARK`/`SHOW` などの内部フラグ以外は virtual 経由で扱える状態に

---

## 提案 16: mflag (MonsterTemporaryFlagType) の virtual 化 ✅ 完了

### 背景

提案 15 と同じパターンで、一時フラグ (`mflag`) の参照・更新も多数の
箇所に散在していた。

### 作業内容 (commit `8e6668ab6`)

- `has_temporary_flag(flag)` 共通ヘルパ
- `set_temporary_flag(flag)` / `reset_temporary_flag(flag)` の write 系
  virtual
- 個別アクセサ (6 個):
  - `is_in_view` (VIEW)
  - `is_marked_for_los` (LOS)
  - `is_sensed_by_esp` (ESP)
  - `was_present_at_turn_start` (PRESENT_AT_TURN_START)
  - `has_prevent_magic` (PREVENT_MAGIC)
  - `has_sanity_blast` (SANITY_BLAST)
- 10 ファイルで 40+ 箇所の `mflag` 直接アクセスを virtual 経由に変換

### 効果

- 一時フラグのテレパシー/視認/PREVENT_MAGIC 判定が OO 経由に統一
- `mflag.clear()` (savefile 復元・初期化) のみ direct access を残置 →
  提案 20 で virtual 化

---

## 提案 9b: MonsterProfile setter の virtual 集約 ✅ 完了

### 背景

提案 9 で read-side virtual を整備したのに対し、書込側はまだ
`monster.get_monster_profile().alliance_idx = X` 等の直書きが残っていた。

### 作業内容 (commit `243ad252b`)

- 6 setter virtual を追加:
  - `set_alliance_idx(AllianceType)`
  - `set_sub_align(BIT_FLAGS8)` / `add_sub_align(BIT_FLAGS8)`
  - `set_parent_m_idx(MONSTER_IDX)`
  - `add_smart_flag(MonsterSmartLearnType)` / `clear_smart_flags()`
- 11 ファイルで 40 箇所の MonsterProfile 直書きを virtual 経由に変換
- 副次効果: `is_player()` ガードが基底側に集約され、誤ってプレイヤー
  に書き込んでも no-op で安全になる

---

## 提案 17: モンスター inventory 操作の OO 統合 ✅ 完了

### 背景

`store_item_to_inventory(creature, ItemEntity *)` 等の free function
が広く使われており、`creature.store_item(item)` の OO 形式と二重に
存在していた。

### 作業内容 (commit `53591dd83`)

- `can_store_item(const ItemEntity &)` virtual を追加 (既存
  `store_item` / `acquire_item` / `drop_all_inventory` を補完)
- 約 20 箇所の `store_item_to_inventory(creature, &item)` →
  `creature.store_item(item)` に変換
- `monster_drop_carried_objects(creature, target)` ラッパを削除し
  `target.drop_all_inventory(creature)` に直接置換 (cmd-pet /
  monster-death の 2 箇所、ヘッダ宣言とインクルードも削除)

### 効果

- インベントリ操作はプレイヤー・モンスター共に同じ OO 形式で記述
- free function は内部実装としてのみ存続 (CreatureEntity の virtual
  実装が委譲)

---

## 提案 18: mflag2 書込パターンの virtual 集約 ✅ 完了

### 背景

提案 15/15b で読取り、提案 9b で MonsterProfile の他の setter を
virtual 化したが、`mflag2.set/reset(...)` 直書きはまだ 60+ 箇所残存
していた。

### 作業内容 (commit `8185b0a6b`)

- `set_constant_flag(flag)` / `reset_constant_flag(flag)` virtual 追加
- 初期化リスト版 `set_constant_flags({...})` /
  `reset_constant_flags({...})` も追加 (MARK/SHOW 同時セット等)
- 14 ファイルで 60+ 箇所の `mflag2.set/reset(...)` を virtual に変換
- 不随修正: `check_store_item_to_inventory()` を `const CreatureEntity &`
  受けに変更し、提案 17 で追加した `can_store_item()` const メンバから
  呼べるように修正。提案 17 の sed 移行で混入した
  `check_creature.store_item(item)` 不正パターン 4 箇所も修正

---

## 提案 19: 変身・自壊カウンタの virtual 化 ✅ 完了

### 背景

`MonsterProfile::transform_r_idx` / `transform_hp_threshold` /
`has_transformed` / `death_count` の各フィールドへの直書きが残存。

### 作業内容 (commit `d44abbfcb`)

- 9 virtual 追加:
  - `get_transform_r_idx` / `set_transform_r_idx`
  - `get_transform_hp_threshold` / `set_transform_hp_threshold`
  - `has_transformed` / `set_has_transformed`
  - `get_death_count` / `set_death_count` / `decrement_death_count`
- 5 ファイルで 20 箇所を virtual 経由に変換 (one-monster-placer /
  monster-damage / monster-processor / monster-loader-savefile50 /
  monster-writer)

### 効果

- MonsterProfile への `=` 直接代入は基本的に消滅
- 残るのは savefile 復元時の `mflag/mflag2.clear()` 一括クリアと
  bitset 演算のみ → 提案 20 で対応

---

## 提案 20: 残り mflag/mflag2 一括操作の virtual 化 ✅ 完了

### 背景

提案 18 までで個別 set/reset は集約したが、savefile 復元時の bitset
一括代入 (`mflag2[X] = bool`)、全クリア (`mflag.clear()`)、モンスター
ボール捕獲時の bit 集合まるごとコピー等が残存していた。

### 作業内容 (commit `72f4f76c3`)

- 5 virtual 追加:
  - `assign_constant_flag(flag, bool)` - savefile での `mflag2[X] = bool`
    風代入の OO 形
  - `clear_constant_flags()` - mflag2 全クリア
  - `clear_temporary_flags()` - mflag 全クリア
  - `get_all_constant_flags()` - モンスターボール捕獲時の mflag2 抽出
  - `set_all_constant_flags(group)` - モンスターボール解放時の mflag2 適用
- 4 ファイルで一括操作系 9 箇所を virtual 経由に変換 (one-monster-placer /
  monster-loader-savefile50 / monster-ball / effect-monster-charm)

### 効果

- MonsterProfile の各フィールドへの外部からの直接書き込みは事実上
  消滅
- 残る direct access は `migrate_bitflag_to_flaggroup()` / `rd_FlagGroup()`
  等の低レベルヘルパに参照を渡す savefile 専用箇所のみ

---

## 提案 21: MonsterProfile を class 化して private 化 ✅ 完了

### 背景

提案 9b〜20 で外部書き込みパスを CreatureEntity の virtual API に
集約し終えたため、MonsterProfile の構造体を class に変更し、全データ
メンバを private 化する。これにより今後フィールド追加時に CreatureEntity
側へのアクセサ実装を強制できる。

### 作業内容 (commit `e73be7d6b`)

- `struct MonsterProfile` を `class MonsterProfile` に変更し、12
  個のデータメンバを private に
- friend 宣言:
  - `CreatureEntity` (virtual アクセサ実装用)
  - `MonsterLoader50` (savefile 復元時の bitset I/O)
  - `MonsterWriter` (savefile 書出時の bitset / フラグ抽出)
- 残っていた直接アクセス 3 箇所を migrate (player-processor /
  monster-update / target-preparation)

### 効果

- MonsterProfile が完全にカプセル化された
- フィールド追加時に CreatureEntity 側の virtual アクセサ整備が必須
  となり、API 統一性が構造的に維持される

---

## 提案 22: モンスター identity setter の virtual 化 ✅ 完了

### 背景

CreatureEntity 直下フィールドのうち、polymorph・evolution・savefile
復元等で書き換えられる identity 系 (`r_idx` / `ap_r_idx` / `riding`)
は依然として直書きされていた。書き込み経路を virtual に集約することで
identity 変化を追跡可能な形に整える。

### 作業内容 (commit `63ad4d86b`)

- 4 virtual 追加:
  - `set_r_idx(MonraceId)`
  - `set_ap_r_idx(MonraceId)`
  - `polymorph_to(MonraceId)` - r_idx と ap_r_idx をまとめて同期
  - `set_riding(MONSTER_IDX)` - mflag2 RIDING を更新しない低レベル setter
    (compaction や floor 切替時の付替え用。通常の騎乗は `ride_monster()`)
- 8 ファイル 11 箇所の write site を OO 経由に統一

### 注記

- read アクセス (`==` 比較等 70+ 箇所) はフィールド直接アクセスのまま
  維持。書き込み経路の一元化が目的のため大規模な read 側変更は不要と
  判断。
- CreatureEntity 直下のその他の生フィールド (`inven_cnt` / `equip_cnt` /
  `inventory[]` / `age` / `ht` / `wt` 等) のアクセサ整備は別提案
  (24 以降) として残置。

---

## 提案 23: ロードマップと CLAUDE.md の更新 ✅ 完了

### 背景

提案 14-22 (および本提案以降の 24-26) を実装した状態を反映できておらず、
CLAUDE.md の上流マージ用変換マッピング表も古い情報のままだった。

### 作業内容

- `docs/creature-entity-refactoring-roadmap.md` に提案 14 / 14b / 15 /
  15b / 16 / 9b / 17 / 18 / 19 / 20 / 21 / 22 (+ 後に 24 / 25 / 26) の
  節を追加
- `CLAUDE.md` の変換マッピング表に新規 virtual 全種を網羅
- Phase 9-22 の節を新設、API 整備の到達点を要約

提案 23 自体は文書のみで実装変更なし。提案 24 以降が新たに完了する都度、
随時更新する運用を継続する。

---

## 提案 24: CreatureEntity 直下生フィールドの setter 整備 ✅ 完了

### 背景

`inven_cnt` / `equip_cnt` / `age` / `ht` / `wt` / `prestige` の直書きが
各所に残存していたため、書き込み経路を OO 形式に統一する。

### 作業内容 (commit `1fff01033`)

- 新規 virtual (13 個):
  - `set_inven_cnt(short)` / `increment_inven_cnt()` /
    `decrement_inven_cnt()` (提案 25 で削除)
  - `set_equip_cnt(short)` / `increment_equip_cnt()` /
    `decrement_equip_cnt()` (提案 25 で削除)
  - `set_age(int16_t)` / `add_age(int16_t)`
  - `set_ht(int16_t)` / `set_wt(int16_t)`
  - `set_prestige(int16_t)` / `add_prestige(int16_t)` /
    `divide_prestige(int)`
- 15 ファイル約 30 箇所の生フィールド書込を OO 経由に統一

### 注記

`birther` 構造体 (`previous_char`) は CreatureEntity ではないため、
load/birth-loader での age/ht/wt/prestige 代入は直接フィールド
アクセスのまま維持。

---

## 提案 25: inven_cnt / equip_cnt の inventory[] からの自動計算化 ✅ 完了

### 背景

提案 24 で setter virtual を整備したが、これらカウントフィールドは
本来 inventory[] からの派生情報であり二重管理 (cache + 真実源) を
強いていた。inventory[] を唯一の真実源とし、カウントは on-the-fly
計算する形に整える。

### 作業内容 (commit `0d8b73e04`)

- フィールド `inven_cnt` / `equip_cnt` を CreatureEntity から削除
- 提案 24 で追加した cnt 系 setter virtual (`set_*` / `increment_*` /
  `decrement_*` 計 6 個) を全削除
- 計算 virtual を新設:
  - `get_inven_cnt()` - inventory[0..INVEN_PACK) を走査
  - `get_equip_cnt()` - inventory[INVEN_MAIN_HAND..INVEN_TOTAL) を走査
- 15 ファイルで読取り 14 箇所を `get_*()` に置換、setter 呼出を全削除
- savefile loader (inventory-loader / monster-loader-savefile50) も
  簡潔化 (cnt 操作を削除、inventory[] を埋めるだけに)

### 副次効果

- inventory[] を直接いじる箇所での cnt 同期忘れバグを構造的に防止
- フィールド数 2 個分メモリ削減
- savefile 互換性に影響なし (cnt は元々保存されておらず、ロード時に
  再計算されていた)

### 性能影響

- `get_inven_cnt()` は最大 INVEN_PACK (23) スロットの線形スキャン
- ホットパスで性能影響を観測した場合、提案 25b (キャッシュ層の再導入)
  を検討する余地あり
- → 後日 (提案 23 再々実施時) call site 数と頻度を検証した結果、全 12
  call site が低頻度 (savefile load / inventory pickup / death message /
  character dump 等)。最頻でも数十回/ターン × 70ns 程度で再導入不要と
  判定。詳細は本書末尾「今後の候補」節の 25b エントリ参照。

---

## 提案 26: ambush/food/town_num/level に setter virtual 追加 ✅ 完了

### 背景

CreatureEntity 直下の残り plain field のうち、書き換え経路が散在
していた ambush_flag / food / town_num / level に setter virtual を
整備し、書き込みを OO 経由に統一する。

### 作業内容 (commit `d3a5db8cd`)

- 新規 virtual (4 個):
  - `set_ambush_flag(bool)`
  - `set_food(int16_t)`
  - `set_town_num(int16_t)`
  - `set_level(int16_t)`
- 12 ファイル約 35 箇所の write site を OO 経由に統一
  - `ambush_flag` (12 箇所): wild / floor-changer / dungeon-processor /
    movement-execution / world-loader
  - `food` (3 箇所): player-info-loader / digestion-processor /
    game-play-initializer
  - `town_num` (15 箇所): wild / player-info-loader /
    store-key-processor / cmd-store
  - `level` (4 箇所): wizard-spoiler / game-closer /
    player-info-loader / game-play-initializer
- 読取りはフィールド直接アクセスのまま維持

---

## 提案 27: max_plv / msp / 経験値系の setter virtual ✅ 完了

### 背景

CreatureEntity 直下の戦闘ボーナス・経験値系フィールドのうち、
書き換え経路が散在している max_plv / msp / max_max_exp / max_exp / exp
に setter virtual を整備する。

### 作業内容 (commit `b996eb87b`)

- 新規 virtual (5 個):
  - `set_max_plv(int16_t)`
  - `set_msp(int)`
  - `set_exp(EXP)` / `set_max_exp(EXP)` / `set_max_max_exp(EXP)`
- 14 ファイル約 30 箇所の write site を OO 経由に統一
  - max_plv (6 箇所): player-info-loader / world-loader /
    dungeon-processor / display-lore / player-status /
    game-play-initializer
  - msp (3 箇所): player-info-loader / player-status
  - exp/max_exp/max_max_exp (約 20 箇所): wizard-special-process /
    player-info-loader / realm-hex / racial-android / inventory-curse /
    status/experience / player-status / monster-status / game-closer /
    monster-loader-savefile50

`au` / `csp` 等の compound assignment 中心のフィールドは別提案 (27b)
で対応。

---

## 提案 27b: au / csp の add/sub 系 virtual と compound assignment 移行 ✅ 完了

### 背景

提案 27 で残置した、`+=` / `-=` が大量に存在する `au` (所持金) /
`csp` (現在 MP) に setter / add / sub virtual を整備し、書き込み
経路を一元化する。

### 作業内容 (commit `b07bc1a33`)

- 新規 virtual (7 個):
  - `set_au(int)` / `add_au(int)` / `sub_au(int)` / `divide_au(int)`
  - `set_csp(int)` / `add_csp(int)` / `sub_csp(int)`
- 49 ファイル約 110 箇所の compound assignment を OO 経由に統一
  - au (45 箇所): cmd-building / cmd-trade / store / wizard / load /
    birth / racial-android / process-death 等
  - csp (66 箇所): spell-realm / realm / mind / cmd-* / mutation /
    hpmp / racial-android 等

### 効果

- 「魔法消費 → MP 減」「金額決済 → AU 減」等の操作が `add_csp` /
  `sub_au` 等の意味の明確な API に集約された
- 将来的に「MP/金額変化を hook で記録」「変化量を制限する rule」
  等の組み込みポイントとして利用可能

---

## 提案 28: r_idx / ap_r_idx / riding に getter virtual を追加 ✅ 完了

### 背景

提案 22 で setter virtual を整備した monster identity 系
(r_idx / ap_r_idx / riding) について、対応する getter virtual を
追加し、散在していた `monster.r_idx == X` 等の直接フィールド読取りを
`monster.get_r_idx() == X` 形式に統一する。これは将来的なフィールド
private 化 (提案 29) の前提条件となる。

### 作業内容 (commit `a4a41bf95`)

- 新規 virtual (3 個):
  - `get_r_idx()` / `get_ap_r_idx()` / `get_riding()`
- 75 ファイル約 210 箇所の `(creature|monster|target|m_ref|back_m).r_idx`
  / `.ap_r_idx` / `.riding` パターンの直接アクセスを sed で機械的に
  getter 経由に変換

ポインタ経由 (`m_ptr->r_idx` 等) のパターンは別途扱うため未対応 →
提案 28b で対応。

---

## 提案 28b: ポインタ経由の r_idx / ap_r_idx / riding を getter へ移行 ✅ 完了

### 背景

提案 28 で対象外とした `m_ptr->r_idx` 等のポインタ経由パターンを
getter virtual 経由に変換する。pointer dereference 経由でも
`->get_r_idx()` 等で読み取れる。

### 作業内容 (commit `5c56b14c0`)

- 22 ファイル約 80 箇所の `(m_ptr|md_ptr->m_ptr|md.m_ptr|m1_ptr|
  m2_ptr|y_ptr|t_ptr)->(r_idx|ap_r_idx|riding)` 読取りパターンを
  sed で `->get_*()` 形式に変換
- sed の trailing char クラス `[^a-zA-Z_0-9]` が末尾の空白を許容して
  しまったため、`m_ptr->r_idx = X` (write site) が誤って
  `m_ptr->get_r_idx() = X` に変換されていた箇所
  (one-monster-placer.cpp の chameleon 処理 6 箇所 + appearance 同期
  1 箇所) を発見し、適切な setter (`set_r_idx` / `set_ap_r_idx` /
  `polymorph_to`) に修正

### 効果

pointer dereference 経由の r_idx / ap_r_idx / riding 読取りも
すべて virtual API 経由になり、フィールド private 化 (提案 29) の
前提条件が完全に整った。

---

## 提案 29: r_idx / ap_r_idx / riding を private 化 ✅ 完了

### 背景

提案 28 / 28b で getter virtual の整備と read site 移行を完了したため、
CreatureEntity の r_idx / ap_r_idx / riding フィールドを実際に
private に移動する。これらは CreatureEntity 直下フィールドで
**完全 private 化に成功した最初の例** となる。

### 作業内容 (commit `e0e84a26a`)

- CreatureEntity::r_idx, ap_r_idx, riding を public から private に変更
  (該当箇所に `private:` セクションを挿入し、後続フィールドを
  `public:` に戻す)
- private 化に伴い破綻した残存直接アクセス約 15 箇所を getter 経由に
  追加移行:
  - effect/spells-effect-util / effect-monster-curse
  - monster-describer / monster-floor/one-monster-placer
  - player/player-skill (3 箇所)
  - target/target-sorter / target-preparation
  - main/scene-table-monster / window/display-sub-windows
  - action/activation-execution / floor/floor-changer / floor-leaver
  - mspell/mspell-special

### 注記

`CapturedMonsterType.r_idx` は別 struct のフィールドで CreatureEntity
ではないため migration 対象外。提案 28b の sed で誤って `cap_mon.r_idx`
を `cap_mon.get_r_idx()` に変換していた箇所を本提案で revert した。

### 効果

- これらフィールドへのアクセスは `get_r_idx()` / `set_r_idx()` /
  `polymorph_to()` / `get_riding()` / `set_riding()` /
  `ride_monster()` の virtual API 経由でのみ可能
- 同様の手法で他のアクセサ整備済みフィールド (au, csp, inventory,
  level 等) も今後 private 化できる雛形が確立された

---

## 提案 30: 戦闘ボーナスと stat[] 配列に setter virtual を追加 ✅ 完了

### 背景

CreatureEntity 直下の戦闘ボーナス系 (to_h_b / to_h_m / to_d_m /
to_a) と能力値配列 stat_*[A_MAX] への直接書込が散在していた。
to_h*/to_a 系は `calc_*()` 経由で再計算されるため write site は
少ないが、stat[] は 50 箇所以上の write site があった。

### 作業内容 (commit `1ef9f7722`)

- 新規 virtual (11 個):
  - 戦闘ボーナス: `set_to_h_b` / `set_to_h_m` / `set_to_d_m` /
    `set_to_a`
  - 能力値配列: `set_stat_max(idx, val)` / `set_stat_cur(idx, val)` /
    `add_stat_cur(idx, delta)` / `set_stat_max_max` /
    `set_stat_use` / `set_stat_top` / `set_stat_add` /
    `set_stat_index`
- 10 ファイル約 55 箇所を OO 経由に統一
  - 戦闘ボーナス (4 箇所): player-status
  - 能力値配列 (約 51 箇所): wizard-special-process / player-status /
    realm-hex / spells-status / base-status / birth-stat /
    quick-start / player-info-loader / player-basic-statistics

### 注記

- `previous_char` (struct birther) は CreatureEntity ではないため、
  load/birth-loader での stat 系代入は migration 対象外
- sed が誤って `creature.set_stat_cur(i, creature.set_stat_max(i,
  val))` のように void 戻り値を引数に渡す形に変換していた 2 箇所
  を 2 行に分けて修正
- 読取り (`stat_use[A_DEX]` 等の比較・参照) は引き続きフィールド
  直接アクセスのまま維持

---

## 提案 31: 残り plain field の getter virtual 整備と read 側移行 ✅ 完了

### 背景

提案 24-30 で setter 系を整備した CreatureEntity 直下フィールドに
ついて、対応する getter virtual を追加し、散在していた直接読取りを
getter 経由に統一する。これは将来的なフィールド private 化
(提案 32) の前提条件となる。

### 作業内容 (commit `b809ce221`)

新規 virtual (18 個):
- getter (14 個): `get_au` / `get_csp` / `get_food` / `get_town_num` /
  `get_age` / `get_ht` / `get_wt` / `get_prestige` / `get_max_plv` /
  `get_msp` / `get_exp` / `get_max_exp` / `get_max_max_exp` /
  `get_ambush_flag`
- 補助 (4 個): `add_exp` / `sub_exp` / `add_max_exp` / `sub_max_exp`
  (sed 誤変換修正用)

migration 対象 (109 ファイル, 約 350 箇所):
- `creature/monster/target.field` 直接アクセスの読取りパターンを
  getter 経由に sed で機械的に変換
- `s64b_sub/add(&creature.csp, ...)` のような lvalue が必要な 5 箇所
  は直接フィールドアクセスに戻す (hpmp-regenerator / experience /
  spells-song / spells-hex / player-processor)
- 後置 `creature.get_csp()--` 等の inc/dec 残骸は `sub_csp` /
  `sub_exp` 等に再修正
- write 操作 (`+=` / `-=`) が誤って `get_*() +=` 形に化けた約 8 箇所
  は `add_*` / `sub_*` virtual に再修正 (ring-of-power /
  realm-hex / inventory-curse / status/experience /
  monster/monster-status)

### 残置

- `level` (628 read sites) は分量が多いため別提案 (31c) として残置
- `stat_*[]` 配列 (200+ read sites) は提案 31b で対応

---

## 提案 31b: stat_*[] と to_h*/to_d*/to_a の getter virtual 整備と read 移行 ✅ 完了

### 背景

提案 30 で setter virtual を整備した能力値配列 (`stat_*[]`) と
戦闘ボーナス (`to_h*` / `to_d*` / `to_a`) について、対応する getter
virtual を追加し、read site を getter 経由に統一する。これにより
これらフィールド群の private 化 (将来の提案 32) の前提条件が整う。

### 作業内容 (commit `38b69beb5`)

新規 virtual (15 個):
- stat 系 getter (7 個): `get_stat_max(idx)` / `get_stat_cur(idx)` /
  `get_stat_max_max(idx)` / `get_stat_use(idx)` / `get_stat_top(idx)` /
  `get_stat_add(idx)` / `get_stat_index(idx)`
- 戦闘ボーナス getter (6 個): `get_to_h_b` / `get_to_h_m` /
  `get_to_d_m` / `get_to_a` / `get_to_h(hand)` / `get_to_d(hand)`
- 補助 setter (2 個): `set_to_h(hand, val)` / `set_to_d(hand, val)`
  (sed で誤って `creature.get_to_h(0) = X` に化けた write site を
  `set_to_h(0, X)` に再修正するため)

migration 対象 (62 ファイル, 約 270 箇所):
- `stat_*[idx]` read (約 200 箇所): 各種計算・表示・装備処理
- `to_h_b` / `to_h_m` / `to_d_m` / `to_a` の単純 read (約 25 箇所)
- `to_h[hand]` / `to_d[hand]` の配列 read (約 80 箇所)

### 残置

- `level` (628 read sites) は提案 31c で対応

---

## 提案 31c: level read site を get_level() 経由に移行 ✅ 完了

### 背景

CreatureEntity::level の read site は 628 箇所と分量が突出しており、
提案 31 / 31b では別提案として残置していた。本提案で残り全てを
`get_level()` virtual 経由に統一する。

`get_level()` は既存実装で「`level > 0` ならその値、そうでなければ
`monrace.level / 2` を返す」セマンティクスを持つため、モンスター
初期化漏れ等に対する安全な fallback も得られる。

### 作業内容 (commit `1234f65a3`)

migration 対象 (175 ファイル, 約 628 箇所):
- `(creature|monster|target).level` 直接読取りを `get_level()` に
  sed で機械的に変換

副次修正:
- sed が `creature.level--` / `creature.level++` を
  `creature.get_level()--` / `creature.get_level()++` に化けさせて
  いた 2 箇所 (player-status.cpp の experience レベルダウン/アップ)
  を `set_level(get_level() - 1)` / `set_level(get_level() + 1)` に
  再修正

### 注記

各種別 struct (`monrace.level` / `quest.level` / `baseitem.level` /
`m_lev.level` 等) は CreatureEntity ではないため migration 対象外。

### 効果

- CreatureEntity の主要フィールド read/write 両側が完全に virtual
  API 経由に統一された
- 残るは将来の提案 32 (整備済みフィールドの private 化) のみ

---

## 提案 32: 安全な 7 フィールドを private 化 ✅ 完了 (縮小スコープ)

### 背景

提案 24-31c で setter + getter virtual を整備した CreatureEntity 直下
フィールドを、提案 29 と同じパターンで実際に private に移動する。
当初は約 20 フィールドの一括 private 化を試みたが、`level` /
`food` / `csp` / `exp` 等の名前が ItemEntity / MonraceDefinition /
ArtifactType / ActivationType / QuestType 等多くのクラスと衝突する
ため、広範な sed 移行では誤って他クラスのフィールドも書き換えて
しまう問題が発生した。

そのため、本提案は他クラスと名前が衝突しない**安全な 7 フィールド
のみ**を対象とする縮小スコープで完了とする。

### 作業内容 (commit `7dc9857ba`)

private 化したフィールド (7 個):
- `ambush_flag` - `get_ambush_flag()` / `set_ambush_flag()` 経由
- `prestige` - `get_prestige()` / `set_prestige()` / `add_prestige()` /
  `divide_prestige()` 経由
- `max_max_exp` - `get_max_max_exp()` / `set_max_max_exp()` 経由
- `max_plv` - `get_max_plv()` / `set_max_plv()` 経由
- `to_h_b` - `get_to_h_b()` / `set_to_h_b()` 経由
- `to_h_m` - `get_to_h_m()` / `set_to_h_m()` 経由
- `to_d_m` - `get_to_d_m()` / `set_to_d_m()` 経由

各フィールドは提案 24-31c で migration 済みのため、private 化しても
直接アクセス エラーなしでビルドが通る (全 read/write が virtual API
経由になっている)。

### 未対応 (他クラスとの名前衝突のため)

以下のフィールドは他クラスに同名フィールドが存在し、広範な sed
移行が困難:
- `level` (MonraceDefinition / QuestType / BaseitemDefinition /
  ArtifactType / ActivationType 等)
- `food` / `age` / `ht` / `wt` / `town_num` / `au` / `csp` / `msp` /
  `exp` / `max_exp` / `to_h[]` / `to_d[]` / `to_a` / `stat_*[]`

これらは setter + getter virtual は完備されているため、call site の
直接アクセスは構造的に減らされている。完全な private 化には個別
ファイルごとに型を判別して手作業で migration する必要があり、別途
提案 32b 以降で順次対応する方針。

### 到達点

- **MonsterProfile** (12 フィールド) + **r_idx / ap_r_idx / riding**
  + **本提案の 7 フィールド** = 計 **22 フィールドが完全 private 化**
- `inven_cnt` / `equip_cnt` は提案 25 でフィールド自体を廃止
  (inventory[] から自動計算)

---

## 提案 32b: 残り 14 個の plain field を private 化 ✅ 完了

### 背景

提案 32 で安全な 7 フィールドのみ private 化したが、提案 32 で他クラス
との名前衝突を避けるためスコープから除外していた残りのフィールドに
ついても、個別に build エラーを fix する形で完全 private 化する。

### 作業内容 (commit `5221ea0ba`)

private 化したフィールド (15 個、to_a 再確認分含む):
- 基本情報: `age` / `ht` / `wt`
- ステータス配列: `stat_max[]` / `stat_max_max[]` / `stat_cur[]` /
  `stat_use[]` / `stat_top[]` / `stat_add[]` / `stat_index[]` (7 個)
- 経験値: `max_exp` / `exp`
- MP: `msp` / `csp`
- 街番号: `town_num`
- レベル: `level`
- 所持金: `au`
- 食料: `food`
- 戦闘ボーナス配列: `to_h[]` / `to_d[]`

新規 virtual (3 個):
- `add_csp_with_frac(int, uint32_t)` / `sub_csp_with_frac(int, uint32_t)`
- `add_exp_with_frac(EXP, uint32_t)`

(`s64b_add/sub` に `&creature.csp` のような private フィールドへの
lvalue 参照を直接渡せないため、ラッパとして導入)

### migration (10 ファイル)

- `core/player-processor`: cost s64b_sub
- `hpmp/hp-mp-regenerator`: this->food / s64b_add/sub for csp
- `mind/mind-power-getter`: level / csp / stat_index 関連
- `monster-floor/monster-sweep-grid`: csp/msp 比較式
- `monster-floor/one-monster-placer`: stat_max/cur/max_max/use の
  modifiers 適用 + `m_ptr->exp` 初期化
- `player-base/player-race`: `creature_ptr->level` 系
- `player/player-damage`: `this->level`
- `specific-object/stone-of-lore`: msp/csp 消費処理
- `spell-class/spells-mirror-master`: csp/msp/level
- `spell-realm/spells-hex`: s64b_sub for csp
- `spell-realm/spells-song`: s64b_sub for csp
- `status/experience`: s64b_add for exp
- `view/display-lore`: `dummy.level` 初期化
- `wizard/wizard-spoiler`: `dummy_p.level` 初期化

### 最終到達点

| カテゴリ | 完全 private 化済 |
|---|---|
| MonsterProfile (提案 21) | 12 個 |
| r_idx / ap_r_idx / riding (提案 29) | 3 個 |
| 提案 32 安全フィールド | 7 個 |
| 提案 32b 残りフィールド | 15 個 |
| 提案 33 BIT_FLAGS 群 (ESP / 装備集計 / 特殊攻撃防御) | 35 個 |
| 提案 34 表示用既知値 (dis_to_h/d/a/ac) | 5 個 |
| 提案 41 呪文マスク (spell_learned/worked/forgotten 1/2) | 6 個 |
| **合計** | **83 個** |
| inven_cnt / equip_cnt (提案 25) | フィールド廃止 |

CreatureEntity のフィールドカプセル化が**事実上最終形**に到達。
これらフィールドへのアクセスは全て virtual API 経由でのみ可能となり、
将来 friend 宣言を撤廃すれば完全閉鎖系となる。

---

## 提案 33: ESP / 装備集計 BIT_FLAGS の private 化 ✅ 完了

### 背景

提案 32 / 32b で plain field (能力値・経験値・所持金等) の private 化に
成功した後、CreatureEntity 直下に残る public BIT_FLAGS フィールド
(ESP 系 15 個 + 装備集計系 19 個 + special_attack/defense 2 個 = 36 個) を
カプセル化する。

read 側は既に提案 4 系列で `has_telepathy()` / `has_esp_X()` 等の
virtual に統一済みだったため、書込みパターンの集約と差分検出キャッシュ
読取りの整備のみで private 化可能。

### 完了内容

- ✅ ESP 15 個 (telepathy / esp_animal / esp_nasty / esp_homo /
  esp_undead / esp_demon / esp_orc / esp_troll / esp_giant /
  esp_dragon / esp_human / esp_evil / esp_good / esp_nonliving /
  esp_unique) の `set_X(BIT_FLAGS)` virtual を追加
- ✅ 装備集計系 19 個 (can_swim / levitation / free_act / see_inv /
  regenerate / hold_exp / slow_digest / lite / warning / impact /
  earthquake / dec_mana / easy_spell / hard_spell / mighty_throw /
  see_nocto / anti_magic / anti_tele / bless_blade / xtra_might) の
  `set_X(BIT_FLAGS)` virtual を追加
- ✅ `has_bless_blade()` virtual を新規追加 (既存 `has_X()` 群に
  欠けていたため補完)
- ✅ `get_X_flags()` 系 read virtual を 15 ESP + see_inv +
  mighty_throw + impact + earthquake に追加 (差分検出キャッシュ・
  bitmask 読取り用)
- ✅ special_attack / special_defense に
  `set_X_flags(BIT_FLAGS)` / `get_X_flags()` /
  `add_X(flag)` / `remove_X(flag)` の 4 virtual セットを追加
- ✅ player-status.cpp の update_creature() 内 35 箇所の
  `creature.X = has_X(creature)` 形式書込みを `creature.set_X(...)`
  形式に migration
- ✅ player-status.cpp の差分検出キャッシュ 15 箇所の
  `creature.X != old_X` を `creature.get_X_flags() != old_X` に migration
- ✅ special_attack/defense の compound assignment 約 18 箇所
  (`|= flag` / `&= ~flag`) を `add_X(flag)` / `remove_X(flag)` に migration
  (spell-realm/spells-craft / mspell/mspell-dispel / realm/realm-chaos /
   player-attack/attack-chaos-effect / scroll-read-executor / quaff-effects
   / status/bad-status-setter)
- ✅ savefile load/save 経路 (player-attack-loader / player-writer)
  と reset 経路 (status/buff-setter) を `set_X_flags(value)` 経由に
- ✅ player-attack/player-attack の `does_weapon_has_flag(BIT_FLAGS &)`
  を `does_weapon_has_flag(BIT_FLAGS)` に変更 (rvalue 受け入れのため)
- ✅ body-improvement-info.cpp / flavor-describer.cpp の bool 用途
  read を `has_X()` に置換
- ✅ **ESP 15 + 装備集計 19 + special_attack 1 = 35 個を private 化**

### 効果

- CreatureEntity 直下の plain public BIT_FLAGS field がほぼ消滅
- **合計 private 化フィールド数: 37 → 72** (約 2 倍)
- 装備変更時の状態再計算経路が明示的な setter API 経由に統一され、
  将来 setter 内で監査ログ・レンダフラグ自動連動などを追加可能
- special_attack/defense の compound assignment 約 18 箇所が
  意味的に明確な OO 形式 (`add_special_attack(ATTACK_X)`) に統一

### 残作業

なし。残る public BIT_FLAGS は `cursed` / `cursed_special`
(EnumClassFlagGroup 型、外部走査多数のため別提案で扱う)、
`cur_lite` / `old_lite` (POSITION 型キャッシュ、提案 24 系列で
扱う候補)、`hack_mutation` / `is_fired` / `level_up_message` /
`invoking_midnight_curse` (短命 bool フラグ、private 化価値低) のみ。

---

## 提案 34: 表示用既知値 (dis_to_h/d/a/ac) の private 化 ✅ 完了

### 背景

提案 30 で戦闘ボーナス本体 (to_h / to_d / to_a) を private 化したのに
対称な仕上げタスク。表示用の既知値キャッシュ 5 フィールド:

- `HIT_PROB dis_to_h[2]`: 表記上の近接武器命中修正値
- `HIT_PROB dis_to_h_b`: 表記上の射撃武器命中修正値
- `int dis_to_d[2]`: 表記上の近接武器ダメージ修正値
- `ARMOUR_CLASS dis_to_a`: 表記上の装備 AC 修正値
- `ARMOUR_CLASS dis_ac`: 表記上の装備 AC 基礎値

これらは player-status.cpp の update_creature() が装備状態から再計算し、
画面表示系 (display-player-middle / main-window-left-frame /
status-first-page / io-dump / cmd-building) が読み取るキャッシュ。

### 完了内容

- ✅ 5 フィールドの getter / setter virtual (合計 10 virtual) を追加
  - `get_dis_to_h(hand)` / `set_dis_to_h(hand, val)`
  - `get_dis_to_h_b()` / `set_dis_to_h_b(val)`
  - `get_dis_to_d(hand)` / `set_dis_to_d(hand, val)`
  - `get_dis_to_a()` / `set_dis_to_a(val)`
  - `get_dis_ac()` / `set_dis_ac(val)`
- ✅ 全 19 アクセスサイト migration
  - player-status.cpp 内 write 7 箇所
  - player-status.cpp 内 diff-check read 3 箇所
  - display-player-middle.cpp / main-window-left-frame.cpp /
    status-first-page.cpp / io-dump / cmd-building.cpp 内 read 9 箇所
- ✅ 5 フィールドを private 化

### 効果

- **合計 private 化フィールド数: 72 → 77**
- 提案 30 (to_h/d/a 本体) と提案 34 (表示用キャッシュ) で
  戦闘ボーナス系のカプセル化が完結
- 将来「装備変更時に dis_* と to_* を同時更新するインバリアント」を
  setter 内で強制可能に

### 残作業

なし。

---

## 提案 35: is_player() 分岐の縮減 ✅ 完了 (調査ベース・縮小スコープ)

### 背景

CreatureEntity 統合進行に伴い、コード中に `if (!creature.is_player()) return;`
等の早期 return ガードや `if (creature.is_player()) { ... }` の正の分岐が
合計約 83 サイト残存している。これらを virtual メソッド経由に変換できれば、
プレイヤー/モンスター分岐の明示性を減らせるという仮説の下に着手。

### 調査結果

調査の結果、コードベース上の `is_player()` 分岐は以下のように分類され、
**機械的な削減で得られる改善は極めて限定的**であることが判明:

| カテゴリ | サイト数 | 削減可否 |
|---|---:|---|
| 早期 return ガード (関数の player-only 契約) | ~63 | 削除不可 (型契約) |
| 表示分岐 (player/monster で表示内容が異なる) | ~10 | virtual 化可能だが大手術 |
| 真の振る舞い分岐 (chg_virtue 等) | ~10 | 多くは内部 no-op で機能上問題なし |
| **冗長な外部ガード (内部 no-op 関数を囲む)** | **1** | **削除可能** |

特筆すべきは、`disturb()` / `chg_virtue()` のような「内部で
is_player() ガードを持つ関数」を外部から `if (is_player()) X(...)` で
囲うパターンは、コードベース全体で **1 サイトのみ** だった
(`melee/monster-attack-monster.cpp:365`)。

### 完了内容

- ✅ `melee/monster-attack-monster.cpp:365` の冗長ガード削除
  (`if (creature.is_player() && is_riding()) disturb(...)` →
   `if (is_riding()) disturb(...)`)
- ✅ コメント追加で「disturb は no-op なのでガード不要」を明記

### 削減できなかった理由

1. **早期 return ガード (63 サイト)**: `spells-status.cpp` 15 個,
   `spell-kind/spells-world.cpp` 6 個, `player-class.cpp` 8 個 等の
   ガードは「この関数はプレイヤー専用」という意味的契約を表現する
   もの。CreatureEntity 統合方針 (関数引数を `CreatureEntity &` で
   統一) と整合的で、削除すると型安全性が失われる。
   - 削除には `bool foo(PlayerType &)` への signature 変更が必要
   - これは別提案 (例: 「プレイヤー専用関数の型契約化」) として扱うべき

2. **表示分岐 (10 サイト)**: `display-player-middle.cpp` 等の
   表示ロジックが player と monster で異なるケース。virtual 化は
   可能だが、表示テーブル全体の再設計が必要で工数大。

3. **真の振る舞い分岐**: `effect-monster-*.cpp` の
   `em_ptr->is_player()` は EffectMonster の「攻撃者がプレイヤーか」
   判定で、`CreatureEntity::is_player()` とは別概念。

### 教訓

「機械的な branch reduction」よりも「特定の関数群を player-only
にする型契約化」のほうが有意義。提案 35 は現状の整理されたコード
ベースでは投資対効果が低い。

### 残作業

なし (現状のコードベースでは追加の冗長ガード除去候補なし)。

---

## 提案 39: 装備派生キャッシュフィールドの private 化 ✅ 完了

### 背景

`player-status.cpp` の `update_creature()` が装備状態から再計算する
キャッシュフィールド群が `CreatureEntity` 直下に public で多数残存
していた。書込元は基本的に `update_creature()` 1 箇所だが、読込は
戦闘・表示・AI・ペット処理など多数の場所から発生し、外部による
意図しない書込のリスクを抱えていた。

### 完了内容

#### API 整備 (23 個の virtual メソッド)

`CreatureEntity` に getter / setter virtual を追加:

| メソッド | 役割 |
|---|---|
| `set_ac(value)` | 基本 AC を設定 (get_ac() は既存) |
| `get_num_blow(hand)` / `set_num_blow(hand, value)` | 近接攻撃回数 |
| `get_num_fire()` / `set_num_fire(value)` | 遠隔攻撃回数 |
| `get_to_m_chance()` / `set_to_m_chance(value)` | 詠唱成功率減算値 |
| `get_cur_lite()` / `set_cur_lite(value)` | 光源半径 |
| `is_cumber_armor()` / `set_cumber_armor(bool)` | 鎧によるマナ減耗 |
| `is_cumber_glove()` / `set_cumber_glove(bool)` | 篭手によるマナ減耗 |
| `is_heavy_wield(hand)` / `set_heavy_wield(hand, bool)` | 重武器装備 |
| `is_icky_wield(hand)` / `set_icky_wield(hand, bool)` | 不適切武器装備 |
| `is_icky_riding_wield(hand)` / `set_icky_riding_wield(hand, bool)` | 不適切騎乗武器 |
| `is_riding_ryoute()` / `set_riding_ryoute(bool)` | 騎乗両手持ち |
| `is_monlite()` / `set_monlite(bool)` | モンスター光源照射 |

#### フィールドリネーム

メソッド名衝突回避のためフィールドをリネーム:

- `is_icky_wield[2]` → `icky_wield[2]`
- `is_icky_riding_wield[2]` → `icky_riding_wield[2]`

(`old_icky_wield[2]` / `old_riding_wield[2]` は public のまま残置。
別提案で扱う)

#### 移行

31 ファイル全 read/write site を新 API 経由に統一:

- `player/player-status.cpp`: 最大の集約箇所 (update_creature, 重武器/不適切
  装備判定, mana cumber 検出)
- `specific-object/torch.cpp::update_lite_radius()`: 多数の compound 演算
  (`+=`, `++`) をローカル変数で集約し、最後に `set_cur_lite()` 1 回で書込
- `pet/`, `mind/mind-ninja.cpp`, `monster-floor/monster-lite.cpp`,
  `floor/floor-leaver.cpp`: chained assignment
  (`creature.riding_ryoute = creature.old_riding_ryoute = false;`) を
  2 文に分割 (old_ は public 直接書込、新は setter 経由)
- `monster-floor/one-monster-placer.cpp`: モンスター AC 初期化 (place_monster)
- その他戦闘・表示・AI 各所の読取り site

#### Private 化

11 フィールドを private 化:
`to_m_chance`, `num_blow[2]`, `num_fire`, `cur_lite`, `cumber_armor`,
`cumber_glove`, `heavy_wield[2]`, `icky_wield[2]`, `icky_riding_wield[2]`,
`riding_ryoute`, `monlite`

`ac` フィールドは書込のみ `set_ac()` 経由に統一し、フィールド自体は public
のまま残置 (`io-dump/player-status-dump-json.cpp` で raw 値が JSON 出力
されており、`get_ac()` は computed 値を返すため意味が異なる。完全 private
化は別提案で扱う)。

### 効果

- **合計 private 化フィールド数: 83 → 94**
- 装備派生キャッシュの書込元が `update_creature()` 系に集約され、外部からの
  意図しない書込を型システムで防止
- `is_icky_wield()` 等の意図明示形 getter で読取意図が明確に
- 将来 monster 側で装備派生キャッシュを別計算する override 点を確保

### 残作業

- `ac` フィールドの完全 private 化 (io-dump の raw アクセス整理)
- `old_*` 系フィールド (old_cumber_armor / old_heavy_wield 等) の private 化
  (現状 player-status.cpp の diff 検出で直接アクセスされる)

---

## 提案 40: ペット関連フィールドの private 化 ✅ 完了

### 背景

`pet_extra_flags` (BIT_FLAGS16 ビットマスク、`PF_OPEN_DOORS` 等 7 フラグ) /
`pet_follow_distance` / `pet_t_m_idx` / `riding_t_m_idx` (4 フィールド) が
CreatureEntity 直下に public で残存していた。

加えて調査の結果、`health_who` (IDX) は宣言以外で一切使用されていない
**デッドフィールド**であることが判明した。

### 完了内容

#### API 整備

`CreatureEntity` に 11 個の virtual を追加:

| メソッド | 役割 |
|---|---|
| `has_pet_extra_flag(BIT_FLAGS16)` | `& flag`、`any_bits()` 相当 |
| `add_pet_extra_flag(BIT_FLAGS16)` | `\|= flag` |
| `remove_pet_extra_flag(BIT_FLAGS16)` | `&= ~flag` |
| `get_pet_extra_flags()` | 全体読取り (savefile save 用) |
| `set_pet_extra_flags(BIT_FLAGS16)` | 全体代入 (savefile load 用) |
| `get_pet_follow_distance()` / `set_pet_follow_distance()` | scalar getter/setter |
| `get_pet_t_m_idx()` / `set_pet_t_m_idx()` | 同 |
| `get_riding_t_m_idx()` / `set_riding_t_m_idx()` | 同 |

#### 移行

19 ファイル、約 102 サイトを移行:

- `cmd-action/cmd-pet.cpp`: 最大の集約箇所 (38+ サイト、ペット行動設定 UI)
- `monster-floor/monster-direction.cpp`: 13 サイト (`pet_follow_distance`
  の reference-mutate-restore パターンをローカル変数 + 明示的 setter
  呼出に書換え)
- `melee/melee-spell-flags-checker.cpp`: 9 サイト (うち
  `(flags & MASK) != MASK` の「全 bit セット」セマンティクスは
  `!has_X(A) \|\| !has_X(B)` の 2 個別チェックに分割)
- savefile load/save、ペット制御コマンド、io-dump、フロア切替、
  モンスター削除/圧縮 等

#### Private 化と削除

- 4 フィールド (`pet_extra_flags` / `pet_follow_distance` / `pet_t_m_idx` /
  `riding_t_m_idx`) を完全 private 化
- `health_who` を完全削除 (デッドフィールド)

### 効果

- **合計 private 化フィールド数: 99 → 103**
- ペット制御フラグの操作意図 (有効化/無効化/問合せ) が明示化
- 将来モンスター AI 側でもペット相当の「制御フラグ・追従距離・標的」
  を持たせる設計余地を確保
- 不要フィールド (`health_who`) の整理によりオブジェクトサイズ若干削減

---

## 提案 42: 旧差分検出キャッシュ (`old_*`) の private 化

**対象**: `old_race1/2` / `old_realm` / `old_food_aux` / `old_lite` / `old_monlite` /
`old_heavy_*` / `old_icky_*` / `old_riding_*` / `old_spells` / `old_cumber_*` (10+ フィールド)
**規模**: 主に `update_creature()` 内部のみで使用 — call site 集中
**価値**: 「前回値スナップショット」の意味的グループ化

---

## 提案 43: 行動状態フラグの private 化

**対象**: `resting` / `running` / `action` / `fishing_dir` / `sutemi` / `yoiyami` /
`timewalk` / `teleport_town` / `is_fired` / `leaving` / `playing` / `now_damaged` /
`monk_notify_aux` / `last_message` / `level_up_message` (15+ フィールド)
**規模**: access 多数 (`running` で 8 ファイル、`resting` で 4 等)
**価値**: モンスター AI と共通化余地大 (徘徊・休息など)

---

## 提案 44: 突然変異/呪い系フラグの private 化 ✅ 完了

### 背景

`muta` / `trait` (EnumClassFlagGroup<PlayerMutationType>) / `cursed`
(EnumClassFlagGroup<CurseTraitType>) / `cursed_special`
(EnumClassFlagGroup<CurseSpecialTraitType>) / `patron` (int16_t) の
5 フィールドが CreatureEntity 直下に public で残存していた。

`get_mutations()` / `get_traits()` / `get_cursed_flags()` /
`get_cursed_special_flags()` の const ref getter は既存だが、書込
パターン (`creature.muta.set(X)` / `creature.muta.reset(X)` /
`creature.cursed.set(...)` / `creature.cursed.clear()` 等) は外部から
直接フィールドにアクセスする形が残っていた。`patron` は
`get_patron()` / `set_patron()` 既存だが直接 `creature.patron = X` の
書込みも残存。

### 完了内容

#### API 整備

`CreatureEntity` に flag 操作 virtual を追加:

| メソッド | 役割 |
|---|---|
| `has_mutation(PlayerMutationType)` | 単一変異判定 |
| `add_mutation(PlayerMutationType)` | 単一変異設定 |
| `remove_mutation(PlayerMutationType)` | 単一変異クリア |
| `clear_mutations()` | 全変異クリア |
| `set_mutations(const EnumClassFlagGroup<>&)` | 一括代入 (savefile load) |
| `has_trait` / `add_trait` / `remove_trait` / `clear_traits` / `set_traits` | trait 用同等 |
| `has_curse(CurseTraitType)` | 単一呪い判定 |
| `add_curse(CurseTraitType)` | 単一呪い設定 |
| `add_curses(const EnumClassFlagGroup<>&)` | 装備呪いフラグ集計 (`obj_curse_flags` 等) の `set(flags)` 相当 |
| `remove_curse(CurseTraitType)` | 単一呪いクリア |
| `clear_curses()` | 全呪いクリア |
| `set_curses(const EnumClassFlagGroup<>&)` | 一括代入 (savefile load) |
| `has_curse_special` / `add_curse_special` / `remove_curse_special` / `clear_curses_special` / `set_curses_special` | cursed_special 用同等 |
| `get_patron()` / `set_patron()` | 既存 (継続利用) |

`get_mutations()` / `get_traits()` / `get_cursed_flags()` /
`get_cursed_special_flags()` の const ref getter は引き続き残置
(savefile save の `get_X_flags()` 経由読取り用)。

#### 移行

- `mutation/mutation-investor-remover.cpp`: 20+ サイトの
  `creature.muta.reset(X)` / `creature.muta.set(X)` を
  `remove_mutation(X)` / `add_mutation(X)` に migration
- `mutation/mutation-processor.cpp` / `wizard/wizard-mutation.cpp` /
  `object-use/quaff/quaff-effects.cpp`: 同様
- `player/player-status-flags.cpp`: cursed/cursed_special 書込み (26 + 4 = 30 サイト)
- `birth/` 系・`load/player-info-loader.cpp` / `birth-loader.cpp` /
  `quick-start.cpp` / `birth-select-patron.cpp` / `birth-wizard.cpp`:
  patron の `=` 代入を `set_patron()` 経由に migration
- read 側 (`.muta.has(X)` 等) は `has_mutation(X)` に migration

#### Private 化

5 フィールド (`muta`, `trait`, `cursed`, `cursed_special`, `patron`) を
完全 private 化。

### 効果

- **合計 private 化フィールド数: 94 → 99**
- 変異・特性・呪いフラグの書込パターンが意図明示形 (`add_mutation()` /
  `remove_curse()` 等) に統一
- 将来モンスターに種族由来変異 (e.g. ドラゴンの吐息特性) を導入する際の
  override 点を確保
- `get_mutations()` const ref getter は savefile save 経路で読取り用に
  残置 (`get_X_flags()` 系と同じ役割)

---

## 提案 45: pet_t_m_idx / riding_t_m_idx 系を `target` Pos2D に統合

`riding` の getter/setter 化と同様、ターゲットモンスター idx を
CreatureEntity 共通化。**規模**: 10-15 サイト

---

## 提案 46: ESP / 装備集計の差分検出を `update_creature()` 内 raw access に閉じる

提案 33 で telepathy/esp_* は private 化済だが、`get_X_flags()` が
差分検出キャッシュ用に依然外部公開。これを `update_creature()`
内部完結に閉じる試み。

---

## 提案 41: 呪文マスク (spell_learned/worked/forgotten) の集約 ✅ 完了

### 背景

`BIT_FLAGS spell_learned1` / `spell_learned2` / `spell_worked1` / `spell_worked2`
/ `spell_forgotten1` / `spell_forgotten2` の 6 フィールドは、64 個の呪文
(2 領域 × 32 呪文) の習得/使用/忘却状態をビットマスクで保持する古い仕様。

各呼び出し箇所では `is_realm1 ? *_1 : *_2` の三項演算子で領域選択し、
`(1UL << spell_id)` でビット操作する冗長なパターンが反復していた。

### 完了内容

#### API 整備 (12 個の virtual メソッド)

`CreatureEntity` に realm_idx (0/1) ベースの virtual API を追加:

| メソッド | 役割 |
|---|---|
| `get_spell_learned_flags(realm_idx)` | 領域全体の BIT_FLAGS 取得 (savefile 用) |
| `get_spell_worked_flags(realm_idx)` | 同上 (worked) |
| `get_spell_forgotten_flags(realm_idx)` | 同上 (forgotten) |
| `set_spell_learned_flags(realm_idx, value)` | 領域全体の BIT_FLAGS 設定 |
| `set_spell_worked_flags(realm_idx, value)` | 同上 (worked) |
| `set_spell_forgotten_flags(realm_idx, value)` | 同上 (forgotten) |
| `has_learned_spell(realm_idx, spell_id)` | 個別呪文の習得判定 (0..31) |
| `has_worked_spell(realm_idx, spell_id)` | 個別呪文の使用判定 |
| `has_forgotten_spell(realm_idx, spell_id)` | 個別呪文の忘却判定 |
| `set_learned_spell(realm_idx, spell_id, value)` | 個別呪文の習得状態設定 |
| `set_worked_spell(realm_idx, spell_id, value)` | 個別呪文の使用状態設定 |
| `set_forgotten_spell(realm_idx, spell_id, value)` | 個別呪文の忘却状態設定 |

#### 移行

- `player/player-spell-status.cpp`: 9 site (PlayerSpellStatus::Realm の
  initialize/is_X/set_X 各メソッドが三項演算子で *_1/*_2 を選択していた)。
  全て virtual API 経由に書き換え、コード行数も大幅減
- `window/display-sub-window-spells.cpp`: 3 site (表示色判定の
  ネスト三項演算子) を `has_X_spell(j, i % 32)` に簡素化
- `load/load.cpp`: 6 site (savefile load を `set_spell_X_flags()` 経由)
- `save/save.cpp`: 6 site (savefile save を `get_spell_X_flags()` 経由)

#### Private 化

6 フィールド (spell_learned1/2, spell_worked1/2, spell_forgotten1/2)
を完全 private 化。

### 効果

- **合計 private 化フィールド数: 77 → 83**
- 三項演算子による領域選択ロジックが realm_idx パラメータの伝播に簡素化
- 個別呪文アクセスが `has_X_spell(realm, idx)` の意図明示形に
- savefile load/save の API 統一
- 将来モンスター呪文習得概念を導入する際の override 点を確保

### 残作業

`std::vector<int> spell_order_learned` も呪文関連のフィールドだが、
これは vector 型でマスクとは別の構造 (習得順序リスト) を持つ。
`std::erase_if` での mutation も含むため、抽象化価値が低く今回は対象外。

### 完了済み (大規模フェーズ)

- ✅ **提案 7-10**: virtual アクセサ整備 (ml/回復計算/MonsterProfile/配列)
- ✅ **提案 11**: モンスター inventory[] 統一 (フェーズ A、5 サブフェーズ)
- ✅ **提案 12**: モンスター装備有効化 (フェーズ B、AC/耐性/攻撃/二刀流)
- ✅ **提案 13**: モンスター AI アイテム使用 (フェーズ C、ポーション/巻物/杖/ロッド + UI)
- ✅ **提案 14 / 14b**: AI ターゲット選定の共通 API 化 + m_list 走査の集約
- ✅ **提案 15 / 15b**: mflag2 の virtual アクセサ集約 (体格・状態・行動修飾子)
- ✅ **提案 16**: mflag (一時フラグ) の virtual 化
- ✅ **提案 9b**: MonsterProfile setter の virtual 集約
- ✅ **提案 17**: モンスター inventory 操作の OO 統合 + free function ラッパ削除
- ✅ **提案 18**: mflag2 書込パターンの virtual 集約 (set/reset)
- ✅ **提案 19**: 変身/自壊カウンタ (transform_*/death_count) の virtual 化
- ✅ **提案 20**: mflag/mflag2 一括操作 (assign/clear/get_all/set_all) の virtual 化
- ✅ **提案 21**: MonsterProfile を class 化してメンバを完全 private 化
- ✅ **提案 22**: モンスター identity setter (r_idx / ap_r_idx / riding) の virtual 化
- ✅ **提案 23**: ロードマップと CLAUDE.md の更新 (随時継続)
- ✅ **提案 24**: CreatureEntity 直下の生フィールド (age / ht / wt /
  prestige + 旧 cnt 系) の setter virtual 整備
- ✅ **提案 25**: inven_cnt / equip_cnt を inventory[] からの自動計算に
  変更してフィールド廃止
- ✅ **提案 26**: ambush_flag / food / town_num / level の setter virtual 化
- ✅ **提案 27**: max_plv / msp / 経験値系 (exp/max_exp/max_max_exp) の setter virtual 化
- ✅ **提案 27b**: au (所持金) / csp (現在 MP) に add/sub/divide virtual 整備、110 箇所の compound assignment 移行
- ✅ **提案 28**: r_idx / ap_r_idx / riding に getter virtual を追加し読取り (約 210 箇所、参照形式) を移行
- ✅ **提案 28b**: ポインタ経由 (`m_ptr->X`) の同フィールド読取り (約 80 箇所) を getter virtual 経由に移行
- ✅ **提案 29**: `r_idx` / `ap_r_idx` / `riding` を CreatureEntity の private 化 (CreatureEntity 直下フィールドの完全 private 化に成功した最初の例)
- ✅ **提案 30**: 戦闘ボーナス系 (to_h_b / to_h_m / to_d_m / to_a) と stat 系 (stat_max / stat_cur / stat_max_max / stat_use / stat_top / stat_add / stat_index) の setter virtual 整備、約 55 箇所 migration
- ✅ **提案 31**: 残り plain field 14 種 (au / csp / food / town_num / age / ht / wt / prestige / max_plv / msp / exp / max_exp / max_max_exp / ambush_flag) の getter virtual 整備と read site 約 350 箇所の migration
- ✅ **提案 31b**: stat_*[] (7 種) と to_h_b / to_h_m / to_d_m / to_a / to_h[hand] / to_d[hand] の getter virtual 整備と read site 約 270 箇所の migration
- ✅ **提案 31c**: level read site (628 箇所) を get_level() 経由に統一 (175 ファイル migration)
- ✅ **提案 32**: 安全な 7 フィールド (ambush_flag / prestige / max_max_exp / max_plv / to_h_b / to_h_m / to_d_m) を CreatureEntity の private 化 (縮小スコープ。他クラス名前衝突を避けたため)
- ✅ **提案 32b**: 残り 14 個の plain field (age / ht / wt / stat_*[] / max_exp / exp / msp / csp / town_num / level / au / food / to_h[] / to_d[]) を CreatureEntity の private 化 (個別 fix で対応、合計 37 フィールドが完全 private 化)

### 既存提案の残作業 (中規模)

1. **提案 1** - プレイヤー専用フィールドのクリーチャー共通化（初期値・アクセサ整備）
2. **提案 2** - プレイヤー専用 virtual メソッドの共通化（提案 1 と並行可）
3. **提案 5** - TimedEffects 二重管理解消
4. **提案 4** - 残存状態チェック関数の仮想化
5. **提案 3** - `PlayerType::get_instance()` 削減 ✅ 完了
6. **提案 6** - フィールド命名統一（最後）

### 今後の候補 (追加提案)

提案 14-32b で CreatureEntity の主要フィールド (37 個) は完全 private
化に到達した。残作業は以下のロードマップ既存提案 (1, 2, 4, 5, 6) に
集約される:

1. **提案 1** - プレイヤー専用フィールドのクリーチャー共通化
2. **提案 2** - プレイヤー専用 virtual メソッドの共通化
3. **提案 5** - TimedEffects 二重管理解消
4. **提案 4** - 残存状態チェック関数の仮想化
5. **提案 6** - フィールド命名統一
- **提案 25b**: 性能影響が出た場合の `get_inven_cnt()` キャッシュ層再導入
  → **検証済 (不要)**: 全 12 call site が低頻度 (savefile load / inventory
  pickup / death message / character dump / 個別 UI コマンド)。最頻箇所
  でも `INVEN_PACK = 23` の線形スキャン (~70ns) × 数十回/ターン 程度
  で、ターン全体の処理量に比べ無視できる。`reorder_pack` /
  `store_item_to_inventory` / `check_store_item_to_inventory` のいずれも
  per-turn loop ではない。**現状で再導入の必要なし。**

### 提案 13 の延長検討対象

- ペット AI で攻撃 wand/rod を hostile monster に向けて発動
  ✅ 完了
- 戦略的脱出 (SCROLL_DESTRUCTION/GENOCIDE)
- 装備耐久・損耗
- B-3: 装備フラグキャッシュ (性能必要時)

各提案は独立した PR として進めること。

---

## 作業時の共通留意事項

- 新規提案を完了したら本書と `CLAUDE.md` の両方を更新し、進捗を反映する
- 提案番号は維持し、完了したものには `✅ 完了` マーカーを付与
- 提案を統合・分割する場合は変更履歴をコメントで残す
- ビルド確認は必ず `sh .github/scripts/ci-build-test.sh` で実施
- 変愚マージ作業との競合に注意（`CLAUDE.md` の「変愚蛮怒（上流）からのマージ指針」節参照）
- **プレイヤー固有フィールドを `PlayerProfile` 等に切り出してモンスター
  から隔離する方向は取らない。** モンスターにも共通で持たせていく
  方針を維持すること。
