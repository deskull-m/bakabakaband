# CreatureEntity 統合リファクタリング 残タスク・ロードマップ

本書は `CLAUDE.md` の「CreatureEntity 統合リファクタリング」節の続編として、
Phase 1-8 完了後に残存している統合作業項目を整理したもの。
新規の統合作業を行う際の指針として参照すること。

作業着手時は該当提案の Issue を立てるか、既存の変愚マージ ISSUE と
区別可能なタイトル（例: `refactor: TimedEffects 一本化`）で PR を作成する。

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

## 提案 1: プレイヤー専用フィールドのクリーチャー共通化 🚧 一部完了

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
| 熟練度 | `spell_exp[]`, `weapon_exp[][]`, `skill_exp[]` | 経験を積むモンスター・成長するボス |
| 魔法領域 | `realm1`, `realm2`, `element_realm` | 魔法使い系モンスターのスペル選択根拠 |
| 突然変異 | `muta`, `trait`, `patron` | 変異個体・カオスパトロン配下 |
| キャラクタ履歴 | `old_race1/2`, `old_realm`, `history[4][60]`, `player_hp[PY_MAX_LEVEL]` | モンスターのレベル成長履歴 |
| ESP/特殊能力 BIT_FLAGS | `telepathy`, `esp_*`, `cursed`, `special_defense`, `special_attack`, `dec_mana`, `easy_spell` 等 | モンスターの ESP / 呪い装備 / 特殊攻撃防御 |
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

### 進捗

- ✅ モンスター生成時に `prace` / `pclass` / `realm1` / `realm2` /
  `element_realm` を NONE に明示初期化（`init_monster_profile()`）
- ✅ `ppersonality` の NONE 値 (`PERSONALITY_NONE`) を `init_monster_profile()`
  で初期化
- ✅ `player_sex` enum に `SEX_NONE = 4` を追加 (sex_info に「未設定」エントリ
  も追加)。`init_monster_profile()` で `psex = SEX_NONE` に初期化し、
  `get_sex_info()` は範囲外の値を SEX_NONE にフォールバック
- ✅ virtual アクセサ `get_psex()` / `get_ppersonality()` を `CreatureEntity`
  に追加 (将来モンスター固有のオーバーライド余地を確保)

---

## 提案 2: プレイヤー専用仮想メソッドのクリーチャー共通化 ✅ 主要部分完了

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
- 🚧 残り: 配列系フィールド（`spell_exp[]` / `weapon_exp[][]` /
  `skill_exp[]` / `stat_cur[]` / `stat_max[]` 等）は virtual 化が
  構文的に困難で保留。書込み系セッターの整備は setter 間接コスト
  vs 効果のバランスで現状維持の判断

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

## 提案 4: 未統合の状態チェック関数の仮想化 ✅ 主要部分完了

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

提案 1・2 と同様、モンスター側でも意味ある応答を返す主体として
扱う。耐性・免疫・光源判定等はモンスター種族フラグ (`MonraceDefinition`
側の flags) から導出できるケースが多いので、`CreatureEntity` 共通
実装で `MonsterProfile` 経由の参照と `PlayerType` 経由の参照を
統合する。

### 進捗

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
- 🚧 残り: 自由関数本体 (player-status-flags.cpp) の virtual メソッド
  実装への完全移植。現状は virtual → 自由関数へ委譲する形だが、
  構造的には重複。`common_cause_flags` など内部ヘルパの整理も
  必要。工数大で効果は限定的のため優先度低。

---

## 提案 5: TimedEffects オブジェクトとの二重管理解消 🚧 一部完了

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
  - 提案 1 の「プレイヤー専用フィールドをモンスターにも開放する」方針とも整合

### 推奨: (b)

本書の設計方針に合わせ、モンスターにも `TimedEffects` オブジェクトを
持たせて運用する方向で統一する。メッセージ処理・ティック処理の
プレイヤー依存部分は virtual メソッド経由で差し替え可能にしておく。

### 進捗

- ✅ 全クリーチャーが `timed_effects` shared_ptr を基底クラス
  コンストラクタで確保する形に統一（`CreatureEntity()`）
- ✅ 基底クラス `CreatureEntity::get_timed_effect` / `set_timed_effect` に
  TimedEffects オブジェクト優先の分岐ロジックを集約。`PlayerType` 側の
  オーバーライドを廃止し、プレイヤー・モンスターで API 経路を統一
- ✅ `CreatureTimedEffect` enum に `HALLUCINATION` / `CUT` / `POISON` /
  `PROTECTION` を追加し、これまで TimedEffects オブジェクト経由でのみ
  アクセスできた効果も `get/set_timed_effect()` 統一 API からアクセス可能に
  (TimedEffects オブジェクトが持つ 11 効果すべてが enum 統合済み)
- ✅ `is_cut()` / `is_poisoned()` を virtual メソッド化 (既存
  `is_blind()` / `is_paralyzed()` 等と同等のデフォルト実装)
- ✅ `bad-status-setter.cpp` / `buff-setter.cpp` / `cmd-inn.cpp`
  の単純な `effects()->X().current()` / `reset()` / `is_X()` 呼出を
  統一 API 経由に置換 (10 箇所超)
- 🚧 残り: 残効果（HERO/BLESSED/INVULN 等の map 経由効果）も
  `TimedEffects` オブジェクトに移すか、`TimedEffects` を縮退させて
  全て map 経由にするかの方針決定・実装。PlayerStun::get_rank() /
  PlayerCut::get_accumulation() 等の高機能 API を呼ぶ箇所は
  プレイヤー固有機能として残存 (17 箇所)。完全統合にはこれらを
  static ユーティリティに分離してから map 値と組み合わせる再設計が
  必要で、工数大。現状の「API 経路統一 + 個別高機能 API 残置」で
  機能上問題ないため、優先度低。

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

## 提案 10: 配列フィールド virtual インデックスアクセサ ✅ 一部完了

### 背景

`spell_exp[64]` (固定配列) / `weapon_exp` / `weapon_exp_max` /
`skill_exp` (`std::map`) はプレイヤーの熟練度システム用フィールドだが
`CreatureEntity` 直下に存在するため、モンスターからもアクセス可能な
構造になっている。直アクセスのみで virtual がなく、将来モンスターに
熟練度概念を導入する際の拡張点が無かった。

### 作業内容

`CreatureEntity` に 3 つの read-only virtual インデックサを追加:

| 仮想関数 | 役割 | デフォルト動作 |
|---|---|---|
| `get_spell_exp(int idx)` | 呪文熟練度 | `spell_exp[idx]` を返す |
| `get_skill_exp(PlayerSkillKindType)` | スキル熟練度 | map に存在すれば値、無ければ 0 |
| `get_weapon_exp(ItemKindType, int sval)` | 武器熟練度 | map に存在すれば値、無ければ 0 |

### 移行結果

- 読取り 31 箇所 (spell_exp 4, skill_exp 22, weapon_exp 5) を新 virtual 経由に置換
- 書込み・参照取得 (`auto &exp = ...`)・複合代入 16 箇所は従来通り
  直接配列アクセス形式で残置 (mutation/reference 取得には virtual が
  対応できないため)

### 効果

- 読取りパスがプレイヤー・モンスター共通の API を通る
- 将来モンスターに `get_X_exp()` を override させて種族別/個体別の
  熟練度ロジックを導入可能

### 残作業

- 書込み setter virtual 整備は呼び出しパターン (集計加算/上限クランプ/
  `auto &` 経由 in-place 変更) が多様で抽象コストが大きく、優先度低
  として保留
- 配列全体走査 (`for (auto &exp : creature.weapon_exp[tval])`) は
  span 経由の virtual 化も検討余地あり

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

## 推奨実施順序

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

### 既存提案の残作業 (中規模)

1. **提案 1** - プレイヤー専用フィールドのクリーチャー共通化（初期値・アクセサ整備）
2. **提案 2** - プレイヤー専用 virtual メソッドの共通化（提案 1 と並行可）
3. **提案 5** - TimedEffects 二重管理解消
4. **提案 4** - 残存状態チェック関数の仮想化
5. **提案 3** - `PlayerType::get_instance()` 削減 ✅ 完了
6. **提案 6** - フィールド命名統一（最後）

### 今後の候補 (追加提案)

- **提案 31**: その他の plain field (`au` / `csp` / `food` / `level`
  / `town_num` / `age` / `ht` / `wt` / `prestige` / `to_h` /
  `to_d` / `to_a` / `stat_*` 等) の read 側アクセサ化
- **提案 32**: 提案 31 完了後に同フィールドを実際に private 化
  (提案 29 と同じパターン)
- **提案 25b**: 性能影響が出た場合の `get_inven_cnt()` キャッシュ層再導入
  → **検証済 (不要)**
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
