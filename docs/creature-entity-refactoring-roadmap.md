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
| [6](#提案-6-フィールド名の命名統一--完了-mp-系) | フィールド名の命名統一 (MP 系) | ✅ 完了 | csp/msp → current_mp/max_mp (403 箇所改名) |
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
| [42](#提案-42-旧差分検出キャッシュ-old_-の-private-化--完了) | 旧差分検出キャッシュ (`old_*`) private 化 | ✅ 完了 | **+12 = 115 個** (old_race1/2 / old_realm / old_cumber_* / old_heavy_* 等、old_food_aux 削除) |
| [43](#提案-43-行動状態フラグの-private-化--完了) | 行動状態フラグ private 化 | ✅ 完了 | **+14 = 129 個** (action / running / resting / leaving / playing 等) |
| [44](#提案-44-突然変異呪い系フラグの-private-化--完了) | 突然変異/呪い系フラグ private 化 | ✅ 完了 | **+5 = 99 個** (muta / cursed / patron 等) |
| [45](#提案-45-pet_t_m_idx--riding_t_m_idx-のターゲット保守ロジック集約--完了) | pet/riding ターゲット保守ロジック集約 | ✅ 完了 | reset/clear/remap_pet_riding_targets |
| [46](#提案-46-esp--知覚系フラグの差分検出スナップショット化--完了) | ESP / 知覚系フラグの差分検出スナップショット化 | ✅ 完了 | PerceptionFlagsSnapshot、getter 15 個撤廃 |
| [47](#提案-47-その他小規模フィールドの-private-化--完了) | その他小規模フィールドまとめ | ✅ 完了 | **+6 = 135 個** (dealt_damage / run_py/px / vanish_stairs_flag / suppress_multi_reward / tracking_bi_id、tval_xtra 削除) |
| [48](#提案-48-追加の小規模フィールドの-private-化--完了) | 追加の小規模フィールドまとめ | ✅ 完了 | **+6 = 141 個** (tval_ammo / dtrap / autopick_autoregister / recall_dungeon / enchant_energy_need / energy_use) |
| [49](#提案-49-モンスター時限効果付与プリミティブの集約-b1-第1段--完了) | モンスター時限効果付与プリミティブ集約 (B1 第1段) | ✅ 完了 | FloorType::set_monster_timed_effect、7 setter 集約 |
| [50](#提案-50-set_timed_effect-への-mproc-保守統合-b1-後続段--完了-要実機smoke-test) | set_timed_effect への mproc 保守統合 (B1 後続段) | ✅ 完了 (要実機smoke-test) | get_self_m_idx、mproc を set_timed_effect に内包 |
| [51](#提案-51-残り-public-bool-フィールドの-private-化-a-1-第1弾--完了) | 残り public bool フィールドの private 化 (A-1 第1弾) | ✅ 完了 | counter / select_ring_slot / no_flowed / hack_mutation / invoking_midnight_curse |
| 52 | `extended_inventory` (拡張装備スロット) の private 化 | ✅ 完了 | get_extended_inventory_size / get_extended_item / ensure_extended_item / get_extended_inventory。**残存 public plain field は class_specific_data のみ** |

**累計 private 化フィールド数 (主要マイルストーン):**
- 提案 29 (3 個) → 32 (10 個) → 32b (37 個) → 33 (72 個) → 34 (77 個) →
  41 (83 個) → 39 (94 個) → 44 (99 個) → 40 (103 個) → 42 (115 個) →
  43 (129 個) → 47 (135 個) → **48 (141 個)**

未完了の提案は 6 / 46 のみ (いずれも private 化フィールド数を増やさない命名統一・
アクセス閉域化)。private 化フィールド数は **141 個** が事実上の最終形。

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
| キャラクタ履歴 | `old_race1/2`, `old_realm`, `history[4][60]`, `hp_table[PY_MAX_LEVEL]` (旧 `player_hp`) | モンスターのレベル成長履歴 |
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

## 提案 6: フィールド名の命名統一 ✅ 完了 (MP 系)

### 背景

モンスター / プレイヤー兼用になったフィールドで、プレイヤー時代の
命名を引きずっているものがある。統合が進んだ今、汎用的な名前に
改名することで可読性が向上する。

### 完了内容 (MP 系、提案 6 第1弾)

`csp` / `msp` (Angband 由来の current/max spell points) を MP 概念に
合わせて改名。全体識別子 `\b...\b` アンカーの一括スクリプトで実施し、
`mspell` 等の部分一致衝突を回避。**403 箇所 / 65 ファイル** を置換、
フルビルド (g++ -O3 -Werror) と clang-format-18 で検証済み。

| 旧名称 | 新名称 |
|---|---|
| `csp` (フィールド) | `current_mp` |
| `msp` (フィールド) | `max_mp` |
| `csp_frac` | `current_mp_frac` |
| `get_csp` / `set_csp` / `add_csp` / `sub_csp` | `get_current_mp` / `set_current_mp` / `add_current_mp` / `sub_current_mp` |
| `add_csp_with_frac` / `sub_csp_with_frac` | `add_current_mp_with_frac` / `sub_current_mp_with_frac` |
| `get_msp` / `set_msp` | `get_max_mp` / `set_max_mp` |
| ローカル `max_csp` / `old_csp` / `baseline_msp` | `max_current_mp` / `old_current_mp` / `baseline_max_mp` |

**上流マージへの影響**: フィールド自体は提案 27b で private 化済みのため、
上流の `creature->csp` アクセスは元々変換が必要だった。本改名で変換先が
`get_current_mp()` 等に変わるのみ (CLAUDE.md のマッピング表に追記済み)。

### 残候補 (未着手)

| 現名称 | 候補名 | 備考 |
|---|---|---|
| `exp` / `max_exp` / `max_max_exp` | そのままクリーチャー共通で | 改名せず維持 |
| `hp_frac` | `hp_fraction` | 可読性のみ。MP 系と対称にするなら別途検討 |

### 注意 (今後の改名作業)

改名 PR は diff が巨大化し、変愚マージ時の衝突を増やす。
改名時は全体識別子 `\b...\b` アンカーの一括スクリプト + ビルド確認を
用いること (素朴な部分一致 sed は `mspell`↔`msp` 等を破壊する)。

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

## 提案 42: 旧差分検出キャッシュ (`old_*`) の private 化 ✅ 完了

### 背景

CreatureEntity 直下に「前回値スナップショット」を保持する `old_*`
フィールド 13 個 (内 1 個は実質デッド) が public で残存していた。
これらは主に `update_creature()` / `put_equipment_warning()` /
`monster-lite.cpp` 等で状態変化検出 (cumber/heavy_wield/icky_wield/
riding 等のメッセージ出力タイミング判定) に使われ、savefile 経由でも
一部 (old_race1/2 / old_realm) が永続化される。

### 完了内容

#### API 整備

`CreatureEntity` に 25 個の virtual を追加:

| メソッド | 役割 |
|---|---|
| `get_old_lite() / set_old_lite()` | 光源半径前回値 |
| `get_old_race_flags1/2() / set_old_race_flags1/2()` | 種族変身履歴ビットマスク |
| `get_old_realm() / set_old_realm()` | 魔法領域変更履歴 |
| `get_old_spells() / set_old_spells()` | 学習可能呪文数前回値 |
| `was_cumber_armor() / set_was_cumber_armor()` | 装備過重 (鎧) 前回値 |
| `was_cumber_glove() / set_was_cumber_glove()` | 装備過重 (篭手) 前回値 |
| `was_heavy_wield(hand) / set_was_heavy_wield(hand, value)` | 重武器装備前回値 |
| `was_heavy_shoot() / set_was_heavy_shoot()` | 重弓装備前回値 |
| `was_icky_wield(hand) / set_was_icky_wield(hand, value)` | 不適切武器装備前回値 |
| `was_icky_riding_wield(hand) / set_was_icky_riding_wield(hand, value)` | 不適切騎乗武器前回値 |
| `was_riding_ryoute() / set_was_riding_ryoute()` | 騎乗両手持ち前回値 |
| `was_monlite() / set_was_monlite()` | モンスター光源照射前回値 |

bool 系は意図明示のため `was_X()` 命名 (現在値は `is_X()`、前回値は
`was_X()` で対称形)。

#### 移行

11 ファイル約 46 サイトを移行:

- `player/player-status.cpp`: 最大の集約箇所 (cumber/heavy_wield/
  icky_wield/icky_riding_wield/riding_ryoute/heavy_shoot/spells の
  diff 検出)
- `specific-object/torch.cpp`: old_lite
- `monster-floor/monster-lite.cpp` / `mind/mind-ninja.cpp`: old_monlite
- `pet/pet-util.cpp` / `pet/pet-fall-off.cpp` / `cmd-action/cmd-pet.cpp` /
  `floor/floor-leaver.cpp`: old_riding_ryoute (騎乗解除時のリセット)
- `load/player-info-loader.cpp` / `save/player-writer.cpp`: savefile
  load/save (old_race1/2 / old_realm)
- `io-dump/character-dump.cpp`: 種族・領域履歴ダンプ
- `cmd-action/cmd-spell.cpp` / `status/shape-changer.cpp`:
  履歴ビット追加 (`|= 1UL << X` 形式を `set(get() | ...)` に書換え)
- `birth/birth-stat.cpp`: 履歴リセット (種族再生成時)

#### Private 化と削除

- 12 フィールド (`old_lite` / `old_race1` / `old_race2` / `old_realm` /
  `old_spells` / `old_cumber_armor` / `old_cumber_glove` / `old_heavy_wield[2]` /
  `old_heavy_shoot` / `old_icky_wield[2]` / `old_riding_wield[2]` /
  `old_riding_ryoute` / `old_monlite`) を完全 private 化
- `old_food_aux` を完全削除 (デッドフィールド、宣言以外で未使用)

### 効果

- **合計 private 化フィールド数: 103 → 115**
- 差分検出キャッシュの読書きが意図明示形 (`was_X()` / `set_was_X()`) に
  統一され、現在値と前回値の混同を防止
- 将来モンスターでも独自の状態変化検出 (例: ペットモンスターの装備
  変更通知) を行う場合の override 点を確保
- デッドフィールド削除によりオブジェクトサイズ若干削減

---

## 提案 43: 行動状態フラグの private 化 ✅ 完了

### 背景

CreatureEntity 直下に「現在の行動状態」を示すスカラー / bool フラグが
public で 14 個残存していた。プレイヤーの行動制御に使われるが、
モンスター AI 共通化の余地がある (`resting` / `running` 相当の徘徊・
休息ロジックなど)。

### 完了内容

#### API 整備

`CreatureEntity` に 28 個の virtual を追加:

| メソッド | 役割 |
|---|---|
| `get_action() / set_action()` | 常時行動 ID (ACTION_LEARN 等) |
| `get_running() / set_running()` | 走行カウンタ |
| `get_resting() / set_resting()` | 休息カウンタ (GAME_TURN) |
| `is_fired() / set_is_fired()` | 発射済みフラグ (bool 値) |
| `has_level_up_message() / set_level_up_message()` | レベルアップメッセージ表示要求 |
| `is_timewalking() / set_timewalking()` | 時間停止中 |
| `is_now_damaged() / set_now_damaged()` | 直近ダメージ受領 |
| `is_playing() / set_playing()` | プレイ中フラグ |
| `is_leaving() / set_leaving()` | フロア離脱中 |
| `get_monk_notify_aux() / set_monk_notify_aux()` | 修行僧の重装備通知済フラグ |
| `is_teleport_town() / set_teleport_town()` | 街へのテレポート要求 |
| `get_yoiyami() / set_yoiyami()` | 宵闇 (BIT_FLAGS) |
| `is_sutemi() / set_sutemi()` | 捨て身フラグ |
| `get_fishing_dir() / set_fishing_dir()` | 釣り方向 |

#### フィールドリネーム

メソッド名衝突回避のためフィールドをリネーム:
- `bool is_fired` → `bool fired` (メソッド `is_fired()` と衝突回避)

#### 移行

73 ファイル、約 180 サイトを migration:

- `.action` 名前空間衝突への注意深い扱い:
  `terrain.state[i].action` (TerrainCharacteristics) /
  `autopick_list[idx].action` (autopick entries) /
  `cmd.action(creature)` (text command 関数ポインタ) /
  `dungeon.action` (DungeonDefinition 引数名) は全て CreatureEntity
  ではないので migration 対象外として残置
- `running` / `leaving` についても同様に CreatureEntity field のみ migration
- `++` / `--` compound assignment は `set_X(get_X() + 1)` 形式に展開
- `this->X` (CreatureEntity 派生メソッド内、`hp-mp-regenerator.cpp` /
  `player-damage.cpp`) も `this->get_X()` / `this->set_X()` に migration

#### Private 化

14 フィールド (`action` / `running` / `resting` / `fired` /
`level_up_message` / `timewalk` / `now_damaged` / `playing` / `leaving` /
`monk_notify_aux` / `teleport_town` / `yoiyami` / `sutemi` / `fishing_dir`)
を完全 private 化。

### 効果

- **合計 private 化フィールド数: 115 → 129**
- 行動状態の更新が意図明示形 (`set_action(ACTION_LEARN)` /
  `is_leaving()` 等) に統一され、外部からの意図しない書込みを型システムで防止
- 将来モンスター AI で `resting` / `running` 相当の徘徊・休息ロジックを
  共通化する設計余地を確保
- `is_fired` のフィールド名衝突を回避することで、is_X() 系メソッドの
  自然な命名規則を全フィールドに適用可能に

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

## 提案 45: pet_t_m_idx / riding_t_m_idx のターゲット保守ロジック集約 ✅ 完了

### スコープ修正

当初案は「`target` Pos2D に統合」だったが、`pet_t_m_idx`(ペット追従先) /
`riding_t_m_idx`(騎乗ターゲット) は**移動するモンスターを idx で追跡**する
ものであり、固定座標 (Pos2D) へ畳むと「動く対象を追う」セマンティクスが
壊れるため不適。また両フィールドの「private 化 + virtual getter/setter +
CreatureEntity 共通化」は**提案 40 で既に達成済み**。

したがって本提案は、残っていた真の課題である**ターゲット idx 保守ロジック
の重複集約**に再定義した。

### 完了内容

モンスター index が除去・付替え・全消去される際に、プレイヤーが指定した
追従先 / 騎乗ターゲットを一貫更新する不変条件が、複数ファイルに同一ロジック
として分散していた。これを `CreatureEntity` の 3 メソッドへ集約:

| メソッド | 役割 |
|---|---|
| `reset_pet_riding_targets()` | 両ターゲットを 0 にクリア (フロア離脱・全消去時) |
| `clear_pet_riding_targets_pointing_to(m_idx)` | 指定 idx を指すターゲットのみクリア (個別除去時) |
| `remap_pet_riding_targets(from, to)` | 指定 idx を付替え (コンパクション時) |

#### 移行 (4 サイト集約)

- `monster-floor/monster-remover.cpp`: `delete_monster()` の個別クリア →
  `clear_pet_riding_targets_pointing_to(m_idx)`、`wipe_monsters_list()` の
  両クリア → `reset_pet_riding_targets()`
- `monster/monster-compaction.cpp`: i1→i2 付替え → `remap_pet_riding_targets(i1, i2)`
- `dungeon/dungeon-processor.cpp`: フロア処理終了時の両クリア →
  `reset_pet_riding_targets()`

`cmd-action/cmd-pet.cpp` の単独 `set_pet_t_m_idx()`(コマンド処理中の個別
ターゲット設定) は性質が異なるため対象外。

### 効果

- 「モンスター idx 変化時にプレイヤーのターゲット idx を保つ」不変条件が
  1 箇所に明示集約され、将来同種フィールド追加時の保守漏れを防止
- フルビルド (g++ -O3 -Werror -Wall -Wextra) と clang-format-18 で検証済み

---

## 提案 46: ESP / 知覚系フラグの差分検出スナップショット化 ✅ 完了

### 背景

提案 33 で telepathy / esp_* / see_inv / mighty_throw は private 化済だが、
旧値スナップショット (差分検出キャッシュ) 用に 15 個の `get_X_flags()`
BIT_FLAGS getter が外部公開されたまま残っていた。これらは
`update_bonuses()` (player-status.cpp) の「再計算前に旧値退避 → 再計算 →
新旧比較で再描画フラグ設定」という差分検出のためだけに使われており、
内部表現 (raw BIT_FLAGS) を外部に漏らしていた。

### 完了内容

- `CreatureEntity` にネスト構造体 `PerceptionFlagsSnapshot` を追加
  (15 フラグ: telepathy / esp 12 種 / see_inv / mighty_throw)。
  差分判定ヘルパ 2 種を提供:
  - `mighty_throw_differs_from(other)` … 強投擲変化 (→ インベントリ再描画)
  - `monster_perception_differs_from(other)` … テレパシー/ESP/透明視認の
    いずれか変化 (→ モンスター状態再計算)
- スナップショット取得 `capture_perception_flags()` を追加
  (private フィールドを内部で読むため、個別 getter は不要)
- **個別 `get_X_flags()` 15 個を撤廃**
  (impact / earthquake は player-attack の実ロジックで使うため残置)
- `player-status.cpp::update_bonuses()` を書き換え:
  - 旧 15 行の `BIT_FLAGS old_X = get_X_flags();` → `const auto
    old_perception = creature.capture_perception_flags();`
  - 旧 27 行の個別比較ブロック → スナップショット 2 メソッド比較

### 効果

- 内部表現を漏らす public getter 15 個を撤廃し、差分検出を
  「スナップショット取得 + 比較」という呼出側の関心事に閉じた
- update_bonuses() の差分検出が ~42 行 → ~10 行に簡素化
- player-status.cpp の getter 呼出 30 箇所が 2 箇所に集約
- フルビルド (g++ -O3 -Werror -Wall -Wextra) と clang-format-18 で検証済み

### 残置

`get_impact_flags()` / `get_earthquake_flags()` は player-attack.cpp が
FLAG_CAUSE_* ビットマスク (どちらの手由来か) を実判定に使うため公開維持。

---

## 提案 47: その他小規模フィールドの private 化 ✅ 完了

### 背景

CreatureEntity 直下に public で残存していた小規模 (アクセスサイト 10
以下) のフィールド群をまとめて private 化。単独で提案を立てるほど
規模は大きくないが、フィールドカプセル化の徹底のため一括処理した。

### 完了内容

#### API 整備

15 個の virtual を追加:

| メソッド | 役割 |
|---|---|
| `get_dealt_damage() / set_dealt_damage() / add_dealt_damage()` | 累積与ダメージ (プレイヤー・モンスター共通) |
| `get_run_py() / set_run_py()` | 走行目標 Y 座標 |
| `get_run_px() / set_run_px()` | 走行目標 X 座標 |
| `is_vanish_stairs_flag() / set_vanish_stairs_flag()` | 階段消去フラグ |
| `is_suppress_multi_reward() / set_suppress_multi_reward()` | パトロン報酬多重防止 |
| `get_tracking_bi_id() / set_tracking_bi_id()` | アイテム種類トラッキング ID |

`dealt_damage` には compound assignment 用に `add_dealt_damage(delta)` も
整備 (`this->dealt_damage += damage` パターン対応)。

#### 移行

10 ファイル、約 24 サイトを migration:

- `player/player-damage.cpp`: `dealt_damage` の `+=` / 上限クリップ
- `monster/monster-{damage,processor,status}.cpp` / `monster-floor/
  one-monster-placer.cpp` / `core/game-play.cpp`: モンスター `dealt_damage`
  の初期化・参照
- `save/monster-writer.cpp` / `save/player-writer.cpp` /
  `load/player-info-loader.cpp` / `load/old/monster-loader-savefile50.cpp`:
  savefile 経路 (MonsterLoader50 / MonsterWriter は MonsterProfile の
  friend のみで CreatureEntity の friend ではないため、virtual API 経由)
- `action/run-execution.cpp`: `run_py` / `run_px` 設定・クリア
- `floor/floor-changer.cpp` / `cmd-action/cmd-move.cpp`:
  `vanish_stairs_flag`
- `player/patron.cpp` / `core/magic-effects-timeout-reducer.cpp`:
  `suppress_multi_reward`
- `core/stuff-handler.cpp`: `tracking_bi_id`

#### Private 化と削除

- 6 フィールド (`dealt_damage` / `run_py` / `run_px` / `vanish_stairs_flag` /
  `suppress_multi_reward` / `tracking_bi_id`) を完全 private 化
- `tval_xtra` (Unused) を完全削除 (デッドフィールド)

### 効果

- **合計 private 化フィールド数: 129 → 135**
- 小規模フィールドのカプセル化により、CreatureEntity のフィールド
  アクセス契約が virtual API 経由でほぼ完全に統一
- デッドフィールド削除によりオブジェクトサイズ若干削減

---

## 提案 48: 追加の小規模フィールドの private 化 ✅ 完了

### 背景

提案 47 で残った小規模フィールドのうち、コンパウンド代入 (`+=`, `-=`,
`*=`, `/=`) を含む 2 個と、単純な scalar/bool/enum 系 4 個を一括 private
化する 2 巡目。

### 完了内容

#### API 整備

18 個の virtual を追加:

| メソッド | 役割 |
|---|---|
| `get_tval_ammo() / set_tval_ammo()` | 弾種 ItemKindType |
| `is_dtrap() / set_dtrap()` | トラップ安全地帯フラグ |
| `is_autopick_autoregister() / set_autopick_autoregister()` | 自動登録モード |
| `get_recall_dungeon() / set_recall_dungeon()` | 帰還ダンジョン DungeonId |
| `get_enchant_energy_need() / set_enchant_energy_need() / add_enchant_energy_need(delta) / sub_enchant_energy_need(delta)` | 次のエンチャント効果までのエネルギー |
| `get_energy_use() / set_energy_use() / add_energy_use(delta) / sub_energy_use(delta) / mul_energy_use(factor) / div_energy_use(divisor)` | 今ターンのエネルギー消費 |

`energy_use` は `PlayerEnergy` クラスのラッパー (`+=`, `-=`, `*=`, `/=`)
が既に存在するため、CreatureEntity 側にも対応する compound assignment
virtual を追加し、PlayerEnergy はそれらを呼び出す形に切替え。

#### 移行

25 ファイル、約 61 サイトを migration:

- `combat/shoot.cpp` / `player/player-status.cpp` 等の `tval_ammo`
  読取 (4 + 4 + 1 + 1 = 10 サイト)
- `floor/floor-changer.cpp` / `spell-kind/spells-detection.cpp` /
  `action/run-execution.cpp` / `action/travel-execution.cpp` /
  `player/player-move.cpp` の `dtrap` (8 サイト)
- `load/world-loader.cpp` / `save/player-writer.cpp` /
  `autopick/autopick-registry.cpp` / `autopick/pref-file-expressor.cpp` の
  `autopick_autoregister` (8 サイト、savefile 経由も含む)
- `floor/floor-leaver.cpp` / `wizard/wizard-special-process.cpp` /
  `load/player-info-loader.cpp` / `load/world-loader.cpp` /
  `save/player-writer.cpp` / `world/world-movement-processor.cpp` /
  `io-dump/player-status-dump-json.cpp` / `spell-kind/spells-world.cpp` /
  `birth/game-play-initializer.cpp` の `recall_dungeon` (13 サイト)
- `core/player-processor.cpp` 等の `enchant_energy_need`
  (5 サイトの compound assignment を含む 7 サイト)
- `player-attack/player-attack.cpp` / `player-status/player-energy.cpp` /
  `cmd-action/cmd-move.cpp` / `io/input-key-processor.cpp` /
  `core/player-processor.cpp` の `energy_use` (15 サイト、
  PlayerEnergy ラッパーの `=`/`+=`/`-=`/`*=`/`/=` を含む)

#### Private 化

6 フィールド (`tval_ammo` / `dtrap` / `autopick_autoregister` /
`recall_dungeon` / `enchant_energy_need` / `energy_use`) を完全 private 化。

### 効果

- **合計 private 化フィールド数: 135 → 141**
- ENERGY 系フィールド (`enchant_energy_need` / `energy_use`) の
  compound assignment を意図明示形 (`add_X(delta)` / `sub_X(delta)` /
  `mul_X(factor)` / `div_X(divisor)`) に統一
- `PlayerEnergy` ラッパークラスの内部実装も virtual API 経由に
  切替え、フィールド直接アクセスを完全排除

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

## 提案 49: モンスター時限効果付与プリミティブの集約 (B1 第1段) ✅ 完了

### 背景 (B1「状態異常付与処理の統合」の調査結果)

第2層 B1 として「状態異常付与ロジック (耐性判定＋メッセージ＋セット) を
`creature.try_inflict_X()` 的な virtual に共通化」を狙ったが、調査の結果
**プレイヤー経路と モンスター経路の共有ロジックは想定より小さい**ことが判明:

- **プレイヤー** (`BadStatusSetter`): 値クランプ + 豊富な副作用 (行動中断 /
  構え崩し / 徳変化 / 連携リセット等) + プレイヤー向けメッセージ + `set_timed_effect`。
- **モンスター** (`set_monster_*`): 値クランプ + **mproc キュー保守**
  (毎ターン処理対象への登録/解除、`m_idx` キー) + `set_timed_effect`。

両者で真に共通なのは `set_timed_effect` のみ。モンスターは mproc という
固有機構を、プレイヤーは固有副作用を持つため、単純な `try_inflict_X()`
virtual では中身が分離した 2 経路になり価値が薄い。さらに統一 `set_timed_effect`
(提案 5) は mproc を保守しないため、モンスターに直接呼ぶと効果が毎ターン
処理されない**フットガン**が存在する (各 set_monster_* がこれを個別に吸収)。

**真の統一 (set_timed_effect が mproc を自動保守) には、モンスターが自身の
`m_idx` を知る仕組みが必要**で、これはモンスターデータ構造とホットパスに
関わる別提案 (アーキテクチャ判断要) とする。

### 完了内容 (B1 の安全な第 1 段)

モンスター側の「時限効果付与 (mproc 保守込み)」をまず単一プリミティブへ集約:

- `FloorType::set_monster_timed_effect(m_idx, mte, v, max_value)` を新設。
  クランプ + mproc 保守 (付与時 add / 解除時 remove) + 値設定 + 変化有無
  (notice) 返却を 1 箇所に集約。`is_X()` ≡ `get_timed_effect(X) > 0` の
  等価性を確認の上で共通化したため挙動不変。
- `set_monster_csleep` / `fast` / `slow` / `stunned` / `confused` /
  `monfear` / `invulner` の 7 setter が、それぞれ複製していたコア
  (クランプ + mproc + set) を本プリミティブへ委譲。各 setter 固有の
  前段ガード (monfear の狂乱) と後段再描画は維持。
- `stunned` / `confused` は 1 行に縮約。`invulner` の解除時エネルギー消費は
  `notice && v<=0` で等価に再現。

### 効果

- モンスター時限効果付与の「mproc 保守を伴う正しい設定」が単一メソッドに
  集約され、フットガン (mproc 保守漏れ) を構造的に防止
- 7 setter 合計で重複コア (1 setter あたり ~10 行) を削減
- 将来の真の player/monster 時限効果統一 (set_timed_effect への mproc 統合)
  の足場が整備
- フルビルド (g++ -O3 -Werror -Wall -Wextra) と clang-format-18 で検証済み

### 残作業 (B1 後続段 / 別提案)

- **アーキテクチャ前提**: モンスター自身の `m_idx` 参照手段の導入
  (これにより `CreatureEntity::set_timed_effect` がモンスターの mproc を
  自動保守でき、player/monster の時限効果 API が真に一本化する)
- 上記完了後、`creature.try_inflict_X()` 的な付与 API の共通化を再検討

---

## 提案 50: set_timed_effect への mproc 保守統合 (B1 後続段) ✅ 完了 (要実機smoke-test)

### 背景・前提調査

提案 49 で「モンスター側の時限効果付与は mproc キュー (毎ターン処理リスト) 保守を
伴う」ことを集約したが、統一 API `CreatureEntity::set_timed_effect()` を
モンスターへ直接呼ぶと mproc が保守されない**フットガン**が残っていた
(`set_monster_*` 経由でのみ保守)。これを解消し、player/monster で
時限効果設定経路を真に一本化する。

調査で判明した mproc のアーキテクチャ:

- **時限効果 map が source of truth** (savefile に保存)。
- **mproc は派生キャッシュ**。`FloorType::reset_mproc()` が map から全走査で
  再構築し、**フロア入場時 (`dungeon-processor` のターンループ直前 L215)**・
  セーブ後・ポリモーフ後に呼ばれる。さらにゲームプレイ中は付与経路が
  逐次保守する 2 段構成。

この「フロア入場時 reset_mproc がターン処理前に必ず走る」安全網により、
ロード中等で mproc 保守が漏れても (または一時的に誤っても) 処理前に
再構築され整合する。

### 完了内容

- `CreatureEntity::get_self_m_idx()` を追加。モンスターは自身の m_idx を保持
  しないが、`floor.m_list` 上のインデックスは配置先グリッドの m_idx と一致する
  ため、現在位置のグリッドから導出する (プレイヤー・未配置・フロア未設定は 0)。
- `CreatureEntity::set_timed_effect()` に **mproc 保守を内包**。値が 0 を
  またいで変化した時のみ、対象が「配置済みモンスター」かつ
  `MONSTER_TIMED_EFFECT_LIST` の 7 効果なら `add_mproc`/`remove_mproc` を行う
  (file-local `maintain_monster_mproc_on_toggle`)。プレイヤー・未配置時は
  no-op で `reset_mproc()` に委ねる。
- `FloorType::set_monster_timed_effect()` から明示 mproc 呼出を撤去
  (set_timed_effect に集約)。クランプ + notice 算出のみに簡素化。
- これで mproc を触る箇所は **`reset_mproc()` (全再構築) と
  `set_timed_effect()` (逐次保守) の 2 箇所のみ**に集約。二重保守なし。

### 安全性の根拠

- ゲームプレイ中: モンスターはグリッド上にあり m_idx が正しく導出され逐次保守。
- ロード/生成中: モンスター未配置で m_idx=0 のため保守はスキップされるが、
  フロア入場時の `reset_mproc()` がターン処理前に map から再構築するため整合。
- `add_mproc` は他に呼出元がなく二重登録経路なし。`remove_mproc` は不在
  エントリに対し no-op で冪等。

### ⚠️ 残確認 (実機 smoke-test 推奨)

セーブ/ロード・per-turn 処理という critical path への変更のため、ビルド
(g++ -O3 -Werror) 検証に加え、実機での **(1) 減速/混乱/恐怖等を受けた
モンスターのセーブ→ロード後の継続、(2) フロア移動を跨いだ効果処理、
(3) 通常戦闘での状態異常付与/解除** の動作確認を推奨。問題があれば
`maintain_monster_mproc_on_toggle` のガード条件を見直すこと。

### 今後 (B1 後続)

- mproc 保守が set_timed_effect に集約されたことで、`set_monster_*` 自由関数
  および `FloorType::set_monster_timed_effect` は「クランプ + メッセージ +
  再描画」のみの薄いラッパとなった。将来これらを `CreatureEntity` の
  状態異常付与 API (例: `inflict_*`) へ寄せる余地がある (ただしプレイヤーの
  豊富な副作用とは別経路のまま)。

---

## 提案 51: 残り public bool フィールドの private 化 (A-1 第1弾) ✅ 完了

### 背景

提案 24-48 で 141+ フィールドを private 化したが、未カプセル化の public
フィールドがまだ残存している (棚卸し結果: 単純スカラ/bool 約 10、energy_need /
knowledge / element_realm、配列/vector 系、汎用名 count 等)。本提案はその
うち最も安全な「単純 bool 5 個」を private 化する第1弾。

### 完了内容

以下 5 個の public bool フィールドを private 化し、`is_X()` / `set_X()`
virtual アクセサを整備:

| フィールド | 用途 |
|---|---|
| `counter` | 侍カウンター攻撃の構え |
| `select_ring_slot` | 指輪スロット選択中フラグ (UI 一時) |
| `no_flowed` | モンスター流れ込み AI 抑制フラグ |
| `hack_mutation` | 誕生時の突然変異強制フラグ |
| `invoking_midnight_curse` | 深夜の呪い発動中フラグ |

- 16 ファイル / 32 アクセスサイト (書込 21・読取 11) を migration。
  bool 書込は全て `= true`/`= false` のため `set_X(true/false)` に、
  読取は `is_X()` に機械置換。
- フルビルド (g++ -O3 -Werror -Wall -Wextra) と clang-format-18 で検証済み。

### 提案 A-1 第2弾 ✅ 完了

提案 51 (第1弾, bool 5 個) に続き、BIT_FLAGS 2 個と scalar 3 個を private 化:

| フィールド | 型 | 用途 |
|---|---|---|
| `easy_2weapon` | BIT_FLAGS | 二刀流ペナルティ軽減 (手別ビット) |
| `down_saving` | BIT_FLAGS | 劣化セーヴィングスロー |
| `mutant_regenerate_mod` | PERCENTAGE | ミュータント体質の自然回復補正(%) |
| `learned_spells` | int16_t | 習得済み呪文数 |
| `add_spells` | int16_t | 追加習得可能呪文数 |

- 10 個の virtual アクセサ (`get_X()` / `set_X()`) を整備し、12 ファイル
  約 22 アクセスサイトを migration。
- 読取は `get_X()`、書込は `set_X()`、`++` (learned_spells / add_spells) は
  `set_X(get_X() + 1)` に展開。`PlayerType::apply_creature_specific_regen_modifier`
  の `this->mutant_regenerate_mod` も `this->get_mutant_regenerate_mod()` に変換。
- 既存の `has_down_saving()` / `has_easy2_weapon()` は装備集計を計算する別物
  (キャッシュ値の格納先がこのフィールド)。フィールド getter は `get_down_saving()`
  / `get_easy_2weapon()` と命名して衝突回避。
- フルビルド (g++ -O3 -Werror -Wall -Wextra) と clang-format-18 で検証済み。

### 提案 A-2 ✅ 完了

3 フィールド (`energy_need` / `element_realm` / `knowledge`) を private 化:

| フィールド | 型 | アクセサ |
|---|---|---|
| `energy_need` | ACTION_ENERGY | `get_energy_need()` / `set_energy_need()` (既存) + 新規 `add_energy_need()` / `sub_energy_need()` |
| `element_realm` | ElementRealmType | `get_element_realm()` / `set_element_realm()` (既存) |
| `knowledge` | BIT_FLAGS8 | 新規 `has_knowledge()` / `add_knowledge()` / `remove_knowledge()` / `get_knowledge()` / `set_knowledge()` |

- `energy_need`: get/set は既存だったが約 38 サイトが compound assignment
  (`+=` / `-=`) や直接読取りでフィールドに触れていた。`add_energy_need()` /
  `sub_energy_need()` を新設し、`+=`→`add_`、`-=`→`sub_`、`=`→`set_`、
  読取り→`get_` に regex 一括変換 (`creature` / `monster` / `m_ptr` の
  `.` / `->` 両アクセス)。プレイヤー・モンスター共用フィールドのため
  両系統の access site を含む。
- `element_realm`: get/set 既存。約 25 read + 5 write を migration。
  CreatureEntity 内部 (`creature-entity.cpp`) の `this->element_realm` 代入は
  private 内アクセスのため残置。
- `knowledge`: 自己分析知識フラグ (KNOW_STAT / KNOW_HPRATE)。`& FLAG`→
  `has_knowledge(FLAG)`、`|= FLAG`→`add_knowledge(FLAG)`、`&= ~FLAG`→
  `remove_knowledge(FLAG)`、`= 0`/load→`set_knowledge()`、save→`get_knowledge()`。
- フルビルド (g++ -O3 -Werror -Wall -Wextra) と clang-format-18 で検証済み。

### 提案 A-3 ✅ 完了 (一部スコープ確定で残置)

配列・汎用名フィールドのうち、スカラ／固定長配列の 3 個を private 化:

| フィールド | 型 | アクセサ |
|---|---|---|
| `extra_blows[2]` | int[2] | `get_extra_blows(hand)` / `set_extra_blows(hand, value)` / `add_extra_blows(hand, delta)` |
| `count` | uint32_t | `get_count()` / `set_count()` |
| `hp_table[PY_MAX_LEVEL]` | int[] | `get_hp_table(idx)` / `set_hp_table(idx, value)` |

- `extra_blows`: 装備由来追加攻撃。`+=` は `add_extra_blows()`、`= ... = 0` は
  2 文へ分解。7 サイト migration。
- `count`: セーブ用カウンタ。`.count` の大多数は `std::map/set::count()` や
  `ItemEntity::count` など別物のため、`creature.count` のみを慎重に抽出して 6 サイト
  migration。`++creature.count` は `set_count(get_count()+1)` に展開。
- `hp_table`: レベル別累積HPテーブル。CreatureEntity 内部 (`roll_hp_table()` /
  `roll_monster_hp_table()` / `grow_hp_table_to_level()`) の `this->hp_table` 直接操作は
  private 内アクセスで残置。外部の `creature.hp_table` 読書 9 サイトを migration
  (`Birther::hp_table` / `previous_char.hp_table` は別構造体のため対象外、慎重に除外)。
- フルビルド (g++ -O3 -Werror -Wall -Wextra) と clang-format-18 で検証済み。

**残置 (複合型、参照アクセサではカプセル化効果が薄いため見送り):**

- `class_specific_data` (`ClassSpecificData` = std::variant): `std::visit` /
  `std::get_if` / `std::holds_alternative` / 直接代入で使われ、可変参照を返す
  アクセサでは実質的な保護にならない。かつ完全にプレイヤー専用データ。
- `extended_inventory` (`std::vector<std::shared_ptr<ItemEntity>>`): `[i]` への
  代入・`.size()`・range-for で多用され、これも可変参照アクセサでは保護が薄い。
  プレイヤー／モンスター共用だが既に `init_extended_inventory()` /
  `store_item()` 等の OO API で操作経路は整備済み。

**留意**: A-1〜A-3 で CreatureEntity 直下のスカラ／固定長配列フィールドは
ほぼ全て private 化が完了した。残るのは上記 2 個の複合型のみで、これらは
参照アクセサ化の費用対効果が低いため現状維持とする。`creature-entity.h`
変更は上流マージ衝突を増やすため、今後の追加カプセル化は効果と費用を
勘案して判断する。

---

# B トラック: 処理（振る舞い）統合ロードマップ

A トラック（フィールドのカプセル化）が一段落したため、第 2 層として
**プレイヤーとモンスターで並行・別実装になっている「処理（振る舞い）」を
`CreatureEntity` ベースの共通化により統合する**作業を B トラックとして整理する。

## B トラックの設計原則（5 ドメイン横断調査の結論）

ダメージ・行動/エネルギー・攻撃・状態異常付与・効果適用の 5 ドメインを
横断調査した結果、**統合の可否は「プリミティブ層」と「オーケストレータ層」で
明確に分かれる**ことが判明した。

- **プリミティブ層（演算・記憶）は統合に適する。** 命中判定の素の計算、
  スレイ／属性倍率、HP 減算、時限効果の記憶などは型に依存しない純粋ロジック。
  実際、以下は既に共通化済み:
  - `apply_raw_damage()` / `on_take_hit()` / `on_death()` フック（Phase 4）
  - `calc_attack_damage_with_slay()` / `mult_slaying()` / `mult_brand()`
    （攻撃側・目標側とも `CreatureEntity` を取る完全汎用、3 経路で共用）
  - `set_timed_effect()` の記憶＋mproc 保守（B1 / 提案 49・50）
  - 自然回復計算（提案 8）、AI ターゲット選定（提案 14）

- **オーケストレータ層（手続き全体）は型固有の副作用を多く抱え、統合価値が低い。**
  `take_hit()`（プレイヤー: オートセーブ／インベントリ破壊／日記／ラストワード）、
  `MonsterDamageProcessor`（モンスター: ドロップ／経験値／アライアンス更新）、
  `effect-player-switcher` vs `effect-monster-switcher`（構造体・メッセージ系・
  耐性モデルが別物）、`BadStatusSetter`（プレイヤー固有の徳変化／構え崩し／
  呪術停止）と `set_monster_*`（最小限）は **本質的に分岐**しており、
  単一ディスパッチャに束ねても call site は型を意識し続ける。これは
  CLAUDE.md Phase 4 で「ディスパッチャ不要」と判断したのと同じ構図。

**したがって B トラックは「巨大オーケストレータの統合」を狙わず、
プリミティブ層に残る重複を外科的に抽出する小粒な提案の積み上げで進める。**
各提案は A トラック同様 1 論理変更＝1 コミットとし、ビルド＋clang-format で検証する。

## B トラック索引（B1 完了済、B2 以降は計画）

| 番号 | 提案 | ドメイン | 価値 | リスク/規模 | 状態 |
|---|---|---|---|---|---|
| B1 | 時限効果付与＋mproc 保守の統合 (提案 49/50) | 状態異常 | — | — | ✅ 完了 |
| B2 | ターン・エネルギー消費カーネルの共通化 | 行動 | 中→小 | 低（ただし critical path） | ✅ 完了（縮小） |
| B3 | モンスター命中判定 2 関数の統合 | 攻撃 | 中 | 低 | ✅ 完了 |
| B4 | モンスター打撃の武器スロット選択共通化 | 攻撃 | 中 | 低〜中 | ✅ 完了 |
| B5 | HP 回復プリミティブ `heal_hp()` の共通化 | 効果適用 | 中 | 低 | ✅ 完了 |
| B6 | `is_player()` 真分岐の限定的 virtual 化 | 横断 | 低 | — | ⏭️ 見送り（調査ベース） |
| B7 | テレポート先判定の共通化 | 効果適用 | 低 | — | ⏭️ 見送り（調査ベース） |
| B8 | 大規模統合（effect switcher / 状態異常付与 API / damage sink） | 横断 | 低 | 高 | **見送り（要再評価）** |

---

## 提案 B2: ターン・エネルギー消費カーネルの共通化 ✅ 完了（縮小スコープ）

**現状の分離:** プレイヤー (`player-processor.cpp:141`) とモンスター
(`monster-processor.cpp:1610`) が同一の
`sub_energy_need(speed_to_energy(speed))` を手書きしていた。

**調査で判明した分岐:** 当初「速度減算＋ゲート判定」を共通化する想定だったが、
精査の結果 **真に共有可能なのは速度→エネルギー減算の 1 行のみ**と判明:
- 速度の決定が分岐（プレイヤーは自速度、モンスターは騎乗時搭乗者速度/通常は
  一時速度 `get_temporary_speed()`）。
- 手番後の加算も分岐（プレイヤー `energy_use * ENERGY_NEED()/100`、
  モンスター一律 `ENERGY_NEED()`）。
- ゲート判定 `get_energy_need() > 0` は既に getter 1 行。

**完了内容（縮小）:** 唯一純粋共有可能な減算を
`CreatureEntity::consume_energy_by_speed(int speed)`
（= `sub_energy_need(speed_to_energy(speed))`、定義は creature-entity.cpp で
speed-table.h を本ヘッダに持ち込まない）に集約し、両 call site を移行。速度の
決定は呼出側に残す。挙動完全不変。

**規模/価値:** 当初想定より小。速度→エネルギー変換と減算方向が 1 箇所に集約され、
将来式を変える際の単一編集点になる。**critical path のため挙動不変を厳守**し、
ビルド (g++ -O3 -Werror) で検証。ゲート判定や加算側は型固有のため統合しない判断。

---

## 提案 B3: モンスター命中判定 2 関数の統合 ✅ 完了

**現状の分離:** `check_hit_from_monster_to_player()`
(`attack-accuracy.cpp:70-90`) と `check_hit_from_monster_to_monster()`
(`同:100-115`) は **5% 床/天井・`power + level*3` vs `ac*3/4`・朦朧で 1/2 ミス**
まで完全同一。唯一の差は前者が目標の AC を `creature.get_ac()` から解決し
`ATTACK_SUIKEN` 時に `level*2` を加算する点のみ。

**完了内容:** `check_hit_from_monster_to_player()` を、目標 AC（+SUIKEN 補正）を
解決して `check_hit_from_monster_to_monster()` に委譲する薄いラッパへ簡素化。
AC 解決は乱数を消費しないため、カーネル内の乱数列（`randint0(100)` →
`one_in_(2)` → `randint1(i)`）の順序・挙動はモンスター対モンスターと**完全一致**し
挙動不変。重複していた命中式が 1 本化された。

**規模/価値:** 小・低リスク（純粋計算、目標の AC 解決のみ差分）。将来の命中式
調整が 1 箇所で済む。フルビルド (g++ -O3 -Werror -Wall -Wextra) と
clang-format-18 で検証済み。

---

## 提案 B4: モンスター打撃の武器スロット選択共通化 ✅ 完了

**現状の分離:** モンスター対プレイヤー (`monster-attack-player.cpp:176-198`) と
モンスター対モンスター (`melee/monster-attack-monster.cpp:354-373`) が、打撃
メソッド (HIT/PUNCH/SLASH/STING) に対する二刀流の blow-index 交互選択・単手選択・
素手 (-1) のロジックを重複実装していた。

**完了内容:** 攻撃側は両経路とも `CreatureEntity`（モンスター）なので、
`CreatureEntity::select_melee_weapon_slot(int blow_index, RaceBlowMethodType
method) const` メソッドに集約し、両 call site の switch ブロック (~20 行 ×2) を
1 行呼出に置換。`RaceBlowMethodType` は creature-entity.h に前方宣言を追加し、
定義は creature-entity.cpp（`monster-attack-table.h` を include）に置いた。
**新規ファイルを作らず既存 TU（creature-entity）に収め、Makefile.am / VS
プロジェクトの更新を回避**した。挙動不変。

**規模/価値:** 中。リスク低〜中（攻撃ループ内のため要回帰確認、ビルドで検証）。
フルビルド (g++ -O3 -Werror -Wall -Wextra) と clang-format-18 で検証済み。

---

## 提案 B5: HP 回復プリミティブ `heal_hp()` の共通化 ✅ 完了

**現状の分離:** モンスター回復経路が
`hp += dam; if (hp > maxhp) hp = maxhp;` / `hp = std::min(hp + X, maxhp)` の
手書きクランプを各所で繰り返していた。

**完了内容:** `CreatureEntity::heal_hp(int amount)`（`apply_raw_damage()` と対称な
回復側プリミティブ。現在 HP に加算し `maxhp` でクランプ）を新設し、**生クランプが
純粋形だった 6 サイトを移行**:
- `effect-monster-spirit.cpp` (ドレインによる回復)
- `monster-eating.cpp:284` (吸収攻撃)
- `melee/monster-attack-monster.cpp:52` (吸収攻撃)
- `monster-processor.cpp` の `heal` ラムダ 2 箇所 (ポーション)
- `monster-damage.cpp:533` (マゾヒスト反応)

**意図的にスキップ:** メッセージと絡む `mspell-status.cpp`（full-heal 判定が
`>=` で分岐）、事前クランプ形＋変数再利用の `monster-eating.cpp:234`、
`MONSTER_MAXHP` ガード付きの `effect-monster-oldies.cpp` は挙動不変を優先して
現状維持。

**規模/価値:** 小〜中。リスク低（クランプ意味論は明確）。HP 直接操作の散在を削減し、
将来「最大 HP 一時減少」異常導入時に回復上限を 1 箇所で扱える足場とした。
フルビルド (g++ -O3 -Werror -Wall -Wextra) と clang-format-18 で検証済み。

---

## 提案 B6: `is_player()` 真分岐の限定的 virtual 化 ⏭️ 見送り（調査ベース）

**当初の着手候補を精査した結果、真の dual-arm 重複は存在せず**、機械的
virtual 化は「ガードの移設」にしかならないと判明したため見送る（提案 35 の
結論を再確認・強化する形）。

**精査内容:**
- **呪い効果分岐** (`effect-monster-curse.cpp:14/30/47/64`): エージェント報告は
  「player/monster で別処理」としていたが、実際は `em_ptr->is_player()` が
  **詠唱者がプレイヤーか**を見て**プレイヤー詠唱時のフレーバーメッセージを
  出すだけの単腕ガード**。モンスター側で別処理を行う else 腕は無く、共通化
  対象の重複ではない。
- **撃破時アーティファクト生成** (`special-death-switcher.cpp:194/253/466` 等):
  すべて `if (killer.is_player())`（プレイヤー撃破時のみ生成）または
  `if (!killer.is_player()) return`（型契約ガード）の**単腕**。`virtual
  on_defeat_monster()` へ寄せても巨大 switch 内の複数ハンドラにガードを
  分散移設するだけで重複削減にならない。
- 全体survey: `is_player()` 約 102 サイトのうち、両腕が実処理を持つ真の分岐は
  ごく僅かで、その大半は**プレイヤーのみが UI/画面を持つことに起因する
  表示・知覚分岐**（`main-window-left-frame` / `melee-util` / `display-player`
  等）であり本質的にプレイヤー固有。

**結論:** B6 に機械的削減で価値の出る対象は無い。`creature-entity.h` への
virtual 追加（マージ衝突増）に見合う重複削減が得られないため**現状維持**。
将来モンスターに徳・詠唱・プレイヤー型死亡処理等を持たせる C トラック着手時に、
個別ハンドラ単位で再評価する。

---

## 提案 B7: テレポート先判定の共通化 ⏭️ 見送り（調査ベース）

**精査結果:** `cave_player_teleportable_bold()` (`grid.cpp:881`) と
`cave_monster_teleportable_bold()` (`grid.cpp:838`) の**真に共通な部分は
`TELEPORTABLE` 地形フラグ判定と `TELEPORT_PASSIVE` 早期 return のみ**で、
実質 1〜2 行。中核ロジックは大きく分岐:

| 観点 | プレイヤー | モンスター |
|---|---|---|
| vault 回避 | icky グリッド拒否 (`!NONMAGICAL`) | 無し |
| ルーン回避 | 無し | 保護/爆発ルーン拒否 |
| 占有判定 | 騎乗中モンスターのみ許可 | 自分以外の m_idx を拒否 |
| 罠回避 | `HIT_TRAP` 拒否 | 無し |
| 最終進入判定 | `player_can_enter()` | `monster_can_cross_terrain()` |

**結論:** 共通化すると `is_teleportable()` 内部に `is_player()` 分岐を
**再導入**することになり、まさに排除したいフォークが復活する。共有可能な
プリミティブは `terrain.flags.has(TELEPORTABLE)` という既に 1 行の判定のみで、
抽出してもインダイレクションが増えるだけ。**現状維持**とする
（B トラック設計原則「オーケストレータは型固有副作用で分岐するため統合しない」
の典型例）。

---

## 提案 B8: 大規模オーケストレータ統合（見送り・要再評価）

以下は LOC 上は重複が大きく見えるが、**型固有の副作用・データ構造が本質的に
分岐**しており、現時点では統合の費用対効果が見合わない。設計原則に従い見送り、
将来モンスター側に該当機能（徳・呪術・クラス能力など）を持たせる方針が
具体化した時点で再評価する。

- **effect switcher の統合** (`effect-player-switcher` 169 行 vs
  `effect-monster-switcher` 552 行): 行数差は「重複」ではなくモンスター側が
  多くの属性ケースを個別処理しているため。`EffectPlayerType` と `EffectMonster`
  の構造体統合・メッセージ系の差異吸収まで必要で高リスク。
- **状態異常付与 API** (`BadStatusSetter` vs `set_monster_*`): コア
  (`set_timed_effect`+mproc) は B1 で統合済み。残るプレイヤー側副作用
  （徳変化・構え崩し・呪術停止・ステータス減少・スナイパー集中リセット）は
  完全にプレイヤー固有で、`virtual inflict_*()` に寄せても中身は 2 経路のまま。
- **damage sink の統合** (`take_hit` / `mon_take_hit` / `mon_take_hit_mon`):
  死亡・経験値・ドロップ・セーブ等の後処理が全面的に分岐。Phase 4 で
  ディスパッチャ不要と判断済みの構図と同一。

**再評価のトリガ:** モンスターに徳／クラス能力／プレイヤー型死亡処理を
持たせる C トラック（大規模機能）に着手する際、これらの統合価値が上がる。

---

## B トラック実施結果

**B3 → B5 → B2 → B4 → B6 → B7** の順で実施した。結果:

| 提案 | 結果 | 要約 |
|---|---|---|
| B3 | ✅ 完了 | 命中判定 2 関数を委譲で統合（純粋計算、挙動不変） |
| B5 | ✅ 完了 | `heal_hp()` プリミティブで回復クランプ 6 サイト集約 |
| B2 | ✅ 完了（縮小） | `consume_energy_by_speed()` で速度→エネルギー減算を集約（共有は 1 行のみと判明） |
| B4 | ✅ 完了 | `select_melee_weapon_slot()` で武器スロット選択 ~40 行重複を集約 |
| B6 | ⏭️ 見送り | `is_player()` 真分岐に dual-arm 重複は無く、virtual 化はガード移設のみ |
| B7 | ⏭️ 見送り | テレポート判定は共通部が 1 行で、統合は `is_player()` 分岐の再導入になる |
| B8 | ⏭️ 見送り | 大規模オーケストレータは型固有副作用で本質的に分岐 |

**総括:** B トラックの設計原則（**プリミティブ層は統合に適し、オーケストレータ層は
型固有副作用で分岐するため統合価値が低い**）が全提案で裏付けられた。真のプリミティブ
だった B3/B5 はクリーンに統合でき、薄い共有しか持たない B2 は縮小実施、
オーケストレータ寄りの B6/B7/B8 は統合を見送った。**B トラックは概ね完了**で、
残る統合価値はモンスターにプレイヤー機能（徳・詠唱・死亡処理等）を持たせる
C トラック着手時に再評価する。

---

# C トラック: モンスターへのプレイヤー機能付与ロードマップ

A トラック（フィールドのカプセル化）・B トラック（処理の統合）で
`CreatureEntity` 基盤は概ね整備された。第 3 層 C トラックは、その基盤を使って
**モンスターに実際のプレイヤー的能力（種族・職業・成長・ESP・詠唱・突然変異等）を
運用させる機能開発**を扱う。

## C トラックの性質と設計原則（3 ドメイン調査の結論）

**C トラックは A/B と異なり「挙動を変えないリファクタリング」ではなく
「ゲームバランスを変える機能開発」である。** したがって各提案は必ず
**ゲームデザイン判断（どのモンスターに・どの程度・どんな数値で）を伴う**。
実装前にメンテナ（deskull-m 氏）の意思決定が要る箇所を明示する。

調査で判明した重要事実:

- **フィールド層は既にほぼ統一済み。** 種族/職業/性格/魔法領域/ESP/MP/
  突然変異/熟練度の各フィールドは `CreatureEntity` 上にあり virtual アクセサ
  経由でモンスターからも読み書きできる（提案 1/2/6/10/33/44 等）。
- **フォークは UI/コマンド層に残る。** `cmd-spell` / `display-self-info` /
  `mutation-processor` のメッセージ・`teleport_player()` 等プレイヤー専用
  副作用が統合の障壁。ESP のようにモンスター AI が既に読む系統は障壁が無い。
- **既に一部は稼働中。** モンスター生成時に `psex` / `ppersonality` /
  `realm1`/`realm2` / `stat_modifiers` は割り当て済み。レベルアップ・HP 成長も
  第 4 段で稼働。`player_birth_as_monster()` は種族/職業/性格/領域を完全に
  割り当てる実績あるリファレンス実装。

### C トラック設計原則: JSON オプトインで既定バランスを保つ

bakabakaband は既に **per-monrace の JSON 指定**で個体をカスタムする仕組みを
持つ（`personality` / `stat_modifiers` / `hit_point_per_level`）。C トラックの
機能も原則この方式に倣い、**既定では無効（現行バランス不変）・特定モンスターに
JSON で明示付与**する形で導入する。これにより:

- 2,300 種全体のバランスを一括で崩さず、狙った個体（ユニーク・ボス・
  召喚英雄）にのみ機能を付与できる。
- メンテナのデザイン判断を JSON データに閉じ込め、コード側は汎用機構に徹する。
- 段階的検証が可能（1 体に付与して挙動確認 → 横展開）。

## 現行フォーメーション（機能別ステータス）

| 機能 | フィールド基盤 | モンスター運用 | C 提案 |
|---|---|---|---|
| 性格 `ppersonality` | ✅ | ✅ 生成時割当・稼働 | （完了） |
| 性別 `psex` | ✅ | ✅ 生成時割当・稼働 | （完了） |
| 能力値補正 `stat_modifiers` | ✅ | ✅ 生成時適用・稼働 | （完了） |
| レベルアップ/HP 成長 | ✅ | ✅ 第 4 段で稼働 | C2 が拡張 |
| ESP/知覚 `esp_*` | ✅ | 🔴 プレイヤーのテレパシー専用（モンスター AI は自 ESP を読まない） | C3（要新規 AI） |
| 種族 `prace` | ✅ | 🟡 NONE 固定（cache は有） | **C1** |
| 職業 `pclass` | ✅ | 🟡 NONE 固定（cache は有） | **C1** |
| 能力値成長 | ✅ | 🔴 生成後不変 | C2 |
| 熟練度成長 `weapon/skill_exp` | ✅ (アクセサ) | 🔴 未使用 | C2 |
| MP 消費詠唱 `current_mp` | ✅ | 🟡 pool 有・消費せず | C4 |
| 突然変異 `muta` | ✅ | 🟡 処理は汎用・付与機構なし | C5 |
| 魔法領域詠唱 `realm1/2` | ✅ (割当済) | 🔴 別系統(mspell)で未使用 | C6 |
| class_specific_data | ✅ | 🔴 pclass=NONE で空 | C7（限定） |
| 徳 `virtues` | ✅ | 🔴 is_player ガード・UI 束縛 | C8（見送り） |
| 空腹 `food` | ✅ | 🔴 モンスター read 皆無 | C8（見送り） |

## C トラック提案索引

| 番号 | 提案 | 基盤 | 工数 | 価値 | 主なデザイン判断 |
|---|---|---|---|---|---|
| C0 | 成長状態の savefile 完全永続化 (`hp_table`) | ほぼ完了 | 小 | 小 | バージョン bump 要否 |
| C1 | 種族・職業の顕在化（`prace`/`pclass` 付与） | ✅ cache 有 | 中 | 高 | ✅ 1(付与)/2(基本5)/3(二次7)/4(恐怖)/5(自由行動)/6(光闇)/7(複合攻撃) 完了 |
| C2 | 能力成長の拡張（stat / 熟練度） | ✅ 成長機構 | 中 | 中 | ✅ stat成長/熟練度=近接命中/STR→近接ダメージ/STR・DEX→命中/DEX→AC/WIS→セーヴ(全opt-in `applies_stat_combat_bonus`) 完了 |
| C3 | ESP のモンスター付与 | 🔴 新規 AI 要 | 中〜大 | 中 | ESP をモンスター AI にどう効かせるか（新規設計） |
| C4 | MP 消費詠唱 | 🟡 pool 有 | 中 | 中 | ✅ 完了（opt-in・レベル比例コスト） |
| C5 | 突然変異のモンスター運用 | 🟡 処理汎用 | 中 | 中 | ✅ 完了（3段: 付与→専用処理→発火） |
| C6 | 魔法領域詠唱のモンスター運用 | 🔴 別系統 | 大 | 中〜高 | ✅ 完了（案A: realm→ability・opt-in） |
| C7 | class_specific_data の限定運用 | 🔴 | 中 | 低〜中 | ✅ 完了（groundwork: 4クラスの variant 初期化） |
| C8 | 徳・空腹の運用 | 🔴 UI 束縛 | 大 | 低 | **見送り**（UI/物語依存） |

---

## 提案 C0: 成長状態の savefile 完全永続化

**現状（要修正の CLAUDE.md 記述あり）:** 調査の結果、モンスターの `exp` /
`level` / `max_maxhp` / `max_exp` / `max_max_exp` / `exp_frac` は
**creature-common (v52) 経由で既に保存されている**（`creature-common-writer.cpp:31`
等）。CLAUDE.md の「進行中の exp は非保存」は**古い記述**で、common
シリアライザ統合後は保存されている。**未保存なのは `hp_table[]`（レベル別
累積 HP のロール履歴）のみ**。

**影響:** `hp_table[]` 未保存でも `max_maxhp` / `level` は保存されるため、
成長「結果」は永続する。失われるのは中間レベルの HP 分布履歴のみで、
ロード後の再レベルアップは新規ロールになる（最終 HP は同等）。実害は小さい。

**作業:** (a) CLAUDE.md の記述を「exp は保存・hp_table のみ未保存」に修正、
(b) 必要なら `hp_table[]` を creature-common に追加（v54 bump）。C2 で
「レベル成長時に hp_table を厳密再現したい」場合の前提となる。

**デザイン判断:** hp_table を保存する価値があるか（中間履歴の厳密性 vs
savefile サイズ）。**低優先**。

---

## 提案 C1: 種族・職業の顕在化（`prace` / `pclass` 付与） ✅ 第1弾完了（JSON付与・効果なし）

**第1弾 完了内容（メンテナ選択: JSON オプトイン・効果なし）:**
`MonraceDefinition` に `player_race` / `player_class` を追加し、JSON
`"player_race": "HIGH_ELF"` / `"player_class": "MAGE"` で個別モンスターに
プレイヤー種族・職業を固定指定できるようにした。生成時に
`CreatureEntity::assign_fixed_player_race_and_class()`（chameleon 後の実効
monrace を参照）が `prace`/`pclass` に付与する。**効果は未反映**（フィールド
付与のみ、既定バランス完全不変）。`personality` の実装パターンを踏襲。

- トークン表 `r_info_player_race`（132 種）/ `r_info_player_class`（41 職）を
  `race-info-tokens-table` に追加（enum 名から機械生成）。
- reader `set_mon_player_race` / `set_mon_player_class` を追加（`info_grab_one_const`
  経由、未指定=null は NONE のまま）。
- スキーマ `schema/MonraceDefinitions.schema.json` に両キーを登録し CI JSON
  検証を通過（`additionalProperties: false` 対応）。実データへの付与は未実施
  （オプトインのためメンテナが個体選定）。
- フルビルド (g++ -O3 -Werror -Wall -Wextra) / clang-format-18 / validate_json.py で検証済。

**第2弾 ✅ 完了（種族耐性の opt-in 反映）:**
`MonraceDefinition::applies_player_race_resistances`（bool, 既定 false）を追加。
付与された `player_race`（C1第1弾）が基本 5 属性（火/冷/電/酸/毒）の耐性
（`TR_RES_*`）を持つ個体は、`effect-monster-resist-hurt.cpp` の各属性ハンドラで
被ダメージを約 1/3 に軽減する（プレイヤー部分耐性と同水準）。

- 配線: 匿名 namespace の `target_race_resists_element()` /
  `apply_monster_race_resistance()`。免疫優先・弱点排他（`else if`）、毒は D7 の
  DoT 蓄積前に軽減。`prace == NONE` は false（OOB 回避）。
- **既定 false のため誰にも反映されず既定バランス不変。** 軽減率はハンドラ定数で調整可。
- 実コード調査で判明した重要事実: 耐性クエリ経路（`common_cause_flags`）は元々
  `is_player()` ガード無しで monster prace を読むが、**モンスター被ダメージ経路は
  monrace フラグを直接読む**ため prace 付与だけでは戦闘に反映されなかった。本第2弾は
  その被ダメージ経路へ種族耐性を opt-in で明示配線したもの。
- reader（`info_set_bool`）／schema／CLAUDE.md 整備。フルビルド（g++ -O3 -Werror）／
  validate_json.py で検証済。

**第3弾 ✅ 完了（二次属性への耐性反映拡大）:**
第2弾（基本 5 属性）と同じ opt-in フラグ `applies_player_race_resistances` の被覆を
**二次属性 7 種**（地獄 `TR_RES_NETHER` / 混沌 `TR_RES_CHAOS` / 破片 `TR_RES_SHARDS` /
轟音 `TR_RES_SOUND` / 混乱 `TR_RES_CONF` / 劣化 `TR_RES_DISEN` / 因果 `TR_RES_NEXUS`）へ
拡大。

- 各ハンドラのネイティブ resist 条件へ `|| target_race_resists_element(em_ptr, TR_RES_X)`
  を **OR-in** し、種族耐性でネイティブと同じ軽減・**副作用抑止**（轟音の `do_stun`・
  混乱の `do_conf`・混沌の polymorph/混乱）を発火させる。
- 種族由来 resist では monrace ネイティブ耐性の思い出フラグを記録しない
  （`native_resist` ローカルでガード）。
- フラグ被覆拡大のみで新フラグは追加せず、**既定 false のまま＝バランス不変**。
  複合/特殊攻撃（rocket/icee_bolt/void/abyss）は多属性・特殊ロジックのため対象外。
- フルビルド（g++ -O3 -Werror）で検証済。

**第4弾 ✅ 完了（状態異常耐性・恐怖の反映）:**
種族の恐怖耐性 `TR_RES_FEAR` を反映。全恐怖源が通る中央ゲート
`effect_damage_piles_fear`（`effect-monster.cpp`）の `do_fear→恐怖` 変換を、
`NO_FEAR` と同経路へ `|| target_race_resists_element(em_ptr, TR_RES_FEAR)` を OR-in
して無効化。ダメージ属性でない**状態異常耐性を反映した最初の例**。

- 共通述語 `target_race_resists_element(EffectMonster*, tr_type)` を
  `effect-monster-resist-hurt.cpp` の匿名 namespace から **`effect-monster-util.{h,cpp}`
  へ昇格**（属性ダメージ／状態異常の両 TU から使う共通述語に）。
- 同じ opt-in フラグ `applies_player_race_resistances` を使用（新フラグなし）。
  既定 false のままバランス不変。フルビルド（g++ -O3 -Werror）で検証済。

**第5弾 ✅ 完了（自由行動＝睡眠/拘束耐性の反映）:**
種族の自由行動 `TR_FREE_ACT`（耐麻痺）を反映。プレイヤーの麻痺免疫に相当する
モンスター状態異常＝魔法睡眠・拘束(stasis)を無効化。

- **配線:** `effect_monster_old_sleep`（GF_OLD_SLEEP）と `effect_monster_stasis`
  （STASIS/STASIS_EVIL）の `has_resistance` 集約へ
  `|= target_race_resists_element(em_ptr, TR_FREE_ACT)` を OR-in。ネイティブ `NO_SLEEP`・
  UNIQUE・レベルセーヴと同列に扱い、抵抗時は正しく「効果がなかった」メッセージを表示。
- 同じ opt-in フラグ `applies_player_race_resistances` を使用（新フラグなし）。
  既定 false のままバランス不変。フルビルド（g++ -O3 -Werror）で検証済。
- **対象外（niche）:** psi 攻撃由来の睡眠（`effect-monster-psi.cpp`）・円月
  (`effect_monster_engetsu`) の特殊睡眠は据え置き（第3弾の rocket/void 等と同じ
  「主要ハンドラのみ」方針）。

**第6弾 ✅ 完了（光/闇耐性の反映）:**
種族の光 `TR_RES_LITE` / 闇 `TR_RES_DARK` 耐性を反映（HIGH_ELF 等が持つ代表的種族耐性）。
`effect_monster_lite` / `effect_monster_dark`（`effect-monster-lite-dark.cpp`）の
ネイティブ resist 条件へ `|| target_race_resists_element(...)` を OR-in。

- 光は光弱点 `HURT_LITE` より優先（`else if` で弱点経路を回避＝耐性が弱点に勝つ）。
- 種族由来 resist では monrace ネイティブ耐性の思い出フラグを記録しない
  （`native_*_resist` ローカルでガード）。
- 同じ opt-in フラグ `applies_player_race_resistances` を使用（新フラグなし）。
  既定 false のままバランス不変。フルビルド（g++ -O3 -Werror）で検証済。
- **対象外（niche）:** `LITE_WEAK`（光弱点個体専用）・abyss/円月の特殊光闇処理は据え置き。

**第7弾 ✅ 完了（複合攻撃への耐性反映＝カバレッジ完成）:**
第3弾で主要ハンドラのみ対象とした二次属性耐性を、複合攻撃ハンドラへ拡張して
カバレッジを完成させた。

- **rocket**（`effect_monster_rocket`）: 種族の破片耐性 `TR_RES_SHARDS` を
  ネイティブ `RESIST_SHARDS` と同経路へ OR-in（`dam/=2` の部分軽減）。
- **icee_bolt**（`effect_monster_icee_bolt`）: 付随する轟音スタンを種族の
  `TR_RES_SOUND` でも無効化。さらに冷気ダメージ本体も種族の `TR_RES_COLD` で
  約 1/3 軽減（`apply_monster_race_resistance`、`effect_monster_cold` と同水準）。
- 種族由来 resist では monrace ネイティブ耐性の思い出フラグを記録しない
  （`native_resist` ガード）。同じ opt-in フラグ `applies_player_race_resistances`
  を使用（新フラグなし）。既定 false のままバランス不変。フルビルドで検証済。

**第8弾 ✅ 完了（反射＝ボルト反射）:** 反射 `TR_REFLECT` をモンスターのボルト反射へ
opt-in 反映。属性耐性とは別軸の防御特典のため**独立フラグ
`applies_player_race_reflection`（既定 false）** を新設。`effect-processor.cpp` の
ボルト反射判定 `monrace.misc_flags.has(REFLECTING)` へ
`|| monster.has_race_granted_reflection()` を OR-in。述語 `has_race_granted_reflection()`
は `CreatureEntity` メソッド（`has_monster_profile` && フラグ && `prace!=NONE` &&
`CreatureRace(this).tr_flags().has(TR_REFLECT)`）。const 呼出のため CreatureRace は
const_cast（`tr_flags()` は read-only）。reader/schema/CLAUDE.md 整備、既定 false で
バランス不変。フルビルド (BUILD_EXIT=0) / validate_json 8/8 で検証済。

**種族特典反映トラックのまとめ（第2〜8弾で完了）:** プレイヤー種族が持つ
**主要な属性耐性・状態異常防御・防御特典はほぼ全て**モンスターへ opt-in 反映済み（基本5属性・
二次7属性・光/闇・恐怖・自由行動＝睡眠/拘束、複合攻撃 rocket/icee、及び反射）。

**第9弾 ✅ 完了（水耐性）:** `TR_RES_WATER` を `effect_monster_water` のネイティブ
`RESIST_WATER` 判定へ `native_resist` パターンで OR-in（部分軽減 `dam*3/(1d6+6)`、思い出
記録は固有耐性時のみ）。**実データ検証で 14 のプレイヤー種族が `TR_RES_WATER` を付与**する
ことが判明し、下記「対象外」の記述（water を含む）が誤りだったため訂正・実装した。

**第10弾 ✅ 完了（再生）:** `TR_REGEN` を `has_regen_flag()` のモンスター分岐（native
`REGENERATE` 判定）へ `|| has_race_granted_regeneration()` で OR-in。付与種族が再生を持てば
自然回復量が 2 倍（`compute_regen_amount`）。属性耐性・反射とは別軸の常時発動特典のため
**独立フラグ `applies_player_race_regeneration`**（既定 false）。反射・再生の述語は共通の
private ヘルパ `race_grants_tr_flag(tr_type)` に集約。**10 のプレイヤー種族が付与**。
race-perk 集計により、残る未反映特典は AI 要（ESP/see_invis/telepathy）・モンスターに無意味
（sustain/hold_exp/slow_digest）に整理された。

**第11弾 ✅ 完了（加速）:** `TR_SPEED` を生成時速度へ opt-in 反映。**独立フラグ
`applies_player_race_speed`**（既定 false）。`place_monster_one` のサイズ補正後段で
`has_race_granted_speed()` が真なら固定 +3 を `speed` に加算。**設計上の注意:** プレイヤーの
種族速度は `CreatureRace::speed()` 経由で種族依存（KLACKON/SPRITE のみ level/10）かつ
位置/レベル依存で動的であり、TR_SPEED フラグ自体は速度値を持たない（動物種族は表示用）。
モンスター速度は静的 `speed` フィールドのため動的反映は侵襲的すぎるとし、**保守的な固定
近似 (+3、調整用 constexpr)** を採用。これで clean-hook の耐性/防御/常時特典の反映を一通り完了。

**未反映（意図的）:** 職業特典・種族の非耐性特典（ESP・赤外線視）はモンスター AI 索敵の
新規実装が要る（C3 と同課題）ため大。time/gravity/plasma 系は player 種族が
該当耐性を持たないため対象外（water は第9弾で反映済み）。

**（訂正）能力値の戦闘反映は C2 第3弾で解決済み:** かつて「stat 補正はプレイヤー
(percentile 3-18)とモンスター(内部 ×10・線形 30-400)でスケールが異なり反映不可」と
していたが、`stat_value_to_table_index()`（CON→HP と同じ索引変換）により**モンスター
stat を adj テーブル索引へ変換して反映可能**と判明し、C2 第3弾で STR→近接ダメージ／
STR/DEX→命中／DEX→AC／WIS→セーヴを opt-in 反映済み（`applies_stat_combat_bonus`）。

---

### （参考）当初調査時の基盤メモ

**基盤:** `place_monster_one()` が `MonsterProfile::equivalent_player_races[]` /
`equivalent_player_classes[]` を生成時にキャッシュ済み（cache は有）。
`player_birth_as_monster()` の `apply_monrace_race()` / `apply_monrace_class()`
が kind_flags → prace/pclass の優先度マッピングを既に実装（実績あるリファレンス）。
現状、通常生成モンスターの `prace`/`pclass` は NONE 固定（dormant）。

**作業:** 生成時に（またはオプトインで）kind_flags の支配的フラグから
`prace`/`pclass` を設定し、種族・職業由来の問合せ（`get_race_info()` 等）が
モンスターでも意味を返すようにする。`player_birth_as_monster` のマッピングを
共通化して再利用。

**デザイン判断（要メンテナ）:**
- **付与対象:** 全モンスターに一律付与するか、JSON オプトイン
  (`"player_race": "ELF"` 等) の個体のみか。**推奨は JSON オプトイン**
  （種族由来の耐性・能力がモンスター強度を変えるため、既定バランス保護）。
- **反映範囲:** prace/pclass を持たせた時、種族耐性・職業特典を実際に
  戦闘へ反映するか、表示・フレーバのみか。段階導入推奨（まず表示 → 後で効果）。

**工数:** 中。**価値:** 高（種族・職業という最大のプレイヤー概念をモンスターへ開く）。
**リスク:** 中（効果反映まで行うとバランス変動大）。

---

## 提案 C2: 能力成長の拡張（stat / 熟練度）

### 第1弾 ✅ 完了: stat 成長（JSON オプトイン）

**完了内容（メンテナ選択: オプトイン・既定 OFF）:**
`MonraceDefinition` に `grows_stats`（既定 `false`）を追加し、`try_monster_level_up()`
のレベルアップ時に、フラグの立つ個体のみ `CreatureEntity::grow_stats_by_levels()`
で 6 能力値を獲得レベル数ぶん成長させる。

- 成長量は保守的な定数 `stat_growth_per_level = 2`（内部 10 単位 = 表示 0.1/レベル）。
  獲得レベル数は基準レベル上限で有界のため成長も有界。バランス調整はこの定数で行う。
- HP 成長（第 4 段）は従来通り全モンスター共通で不変。本フラグは能力値成長のみ制御。
- reader（`info_set_bool`）／schema／CLAUDE.md を整備。既定 OFF のため実データ・
  既定バランスは不変。
- **注:** 現状モンスター能力値のゲーム効果は限定的（生成時 CON→HP 補正・一部判定）。
  本提案は成長機構・データ経路の確立が主眼で、効果の深化は下記次段以降。
- フルビルド (g++ -O3 -Werror -Wall -Wextra) / clang-format-18 / validate_json.py で検証済。

### 第2弾 ✅ 完了（戦闘習熟＝近接命中補正、モンスター適合形）

**着手前の実コード検証（重要）:** プレイヤーの `weapon_exp`（武器熟練度）は
**モンスターの innate blow（RaceBlow）には適用されない**ことを確認した。モンスター
近接は `power = mbe_info[effect].power`（固定表）・`rlev = monrace.level`（固定の種族
レベル）で命中判定され、`weapon_exp` を一切参照しない（monster-attack-monster /
melee-util / monster-attack-player 全経路で確認）。よって「player weapon_exp の反映」
は**そのままでは実装不可**。

**モンスター適合形で実装:** モンスターの「戦闘習熟」は**撃破による経験値でのレベル
成長（第4段）**に相当するため、それを近接命中へ反映する形とした。現状 `rlev` は
`monrace.level`（固定）で、モンスターがレベルアップしても**命中は向上しない**という
ギャップを埋める。

- `MonraceDefinition::grows_melee_proficiency`（bool・既定 false）を追加。
- `CreatureEntity::get_melee_proficiency_bonus()` を新設。フラグ ON の個体のみ、
  生成時基準 `monrace.level/2` を超えて成長した分（`get_level() - monrace.level/2`、
  0 下限）を返す。プレイヤー・未成長・フラグ OFF では 0（バランス不変）。
- 近接命中の両経路（monster-vs-monster `melee-util.cpp`、monster-vs-player
  `monster-attack-player.cpp`）の `rlev` へ本ボーナスを加算。命中式（B3 共通化済の
  `check_hit_from_monster_to_*`）はそのまま利用。
- 満成長で `rlev` は最大 `monrace.level → 1.5×monrace.level`（有界）。reader/schema/
  CLAUDE.md 整備。既定 OFF のため実データ・既定バランス不変。フルビルド／
  validate_json で検証済。

### 第3弾 ✅ 完了（能力値の戦闘反映＝STR→近接ダメージ、第1弾）

能力値を戦闘へ反映する拡張の第1弾として、**STR を近接ダメージへ opt-in 反映**。

- `MonraceDefinition::applies_stat_combat_bonus`（bool・既定 false）を追加。
- `CreatureEntity::get_melee_stat_damage_bonus()` を新設。フラグ ON の個体のみ、
  プレイヤーと同じ `adj_str_td` テーブル（`stat_value_to_table_index` で monster の
  内部×10 stat を索引化）で STR 補正 `adj-128` を返す。プレイヤー・フラグ OFF では 0。
- 近接ダメージの両経路（monster-vs-monster `monster-attack-monster.cpp`、
  monster-vs-player `monster-attack-player.cpp`）の `damage_dice.roll()` 直後に加算し、
  負値を 0 下限クランプ（explode 時は据え置き）。
- 既定 OFF のため実データ・既定バランス不変。reader/schema/CLAUDE.md 整備。
  フルビルド／validate_json で検証済。

**第3弾の続き ✅ 完了（STR/DEX → 近接命中）:** 同じ `applies_stat_combat_bonus` フラグの
被覆を命中へ拡大。`CreatureEntity::get_melee_stat_hit_bonus()` が、プレイヤーと同じ
`adj_str_th` / `adj_dex_th` テーブル（`stat_value_to_table_index` で索引）で STR/DEX の
命中補正（各 `adj-128` の和）を返す。近接命中の両経路の `power`（`check_hit_from_monster_
to_*` の accuracy 入力）へ加算。ダメージ（STR）と同じフラグで一括制御（新フラグなし）。
既定 OFF のためバランス不変。フルビルドで検証済。

**第3弾の続き ✅ 完了（DEX → AC）:** 同じ `applies_stat_combat_bonus` フラグの被覆を
AC へ拡大。`CreatureEntity::get_ac()` のモンスター分岐で、プレイヤーと同じ
`adj_dex_ta` テーブル（`stat_value_to_table_index` で索引）の DEX 補正（`adj-128`）を
`total_ac` へ加算し、負値は 0 下限クランプ。get_ac() は攻撃側の命中判定
（`mam_ptr->ac = t_ptr->get_ac()`）で参照されるため、高 DEX の被対象個体は被弾しにくく
なる。命中（攻撃側）と対になる守勢反映。既定 OFF のためバランス不変。

**第3弾の続き ✅ 完了（WIS → セーヴィングスロー）:** 同じ `applies_stat_combat_bonus`
フラグの被覆をセーヴへ拡大。`CreatureEntity::get_save_stat_bonus()` が、プレイヤーと同じ
`adj_wis_sav` テーブル（`stat_value_to_table_index` で索引）の WIS 補正を返す。**注意:**
`adj_wis_sav` は STR/DEX 系（中心 128）と異なり **0-19 の直接加算テーブル**なので
`-128` しない。モンスターのセーヴはプレイヤーの `skill_sav`（モンスターでは未計算≈0）
ではなく**レベル基準判定**（`monrace.level > randint1(...)`）で行われるため、本 WIS 補正を
**実効レベルへ加算**する形で反映する。適用先は `effect-monster-oldies.cpp` の状態異常
セーヴ 6 ハンドラ（old_poly / old_slow / old_sleep / old_conf / stasis / stun）。高 WIS
個体は魔法睡眠・混乱・変身・減速・拘束・朦朧に抵抗しやすくなる。既定 OFF のため
バランス不変。フルビルドで検証済。

**WIS→セーヴの一括適用（続き ✅ 完了）:** oldies 6 ハンドラに続き、コードベース全体の
レベル基準モンスターセーヴ（`monrace.level > randint1(...)`）のうち**精神/状態異常系**へ
WIS 補正を統一適用し、「WIS がモンスターのセーヴを助ける」を一貫させた。
- `spells-diceroll.cpp` の `common_saving_throw_impl`（魅了/服従の DRY 共通実装。
  charm / control undead/demon/animal / charm living / domination の 6 呼出を 1 箇所で反映）
- `effect-monster-psi.cpp`（psi 攻撃耐性）
- `effect-monster-spirit.cpp`（mind_blast / brain_smash＝精神攻撃）
- `effect-monster-evil.cpp`（turn undead / turn evil / turn all＝恐怖セーヴ）
- **除外:** `effect-monster-evil.cpp` の RESIST_TELEPORT 判定（`level > randint1(100)`）は
  テレポート耐性であり精神/状態異常セーヴではないため対象外。射撃・魔法命中は
  **モンスター経路が auto-hit（`check_hit` 不使用・`project` で必中）** のため反映先が無く、
  能力値→命中の拡大は近接に限る（エンジン仕様）。
既定 OFF のためバランス不変。フルビルドで検証済。

**WIS→セーヴのレベル判定を述語へ集約（DRY 続き ✅ 完了）:** WIS 補正を注入した
レベル基準セーヴ式 `(monrace.level + WIS) > randint1(max(1, difficulty-10)) + 10` が
約 10 サイトに散在していたため、共通述語 `monster_saves_status_by_level(em_ptr, difficulty)`
（`effect-monster-util.{h,cpp}`）へ集約。`difficulty` は通常 `em_ptr->dam`、術者レベル基準の
効果（mind_blast / brain_smash）は `caster_lev` を渡す。移行サイト: oldies の poly / slow /
sleep / conf / stun / stasis（6）、evil の turn undead / turn evil / turn all（3）、spirit の
mind_blast / brain_smash（2）。`randint1` の消費は 1 回のみ・呼出側の短絡評価も保存され
**挙動完全不変**。WIS 補正の注入点が 1 箇所に一元化され、将来のセーヴ調整が容易になった。
stasis の未使用化した `stasis_damage` ローカルも削除。（psi は `randint1(3*dam)` で式が
異なるため対象外・インライン維持、charm/control は `common_saving_throw_impl` に別途集約済）。

**能力値戦闘反映のまとめ（`applies_stat_combat_bonus`）:** STR→近接ダメージ・
STR/DEX→近接命中・DEX→AC・WIS→状態異常セーヴを 1 フラグで一括制御。プレイヤーの
近接戦闘とセーヴの主要な能力値補正を opt-in でモンスターに反映する形が揃った。

**次段候補（未着手）:** 射撃・魔法命中への能力値反映、能力値→他系統セーヴ
（呪文抵抗・恐怖等のレベル基準判定以外の経路）等。都度 opt-in で段階導入する。

**デザイン判断メモ:** player weapon_exp/skill_exp は innate blow のモンスターに
概念が合わないため、モンスターの熟練度は「戦闘経験＝レベル成長」で表現した。武器を
装備するモンスター（提案12）への weapon_exp 反映は、モンスター攻撃が innate blow
主体のため対象外（将来、装備武器で殴るモンスターを実装する場合の拡張余地）。

---

## 提案 C3: ESP のモンスター付与

**⚠️ 基盤の訂正（実コード検証済）:** 当初調査では「モンスター AI が既に ESP を
読んでいる」としていたが、**誤り**。`has_esp_*` の使用箇所（`monster-update.cpp`
の `update_specific_race_telepathy`）は `update_monster(creature, ...)` の `creature`
が**常にプレイヤー（視認者）**で、これは**プレイヤーのテレパシーがモンスターを
マップ上に暴く**処理。`update_monster` の全呼出元（floor-changer / spells-detection /
one-monster-placer 等）がプレイヤーを渡すことを確認済。**モンスター AI が自分の
`esp_*` を読んで索敵する経路は存在しない。** よってモンスターに esp_* を付与しても
現状 AI 挙動は変わらない。

**作業（当初想定より大）:** モンスターに ESP を機能させるには、モンスターの
索敵・ターゲティング（awareness/detection）に「自 ESP で対象種別を壁越し感知」
する**新規 AI ロジック**の追加が必要。既存エンジンにモンスター ESP の概念は無い。
その上で JSON `"esp": [...]` 付与機構を載せる。

**デザイン判断（要メンテナ・大）:** そもそもモンスター ESP を AI 索敵にどう
効かせるか（壁越し感知の距離・条件）。既存のノイズ/視線ベース索敵との整合。

**工数:** 中〜大（新規 AI）。**価値:** 中（AI に深みは出るが実装は非自明）。
**リスク:** 中〜高（AI 挙動変更）。→ **当初「最優先」としたが基盤誤認により降格。**

### ✅ 第1弾 完了（テレパシー→超隠密無視、保守的スコープ）

**実コード再調査の結論:** モンスター AI の「awareness（プレイヤーに気付くか）」ギャップは
実質 `process_stealth`（`monster-processor.cpp`）の**忍者の超隠密（`s_stealth`）判定のみ**。
通常時モンスターはプレイヤー位置を把握しており「壁越し感知」で埋めるギャップは無い。
プレイヤー無敵化（透明）もモンスター AI 要因ではない（see_invis 反映は対象外）。

→ よって tractable な C3 の第1弾として、**テレパシー `TR_TELEPATHY` を持つモンスターは
超隠密を無視して常に気付く**を実装。`process_stealth` 冒頭で `has_race_granted_telepathy()`
が真なら `return true`。独立フラグ `applies_player_race_telepathy`（既定 false）＋述語は
共通ヘルパ `race_grants_tr_flag(TR_TELEPATHY)`。**C トラックで初めて AI 挙動に触れた反映**
（効果は超隠密無視に限定＝低リスク）。フルビルド (BUILD_EXIT=0) / validate_json 8/8 で検証済。

**残（第2弾以降・大）:** 睡眠中モンスターの ESP による覚醒、種族別 ESP（esp_evil 等）で
プレイヤー種別を感知、壁越し索敵距離等。いずれも新規 AI ＋バランス判断を要する大物。

---

## 提案 C4: MP 消費詠唱 ✅ 完了（opt-in・レベル比例コスト）

**完了内容（メンテナ選択: opt-in・レベル比例の保守的コスト）:**
`MonraceDefinition` に `consumes_mp`（既定 `false`）を追加。`make_attack_spell()`
で、フラグの立つ個体は詠唱前に `max(1, rlev / mp_cost_divisor)`（既定除数 10）の
MP を要求し、不足時はその手番の詠唱をスキップして近接等にフォール、成立時のみ
`sub_current_mp()` で消費する。

- 基盤: モンスターは生成時に満タンの MP（`calc_creature_mana()`）＋自然回復を
  既に持つ（`one-monster-placer.cpp:483-484`。「MP を能力行使に使う」意図はコメント済）。
- 全呪文一律コストのため「支払えなければ詠唱丸ごとスキップ」で実装が単純。
  DRAIN_MANA の既存 MP ガードとも整合。
- コスト調整は `mp_cost_divisor` 定数（小さいほど高コスト）。
- reader（`info_set_bool`）／schema／CLAUDE.md 整備。既定 OFF のため実データ・
  既定バランス不変。
- フルビルド (g++ -O3 -Werror -Wall -Wextra) / clang-format-18 / validate_json.py で検証済。

**将来拡張余地:** 呪文毎の個別コスト（現状はレベル一律）、MP 不足時のより賢い
呪文選択（安い呪文を選ぶ等）。現状は一律コストのため不要。

---

## 提案 C5: 突然変異のモンスター運用

**⚠️ 着手前の実コード検証（C3 同様、要注意）:** `process_world_aux_mutation()` は
**ターンループで `WorldTurnProcessor` からプレイヤーに対してのみ 1 回呼ばれる**
（`world-turn-processor.cpp:98`、引数はプレイヤー）。内部で
`teleport_player(creature, ...)` / `teleport_player_aux(...)` を **`creature`
（=プレイヤー）に対して直接**呼ぶ。よって:
- モンスターに `muta` を持たせても **per-turn 処理は現状発火しない**（no-op）。
- 単純にモンスターへ処理を回すと、`teleport_player` 等がプレイヤー専用のため
  誤作動する（モンスターの変異でプレイヤーがテレポート等）。

**したがって C5 は C1/C2/C4 より大きく高リスク。** 実装には (a) モンスター毎の
per-turn 変異処理ループの新設（性能・正当性）、(b) 副作用の脱プレイヤー化
（`teleport_player`→汎用テレポート、`msg_print`/`disturb` の is_player ガード）、
(c) モンスターに無意味な変異（DEFECATION 等）の除外フィルタ、が必要。

### ✅ 完了（3 段実装、メンテナ承認のもと本格着手）

**採用アーキテクチャの判断:** 巨大なプレイヤー用 `process_world_aux_mutation()`
（~500 行、`get_aim_dir` 等の UI プロンプト・`player_defecate`・`lose_all_info`・
`BadStatusSetter` の徳/構え副作用など深いプレイヤー結合）を creature-agnostic に
改造するのは巨大かつ高リスクで、しかも大半の効果はモンスターに無意味。
**よって「巨大関数の脱プレイヤー化」ではなく、モンスターに意味のある能動変異のみを
扱う専用関数 `process_monster_mutation()` を新設**する方針を採った。プレイヤー経路は
一切変更せず（挙動完全不変・ゼロリスク）、モンスター安全なプリミティブのみ使用する。

- **C5-1（データ経路）**: `MonraceDefinition::mutations`
  (`EnumClassFlagGroup<PlayerMutationType>`) を追加。トークン表 `r_info_mutation`
  (106 種) / reader `set_mon_mutations`（配列）/ schema を整備。生成時
  `assign_fixed_mutations()` が `add_mutation` で付与。この段では per-turn 処理が
  プレイヤー専用呼出のため未発火＝挙動不変。
- **C5-2（専用処理）**: `process_monster_mutation(player, monster)` を新設。
  curated サブセット（BERS_RAGE=激怒→恐怖解除+加速 / COWARDICE=恐怖 /
  RTELEPORT=テレポート / SPEED_FLUX=速度変動）のみを、`set_monster_monfear/fast/slow`
  ・`teleport_away` 等**モンスター安全なプリミティブ**で適用。`BadStatusSetter`・
  UI プロンプト・脱糞等プレイヤー専用副作用や、モンスターに無意味な変異は扱わない。
  メッセージはモンスター視認時のみ。
  - **C5-2b（curated セット拡充）**: 純粋にモンスター安全なプリミティブで完結する
    能動変異 3 種を追加。**INVULN**（`!has_anti_magic()` かつ `one_in_(5000)` で
    `set_monster_invulner()` により一時無敵化。視認時メッセージ）/ **SP_TO_HP**
    （`one_in_(2000)` で傷がある場合、`get_current_mp()` を上限に `heal_hp()` で
    HP を回復し `sub_current_mp()` で MP を消費。HP 変化は `HealthBarTracker` で
    追跡対象時のみ再描画。プレイヤー版同様メッセージ無し）/ **HP_TO_SP**
    （`!has_anti_magic()` かつ `one_in_(4000)` で MP 不足があれば HP を消費して MP を
    回復）。**HP_TO_SP はプレイヤー版が `take_hit` で死亡し得るが、モンスターの自己変異は
    ドロップ・撃破クレジット等の死亡経路を避けるため現在 HP を 1 残す非致死ガードを
    付与**（`converted = min(current_hp - 1, mp_deficit)`）。いずれも死亡経路や
    プレイヤー専用副作用を伴わない純粋自己効果のため安全。
- **C5-3（発火）**: `process_world()` のプレイヤー変異処理直後に、変異持ちモンスターを
  走査して `process_monster_mutation()` を呼ぶ。`process_world` 全体が
  `TURNS_PER_TICK`(10 ゲームターン)でゲートされるため**プレイヤーと同一周期**で、
  発動確率(`one_in_(3000)` 等)がそのまま整合。大多数のモンスターは `none()` 即スキップ。

**バランス:** JSON `"mutations"` を指定した個体のみ発火。既定（未指定）は完全不変。
**工数:** 中（3 コミット）。**価値:** 中（opt-in 個体に確率的な自己効果を付与）。
フルビルド (g++ -O3 -Werror) / clang-format-18 / validate_json.py で検証済。

**将来拡張余地:** 対応変異の追加（受動変異の stat/耐性反映等）、モンスター視点での
より豊かなメッセージ、`MonsterDamageProcessor` 経由で死亡し得る自己ダメージ変異の導入
（現状の `HP_TO_SP` は非致死ガード付き）。現状は能動 7 種の curated セット
（BERS_RAGE / COWARDICE / RTELEPORT / SPEED_FLUX / INVULN / SP_TO_HP / HP_TO_SP）。

---

## 提案 C6: 魔法領域詠唱のモンスター運用（設計フェーズ）

### 実コード検証の結論

- プレイヤーの realm 詠唱実行 `exe_spell(creature, realm, spell, CAST)`
  (`spells-execution.cpp`) は realm 毎の `do_*_spell()` に分岐し、その CAST 経路は
  **`get_aim_dir(creature)` 等の UI プロンプトを直接呼ぶ**（`realm-arcane.cpp` 他多数）。
  → **`exe_spell` はモンスターからヘッドレス呼出できない。**
- モンスター詠唱 `monspell_to_player(creature, MonsterAbilityType, ...)`
  (`assign-monster-spell.h`) は `MonsterAbilityType` 駆動で、**自動ターゲティング・
  MP 消費(C4)・耐性・メッセージ・smart AI を完備**。UI 非依存。
- 既に **Blue Mage が `MonsterAbilityType` 呪文をクリーチャーとして詠唱する逆橋渡し**
  (`blue-magic-caster`) が存在し、「creature が monster ability を撃つ」経路は実績あり。

### 橋渡し方式の選択肢

| 案 | 概要 | 工数 | リスク | 備考 |
|---|---|---|---|---|
| **A: realm→ability マッピング（推奨）** | realm の呪文を対応する `MonsterAbilityType` に写像し、既存 mspell 経路で詠唱。monster は realm 由来の ability セットを得る | 中 | 低 | mspell インフラ全再利用・UI 問題なし・C4(MP) と統合済。realm 呪文≒monster ability の範囲に限定 |
| B: exe_spell のヘッドレス化 | 全 realm 呪文にターゲット引数を追加し UI を外す | 特大 | 高 | 全 realm(13) の全呪文を改修。非現実的 |
| C: curated realm-cast | 数個の realm 呪文を共有効果関数(`fire_ball`等)＋自動ターゲットで個別実装 | 中〜大 | 中 | C5 の process_monster_mutation 流。効果ロジック重複、faithful だが冗長 |

**推奨は案 A**。既存のテスト済み mspell 経路（ターゲティング・MP・耐性・AI）を
そのまま使え、UI 結合を完全に回避でき、C4 の MP 消費とも自然に統合する。realm は
「モンスターがどの ability を得るか」を決めるレンズとして機能する。

### 案 A のオプトイン設計案

- JSON `"casts_realm_spells": true`（既定 false）を立て、かつ realm が設定された
  モンスターに、realm1（必要なら realm2）由来の `MonsterAbilityType` 群を生成時に
  `ability_flags` へ付与。以降は通常の mspell が撃つ。
- realm→ability の写像表は保守的な小集合から開始（各 realm 数個）。メンテナが拡張。

### ✅ 完了（案 A: realm→ability マッピング、メンテナ選択）

**完了内容:** `MonraceDefinition::realm_abilities`（`RealmType`、既定 `NONE`）を追加。
JSON `"realm_abilities": "CHAOS"` を指定した個体は、詠唱時（`msa_type` 構築時）に
その realm 由来の `MonsterAbilityType` 群が `ability_flags` へ **OR-in** され、以降は
**既存 mspell 経路（自動ターゲット・MP消費(C4)・耐性・smart AI）がそのまま撃つ**。

- **race-level を尊重した非破壊設計:** モンスター能力は race 単位
  (`monrace.ability_flags`) で mspell に読まれるため、恒久的に monrace を書き換えず、
  **詠唱文脈の `msa_type.ability_flags` にのみ OR-in**（他所の lore/spoiler は不変）。
  JSON パース順にも非依存。
- realm→ability 写像は `mspell-attack-util.cpp` の file-local
  `add_realm_granted_abilities()` に保守的な初期セットで実装（LIFE/SORCERY/NATURE/
  CHAOS/DEATH/TRUMP/ARCANE/CRAFT/DAEMON/CRUSADE の 10 realm。MUSIC/HISSATSU/HEX は
  技術領域のため現状未マッピング）。バランス調整・拡張はこの表で行う。
- realm トークン表 `r_info_realm`（13 realm）/ reader `set_mon_realm_abilities` /
  schema を整備。
- **既定 `NONE` のため実データ・既定バランスは不変。** 実際に撃たせるには当該
  monrace に `freq_spell > 0` が要る（メンテナのデータ設定）。
- フルビルド (g++ -O3 -Werror) / clang-format-18 / validate_json.py で検証済。

**将来拡張余地:** 写像表の精緻化、realm2 の併用、realm レベル依存の能力段階化。

---

## 提案 C7: class_specific_data の限定運用 ✅ 完了（groundwork）

**完了内容:** C1 で `pclass` を付与されたモンスターのうち、モンスター運用に意味の
ある 4 クラス（**BLUE_MAGE**=学習呪文 / **SAMURAI**・**MONK**=構え / **NINJA**=潜伏）
のみ、生成時に `class_specific_data` variant を初期化する
`CreatureClass::init_monster_specific_data()` を新設し、`place_monster_one` から呼ぶ。

- 鍛冶(essence)・魔道具(チャージ)等プレイヤーのインベントリ/UI 前提のクラスは
  対象外（`no_class_specific_data` のまま）。
- プレイヤー用 `init_specific_data()`（`is_player()` ガード）には一切手を触れず、
  別メソッドとして分離（プレイヤー経路ゼロリスク）。
- **現状は groundwork**（variant を正しい型で用意するのみ）。モンスター側での
  構え・学習呪文の**実効果反映は将来の提案**で段階導入する。C1 opt-in 個体のみ
  対象のため既定バランス不変。
- フルビルド (g++ -O3 -Werror -Wall -Wextra) / clang-format-18 で検証済。

**将来拡張余地:** 初期化した variant を使ったモンスター側の実効果（侍/僧の構えに
よる戦闘補正、青魔の学習など）。読取り側の実装が必要。

---

## 提案 C8: 徳・空腹の運用（見送り）

- **徳 (`virtues`):** `initialize_virtues()` が `is_player()` ガード、意味づけ
  （道徳的選択）と UI 表示がプレイヤー物語前提。モンスター運用の価値が低く
  工数大。**見送り**（モンスター陣営システムを作る場合のみ再検討）。
- **空腹 (`food`):** モンスターの read サイト皆無、代謝シミュはローグライクで
  不要。**見送り**。

---

## C トラック実施結果

**C1 → C2 → C4 → C5 → C6 → C7** を実装完了。C3 は基盤誤認で降格、C8 は見送り。

| 提案 | 結果 | 要約 |
|---|---|---|
| C0 | 一部 | exp 保存済と判明・CLAUDE.md 訂正済。hp_table 保存のみ低優先で残 |
| C1 | ✅ | 種族・職業 JSON 付与（`player_race`/`player_class`、効果なし・第1弾） |
| C2 | ✅ | レベルアップ能力値成長（`grows_stats`、opt-in） |
| C3 | ⏭️ | ESP: 実コード検証で「AI は自 ESP を読まない」と判明、新規 AI 要で降格・見送り |
| C4 | ✅ | MP 消費詠唱（`consumes_mp`、レベル比例コスト） |
| C5 | ✅ | 突然変異運用（3段: 付与→専用処理→発火、curated 4 変異） |
| C6 | ✅ | 魔法領域詠唱（`realm_abilities`、案A realm→ability マッピング） |
| C7 | ✅ | class_specific_data 限定初期化（青魔/侍/僧/忍者、groundwork） |
| C8 | ⏭️ | 徳・空腹: UI/物語束縛で見送り |

**総括:** モンスターにプレイヤー機能を開く C トラックの主要提案（C1/C2/C4/C5/C6/C7）を
すべて **JSON オプトイン・既定バランス完全不変**の原則で実装した。C1/C6 は「効果あり
だが opt-in 個体のみ」、C2/C4/C5 は「opt-in 個体で発火」、C7 は groundwork。着手前の
実コード検証で C3（ESP）と C5（変異処理）の基盤誤認を捕捉し、C3 は降格、C5 は安全な
専用処理で実装した。**残る深化（種族耐性/職業特典の戦闘反映、熟練度成長、変異の
拡充、realm 写像の精緻化、class data の実効果）は将来提案**でメンテナのバランス判断の
もと段階導入する。

**全 C 提案に共通する進め方:** ①汎用機構をコードに実装 → ②JSON で 1 体に
オプトイン付与 → ③実機で挙動・バランス確認 → ④メンテナ判断で横展開。
A/B と違い挙動が変わるため、対象範囲と数値はメンテナ判断（本セッションで確認済）。

---

# D トラック: プレイヤー・モンスター処理の「同化」に欠かせない残要素

4 観点（残存 `PlayerType` 型シグネチャ / per-turn 処理の分岐 / 計算・効果関数の
重複 / `is_player()` ガード）を横断調査した結果を D トラックとして整理する。

## 調査の総括（4 観点横断）

- **シグネチャ移行は事実上完了。** `CreatureEntity &` への型統一は済み、`src/` 全体で
  combat/effect/status を取る自由関数の `PlayerType &`/`*` 引数は **0**
  (残るのは Godot フロントの `player_status_push` 1 個のみ)。`PlayerType::get_instance()`
  の 34 呼出も全て entry-point/UI/IO で、処理コード内に「回避策としての静的アクセス」は
  **無い**。creature-general コードから `PlayerType` への downcast も **0**。
- **`src/status/` は既に完全 creature-general。** `BadStatusSetter` / buff setter /
  base-status は全て `CreatureEntity &` を取り accessors で状態変更する。`is_player()`
  ガードは 1 つも無い。→ **モンスターに渡しても状態は正しく変わる**。
- **したがって残る"同化"の本丸は 2 つ:**
  (1) **creature-general な本体をプレイヤーに閉じている `is_player()` 早期 return ガード**
  (特に回復・状態治療関数群)、
  (2) **general コードに直書きされた 2 人称メッセージ**（`msg_print("あなたは…")`）。
  この 2 つを外す（ガード除去 + メッセージの virtual seam 化）ことが、既に general な
  資産をモンスターへ開く最小工事になる。

## D トラック提案索引

| 番号 | 提案 | 種別 | 工数 | 価値 | 状態 |
|---|---|---|---|---|---|
| D1 | セービングスロー述語 `does_save_against()` 統一 | 純粋refactor | 小 | 中 | ✅ 完了 |
| D2 | 回復・状態治療関数の is_player ガード除去 + メッセージ seam | 同化中核 | 中〜大 | 高 | ✅ 完了（回復・治療9関数＋能力値回復） |
| D3 | 統一クリーチャーテレポートプリミティブ | primitive | 中 | 中 | 計画 |
| D4 | 属性ダメージ分類器（immune/resist/vuln）の共通化 | primitive | 中 | 中 | ✅ 第1弾完了（monster側 immune/hurt 共通化） |
| D5 | 小規模統合（charm/control セーヴ統合ほか） | 純粋refactor | 小 | 低〜中 | ✅ 完了（charm/control。他2件は精査の上見送り） |
| D6 | spoiler/lore の PlayerType dummy 軽量化（※前提訂正） | 構造 | 小 | 低 | 計画（低優先・前提誤り訂正済） |
| D7 | モンスターの poison DoT（opt-in 機能） | 機能(C隣接) | 中 | 中 | ✅ 完了（poison、opt-in・既定OFF） |

---

## 提案 D1: セービングスロー述語 `does_save_against()` の統一 ✅ 完了

**現状:** `randint0(100 + power/2) < creature.get_skill_save()` の魔法防御セーヴ
イディオムが **8 ファイル・約 21 箇所**に重複。`get_skill_save()` は
`CreatureEntity` virtual のため、対象がプレイヤーでもモンスターでも同一経路で判定可能。

**完了内容:** `CreatureEntity::does_save_against(int power)`
（= `randint0(100 + power/2) < get_skill_save()`、定義は creature-entity.cpp）を新設し、
**プレーン形 17 箇所**（monster-attack-status / effect-player-curse / effect-monster-psi /
effect-monster-charm / mspell-status / mspell-floor / effect-player-resist-hurt）を移行。
`randint0` 1 回・`get_skill_save()` 1 回で乱数列・挙動は完全不変。

**残置:** `std::max<short>(5, get_skill_save())` の下限付き 4 箇所
(effect-player-spirit) と `>` 反転 while ループは別式のため現状維持。
フルビルド (g++ -O3 -Werror) と clang-format-18 で検証済。

---

## 提案 D2: 回復・状態治療関数の is_player ガード除去 + メッセージ seam（同化の中核） ✅ 第1弾完了

### ✅ 第1弾 完了内容（メッセージ seam + 回復6関数）

**メッセージ seam の導入:** `CreatureEntity::notify_self(std::string_view)` を新設。
プレイヤーなら `msg_print()`、それ以外 (モンスター) では無表示。状態異常/バフ setter
群の 2 人称メッセージ（`bad-status-setter.cpp` 26 + `buff-setter.cpp` 18 = **44 箇所**）を
`msg_print(...)` → `creature.notify_self(...)` へ機械置換。**プレイヤーでは
`notify_self ≡ msg_print` のため挙動完全不変**（現行の setter 呼出は全てプレイヤー）。

**回復6関数のガード除去:** `heroism` / `berserk` / `cure_light_wounds` /
`cure_serious_wounds` / `cure_critical_wounds` / `true_healing` の
`if (!creature.is_player()) return false;` を撤去。本体は既に creature-general
プリミティブ（`hp_player` / `BadStatusSetter::set_*` / `set_hero` 等）のみで、
メッセージは seam でプレイヤーのみ表示されるため、**モンスターにも安全に適用可能**に
なった。現状これらを monster に渡す呼出は無いため既存挙動は不変（enabling 変更）。

- フルビルド (g++ -O3 -Werror) / clang-format-18 で検証済。

### ✅ 第2弾 完了内容（能力値回復系 + life_stream）

- `restore_all_status` / `status_shuffle` / `life_stream` tail の `is_player` ガードを撤去。
- `base-status.cpp` の能力値メッセージ（`do_dec_stat` / `do_res_stat` / `do_inc_stat`
  の `msg_format` 4 箇所）を `if (creature.is_player())` でガード（可変長メッセージのため
  string_view seam ではなく inline gate、挙動はプレイヤー保存）。`status_shuffle` は
  メッセージ無しの純粋能力値入替のためガード除去のみ。`life_stream` 冒頭の
  `msg_print` は `notify_self` へ載せ替え。
- フルビルド (g++ -O3 -Werror) / clang-format-18 で検証済。**回復・状態治療系 9 関数
  ＋能力値回復がモンスターに安全に適用可能**になった（現状 monster 呼出は無く挙動不変）。

### 第3弾以降（残）

- `notify_self` を**モンスター視認時の 3 人称文**へ拡張（現状はモンスター無表示）。
- setter 群の残るプレイヤー専用テール（stance / spell 停止 / redraw）は no-op で
  モンスター無害だが、必要なら virtual 化で整理。
- これにより「モンスターの回復・状態治療」を実際に使う機能（C トラックの回復
  モンスター等）への足場が完成する。

### 参考: 当初の設計メモ

**現状:** `src/spell/spells-status.cpp` の回復・治療関数は**本体が 100% creature-general
プリミティブ**（`hp_player` / `BadStatusSetter::set_*` / `mod_cut` 等、すべて既に
`CreatureEntity &` で accessors 経由）なのに、先頭の `if (!creature.is_player()) return;`
だけがモンスター利用を阻んでいる。該当:
`true_healing`(:437) / `cure_critical_wounds`(:398) / `cure_serious_wounds`(:369) /
`cure_light_wounds`(:342) / `heroism`(:298) / `berserk`(:320) / `restore_all_status`(:527) /
`status_shuffle`(:693) / `life_stream`(:276 tail、半分は既にモンスター発火)。
ガードなしの `fear_monster`(:192) が既存のお手本。

**障壁:** ガードを外すと、general コードに直書きされた 2 人称メッセージ
（"あなたは元気になった気がする！" 等）がモンスターにも無条件で出る。

**提案:** (a) 回復・治療関数群のガードを除去、(b) メッセージを
`CreatureEntity::notify_status_message(...)` 的な **virtual seam** に載せ替え
（プレイヤー=`msg_print`、モンスター=視認時のみモンスター視点文 or 無音）。これにより
既に general な資産（回復・状態治療）が**そのままモンスターに開く**。同様の seam は
`BadStatusSetter`/buff setter の 2 人称メッセージにも展開でき、状態異常付与の
モンスター運用（C5 の専用処理より汎用的）への道が開ける。

**工数:** 中〜大（メッセージ seam の設計 + 全 setter への波及）。**価値:** 高
（"同化"の本丸）。**リスク:** 中（プレイヤーのメッセージ・挙動を完全保存する必要）。
**進め方:** まず回復 6 関数のガード除去 + 局所メッセージ gating（小さく検証）→
seam を setter 群へ横展開、と段階化する。

---

## 提案 D3: 統一クリーチャーテレポートプリミティブ

**現状:** テレポート系は `teleport_player` / `teleport_player_to` / `teleport_player_aux`
（プレイヤー）と `teleport_away`（モンスター、`CreatureEntity &, m_idx`）に分かれる。
`apply_nexus`(:644) / `teleport_level`(spells-world.cpp:60) / 突然変異の RTELEPORT 等が
プレイヤー版に依存し、モンスター運用時に別プリミティブへ切替が要る。

**提案:** 対象クリーチャーを受けて適切に移動する統一プリミティブ（内部で
is_player 分岐または位置操作の共通化）。これにより apply_nexus / teleport_level の
ガードを外せる。**工数:** 中。**価値:** 中。**リスク:** 中（移動は副作用が広い）。

---

## 提案 D4: 属性ダメージ分類器（immune/resist/vuln）の共通化

**現状:** プレイヤーの `calc_X_damage_rate()`（`player-status-resist.cpp`）は既に
`has_immune_X()` / `has_resist_X()` / `has_vuln_X()` virtual だけで倍率を算出し、
これらの virtual は**モンスターのフラグも解決する**。一方
`effect-monster-resist-hurt.cpp` は同じ判定を `monrace->resistance_flags` から
手書きで再実装（しかも倍率定数が player と異なる: 免疫 `/9`・弱点 `*2`・耐性
`*3/(d6+6)` 等）。

### ✅ 第1弾 完了（monster 側 immune/hurt 処理の共通化）

**設計判断（プレイヤー↔モンスター統合は見送り）:** 実コード検証で、`has_immune_fire()`
等の virtual は **`::has_immune_fire()`（装備由来）＋ monrace フラグの重ね合わせ**で、
monster ハンドラの直接 monrace 読取り（装備を含まない）とは**等価でない**（装備で
耐性が付く monster が現れうる ＝ 挙動変化）。加えて lore 記録 (`r_resistance_flags`)
は**monrace 固有耐性のみ**を記録する意味論のため、virtual への置換は挙動不変にできない。
→ **プレイヤー計算式との統合は「装備耐性を monster に反映する」バランス変更を伴う
別提案**とし、本提案では見送り。

**実施した安全な統合（monster 内の重複解消）:** monster 属性ハンドラの
「免疫 → ダメージ1/9＋『かなり耐性がある』＋思い出記録」「弱点 → ダメージ2倍＋
『ひどい痛手』＋思い出記録」の重複ブロックを、file-local ヘルパ
`apply_monster_element_immune()` / `apply_monster_element_hurt()` に集約。
acid/elec/fire/cold/pois/stungun の 6 ハンドラを移行（**monrace 読取り・思い出記録の
意味論は維持、挙動完全不変**）。フルビルド (g++ -O3 -Werror) / clang-format-18 検証済。

### ✅ 第2弾 完了（部分耐性ヘルパの新設＋immune/hurt 横展開）

**部分耐性ヘルパ `apply_monster_element_resist()` の新設:** 複数ハンドラに **byte 一致**で
重複していた部分耐性ブロック（`note "resists."`・ダメージ `*3/(1d6+6)`・思い出フラグ記録）を
file-local ヘルパへ集約。**byte 一致で移行できた 4 ハンドラ**（plasma / force / inertia /
time）を移行。挙動完全不変（monrace 読取り・思い出記録のセマンティクス維持）。

- **除外（byte 不一致のため見送り）:** gravity / meteor は note 文が `" resists!"`（感嘆符）で
  異なり、gravity は途中に `do_dist = 0` の追加文があるため対象外。water は immune 兄弟枝と
  思い出記録を共有する構造差、C1第3弾の二次属性（nether/chaos/shards/sound/conf/disen/
  nexus）は race 耐性 OR-in の `native_resist` ガードで思い出記録条件が異なるため対象外。
  hell_fire/holy_fire は `r_kind_flags`（`r_resistance_flags` でない）を記録するため別系統。

**immune/hurt ヘルパの横展開:** 第1弾で 6 ハンドラに入れた `apply_monster_element_immune()` /
`apply_monster_element_hurt()` を、残りの byte 一致サイトへ展開: icee_bolt の IMMUNE_COLD /
HURT_COLD、dirt の IMMUNE_POISON（計 3 サイト）。

**種族耐性 OR-in 版ヘルパ `apply_monster_element_resist_native()` の新設:** C1第3弾で
race 耐性 (opt-in) の OR-in を入れた二次属性ハンドラは、思い出記録を `native_resist`
（monrace 固有耐性を持つか）で条件化した部分耐性ブロックを持つ。この byte 一致ブロックを
専用ヘルパへ集約し、chaos（第1分岐）/ shards / disenchant / nexus の 4 サイトを移行。
sound（軽減が `*2` で異なる）・confusion（live `resistance_flags.set(NO_CONF)` を記録する
quirk）・nether（immune 兄弟枝と思い出記録を共有）は byte 不一致のため対象外。

**光/闇ハンドラの専用ヘルパ `apply_monster_lite_dark_resist()`（別ファイル）:** lite / dark は
軽減率が `*2/(1d6+6)`・メッセージが `" resists!"` で resist-hurt の helper 群とは異なるが、
両者間では byte 一致。`effect-monster-lite-dark.cpp` に file-local ヘルパを新設し lite（第1分岐）/
dark の 2 サイトを集約（native_resist 条件も維持、挙動不変）。

合計 13 サイト（immune/hurt 9 + resist 4 + resist_native 4 + lite/dark 2、※ helper 定義除く）の
重複を集約。effect-monster の属性耐性ハンドラの byte 一致重複はこれで概ね解消。フルビルド
(g++ -O3 -Werror) / clang-format-18 で検証済。

**残（第3弾以降）:** プレイヤー計算式との真の統合は上記バランス判断（装備耐性を monster に
反映する変更）が前提のため引き続き別提案。残る部分耐性ブロックは note 文・付随処理が
個別に異なるため、無理な共通化はせず現状維持。

---

## 提案 D5: 小規模統合 ✅ 完了（charm/control。他2件は見送り）

**完了: `common_saving_throw_charm` / `common_saving_throw_control` 統合**
(spells-diceroll.cpp)。~90% 同一（`NO_CONF` 早期 return の有無のみ差）だったため、
`check_no_conf` フラグを取る file-local `common_saving_throw_impl()` に共通実装を集約し、
両公開関数は薄い委譲へ。公開 API・挙動（charm の `resistance_flags.set(NO_CONF)`
という既存の細部含む）は完全保存。フルビルド (g++ -O3 -Werror) / clang-format-18 で検証済。

**精査の上で見送った 2 件:**
- **時限効果満了エンベロープ**: 「減算→満了検出→視認時メッセージ」の外枠が共通に
  見えたが、実際は **player 側は満了メッセージを setter 内 (notice フラグ) に畳み込み、
  monster 側は明示的に出す**構造差があり、共通エンベロープ抽出には一方の再構成が要る。
  per-turn ホットパスへの侵襲・薄い共有ゆえリスク＞価値で現状維持。
- **モンスター対モンスター属性オーラの `fire_dam/cold_dam/elec_dam` 再利用**: これらは
  内部で `take_hit()`（プレイヤー死亡経路）を呼ぶ**プレイヤー専用**関数で、モンスター
  被害者には使えないと判明。再利用不成立で見送り。

**工数:** 小（実施分）。**価値:** 低〜中。

---

## 提案 D6: spoiler/lore の PlayerType dummy 軽量化（前提訂正済・低優先）

**⚠️ 当初前提の訂正（実コード検証）:** 調査時「`CreatureEntity` はインスタンス化
不可」としたが**誤り**。`CreatureEntity` に純粋仮想は無く（`= 0` は全てメンバ既定値）、
モンスターは `std::vector<CreatureEntity> m_list` として**直接インスタンス化されている**
（＝ concrete class）。よって「インスタンス化可能化」という課題自体が存在しない。

**残る小課題:** spoiler/lore が便宜上 `PlayerType dummy;`（重量オブジェクト）を作る
2 箇所 (`display-lore.cpp:122` / `items-spoiler.cpp:167`)。原理的には軽量な
`CreatureEntity` で置換可能だが、callee がアクセスするフィールドを満たす必要があり、
価値は低い（wizard/spoiler の非ゲーム経路）。**低優先。**

**別途の小改善（→ 提案 E2 で完了）:** `PlayerType::should_skip_natural_regen` /
`apply_state_regen_modifier`（Samurai/Monk 構えの再生補正）の `const_cast<PlayerType&>`
排除は E2 で `CreatureClass` const ビューコンストラクタ導入により実施済み。

---

## 提案 D7: モンスターの cut/poison DoT（per-turn ギャップ）

### ✅ 完了（poison DoT、opt-in・既定OFF）

**着手前検証:** cut/poison の毎ターン処理はプレイヤー専用で、モンスターに
**positive な cut/poison を与える経路が存在しない**（monster-processor は 0 セットのみ、
BadStatusSetter は実質プレイヤー専用）。よって tick だけ足しても dead code。
→ **重複解消ではなく「inflict＋tick」の小機能**として、C トラック方針の
**JSON オプトイン・既定OFF**で実装。

**完了内容（poison に限定）:**
- `MonraceDefinition::suffers_poison_dot`（bool、既定 false）+ reader + schema。
- **inflict:** `effect_monster_pois`（D4 で整理済）で、免疫でなく `suffers_poison_dot`
  が立つ個体は毒攻撃で `POISON` を `max(1, dam/2)` 蓄積（`MAX_SHORT` クランプ）。
- **tick:** `process_world()`（10 ゲームターン周期、プレイヤー毒 DoT と同一）で
  `POISON>0` のモンスターに 1/ターンの毒ダメージを `MonsterDamageProcessor`
  (`AttributeType::POIS`) で与え、`POISON` を 1 減らす。無敵中はスキップ。死亡時は
  ドロップ・経験値含め正規経路で処理。
- **既定 OFF のため誰にも POISON が蓄積せず、既定バランス完全不変。** 蓄積量
  (`dam/2`) と tick 量 (1) で調整可能。cut DoT は近接由来で inflict 経路が別のため
  今回は poison のみ（将来同様に opt-in 拡張可）。
- フルビルド (g++ -O3 -Werror) / clang-format-18 / validate_json.py で検証済。

---

## D トラック着手順の推奨

**D1（完了）→ D5（小整理）→ D4（分類器）→ D2（同化中核）→ D3（テレポート）→
D6（構造）→ D7（機能）**。純粋 refactor で安全な D1/D5 を先に、"同化"の本丸で
価値の高い D2 は seam 設計後に段階着手、機能寄りの D7 は C トラック方針
（opt-in・バランス確認）で扱う。

---

## E トラック（API 衛生・重複解消・シリアライズ集約）調査結論

### 位置づけ

A〜D トラックで **フィールドのカプセル化（141 private 化）・状態チェックの
virtual 化・処理の同化・機能付与** がほぼ出揃った。E トラックはその仕上げとして、
**リファクタリング過程で残った重複コード・const 不整合・シリアライズの二重記述**を
掃除する「衛生」トラックである。3 観点（シリアライズ共通ブロック / PlayerType
メンバ hoist / const・アクセサ衛生）を実コード検証した結論を以下に記す。

**重要な調査所見（過大評価の否定）:**

- **メンバ hoist は実質完了**。`PlayerType` は既に **11 メソッド override の薄い殻**で、
  再生・ダメージフックは全て CreatureEntity virtual に移行済み。「大規模 hoist」は残って
  いない。残るのは個別の小整理のみ。
- **const 化は割に合わない**。41 個の非 const `has_resist_*()` を const 化すると
  `player-status-flags.cpp` の約 40 関数 + `CreatureRace`/`CreatureClass` ラッパへ
  カスケードし、692 TU に fan-out するヘッダで churn が甚大。**見送り。**
- **public スカラの private 化残**（hp/maxhp/ac/stat 配列等）は io-dump 等が raw 値を
  読む意図的 public であり、閉じるのは pure churn。**見送り。**

### E トラック提案索引

| 番号 | 提案 | 種別 | 工数 | 価値 | 状態 |
|---|---|---|---|---|---|
| E1 | `regenhp()` の停止判定重複解消（`should_skip_natural_regen()` 利用） | 重複解消+同化 | 小 | 高 | ✅ 完了 |
| E2 | `const_cast<PlayerType&>` 排除（CreatureClass const ビューコンストラクタ） | const 衛生 | 小 | 中 | ✅ 完了 |
| E3 | シリアライズ共通ブロック拡張（`prace`/`pclass`） | 重複解消 | 中 | 中 | ✅ 完了（version 54 bump） |
| E4 | インライン accessor 本体（481 個）の .cpp 移設 | ビルド衛生 | 大（機械的） | 中 | ✅ 完了（3 batch・ヘッダ約32%削減） |
| E5 | `is_player()` 自由関数分岐の virtual 化 | 同化 | 大 | 中 | ✅ 第1弾完了（`get_title()`。残候補は少数・据え置き） |
| E6 | 一時ステータス setter 末尾の共通後処理集約（`notice_bonus_status_change()`） | 重複解消 | 小 | 中 | ✅ 完了（18 サイト・byte 一致） |
| E7 | `MonsterSpellResult` の learnable 生成定型集約（`make_learnable()`） | 重複解消 | 小 | 中 | ✅ 完了（36 サイト・byte 一致） |
| E8 | コマンド反復回数セット定型集約（`set_command_repeat_from_arg()`） | 重複解消 | 小 | 中 | ✅ 完了（9 サイト・byte 一致） |
| E9 | 進路上モンスター打撃定型集約（`attack_monster_in_the_way()`） | 重複解消 | 小 | 低〜中 | ✅ 完了（5 サイト・byte 一致） |
| E10 | アイテム関連サブウィンドウ再描画定型集約（`set_item_related_sub_window_flags()`） | 重複解消 | 小 | 低〜中 | ✅ 完了（7 サイト・byte 一致） |
| E11 | alliance の `isAnnihilated` 全滅判定を共通ヘルパへ集約（`all_monraces_extinct()`） | 重複解消 | 中 | 中 | ✅ 完了（32 サブクラス・同型ロジック） |
| E12 | 部屋の床敷き詰め二重ループを集約（`fill_room_floor()`） | 重複解消 | 小 | 低〜中 | ✅ 完了（rooms-normal の `fill_room_floor(` マッチ 7 件のうち、`src/room/rooms-normal.cpp:24` は関数定義のため除外。呼び出しは 6 サイト） |

---

## 提案 E8/E9: cmd-action の定型重複解消 ✅ 完了

`cmd-action/` を掘り下げ、byte 一致の同型重複を 2 件集約（挙動不変）。

- **E8 `set_command_repeat_from_arg()`**（`io/input-key-requester.{h,cpp}`）: 各コマンド
  ハンドラ冒頭の「数値プレフィックス → 反復回数セット → ACTION 再描画 → arg クリア」
  定型（`command_arg`/`command_rep` グローバルと同 TU に配置）。9 サイト移行
  （open-close 4 / move 2 / others 2 / tunnel 1）。
- **E9 `attack_monster_in_the_way()`**（`cmd-action/cmd-attack.{h,cpp}`）: 開閉/破壊/
  トンネル等で進路にモンスターがいる場合の「ターンエネルギー消費 → メッセージ → 打撃」
  定型。5 サイト移行（open-close 4 / tunnel 1）。**除外:** disarm 分岐はエネルギー消費
  行を持たず定型不一致のため据え置き（既存挙動保存）。

**掘り下げで見送った候補（記録）:**
- **cmd-attack の恐怖リアクション block（berserk/flee メッセージ、3〜4 サイト）**:
  ガードの `monster` 参照の由来・`grid.m_idx` vs `m_idx`・berserk 分岐の `fear = false`
  有無が関数ごとに微妙に異なり、安全な機械抽出には各関数のインデックス由来検証が必要。
  誤抽出の挙動変化リスクを避け、人手レビュー向け候補として据え置き。
- **spell/mane/pet の呪文選択メニュー loop（`while (!flag)` / `input_command`）**:
  各コマンドで選択肢・確定処理が異なる大きな UI ループで機械抽出困難。

---

## 提案 E7: `MonsterSpellResult::make_learnable()` による learnable 生成の集約 ✅ 完了

**現状（実コード検証済）:** モンスター呪文ハンドラが末尾で
`auto res = MonsterSpellResult::make_valid([dam]); res.learnable = 〈式〉; return res;`
という 3〜4 行の定型を多数重複していた（learnable 式は `target_type == MONSTER_TO_PLAYER`
または `proj_res.affected_player`）。

**実施:** `MonsterSpellResult` に静的ファクトリ
`make_learnable(bool learnable, int dam = 0)` を追加し、上記定型を 1 行
`return MonsterSpellResult::make_learnable(〈式〉[, dam]);` へ置換。**byte 一致の 36 サイト**
（`mspell-summon.cpp` 30 / `mspell-status.cpp` 3 / `mspell-attack/abstract-mspell.cpp` 1 /
`mspell-attack/mspell-breath.cpp` 1 / `mspell-attack/mspell-curse.cpp` 1）を移行。

**対象外:** `res` を生成後にさらに加工・条件分岐してから返す関数（`mspell-dispel.cpp` /
`mspell-floor.cpp` / `assign-monster-spell.cpp` 等、`res.learnable = false` を後段で
条件的に上書きする箇所）は末尾定型でないため据え置き。

挙動完全不変（同値のファクトリ抽出）。フルビルド (g++ -O3 -Werror) / clang-format-18 で検証済。

---

## 提案 E6: 一時ステータス setter 末尾の共通後処理集約 ✅ 完了

**現状（実コード検証済）:** 多数の一時ステータス setter（`set_tim_*` / `set_*` 系）が
末尾に **byte 一致**の共通後処理を重複して持っていた:

```cpp
auto &rfu = RedrawingFlagsUpdater::get_instance();
rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
if (!notice) {
    return false;
}
if (disturb_state || Travel::get_instance().is_ongoing()) {
    disturb(creature, false, true);
}
rfu.set_flag(StatusRecalculatingFlag::BONUS);
handle_stuff(creature);
return true;
```

**実施:** 新規 file-local でない共通関数
`notice_bonus_status_change(CreatureEntity &, bool notice)`
（`src/status/status-change-notice.{h,cpp}`）へ集約。**byte 一致の 18 サイト**
（`status/temporary-resistance.cpp` 6 / `status/body-improvement.cpp` 4 /
`status/buff-setter.cpp` 3 / `mind/mind-mirror-master.cpp` 2 /
`mind/mind-magic-resistance.cpp` 1 / `racial/racial-kutar.cpp` 1 /
`spell-realm/spells-song.cpp` 1）を `return notice_bonus_status_change(creature, notice);`
へ置換。各末尾で **約 13 行 → 1 行**（計 200 行超の重複を解消）。

**対象外（byte 不一致のため据え置き）:** `MainWindowRedrawingFlag::BASIC` を使う
`shape-changer.cpp`、`StatusRecalculatingFlag::MONSTER_STATUSES` を `set_flags` で
複数指定する `sight-setter.cpp`、および `rfu` を末尾以前で宣言・別フラグ設定する
一部関数（recalc フラグや再描画対象が異なるため共通化すると挙動が変わる）。

Makefile.am / VisualStudio プロジェクト（`.vcxproj` / `.filters`）に新ファイルを登録。
挙動完全不変（純粋な suffix 抽出）。フルビルド (g++ -O3 -Werror) / clang-format-18 で検証済。

---

## 提案 E1: `regenhp()` の停止判定重複解消（`should_skip_natural_regen()` 利用）

**現状（実コード検証済）:** `regenhp()`（`src/hpmp/hp-mp-regenerator.cpp:121-126`）が
自然回復完全停止判定をインラインで重複実装している:

```cpp
if (CreatureClass(creature).samurai_stance_is(SamuraiStanceType::KOUKIJIN)) {
    return;
}
if (creature.get_action() == ACTION_HAYAGAKE) {
    return;
}
```

これは `PlayerType::should_skip_natural_regen()`（同 75-82、CreatureEntity virtual・
基底既定 false）と**完全に同一のロジック**である。さらに `regenhp()` は
`regenerate_monsters()`（同 288）から**モンスターにも呼ばれる**ため、この
`CreatureClass(monster).samurai_stance_is(...)`（プレイヤー固有の構え判定）が
モンスターに対して走っている（副作用は無いが意味的に不正な重複）。

**修正案:** インライン 2 分岐を `if (creature.should_skip_natural_regen()) return;`
1 行に置換。

- **プレイヤー経路**（`hp-mp-processor.cpp:435`）: PlayerType override が同一判定を
  行うため**挙動完全不変**。
- **モンスター経路**（`regenerate_monsters:288`）: 基底が false を返すためプレイヤー
  固有の構え判定が走らなくなる。そもそも `compute_regen_amount()`（同 42）が先に
  `should_skip_natural_regen()` を適用済みで、停止時は `regen_amount == 0` となり
  `regenerate_monsters:282` の `continue` で `regenhp` に到達しない。**挙動完全不変。**

→ **重複解消 + 意味的正しさ改善**。call site の乱数消費・HP 計算は不変。工数小・価値高。
実装候補筆頭。

---

## 提案 E2: `const_cast<PlayerType&>` の排除（CreatureClass const ビューコンストラクタ） ✅ 完了

**現状（着手前）:** `PlayerType::should_skip_natural_regen()` と
`apply_state_regen_modifier()` が const メソッド内で
`CreatureClass pc(const_cast<PlayerType &>(*this));` を使い構え
（`samurai_stance_is` / `monk_stance_is`）を問い合わせていた。`CreatureClass`
コンストラクタが非 const 参照を要求するための回避策。

**完了内容:** `CreatureClass` に読み取り専用ビュー用のコンストラクタ
`CreatureClass(const CreatureEntity &)` を追加。ゲーム状態としての `CreatureEntity`
は実体が const になることが無い（const はアクセサ修飾子の伝播に過ぎない）ため、
`const_cast` を **CreatureClass 内部の 1 箇所に局所化**。呼び出し側
（`should_skip_natural_regen()` / `apply_state_regen_modifier()`）は
`CreatureClass pc(*this);` に簡素化され、アプリケーションロジックから `const_cast` を
排除。read/mutate 混在の CreatureClass API を分割する大規模改修は避け、最小・
無カスケードで const 正当性を局所改善。フルビルド（g++ -O3 -Werror）で検証済。

**注:** 全面的な const 化（41 個の `has_resist_*()` 等）は E トラック冒頭の所見通り
692 TU への churn が甚大なため引き続き見送り。本提案は const_cast の局所排除に限定。

---

## 提案 E3: シリアライズ共通ブロック拡張（`prace`/`pclass`） ✅ 完了

**着手前の前提訂正（実コード検証）:** 当初調査は `prace`/`pclass`/`r_idx`/`ap_r_idx`
の 4 フィールドを「両 writer で二重記述」としたが**誤り**。実際に二重記述されて
いたのは `prace`/`pclass` のみ（プレイヤー writer が `byte`、モンスター writer が
`s16b`）。`r_idx`/`ap_r_idx` は **モンスター writer にしか無い**（プレイヤーは
monrace 同一性を持たないため）＝重複ではなくモンスター固有。よって共通ブロックへ
移すのは `prace`/`pclass` のみとし、`r_idx`/`ap_r_idx` はモンスター固有経路に残置。

**完了内容:**
- **savefile version 54 へ bump**（`angband-version.h`、履歴コメント追記）。
- `wr_creature_common()` 末尾に `prace`/`pclass` を **`s16b`（NONE=-1 対応）** で追加。
  `rd_creature_common()` に `older_than(54)` ガード付き読込を追加（v53 以前は各固有
  経路で読むため読まない）。→ **この 1 箇所の対称拡張で全経路（プレイヤー・
  モンスター）に反映**（確立済みパターン）。
- **プレイヤー経路:** `player-writer.cpp` の `wr_byte` 2 行を削除、`player-info-loader.cpp`
  の `rd_byte` 2 行を `older_than(54)` ガードで残置（v53 以前のみ読む）。
- **モンスター経路:** `monster-writer.cpp` の `wr_s16b` 2 行を削除、
  `monster-loader-savefile50.cpp` (`rd_monster_v50`) の `rd_s16b` 読込を
  `older_than(54)` ガードで残置。`race`/`pclass_ref` ポインタ復元は
  **バージョン非依存で常に実行**（v54 は共通ブロック値、v53 以前は s16b 読込値を使用）。
- **C1 との整合:** `prace`/`pclass` は C1 でモンスターにも付与可能になった真に共通の
  基底フィールドであり、CLAUDE.md の「モンスターにも持たせたいフィールドが出てきた
  場合に個別に `wr_creature_common()` へ移行する」方針に合致。
- フルビルド（g++ -O3 -Werror）で全経路の対称性を検証済。CLAUDE.md のシリアライズ
  節も version 54 に更新。

**対象外:** エフェクト構造体の残差分は本質的発散のため対象外（据え置き）。

---

## 提案 E4: インライン accessor 本体（481 個）の .cpp 移設 ✅ 完了

**着手前:** `creature-entity.h`（約 4,800 行、161 KB）にインライン定義された virtual
accessor が約 480 個。**692 TU がこのヘッダに fan-out** するため、ヘッダ肥大が
コンパイル時間のボトルネックになり得る。

**完了内容:**
- inline 定義された virtual accessor の本体 **481 個**を `creature-entity.cpp` へ機械的
  移設し、ヘッダは宣言のみへ縮小。**ヘッダは 4,811 → 3,288 行（約 32% 削減）**。
- **include-safe by construction:** `creature-entity.cpp` は `creature-entity.h` を
  include するためヘッダの include 閉包を全て継承する。したがってヘッダで inline
  コンパイルできていた本体は .cpp でも必ずコンパイル可能で、**include 追加は一切不要**
  だった（実際に g++ -O3 -Werror で追加include ゼロで通過）。
- virtual は vtable 経由呼出のため inline 展開の実利益が乏しく、out-of-line 化の
  ランタイムコストは**無視できる見込み**（ヘッダに定義が見えるケースのデビルト化・
  インライン化は理論上影響し得るが、ベンチマーク未取得のため「ゼロ」とは断定しない）。
  純粋な**機能変化なしの移設**。
- **3 batch に分割**（ヘッダ行番号のシフトを避けるため下位行から上位行の順で処理:
  batch1=251 / batch2=187 / batch3=43 メソッド）、各 batch でフルビルド検証・コミット。
- 対象は `virtual` メソッドのみ。非 virtual インラインヘルパ（`is_named()` /
  `calc_min_max_hp()` 等）は **inline 展開の利益があり得るため据え置き**（virtual と
  異なり out-of-line 化で最適化機会を失う）。`on_death()` の空ボディ hook 1 個も
  アクセサではないため据え置き。

**留意:** 大規模な機械的移動のため、上流（変愚）マージ作業と時期が重なると衝突し
やすい。移設スクリプトは `virtual RET NAME(PARAMS) [const]` + 次行 `{` の
clang-format 標準パターンのみを対象とした balanced-brace 抽出で、宣言/pure/template/
デストラクタ/演算子/属性前置行を除外している。

---

## 提案 E5: `is_player()` 自由関数分岐の virtual 化 ✅ 第1弾完了

**全サイト調査結論（75 個の `CreatureEntity::is_player()` 実サイトを分類）:**
生 grep 約 93 件のうち `EffectMonster::is_player()` / `EffectPlayerType::is_player()`
（別ラッパクラス）・ヘッダ宣言・コメントを除いた **75 個が実サイト**。内訳:

- **バケット A（virtual 化可能）: 4 個のみ。** `if (is_player()) {...} else {...}` の
  両枝が同じ論理値を型別に算出する真の対称分岐。
- **バケット B（型契約ガード）: 69 個。** `if (!is_player()) return;` 等の早期 return /
  プレイヤー専用副作用ガード。提案 35 の結論通り、これらは型契約であり削減対象外。
- **バケット C（本質的発散）: 2 個。** 描画サブシステム dispatch 等、小 virtual に
  畳めない大規模型別処理。

→ **E5 は提案 35 時点で既にほぼ枯渇しており、機械的 virtual 化余地は 4 サイトのみ。**

**第1弾 完了内容（A2: `get_title()`）:**
- `print_title()`（`main-window-left-frame.cpp`）の `is_player()` 4 分岐（None /
  wizard / winner / 職業別称号）を `CreatureEntity::get_title()` virtual に集約。
  基底（モンスター）は "なし"、`PlayerType::get_title()` override が
  wizard / winner / 職業・レベル別称号を返す。呼出側は
  `print_field(creature.get_title(), ...)` に簡素化。
- 称号は「クリーチャーの表示属性」でありモンスターに既定値を持たせる形は
  CLAUDE.md のプレイヤー属性共通化方針（`get_pclass()` 等と同じ）に合致。
- フルビルド（g++ -O3 -Werror）で検証済。

**据え置いた A 候補（3 個、理由付き）:**
- **A1 `cmd-draw.cpp:174`（ステータス画面プロンプト文字列）:** UI キーヒント文字列の
  選択。UI 層の局所三項が適切で、`CreatureEntity` に UI 文字列を持たせる価値は低い。
- **A3 `main-window-left-frame.cpp:83`（exp 表示値）:** プレイヤー枝に android /
  `exp_need` の残余分岐があり、単純 getter に畳めない。
- **A4 `melee-util.cpp:18`（近接イベント知覚フィールド）:** 複数フィールド（see_m /
  see_t / do_silly_attack）を設定するため単一 virtual に畳めない。

**結論:** E5 の機械的 virtual 化は `get_title()` で実質完了。残る 69 ガードは型契約、
2 サイトは本質的発散、3 A 候補は上記理由で据え置き。今後の「モンスターにも
振る舞いを持たせる」拡張はガード除去＋モンスター実装（C/D トラックの機能作業）で
個別に扱う方針。

---

## E トラック着手順の推奨

**E1（重複解消・安全・高価値）→ E2（同一ファイル const 衛生）→ E3（シリアライズ
集約・version 54）→ E4（ヘッダ軽量化・小刻み）→ E5（同化継続・サイト別）**。
E1/E2 は同一ファイルで安全なため即着手可。E3 は version bump を伴うが確立済み
パターン。E4/E5 は上流マージ競合に配慮しタイミングを見て進める。

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
