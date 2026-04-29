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
- 🚧 残り: `ppersonality` / `psex` 用の NONE 値追加、virtual アクセサ整備

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
- 🚧 残り: 自由関数本体の削除 or virtual 実装への完全移植。モンスター
  種族フラグ由来の耐性を返すよう MonsterProfile 経由での override
  実装は後続。既存フィールド値アクセサ (例: `has_free_act()` bool) と
  自由関数 virtual 化 (例: `has_free_act()` BIT_FLAGS) の名称衝突を
  整理する必要あり。

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
- 🚧 残り: 残効果（HERO/BLESSED/INVULN 等の map 経由効果）も
  `TimedEffects` オブジェクトに移すか、`TimedEffects` を縮退させて
  全て map 経由にするかの方針決定・実装

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

## 推奨実施順序

1. **提案 1** - プレイヤー専用フィールドのクリーチャー共通化（初期値・アクセサ整備）
2. **提案 2** - プレイヤー専用 virtual メソッドの共通化（提案 1 と並行可）
3. **提案 5** - TimedEffects 二重管理解消
4. **提案 4** - 残存状態チェック関数の仮想化
5. **提案 3** - `PlayerType::get_instance()` 削減
6. **提案 6** - フィールド命名統一（最後）

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
