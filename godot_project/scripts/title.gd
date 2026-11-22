extends Control

## タイトル画面スクリプト
## 新規ゲーム / セーブデータから始める を選択してゲーム画面に遷移する
##
## 配置 (#9317): 左上に news (news_j.txt / news.txt)、左下に横幅 50% のタイトル画像、
## 右下に左揃えのボタン (日英とも New Game / Load Game)。news はタイトル画像に被らないよう、空いた高さに収まる文字の大きさで描く

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

const TitleLayout = preload("res://scripts/title_layout.gd")
## ボタンの文字の大きさ (ボタンの大きさは TitleLayout.BUTTON_SIZE) と書体 (無難なサンセリフ。無ければ順に代わりを使う)
const BUTTON_FONT_SIZE := 32
const BUTTON_FONT_NAMES: PackedStringArray = ["Helvetica", "Helvetica Neue", "Arial", "Liberation Sans", "DejaVu Sans", "Sans-Serif"]
## 左揃えの文字とボタンの左端との間
const BUTTON_TEXT_PADDING := 16.0
## news の文字の大きさの上限と下限 (空いた高さ・幅に収まるよう、この間で決める)
const NEWS_FONT_SIZE_MAX := 20
const NEWS_FONT_SIZE_MIN := 8
const NEWS_LINE_SPACING := 1.15


func _ready() -> void:
	_load_title_image()
	_load_news()
	var button_font := SystemFont.new()
	button_font.font_names = BUTTON_FONT_NAMES
	for button: Button in [$ButtonContainer/VBox/NewGameButton, $ButtonContainer/VBox/LoadGameButton]:
		button.add_theme_font_override("font", button_font)
		button.add_theme_font_size_override("font_size", BUTTON_FONT_SIZE)
		# 左揃えの文字がボタンの縁に付かないよう、各状態の枠に左の余白を足す
		for style_name in ["normal", "hover", "pressed", "focus", "disabled"]:
			var style: StyleBox = button.get_theme_stylebox(style_name)
			if style:
				var padded := style.duplicate() as StyleBox
				padded.content_margin_left = BUTTON_TEXT_PADDING
				button.add_theme_stylebox_override(style_name, padded)
	get_viewport().size_changed.connect(_fit_layout)
	call_deferred("_fit_layout")
	$ButtonContainer/VBox/NewGameButton.pressed.connect(_on_new_game)
	$ButtonContainer/VBox/LoadGameButton.pressed.connect(_on_load_game)
	$SaveSelectPanel/PanelVBox/CancelButton.pressed.connect(_on_cancel_save_select)
	$SaveSelectPanel/PanelVBox/SaveList.item_activated.connect(_on_save_item_activated)

func _load_title_image() -> void:
	var img_path := ProjectSettings.globalize_path("res://../image/BakabakabandTitle.png")
	if not FileAccess.file_exists(img_path):
		push_warning("BakabakabandTitle.png not found: " + img_path)
		return
	var img := Image.load_from_file(img_path)
	if img == null:
		push_warning("Failed to load image: " + img_path)
		return
	$Background.texture = ImageTexture.create_from_image(img)


## news_j.txt (日本語版) / news.txt を読む。ゲーム開始時に端末へ出すものと同じファイル
func _load_news() -> void:
	var news_name := "news_j.txt" if $HengbandGame.is_japanese() else "news.txt"
	var news_path := ProjectSettings.globalize_path("res://../lib/file/" + news_name)
	var text := FileAccess.get_file_as_string(news_path)
	if text.is_empty():
		push_warning("news not found: " + news_path)
	var news: Label = $News
	news.text = TitleLayout.news_body(text)
	var font := SystemFont.new()
	font.font_names = [GameState.font_name, "Courier New", "Monospace"]
	news.add_theme_font_override("font", font)


## 画面の大きさに合わせて、タイトル画像・ボタン・news を置き直す
func _fit_layout() -> void:
	var vp_size := get_viewport().get_visible_rect().size
	var title_rect := TitleLayout.title_image_rect(vp_size, _title_aspect())
	$Background.custom_minimum_size = Vector2.ZERO
	$Background.set_position(title_rect.position)
	$Background.set_size(title_rect.size)

	var buttons_rect := TitleLayout.button_area_rect(vp_size, TitleLayout.BUTTON_SIZE, 2, TitleLayout.BUTTON_SEPARATION)
	var vbox: VBoxContainer = $ButtonContainer/VBox
	for button: Button in vbox.get_children():
		button.custom_minimum_size = Vector2(buttons_rect.size.x, TitleLayout.BUTTON_SIZE.y)
	$ButtonContainer.set_position(buttons_rect.position)
	$ButtonContainer.set_size(buttons_rect.size)
	vbox.set_position(Vector2.ZERO)
	vbox.set_size(buttons_rect.size)

	_fit_news(TitleLayout.news_area_rect(vp_size, title_rect))


func _title_aspect() -> float:
	var tex: Texture2D = $Background.texture
	if tex == null or tex.get_height() == 0:
		return 1.0
	return float(tex.get_width()) / float(tex.get_height())


## news を置き、空いた高さと幅に全行が収まる文字の大きさを選ぶ
func _fit_news(area: Rect2) -> void:
	var news: Label = $News
	news.set_position(area.position)
	news.set_size(area.size)
	var font: Font = news.get_theme_font("font")
	var lines := news.text.split("\n")
	var size := NEWS_FONT_SIZE_MAX
	while size > NEWS_FONT_SIZE_MIN:
		var height := font.get_height(size) * NEWS_LINE_SPACING * lines.size()
		var width := 0.0
		for line in lines:
			width = maxf(width, font.get_string_size(line, HORIZONTAL_ALIGNMENT_LEFT, -1, size).x)
		if height <= area.size.y and width <= area.size.x:
			break
		size -= 1
	news.add_theme_font_size_override("font_size", size)
	news.add_theme_constant_override("line_spacing", int(font.get_height(size) * (NEWS_LINE_SPACING - 1.0)))

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
