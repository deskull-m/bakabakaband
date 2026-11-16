extends PanelContainer

## 3D の地図ビューの上に重ねる、端末の左側の欄 (種族・職業・レベル・経験・所持金・能力値・AC・HP・MP・
## 時空崩壊度・日付・ダンジョン名など)
##
## 端末 (GodotTerminal) は 3D の地図ビューを見ている間も裏で描かれ続けているので、その左側の欄の
## 矩形 (地図の左、COL_MAP 桁) をそのまま写す。並び・色・文言は端末とまったく同じになる。
## 端末の内容が変わったときだけ写し直す (get_content_version())。

const SheetView := preload("res://scripts/character_sheet_view.gd")
## 地図の左にある欄の幅 (src/window/main-window-util.h の COL_MAP)
const SIDEBAR_COLUMNS := 12
## 1 行目はメッセージの行なので写さない
const FIRST_ROW := 1

var _terminal: Node = null
var _view: Control
var _last_version := -1
var _last_rows := -1


func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.0, 0.0, 0.0, 0.55)
	style.set_corner_radius_all(4)
	style.content_margin_left = 4
	style.content_margin_right = 4
	style.content_margin_top = 2
	style.content_margin_bottom = 2
	add_theme_stylebox_override("panel", style)
	_view = SheetView.new()
	_view.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(_view)


## 写す端末 (メインの端末) とフォントを与える
func setup(terminal: Node, font: Font, font_size: int) -> void:
	_terminal = terminal
	set_sheet_font(font, font_size)


func set_sheet_font(font: Font, font_size: int) -> void:
	if _terminal:
		_view.cell_size_override = Vector2(_terminal.get_cell_width(), _terminal.get_cell_height())
	_view.set_font(font, font_size)
	_last_version = -1


func _process(_delta: float) -> void:
	if not visible or _terminal == null:
		return
	var version: int = _terminal.get_content_version()
	var rows: int = _terminal.get_rows()
	if version == _last_version and rows == _last_rows:
		return
	_last_version = version
	_last_rows = rows
	refresh()


func refresh() -> void:
	var rows: int = _terminal.get_rows() - FIRST_ROW
	var region: Dictionary = _terminal.get_region(0, FIRST_ROW, SIDEBAR_COLUMNS, rows)
	var lines := SheetView.region_to_lines(region)
	# 下の空いた行は描かない (窓の高さを中身に合わせる)
	while not lines.is_empty() and (lines[lines.size() - 1]["chars"] as PackedStringArray).is_empty():
		lines.pop_back()
	_view.set_sheet({ "lines": lines, "width": SIDEBAR_COLUMNS })
	reset_size()
