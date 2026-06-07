extends Control

## タイトル画面スクリプト
## 新規ゲーム / セーブデータから始める を選択してゲーム画面に遷移する

# セーブファイル一覧 (scan_save_files の戻り値)
var _save_entries: Array = []

# 種族名テーブル (PlayerRaceType の順番に対応)
const RACE_NAMES := [
	"人間", "ハーフエルフ", "エルフ", "ホビット", "ノーム", "ドワーフ",
	"ハーフオーク", "ハーフトロル", "アンバライト", "ハイエルフ", "バーバリアン",
	"ハーフオーガ", "ハーフジャイアント", "ハーフタイタン", "サイクロプス", "イーク",
	"クラッコン", "コボルド", "ニーベルング", "ダークエルフ", "ドラコニアン",
	"マインドフレイアー", "インプ", "ゴーレム", "骸骨", "ゾンビ",
	"ヴァンパイア", "スペクター", "スプライト", "ビーストマン", "エント",
	"アーコン", "バルログ", "ドゥナダン", "妖精", "クター",
	"アンドロイド", "マーフォーク",
]

# 職業名テーブル (PlayerClassType の順番に対応)
const CLASS_NAMES := [
	"戦士", "メイジ", "プリースト", "盗賊", "レンジャー", "パラディン",
	"戦闘魔術師", "カオス戦士", "修行僧", "精神術士", "高魔術師", "観光客",
	"ものまね師", "魔獣使い", "魔法使い", "射撃の達人", "魔力食い", "吟遊詩人",
	"赤魔術師", "侍", "気功師", "青魔術師", "騎兵", "狂戦士",
	"鍛冶師", "鏡使い", "忍者", "スナイパー", "元素使い",
]

# 性格名テーブル (player_personality_type の順番に対応)
const PERSONALITY_NAMES := [
	"ふつう", "ちからじまん", "きれもの", "しあわせもの", "すばしっこい",
	"いのちしらず", "コンバット", "なまけもの", "セクシーギャル", "ラッキーマン",
	"がまんづよい", "むちゃくちゃ", "チャージマン",
]

func _ready() -> void:
	_load_title_image()
	get_viewport().size_changed.connect(_fit_title_image)
	$ButtonContainer/VBox/NewGameButton.pressed.connect(_on_new_game)
	$ButtonContainer/VBox/LoadGameButton.pressed.connect(_on_load_game)
	$SaveSelectPanel/PanelVBox/CancelButton.pressed.connect(_on_cancel_save_select)
	$SaveSelectPanel/PanelVBox/SaveList.item_activated.connect(_on_save_item_activated)

func _load_title_image() -> void:
	var img_path := ProjectSettings.globalize_path("res://../image/hengband_title.png")
	if not FileAccess.file_exists(img_path):
		push_warning("hengband_title.png not found: " + img_path)
		return
	var img := Image.load_from_file(img_path)
	if img == null:
		push_warning("Failed to load image: " + img_path)
		return
	$Background.texture = ImageTexture.create_from_image(img)
	call_deferred("_fit_title_image")

func _fit_title_image() -> void:
	var tex: Texture2D = $Background.texture
	if tex == null:
		return
	var vp_size := get_viewport().get_visible_rect().size
	var target_w := vp_size.x * 0.7
	var aspect := float(tex.get_width()) / float(tex.get_height())
	var target_h := target_w / aspect
	$Background.custom_minimum_size = Vector2.ZERO
	$Background.set_size(Vector2(target_w, target_h))
	$Background.set_position(Vector2((vp_size.x - target_w) * 0.5, vp_size.y * 0.1))

func _on_new_game() -> void:
	GameState.save_path = ""
	get_tree().change_scene_to_file("res://scenes/main.tscn")

func _on_load_game() -> void:
	_populate_save_list()
	$SaveSelectPanel.visible = true

func _populate_save_list() -> void:
	var list: ItemList = $SaveSelectPanel/PanelVBox/SaveList
	list.clear()
	_save_entries.clear()

	var lib_path := ProjectSettings.globalize_path("res://../lib")
	var entries: Array = $HengbandGame.scan_save_files(lib_path)

	if entries.is_empty():
		list.add_item("(セーブデータが見つかりません)")
		return

	for entry in entries:
		_save_entries.append(entry)
		var name_str: String = entry.get("name", "???")
		var level: int = entry.get("level", 0)
		var prace: int = entry.get("prace", 0)
		var pclass: int = entry.get("pclass", 0)
		var ppersonality: int = entry.get("ppersonality", 0)

		var race_name: String = RACE_NAMES[prace] if prace < RACE_NAMES.size() else "不明"
		var cls_name: String = CLASS_NAMES[pclass] if pclass < CLASS_NAMES.size() else "不明"
		var personality_name: String = PERSONALITY_NAMES[ppersonality] if ppersonality < PERSONALITY_NAMES.size() else "不明"

		var label := "%s  Lv.%d  [%s %s %s]" % [
			name_str, level, personality_name, race_name, cls_name
		]
		list.add_item(label)

func _on_cancel_save_select() -> void:
	$SaveSelectPanel.visible = false

func _on_save_item_activated(index: int) -> void:
	if index < 0 or index >= _save_entries.size():
		return
	var entry = _save_entries[index]
	var path: String = entry.get("path", "")
	if path.is_empty():
		return
	GameState.save_path = path
	get_tree().change_scene_to_file("res://scenes/main.tscn")
