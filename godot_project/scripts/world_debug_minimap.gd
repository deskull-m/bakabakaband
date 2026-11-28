extends Control

## ワールドスナップショット (観測可能ワールドモデル, #9292) のデバッグ表示
##
## Ctrl+Shift+M で表示/非表示を切り替える。表示中だけ HengbandGame にスナップショットを
## 作らせ (set_world_snapshot_enabled)、フロア全体を形状ごとの色で塗る。
## 端末のマップ表示と見比べて、未知のグリッドや見えないクリーチャーが漏れていないかを確認する用途。
##
## 3D 表示と同じ使い方の見本として、最初だけ全体を受け取り、以降は
## get_world_snapshot_changes() で変わったグリッドだけを手元の配列に反映する (#9293)。
## Ctrl+Shift+V で検証モード (差分と全体の突き合わせ) を切り替える。
## 表示中は表示用のゲームイベント (#9295) も受け取り、直近のものを右側に一覧する。

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
	Color(0.9, 0.3, 0.3),       # TRAP
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
var _view_switcher: Node = null
## 手元に保持しているフロアの状態 (差分を反映していく)
var _version: int = 0
var _width: int = 0
var _height: int = 0
var _dun_level: int = 0
var _shapes := PackedByteArray()
var _flags := PackedByteArray()
var _creatures: Array = []
var _player := Vector2i.ZERO
var _last_changed_count: int = 0
var _verify := false
## 直近の表示用ゲームイベント (文字列)
var _event_log: Array[String] = []
const EVENT_LOG_LINES := 24
const EVENT_NAMES := ["?", "MELEE", "PROJECTION", "DAMAGE", "DEATH", "MOVE", "TELEPORT", "APPEAR", "DISAPPEAR"]


func setup(game: Node, view_switcher: Node = null) -> void:
	_game = game
	_view_switcher = view_switcher
	_game.screen_state_changed.connect(func(_s): queue_redraw())
	_game.game_events.connect(_on_game_events)
	_game.world_snapshot_changed.connect(_on_world_snapshot_changed)
	visible = false
	mouse_filter = Control.MOUSE_FILTER_IGNORE


func _input(event: InputEvent) -> void:
	if not (event is InputEventKey) or not event.pressed or event.echo:
		return
	if not (event.ctrl_pressed and event.shift_pressed):
		return
	if event.keycode == KEY_M:
		visible = not visible
		# 3D の地図ビューを使っている間は、閉じてもスナップショットとイベントを止めない
		var keep := visible or GameState.use_map_view
		_game.set_world_snapshot_enabled(keep)
		_game.set_game_events_enabled(keep)
		# 無効の間の変化は追えないので、次は全体から受け取り直す
		_version = 0
		_pull_changes()
		get_viewport().set_input_as_handled()
	elif event.keycode == KEY_V and visible:
		_verify = not _verify
		_game.set_world_snapshot_verify(_verify)
		queue_redraw()
		get_viewport().set_input_as_handled()


func _on_world_snapshot_changed(_new_version: int) -> void:
	if visible:
		_pull_changes()


func _on_game_events(events: Array) -> void:
	for event: Dictionary in events:
		_event_log.append(describe_event(event))
	while _event_log.size() > EVENT_LOG_LINES:
		_event_log.pop_front()
	queue_redraw()


static func describe_actor(actor: Dictionary) -> String:
	var who := "player" if actor["is_player"] else ("#%d" % actor["id"] if actor["id"] != 0 else "?")
	return "%s@(%d,%d)" % [who, actor["x"], actor["y"]] if actor["has_pos"] else who


static func describe_event(event: Dictionary) -> String:
	var type: int = event["type"]
	var text: String = EVENT_NAMES[type] if type < EVENT_NAMES.size() else str(type)
	text += " " + describe_actor(event["actor"])
	match type:
		1:
			text += " -> %s %s" % [describe_actor(event["target"]), "hit" if event["hit"] else "miss"]
		2:
			text += " attr=%d r=%d path=%d area=%d" % [event["attribute"], event["radius"], event["path"].size() / 2, event["area"].size() / 3]
		3:
			text += " dmg=%d" % event["value"]
		5, 6:
			text += " from (%d,%d)" % [event["target"]["x"], event["target"]["y"]]
	return text


func _pull_changes() -> void:
	var changes: Dictionary = _game.get_world_snapshot_changes(_version)
	if changes.is_empty():
		return
	_width = changes["width"]
	_height = changes["height"]
	_dun_level = changes["dun_level"]
	if changes["full"]:
		_shapes = changes["shapes"]
		_flags = changes["flags"]
		_last_changed_count = _shapes.size()
	else:
		var indices: PackedInt32Array = changes["indices"]
		var shapes: PackedByteArray = changes["shapes"]
		var flags: PackedByteArray = changes["flags"]
		for i in indices.size():
			_shapes[indices[i]] = shapes[i]
			_flags[indices[i]] = flags[i]
		_last_changed_count = indices.size()
	_creatures = changes["creatures"]
	_player = Vector2i(changes["player_x"], changes["player_y"])
	_version = changes["version"]
	queue_redraw()


func _draw() -> void:
	if _version == 0 or _shapes.size() != _width * _height:
		return
	var width := _width
	var height := _height
	var shapes := _shapes
	var flags := _flags
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
	for creature: Dictionary in _creatures:
		var c_flags: int = creature["flags"]
		var color := Color(0.2, 1, 0.2) if (c_flags & CREATURE_PET) != 0 else Color(1, 0.2, 0.2)
		if (c_flags & CREATURE_IN_VIEW) == 0:
			color = color.darkened(0.4)
		draw_rect(Rect2(origin + Vector2(creature["x"] * CELL_PX, creature["y"] * CELL_PX), Vector2(CELL_PX, CELL_PX)), color)
	var player_pos := origin + Vector2(_player.x * CELL_PX, _player.y * CELL_PX)
	draw_rect(Rect2(player_pos, Vector2(CELL_PX, CELL_PX)), Color.WHITE)
	var status := "world snapshot v%d  %dx%d  dlv %d  creatures %d  changed %d" % [_version, width, height, _dun_level, _creatures.size(), _last_changed_count]
	if _verify:
		status += "  verify: %d stale" % _game.get_world_snapshot_verify_mismatches()
	var screen: Dictionary = _game.get_screen_state()
	if not screen.is_empty() and _view_switcher != null:
		status += "  screen: %s" % _view_switcher.state_name(screen["state"])
		if screen["has_cursor"]:
			status += " cursor=(%d,%d)" % [screen["cursor_x"], screen["cursor_y"]]
			var cursor_rect := Rect2(origin + Vector2(screen["cursor_x"] * CELL_PX, screen["cursor_y"] * CELL_PX), Vector2(CELL_PX, CELL_PX))
			draw_rect(cursor_rect.grow(1), Color.YELLOW, false)
	draw_string(ThemeDB.fallback_font, origin + Vector2(0, height * CELL_PX + 16), status)
	var log_origin := origin + Vector2(width * CELL_PX + 16, 12)
	for i in _event_log.size():
		draw_string(ThemeDB.fallback_font, log_origin + Vector2(0, i * 14), _event_log[i], HORIZONTAL_ALIGNMENT_LEFT, -1, 12)
