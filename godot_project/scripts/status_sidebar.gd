extends PanelContainer

## 3D の地図ビューの上に重ねる、端末の一部の写し
##
## 端末 (GodotTerminal) は 3D の地図ビューを見ている間も裏で描かれ続けているので、その矩形を
## そのまま写す。並び・色・文言は端末とまったく同じになる。
## 端末の内容が変わったときだけ写し直す (get_content_version())。
##
## 写す範囲 (Region):
## - LEFT_PANEL: 左側の欄 (種族・職業・レベル・経験・所持金・能力値・AC・HP・MP・時空崩壊度・
##   日付・ダンジョン名など)。地図の左の COL_MAP 桁 × メッセージの行の下から最下行の上まで
## - BOTTOM_BAR: 最下行 (状態バー・速度・学習など。ROW_STATBAR / ROW_SPEED = -1)。全幅
## - MESSAGE_LINE: 1 行目 (メッセージ・-more-・プロンプト)。全幅

enum Region { LEFT_PANEL, BOTTOM_BAR, MESSAGE_LINE }

const SheetView := preload("res://scripts/character_sheet_view.gd")
## 地図の左にある欄の幅 (src/window/main-window-util.h の COL_MAP)
const SIDEBAR_COLUMNS := 12
## 1 行目はメッセージの行なので写さない
const FIRST_ROW := 1
## 下端に置くときの、画面の下からの余白 (main.tscn の下のバーの高さ + すき間)
const BOTTOM_MARGIN := 45.0 + 4.0

var region: int = Region.LEFT_PANEL

var _terminal: Node = null
var _view: Control
var _last_version := -1
var _last_rows := -1
var _last_cols := -1


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


## 写す端末 (メインの端末) と範囲・フォントを与える
func setup(terminal: Node, font: Font, font_size: int, region_kind: int = Region.LEFT_PANEL) -> void:
	_terminal = terminal
	region = region_kind
	set_sheet_font(font, font_size)


func set_sheet_font(font: Font, font_size: int) -> void:
	if _terminal:
		_view.cell_size_override = Vector2(_terminal.get_cell_width(), _terminal.get_cell_height())
	_view.set_font(font, font_size)
	_last_version = -1


## 写す矩形 [x, y, w, h] (端末の桁・行)
static func region_rect(region_kind: int, cols: int, rows: int) -> Rect2i:
	if region_kind == Region.BOTTOM_BAR:
		return Rect2i(0, rows - 1, cols, 1)
	if region_kind == Region.MESSAGE_LINE:
		return Rect2i(0, 0, cols, 1)
	# 最下行は状態バーなので左側の欄には含めない (BOTTOM_BAR で写す)
	return Rect2i(0, FIRST_ROW, SIDEBAR_COLUMNS, maxi(0, rows - FIRST_ROW - 1))


func _process(_delta: float) -> void:
	if _terminal == null or not _wants_show:
		return
	var version: int = _terminal.get_content_version()
	var rows: int = _terminal.get_rows()
	var cols: int = _terminal.get_cols()
	if version == _last_version and rows == _last_rows and cols == _last_cols:
		_place()
		return
	_last_version = version
	_last_rows = rows
	_last_cols = cols
	refresh()


## 表示を許されているか (地図ビューを見ている間)。中身が空のときは許されていても隠す
var _wants_show := false


func set_shown(shown: bool) -> void:
	_wants_show = shown
	_last_version = -1
	_apply_visibility()


func _apply_visibility() -> void:
	visible = _wants_show and not _view._lines.is_empty()


func refresh() -> void:
	var rect := region_rect(region, _terminal.get_cols(), _terminal.get_rows())
	var lines := SheetView.region_to_lines(_terminal.get_region(rect.position.x, rect.position.y, rect.size.x, rect.size.y))
	# 下の空いた行は描かない (窓の高さを中身に合わせる)
	while not lines.is_empty() and (lines[lines.size() - 1]["chars"] as PackedStringArray).is_empty():
		lines.pop_back()
	_view.set_sheet({ "lines": lines, "width": _content_width(lines, rect.size.x) })
	_apply_visibility()
	reset_size()
	_place()


## 窓の幅 (桁)。左側の欄は固定、下端の帯は使っている一番右の桁まで
func _content_width(lines: Array, full_width: int) -> int:
	if region == Region.LEFT_PANEL:
		return full_width
	var widest := 0
	for line: Dictionary in lines:
		var cols: PackedInt32Array = line["cols"]
		var widths: PackedByteArray = line["widths"]
		if cols.size() > 0:
			widest = maxi(widest, cols[cols.size() - 1] + widths[widths.size() - 1])
	return widest


func _place() -> void:
	if _terminal == null:
		return
	if region == Region.BOTTOM_BAR:
		position = Vector2(4, get_viewport_rect().size.y - BOTTOM_MARGIN - size.y)
	elif region == Region.MESSAGE_LINE:
		position = Vector2(4, 2)
	else:
		# メッセージの行の窓 (1 行 + 余白) の下。文字の大きさを変えたら行の高さに合わせて動かす
		position = Vector2(4, _terminal.get_cell_height() + 8)
