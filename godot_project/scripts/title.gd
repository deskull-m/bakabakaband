extends Control

## タイトル画面スクリプト
## 新規ゲーム / セーブデータから始める を選択してゲーム画面に遷移する
##
## 起動直後はメーカーロゴと標語を見せてからフェードインする (#9319、splash.gd)
##
## 配置 (#9317): 左上に news (news_j.txt / news.txt)、左下に横幅 50% のタイトル画像、
## 右下に左揃えのボタン (日英とも New Game / Load Game)。news はタイトル画像に被らないよう、空いた高さに収まる文字の大きさで描く

# セーブファイル一覧 (scan_save_files の戻り値)
var _save_entries: Array = []

const TitleLayout = preload("res://scripts/title_layout.gd")
const SaveList = preload("res://scripts/save_list.gd")
## ボタンの文字の大きさ (ボタンの大きさは TitleLayout.BUTTON_SIZE) と書体 (無難なサンセリフ。無ければ順に代わりを使う)
const BUTTON_FONT_SIZE := 32
const BUTTON_FONT_NAMES: PackedStringArray = ["Helvetica", "Helvetica Neue", "Arial", "Liberation Sans", "DejaVu Sans", "Sans-Serif"]
## 左揃えの文字とボタンの左端との間
const BUTTON_TEXT_PADDING := 16.0
## サブタイトルの下のバージョンの表記の文字の大きさ
const VERSION_FONT_SIZE := 18
## news の文字の大きさの上限と下限 (空いた高さ・幅に収まるよう、この間で決める)
const NEWS_FONT_SIZE_MAX := 20
const NEWS_FONT_SIZE_MIN := 8
const NEWS_LINE_SPACING := 1.15
## 収まる大きさからさらに縮める割合 (news は控えめに出す)
const NEWS_FONT_SCALE := 0.65
## ロードするセーブデータの一覧の文字の大きさ (既定の大きさに対する倍率)
const SAVE_LIST_FONT_SCALE := 1.5
## ロード一覧の列の幅に足す余白 (セルの左右の余白と列の間)
const SAVE_LIST_COLUMN_PADDING := 32.0
## ロード一覧の後ろを暗くする色と、一覧の枠の色
const SAVE_LIST_BACKDROP_COLOR := Color(0, 0, 0, 0.6)
const SAVE_LIST_PANEL_COLOR := Color(0.08, 0.08, 0.1, 1.0)

## ロード一覧の並べ方 (SaveList.Column と昇順か)
var _sort_column: int = SaveList.default_sort()[0]
var _sort_ascending: bool = SaveList.default_sort()[1]


func _ready() -> void:
	_start_splash()
	_load_title_image()
	_load_news()
	var version: Label = $Version
	version.text = "%s (Godot)" % $HengbandGame.get_version_string()
	var button_font := SystemFont.new()
	button_font.font_names = BUTTON_FONT_NAMES
	version.add_theme_font_override("font", button_font)
	version.add_theme_font_size_override("font_size", VERSION_FONT_SIZE)
	for button: Button in [$ButtonContainer/VBox/NewGameButton, $ButtonContainer/VBox/LoadGameButton]:
		button.add_theme_font_override("font", button_font)
		button.add_theme_font_size_override("font_size", BUTTON_FONT_SIZE)
		# 狭い画面ではボタンの幅を文字より狭くできるよう、収まらない文字は省略記号で切る
		# (切らないと文字の幅がボタンの最小の幅になり、右の端からはみ出す)
		button.clip_text = true
		button.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
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
	_setup_save_list()

## 起動時の演出 (#9319): ロゴ → 標語 → タイトル画面のフェードイン。起動ごとに 1 度だけ。
## 確認用のスクリプトなどで飛ばすときは環境変数 HB_SKIP_SPLASH=1
func _start_splash() -> void:
	var splash: Control = $Splash
	if GameState.splash_shown or OS.get_environment("HB_SKIP_SPLASH") == "1":
		splash.queue_free()
		return
	GameState.splash_shown = true


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
	var version_rect := TitleLayout.version_rect(title_rect)
	$Version.set_position(version_rect.position)
	$Version.set_size(version_rect.size)

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
	size = maxi(NEWS_FONT_SIZE_MIN, roundi(size * NEWS_FONT_SCALE))
	news.add_theme_font_size_override("font_size", size)
	var line_spacing := int(font.get_height(size) * (NEWS_LINE_SPACING - 1.0))
	news.add_theme_constant_override("line_spacing", line_spacing)
	# 最小の大きさでも全行が入らない狭い画面では、入る行数までにする。
	# Label は行数ぶんの高さを最小の大きさに持つので、clip_text だけではタイトル画像に被る
	var visible := TitleLayout.news_visible_lines(area.size.y, font.get_height(size) + line_spacing, lines.size())
	news.max_lines_visible = -1 if visible >= lines.size() else visible
	news.visible = visible > 0

func _on_new_game() -> void:
	GameState.save_path = ""
	get_tree().change_scene_to_file("res://scenes/main.tscn")

func _on_load_game() -> void:
	$SaveSelectBackdrop.visible = true
	$SaveSelectPanel.visible = true
	_populate_save_list()

## ロード一覧の表 (#9320): 名前・レベル・性格・種族・職業・プレイ時間の列を持ち、見出しを押すとその列で並べ替える
func _setup_save_list() -> void:
	var tree: Tree = $SaveSelectPanel/PanelVBox/SaveList
	var japanese: bool = $HengbandGame.is_japanese()
	var font_size := roundi(ThemeDB.fallback_font_size * SAVE_LIST_FONT_SCALE)
	tree.add_theme_font_size_override("font_size", font_size)
	tree.add_theme_font_size_override("title_button_font_size", font_size)
	for node: Control in [$SaveSelectPanel/PanelVBox/TitleLabel, $SaveSelectPanel/PanelVBox/CancelButton]:
		node.add_theme_font_size_override("font_size", font_size)
	$SaveSelectPanel/PanelVBox/TitleLabel.text = "セーブデータを選択" if japanese else "Select a saved game"
	$SaveSelectPanel/PanelVBox/CancelButton.text = "キャンセル" if japanese else "Cancel"
	for column in SaveList.Column.size():
		tree.set_column_title_alignment(column, HORIZONTAL_ALIGNMENT_LEFT)
		# 名前の列だけ余った幅を受け取り、ほかは中身の幅に合わせる
		tree.set_column_expand(column, column == SaveList.Column.NAME)
		tree.set_column_clip_content(column, column == SaveList.Column.NAME)
	# 後ろのタイトル画面が透けて読みにくいので、枠は不透明にし、その外側を暗くする
	var panel: Panel = $SaveSelectPanel
	var style := StyleBoxFlat.new()
	style.bg_color = SAVE_LIST_PANEL_COLOR
	style.set_corner_radius_all(6)
	panel.add_theme_stylebox_override("panel", style)
	$SaveSelectBackdrop.color = SAVE_LIST_BACKDROP_COLOR
	tree.column_title_clicked.connect(_on_save_list_title_clicked)
	tree.item_activated.connect(_on_save_item_activated)


func _populate_save_list() -> void:
	_save_entries.clear()
	var lib_path := ProjectSettings.globalize_path("res://../lib")
	for entry in $HengbandGame.scan_save_files(lib_path):
		_save_entries.append(entry)
	_refresh_save_list()


## 今の並べ方で表を作り直す。見出しには並べ替えている列に ▲ / ▼ を付ける
func _refresh_save_list() -> void:
	var tree: Tree = $SaveSelectPanel/PanelVBox/SaveList
	var japanese: bool = $HengbandGame.is_japanese()
	var headers := SaveList.headers(japanese)
	for column in SaveList.Column.size():
		var mark := ""
		if column == _sort_column:
			mark = " ▲" if _sort_ascending else " ▼"
		tree.set_column_title(column, headers[column] + mark)
	tree.clear()
	var root := tree.create_item()
	if _save_entries.is_empty():
		var empty := tree.create_item(root)
		empty.set_text(0, "(セーブデータが見つかりません)" if japanese else "(No saved games found)")
		empty.set_selectable(0, false)
		return
	_save_entries = SaveList.sort_entries(_save_entries, _sort_column, _sort_ascending)
	for i in _save_entries.size():
		var row := tree.create_item(root)
		row.set_metadata(0, i)
		for column in SaveList.Column.size():
			row.set_text(column, SaveList.cell_text(_save_entries[i], column))
			if column == SaveList.Column.LEVEL:
				row.set_text_alignment(column, HORIZONTAL_ALIGNMENT_CENTER)
			elif column == SaveList.Column.PLAY_TIME:
				row.set_text_alignment(column, HORIZONTAL_ALIGNMENT_RIGHT)
		# 名前が長くて切れても、ファイル名と合わせて確かめられるようにする
		row.set_tooltip_text(SaveList.Column.NAME, "%s\n%s" % [_save_entries[i].get("name", ""), _save_entries[i].get("file", "")])
	_fit_save_list_columns(tree)
	var first := root.get_first_child()
	if first:
		first.select(0)
	tree.grab_focus()


## 名前以外の列を、見出しと中身のいちばん長い文字が収まる幅にする (既定では狭すぎて種族名などが切れる)
func _fit_save_list_columns(tree: Tree) -> void:
	var font: Font = tree.get_theme_font("font")
	var font_size := tree.get_theme_font_size("font_size")
	var title_font: Font = tree.get_theme_font("title_button_font")
	var title_size := tree.get_theme_font_size("title_button_font_size")
	for column in SaveList.Column.size():
		if column == SaveList.Column.NAME:
			continue
		var width := title_font.get_string_size(tree.get_column_title(column), HORIZONTAL_ALIGNMENT_LEFT, -1, title_size).x
		for entry in _save_entries:
			width = maxf(width, font.get_string_size(SaveList.cell_text(entry, column), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x)
		tree.set_column_custom_minimum_width(column, ceili(width + SAVE_LIST_COLUMN_PADDING))


func _on_save_list_title_clicked(column: int, mouse_button_index: int) -> void:
	if mouse_button_index != MOUSE_BUTTON_LEFT:
		return
	var next := SaveList.next_sort(_sort_column, _sort_ascending, column)
	_sort_column = next[0]
	_sort_ascending = next[1]
	_refresh_save_list()

func _on_cancel_save_select() -> void:
	$SaveSelectPanel.visible = false
	$SaveSelectBackdrop.visible = false

func _on_save_item_activated() -> void:
	var tree: Tree = $SaveSelectPanel/PanelVBox/SaveList
	var item := tree.get_selected()
	if item == null or item.get_metadata(0) == null:
		return
	var index: int = item.get_metadata(0)
	if index < 0 or index >= _save_entries.size():
		return
	var entry: Dictionary = _save_entries[index]
	var path: String = entry.get("path", "")
	if path.is_empty():
		return
	GameState.save_path = path
	get_tree().change_scene_to_file("res://scenes/main.tscn")
