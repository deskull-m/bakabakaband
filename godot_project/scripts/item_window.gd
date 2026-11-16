extends PanelContainer

## メッセージ・持ち物・装備のウィンドウ
##
## HengbandGame の inventory_changed / messages_changed シグナルを受けて中身を描き直す。
## 題名の部分をドラッグすると動かせる。main.gd が種類ごとに 1 つずつ作る。
## データは HengbandGame.set_item_windows_enabled(true) の間だけ作られる。

enum Kind { MESSAGES, INVENTORY, EQUIPMENT }

const TITLES := {
	Kind.MESSAGES: "メッセージ",
	Kind.INVENTORY: "持ち物",
	Kind.EQUIPMENT: "装備",
}
const DEFAULT_SIZES := {
	Kind.MESSAGES: Vector2(520, 220),
	Kind.INVENTORY: Vector2(420, 360),
	Kind.EQUIPMENT: Vector2(420, 300),
}

## 端末の色の番号 (TERM_WHITE 等) → 既定の色。get_term_color が使えないときの予備
const FALLBACK_TEXT_COLOR := Color(0.9, 0.9, 0.9)

var kind: int = Kind.MESSAGES
var _game: Node = null
var _title: Label
var _body: RichTextLabel
var _dragging := false
var _drag_offset := Vector2.ZERO

## ウィンドウが動かされた・閉じられたときに知らせる (位置の保存用)
signal layout_changed(kind: int)


func setup(game: Node, window_kind: int) -> void:
	_game = game
	kind = window_kind
	name = "ItemWindow%d" % kind
	_build()
	if game == null:
		return
	if kind == Kind.MESSAGES:
		game.messages_changed.connect(func(_v: int) -> void: refresh())
	else:
		game.inventory_changed.connect(func(_v: int) -> void: refresh())
	refresh()


func _build() -> void:
	custom_minimum_size = DEFAULT_SIZES[kind]
	size = DEFAULT_SIZES[kind]
	mouse_filter = Control.MOUSE_FILTER_STOP
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.1, 0.07, 0.07, 0.85)
	style.set_corner_radius_all(4)
	style.set_content_margin_all(6)
	add_theme_stylebox_override("panel", style)

	var vbox := VBoxContainer.new()
	add_child(vbox)

	var header := HBoxContainer.new()
	vbox.add_child(header)
	_title = Label.new()
	_title.text = TITLES[kind]
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
		layout_changed.emit(kind))
	header.add_child(close)

	_body = RichTextLabel.new()
	_body.bbcode_enabled = true
	_body.scroll_active = true
	_body.selection_enabled = true
	_body.size_flags_vertical = Control.SIZE_EXPAND_FILL
	_body.focus_mode = Control.FOCUS_NONE
	vbox.add_child(_body)


## 表示中なら最新の中身で描き直す (隠れている間はシグナルを受けても何もしない)
func refresh() -> void:
	if _game == null or not visible:
		return
	match kind:
		Kind.MESSAGES:
			var data: Dictionary = _game.get_messages()
			_body.text = format_messages(data)
			_body.scroll_following = true
		Kind.INVENTORY:
			_body.text = format_inventory(_game.get_inventory(), _color_of)
		Kind.EQUIPMENT:
			_body.text = format_equipment(_game.get_inventory(), _color_of)


func _notification(what: int) -> void:
	if what == NOTIFICATION_VISIBILITY_CHANGED and visible:
		refresh()


func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT:
		if event.pressed and _title and event.position.y <= _title.size.y + 8:
			_dragging = true
			_drag_offset = event.position
			accept_event()
		elif not event.pressed and _dragging:
			_dragging = false
			layout_changed.emit(kind)
			accept_event()
	elif event is InputEventMouseMotion and _dragging:
		position = clamp_position(position + event.position - _drag_offset, size, get_viewport_rect().size)
		accept_event()


func _color_of(term_color: int) -> Color:
	if _game and _game.has_method("get_term_color"):
		return _game.get_term_color(term_color)
	return FALLBACK_TEXT_COLOR


# ---- 純粋な整形関数 (tools/godot/gdscript-unit-check.gd で確かめる) ----

## ウィンドウを画面の中に収める (題名の部分が必ず見えるように)
static func clamp_position(pos: Vector2, window_size: Vector2, screen: Vector2) -> Vector2:
	var max_x := maxf(0.0, screen.x - minf(window_size.x, screen.x))
	var max_y := maxf(0.0, screen.y - 24.0)
	return Vector2(clampf(pos.x, 0.0, max_x), clampf(pos.y, 0.0, max_y))


## BBCode の制御文字を無効にする
static func escape_bbcode(text: String) -> String:
	return text.replace("[", "[lb]")


static func _colored(text: String, color: Color) -> String:
	return "[color=#%s]%s[/color]" % [color.to_html(false), escape_bbcode(text)]


static func format_messages(data: Dictionary) -> String:
	var lines: PackedStringArray = data.get("messages", PackedStringArray())
	var out := PackedStringArray()
	for line in lines:
		out.append(escape_bbcode(line))
	return "\n".join(out)


## 1 品を "a) [記号] 名前   重さ" の形にする
static func format_item_line(label: String, item: Dictionary, color_of: Callable) -> String:
	var symbol := str(item.get("symbol", ""))
	var symbol_part := ""
	if symbol != "":
		symbol_part = _colored(symbol, color_of.call(int(item.get("symbol_color", 1)))) + " "
	var name_part := _colored(str(item.get("name", "")), color_of.call(int(item.get("color", 1))))
	var weight := str(item.get("weight_text", "")).strip_edges()
	return "%s) %s%s  [color=#a0a0a0]%s[/color]" % [label, symbol_part, name_part, escape_bbcode(weight)]


static func format_weight_footer(data: Dictionary) -> String:
	var total := int(data.get("total_weight", 0))
	var limit := int(data.get("weight_limit", 0))
	# 表記はゲーム側で作ったもの (日本語版は kg、英語版は lb)。無ければ 1/10 ポンド単位の値から作る
	var total_text := str(data.get("total_weight_text", "")).strip_edges()
	var limit_text := str(data.get("weight_limit_text", "")).strip_edges()
	if total_text == "" or limit_text == "":
		total_text = "%d.%d lb" % [total / 10, total % 10]
		limit_text = "%d.%d lb" % [limit / 10, limit % 10]
	var text := "重量 %s / %s" % [total_text, limit_text]
	var color := Color(1.0, 0.5, 0.4) if limit > 0 and total > limit else Color(0.7, 0.7, 0.7)
	return "[color=#%s]%s[/color]" % [color.to_html(false), text]


static func format_inventory(data: Dictionary, color_of: Callable) -> String:
	if data.is_empty():
		return ""
	var out := PackedStringArray()
	var pack: Array = data.get("pack", [])
	if pack.is_empty():
		out.append("[color=#a0a0a0](何も持っていない)[/color]")
	for item in pack:
		out.append(format_item_line(str(item.get("label", "")), item, color_of))
	out.append("")
	out.append(format_weight_footer(data))
	return "\n".join(out)


static func format_equipment(data: Dictionary, color_of: Callable) -> String:
	if data.is_empty():
		return ""
	var out := PackedStringArray()
	for slot in data.get("equipment", []):
		var label := str(slot.get("label", ""))
		var slot_name := escape_bbcode(str(slot.get("slot_name", "")))
		var item = slot.get("item")
		if item is Dictionary:
			out.append("%s  [color=#a0a0a0]%s[/color]" % [format_item_line(label, item, color_of), slot_name])
		elif not bool(slot.get("can_equip", true)):
			out.append("[color=#606060]%s) (装備できない部位: %s)[/color]" % [label, slot_name])
		else:
			out.append("[color=#808080]%s) (なし: %s)[/color]" % [label, slot_name])
	return "\n".join(out)
