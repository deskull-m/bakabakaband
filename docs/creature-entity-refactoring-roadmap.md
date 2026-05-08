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
- 戦略的脱出 (SCROLL_DESTRUCTION/GENOCIDE)
- POTION_NEO_TSUYOSHI 等の特殊効果対応
- 装備耐久 (`apply_artifact` に近い仕組みでモンスター装備も損耗)

---

## 推奨実施順序

### 完了済み (大規模フェーズ)

- ✅ **提案 7-10**: virtual アクセサ整備 (ml/回復計算/MonsterProfile/配列)
- ✅ **提案 11**: モンスター inventory[] 統一 (フェーズ A、5 サブフェーズ)
- ✅ **提案 12**: モンスター装備有効化 (フェーズ B、AC/耐性/攻撃/二刀流)
- ✅ **提案 13**: モンスター AI アイテム使用 (フェーズ C、ポーション/巻物/杖/ロッド + UI)

### 既存提案の残作業 (中規模)

1. **提案 1** - プレイヤー専用フィールドのクリーチャー共通化（初期値・アクセサ整備）
2. **提案 2** - プレイヤー専用 virtual メソッドの共通化（提案 1 と並行可）
3. **提案 5** - TimedEffects 二重管理解消
4. **提案 4** - 残存状態チェック関数の仮想化
5. **提案 3** - `PlayerType::get_instance()` 削減 ✅ 完了
6. **提案 6** - フィールド命名統一（最後）

### 提案 13 の延長検討対象

- ペット AI で攻撃 wand/rod を hostile monster に向けて発動
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
