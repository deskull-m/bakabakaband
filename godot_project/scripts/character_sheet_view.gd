extends Control

## 端末の文字の格子を描く (ステータス画面のダンプ用)
##
## HengbandGame.get_character_sheet() の lines を受け、各文字を「桁 × 文字幅」の位置に描く。
## 桁の位置はゲーム側 (端末) で決まっているので、フォントの字幅が端末と違っても桁は揃う。
## 全角の文字は 2 桁ぶんの幅に収まるよう、はみ出すときだけ横に縮めて描く。

## 端末の色が取れないときの色
const FALLBACK_COLOR := Color(0.9, 0.9, 0.9)

var _font: Font
var _font_size := 14
var cell_size := Vector2(8, 16)
var _columns := 80
var _lines: Array = []
var _palette := {} ## 端末の色の番号 → Color
var _game: Node = null


func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_PASS


## 格子の 1 桁の大きさを決めておく (端末に合わせるとき)。Vector2.ZERO ならフォントから測る
var cell_size_override := Vector2.ZERO


func set_font(font: Font, font_size: int) -> void:
	_font = font
	_font_size = font_size
	cell_size = cell_size_override if cell_size_override != Vector2.ZERO else measure_cell(font, font_size)
	_update_minimum_size()
	queue_redraw()


## 端末の矩形 (GodotTerminal.get_region()) を lines の形に直す。全角の右半分 (0) は前の字の幅に数える
## 色は下位 4 ビット (日本語版の端末が全角の桁に重ねる印を除く)
static func region_to_lines(region: Dictionary) -> Array:
	var cols := int(region.get("cols", 0))
	var rows := int(region.get("rows", 0))
	var chars: PackedInt32Array = region.get("chars", PackedInt32Array())
	var colors: PackedByteArray = region.get("colors", PackedByteArray())
	var lines: Array = []
	for y in rows:
		var line_cols := PackedInt32Array()
		var widths := PackedByteArray()
		var line_chars := PackedStringArray()
		var line_colors := PackedByteArray()
		for x in cols:
			var c := chars[y * cols + x]
			if c == 0:
				if widths.size() > 0 and line_cols[line_cols.size() - 1] == x - 1:
					widths[widths.size() - 1] = 2
				continue
			if c == 32:
				continue
			line_cols.append(x)
			widths.append(1)
			line_chars.append(String.chr(c))
			line_colors.append(colors[y * cols + x] & 0x0F)
		lines.append({ "cols": line_cols, "widths": widths, "chars": line_chars, "colors": line_colors })
	return lines


## 格子の 1 桁の大きさ (半角の字幅 × 行の高さ)
static func measure_cell(font: Font, font_size: int) -> Vector2:
	var width := font.get_string_size("M", HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x
	return Vector2(ceilf(width), ceilf(font.get_height(font_size)))


func set_sheet(sheet: Dictionary) -> void:
	_lines = sheet.get("lines", [])
	_columns = int(sheet.get("width", 80))
	if _game == null:
		_game = _find_game()
	_update_minimum_size()
	queue_redraw()


func _update_minimum_size() -> void:
	custom_minimum_size = Vector2(cell_size.x * _columns, cell_size.y * maxi(_lines.size(), 1))


func _find_game() -> Node:
	var nodes := get_tree().root.find_children("*", "HengbandGame", true, false) if is_inside_tree() else []
	return nodes[0] if not nodes.is_empty() else null


func _color(index: int) -> Color:
	if not _palette.has(index):
		_palette[index] = _game.get_term_color(index) if _game else FALLBACK_COLOR
	return _palette[index]


func _draw() -> void:
	if _font == null:
		return
	var ascent := _font.get_ascent(_font_size)
	for y in _lines.size():
		var line: Dictionary = _lines[y]
		var cols: PackedInt32Array = line["cols"]
		var widths: PackedByteArray = line["widths"]
		var chars: PackedStringArray = line["chars"]
		var colors: PackedByteArray = line["colors"]
		var baseline := y * cell_size.y + ascent
		for i in cols.size():
			var box := cell_size.x * widths[i]
			var pos := Vector2(cols[i] * cell_size.x, baseline)
			var glyph_width := _font.get_string_size(chars[i], HORIZONTAL_ALIGNMENT_LEFT, -1, _font_size).x
			if glyph_width > box + 0.5:
				# 桁からはみ出す字 (全角の字幅が半角 2 つより広いフォントなど) は横に縮める
				var s := box / glyph_width
				draw_set_transform(Vector2(pos.x * (1.0 - s), 0), 0.0, Vector2(s, 1.0))
				draw_string(_font, pos, chars[i], HORIZONTAL_ALIGNMENT_LEFT, -1, _font_size, _color(colors[i]))
				draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)
			else:
				draw_string(_font, pos, chars[i], HORIZONTAL_ALIGNMENT_LEFT, -1, _font_size, _color(colors[i]))
