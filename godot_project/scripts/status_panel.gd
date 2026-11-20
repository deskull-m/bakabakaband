extends PanelContainer

## ステータスパネル
##
## C コマンドのステータス画面 (キャラクタダンプの先頭と同じ内容) をそのまま、端末と同じ等幅の
## 文字の格子で描く。文字ごとに桁の位置へ置くので、フォントの字幅に依らず桁が揃う (全角は 2 桁)。
## データは HengbandGame.get_character_sheet() から取り、character_sheet_changed で描き直す。
## 表示している間だけゲーム側に作らせる (作るたびにステータス画面を画面外の端末へ 5 枚描くため)。
## 題名の部分をドラッグすると動かせ、位置は GameState.status_panel_pos に保存する。
## main.gd の _ready() から connect_to_game(game) で接続される。

## 一度に見せる行数 (それより長い分はスクロール)
const VISIBLE_ROWS := 26
const SheetView := preload("res://scripts/character_sheet_view.gd")

var _game: Node = null
var _view: Control
var _scroll: ScrollContainer
var _title: Label
var _dragging := false
var _drag_offset := Vector2.ZERO


func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	var vbox := VBoxContainer.new()
	add_child(vbox)
	var header := HBoxContainer.new()
	vbox.add_child(header)
	_title = Label.new()
	_title.text = "ステータス"
	_title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_title.mouse_filter = Control.MOUSE_FILTER_PASS
	_title.add_theme_color_override("font_color", Color(1.0, 0.85, 0.5))
	header.add_child(_title)
	var close := Button.new()
	close.text = "×"
	close.flat = true
	close.focus_mode = Control.FOCUS_NONE
	close.pressed.connect(func() -> void:
		visible = false
		GameState.status_panel_visible = false
		GameState.save_config())
	header.add_child(close)

	_scroll = ScrollContainer.new()
	_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	vbox.add_child(_scroll)
	_view = SheetView.new()
	_scroll.add_child(_view)
	set_sheet_font(_make_font(), GameState.font_size)

	visible = GameState.status_panel_visible
	visibility_changed.connect(_on_visibility_changed)
	# main.tscn は右上に寄せる配置で置いているので、位置は自分で決める
	set_anchors_preset(Control.PRESET_TOP_LEFT)
	_place.call_deferred()


func _make_font() -> Font:
	var font := SystemFont.new()
	font.font_names = [GameState.font_name, "Courier New", "Monospace"]
	return font


## 文字の大きさが変わったら格子と窓の大きさを合わせる
func set_sheet_font(font: Font, font_size: int) -> void:
	_view.set_font(font, font_size)
	_scroll.custom_minimum_size = Vector2(_view.custom_minimum_size.x, _view.cell_size.y * VISIBLE_ROWS)
	reset_size()


func _place() -> void:
	var screen := get_viewport_rect().size
	var pos := Vector2(GameState.status_panel_pos)
	if GameState.status_panel_pos.x < 0:
		pos = Vector2(screen.x - size.x - 4, 4)
	position = clamp_position(pos, size, screen)


## HengbandGame ノードに接続する。既に接続済みの場合は二重接続しない。
func connect_to_game(game: Node) -> void:
	if game == null:
		return
	_game = game
	if not game.is_connected("character_sheet_changed", Callable(self, "_on_sheet_changed")):
		game.connect("character_sheet_changed", Callable(self, "_on_sheet_changed"))
	game.set_character_sheet_enabled(visible)
	_on_sheet_changed(0)


func _on_visibility_changed() -> void:
	if _game:
		_game.set_character_sheet_enabled(visible)
		# 隠している間に届いた更新は捨てているので、開いたら公開済みのものを読み直す
		if visible:
			_on_sheet_changed(0)


func _on_sheet_changed(_version: int) -> void:
	if _game == null or not visible:
		return
	var sheet: Dictionary = _game.get_character_sheet()
	if not sheet.is_empty():
		_view.set_sheet(sheet)


func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT:
		if event.pressed and event.position.y <= _title.size.y + 6:
			_dragging = true
			_drag_offset = event.position
			accept_event()
		elif not event.pressed and _dragging:
			_dragging = false
			GameState.status_panel_pos = Vector2i(position)
			GameState.save_config()
			accept_event()
	elif event is InputEventMouseMotion and _dragging:
		position = clamp_position(position + event.position - _drag_offset, size, get_viewport_rect().size)
		accept_event()


## 窓を画面の中に収める (題名の部分が必ず見えるように)
static func clamp_position(pos: Vector2, window_size: Vector2, screen: Vector2) -> Vector2:
	var max_x := maxf(0.0, screen.x - minf(window_size.x, screen.x))
	var max_y := maxf(0.0, screen.y - 24.0)
	return Vector2(clampf(pos.x, 0.0, max_x), clampf(pos.y, 0.0, max_y))
