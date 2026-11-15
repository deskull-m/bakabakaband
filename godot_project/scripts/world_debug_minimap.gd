extends Control

## ワールドスナップショット (観測可能ワールドモデル, #9292) のデバッグ表示
##
## Ctrl+Shift+M で表示/非表示を切り替える。表示中だけ HengbandGame にスナップショットを
## 作らせ (set_world_snapshot_enabled)、フロア全体を形状ごとの色で塗る。
## 端末のマップ表示と見比べて、未知のグリッドや見えないクリーチャーが漏れていないかを確認する用途。

## ObservedTerrainShape (src/view/observable-world.h) と同じ並び
const SHAPE_COLORS: Array[Color] = [
	Color(0, 0, 0, 0),          # UNKNOWN
	Color(0.35, 0.35, 0.35),    # FLOOR
	Color(0.75, 0.7, 0.6),      # WALL
	Color(0.6, 0.35, 0.1),      # DOOR_CLOSED
	Color(0.85, 0.55, 0.25),    # DOOR_OPEN
	Color(0.3, 0.9, 1.0),       # STAIRS_UP
	Color(0.2, 0.5, 1.0),       # STAIRS_DOWN
	Color(0.1, 0.3, 0.8),       # LIQUID
	Color(0.1, 0.6, 0.2),       # TREE
	Color(1.0, 0.85, 0.2),      # ENTRANCE
	Color(0.5, 0.5, 0.45),      # OTHER_PASSABLE
	Color(0.55, 0.45, 0.45),    # OTHER_OBSTACLE
]

## ObservedGridFlag
const GRID_IN_VIEW := 1 << 1
const GRID_OBJECT := 1 << 3
const GRID_CORPSE := 1 << 4

## ObservedCreatureFlag
const CREATURE_IN_VIEW := 1 << 0
const CREATURE_PET := 1 << 6

const CELL_PX := 4
const MARGIN := 8

var _game: Node = null
var _snapshot: Dictionary = {}


func setup(game: Node) -> void:
	_game = game
	_game.world_snapshot_changed.connect(_on_world_snapshot_changed)
	visible = false
	mouse_filter = Control.MOUSE_FILTER_IGNORE


func _input(event: InputEvent) -> void:
	if not (event is InputEventKey) or not event.pressed or event.echo:
		return
	if event.keycode == KEY_M and event.ctrl_pressed and event.shift_pressed:
		visible = not visible
		_game.set_world_snapshot_enabled(visible)
		if visible:
			_snapshot = _game.get_world_snapshot()
		queue_redraw()
		get_viewport().set_input_as_handled()


func _on_world_snapshot_changed(_version: int) -> void:
	if not visible:
		return
	_snapshot = _game.get_world_snapshot()
	queue_redraw()


func _draw() -> void:
	if _snapshot.is_empty():
		return
	var width: int = _snapshot["width"]
	var height: int = _snapshot["height"]
	var shapes: PackedByteArray = _snapshot["shapes"]
	var flags: PackedByteArray = _snapshot["flags"]
	var origin := Vector2(MARGIN, MARGIN)
	draw_rect(Rect2(origin - Vector2(2, 2), Vector2(width * CELL_PX + 4, height * CELL_PX + 4)), Color(0, 0, 0, 0.8))
	for y in height:
		for x in width:
			var i := y * width + x
			var shape := shapes[i]
			if shape == 0:
				continue
			var color := SHAPE_COLORS[shape] if shape < SHAPE_COLORS.size() else Color.MAGENTA
			if (flags[i] & GRID_IN_VIEW) == 0:
				color = color.darkened(0.5)
			var rect := Rect2(origin + Vector2(x * CELL_PX, y * CELL_PX), Vector2(CELL_PX, CELL_PX))
			draw_rect(rect, color)
			if (flags[i] & (GRID_OBJECT | GRID_CORPSE)) != 0:
				draw_rect(rect.grow(-1), Color(1, 1, 0))
	for creature: Dictionary in _snapshot["creatures"]:
		var c_flags: int = creature["flags"]
		var color := Color(0.2, 1, 0.2) if (c_flags & CREATURE_PET) != 0 else Color(1, 0.2, 0.2)
		if (c_flags & CREATURE_IN_VIEW) == 0:
			color = color.darkened(0.4)
		draw_rect(Rect2(origin + Vector2(creature["x"] * CELL_PX, creature["y"] * CELL_PX), Vector2(CELL_PX, CELL_PX)), color)
	var player_pos := origin + Vector2(_snapshot["player_x"] * CELL_PX, _snapshot["player_y"] * CELL_PX)
	draw_rect(Rect2(player_pos, Vector2(CELL_PX, CELL_PX)), Color.WHITE)
	draw_string(ThemeDB.fallback_font, origin + Vector2(0, height * CELL_PX + 16),
		"world snapshot v%d  %dx%d  dlv %d  creatures %d" % [_snapshot["version"], width, height, _snapshot["dun_level"], _snapshot["creatures"].size()])
