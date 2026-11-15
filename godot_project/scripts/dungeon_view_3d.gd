extends SubViewportContainer

## 3D の地図ビュー (最小構成、#9298)
##
## ワールドスナップショット (#9292/#9293) を受け、地形を MultiMesh、クリーチャー・床アイテム・
## プレイヤーを文字のビルボードで描く。見た目は visual_catalog.gd (#9297) で決める。
## 地図を見ている間だけ view_switcher.gd (#9294) が見せる。
##
## - 地形はグリッドごとに MultiMesh のインスタンスを 1 つ割り当てる (箱用と板用の 2 つ)。
##   差分では変わったグリッドのインスタンスだけを書き換える
## - 座標は X 軸 = 東、Z 軸 = 南、1 グリッド = 1 (input_intents.gd と同じ)
## - 左クリックでトラベル、右クリックでターゲット (#9296)。中ボタンのドラッグでカメラを回し、
##   Shift + ホイールで寄る・引く
## - 視界・光源の表現は #9299 で行う。ここでは視界外のグリッドを暗くするだけ

const VisualCatalog = preload("res://scripts/visual_catalog.gd")
const InputIntents = preload("res://scripts/input_intents.gd")

## ObservedGridFlag / ObservedCreatureFlag
const GRID_KNOWN := 1 << 0
const GRID_IN_VIEW := 1 << 1
const GRID_OBJECT := 1 << 3
const CREATURE_IN_VIEW := 1 << 0

## 置かないインスタンスの変換 (大きさ 0)
var HIDDEN := Transform3D(Basis().scaled(Vector3.ZERO), Vector3.ZERO)
const MOVE_TWEEN_SEC := 0.12

var game: Node = null
var catalog := VisualCatalog.new()

var _viewport: SubViewport
var _world: Node3D
var _camera: Camera3D
var _box_mm: MultiMesh
var _plane_mm: MultiMesh
var _creature_root: Node3D
var _item_root: Node3D
var _player_label: Label3D
var _message: Label

var _version := 0
var _width := 0
var _height := 0
var _terrain := PackedInt32Array()
var _flags := PackedByteArray()
var _tvals := PackedByteArray()
var _symbols := PackedByteArray()
## グリッドごとに置いたもの (KIND_*)
var _grid_kind := PackedByteArray()
const KIND_NONE := 0
const KIND_BOX := 1
const KIND_PLANE := 2
var _creature_labels := {} ## id → Label3D
var _item_labels := {} ## グリッドの添字 → Label3D
var _player := Vector2i.ZERO
var _camera_yaw := 0.0
var _camera_distance := 22.0

## 床の板は端末の色そのままだと明るすぎて上のビルボードが見づらいので落とす
const PLANE_BRIGHTNESS := 0.35
var _dragging := false

## 計測 (ミリ秒)
var last_full_build_ms := 0.0
var last_update_ms := 0.0
var last_changed_grids := 0


func _ready() -> void:
	stretch = true
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	_viewport = SubViewport.new()
	_viewport.own_world_3d = true
	_viewport.handle_input_locally = false
	add_child(_viewport)
	_world = Node3D.new()
	_viewport.add_child(_world)

	var env := WorldEnvironment.new()
	env.environment = Environment.new()
	env.environment.background_mode = Environment.BG_COLOR
	env.environment.background_color = Color.BLACK
	env.environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.environment.ambient_light_color = Color(0.6, 0.6, 0.6)
	_world.add_child(env)
	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-60, 30, 0)
	_world.add_child(sun)

	_camera = Camera3D.new()
	_camera.fov = 50.0
	_world.add_child(_camera)

	_box_mm = _make_multimesh(BoxMesh.new())
	_plane_mm = _make_multimesh(_make_plane_mesh())
	_creature_root = Node3D.new()
	_world.add_child(_creature_root)
	_item_root = Node3D.new()
	_world.add_child(_item_root)
	_player_label = _make_glyph("@", Color.WHITE, 1.0)
	_world.add_child(_player_label)

	_message = Label.new()
	_message.set_anchors_and_offsets_preset(Control.PRESET_TOP_WIDE)
	_message.add_theme_color_override("font_color", Color.WHITE)
	_message.add_theme_color_override("font_outline_color", Color.BLACK)
	_message.add_theme_constant_override("outline_size", 4)
	add_child(_message)


func setup(game_node: Node) -> void:
	game = game_node
	catalog.load_definitions()
	catalog.set_color_provider(game.get_term_color)
	game.world_snapshot_changed.connect(_on_world_snapshot_changed)
	game.game_events.connect(_on_game_events)
	visibility_changed.connect(_on_visibility_changed)


func _make_plane_mesh() -> Mesh:
	var plane := PlaneMesh.new()
	plane.size = Vector2(1, 1)
	return plane


func _make_multimesh(mesh: Mesh) -> MultiMesh:
	var mm := MultiMesh.new()
	mm.transform_format = MultiMesh.TRANSFORM_3D
	mm.use_colors = true
	mm.mesh = mesh
	var material := StandardMaterial3D.new()
	material.vertex_color_use_as_albedo = true
	mesh.surface_set_material(0, material)
	var instance := MultiMeshInstance3D.new()
	instance.multimesh = mm
	_world.add_child(instance)
	return mm


func _make_glyph(text: String, color: Color, scale_factor: float) -> Label3D:
	var label := Label3D.new()
	label.text = text
	label.modulate = color
	label.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	label.pixel_size = 0.02 * scale_factor
	label.font_size = 48
	label.outline_size = 8
	label.no_depth_test = false
	return label


## 見せ始めたら、溜まっている変化を取り込む (見ていない間も world_snapshot_changed は来る)
func _on_visibility_changed() -> void:
	if visible:
		_pull()


func _on_world_snapshot_changed(_new_version: int) -> void:
	if visible:
		_pull()


func _pull() -> void:
	if game == null:
		return
	var changes: Dictionary = game.get_world_snapshot_changes(_version)
	if changes.is_empty():
		return
	var start := Time.get_ticks_usec()
	if changes["full"]:
		_rebuild(changes)
		last_full_build_ms = (Time.get_ticks_usec() - start) / 1000.0
	else:
		_apply_changes(changes)
		last_update_ms = (Time.get_ticks_usec() - start) / 1000.0
	_sync_creatures(changes["creatures"])
	_player = Vector2i(changes["player_x"], changes["player_y"])
	_player_label.position = InputIntents.floor_to_world(_player, 1.0, 0.6)
	_version = changes["version"]
	_update_camera()


func _rebuild(full: Dictionary) -> void:
	if _width != int(full["width"]) or _height != int(full["height"]) or catalog._terrains.is_empty():
		catalog.set_terrain_catalog(game.get_terrain_catalog())
		_terrain_visual_cache.clear()
	_width = full["width"]
	_height = full["height"]
	_terrain = full["terrain"]
	_flags = full["flags"]
	_tvals = full["object_tvals"]
	_symbols = full["symbols"]
	var count := _width * _height
	_grid_kind.resize(count)
	for label in _item_labels.values():
		label.queue_free()
	_item_labels.clear()

	# 全体はインスタンスを 1 つずつ設定せず、バッファ (変換 12 + 色 4 の float) をまとめて渡す
	var box_buffer := PackedFloat32Array()
	var plane_buffer := PackedFloat32Array()
	box_buffer.resize(count * 16)
	plane_buffer.resize(count * 16)
	for i in count:
		var placed := _compute_grid(i)
		_grid_kind[i] = placed[0]
		var target := box_buffer if placed[0] == KIND_BOX else plane_buffer
		if placed[0] != KIND_NONE:
			_write_instance(target, i, placed[1], placed[2])
		_update_item(i)
	_box_mm.instance_count = count
	_plane_mm.instance_count = count
	_box_mm.buffer = box_buffer
	_plane_mm.buffer = plane_buffer
	last_changed_grids = count


## バッファの i 番目に変換と色を書く (MultiMesh の TRANSFORM_3D + 色の並び: 3x4 の行優先 + RGBA)
static func _write_instance(buffer: PackedFloat32Array, i: int, xform: Transform3D, color: Color) -> void:
	var o := i * 16
	var b := xform.basis
	buffer[o + 0] = b.x.x
	buffer[o + 1] = b.y.x
	buffer[o + 2] = b.z.x
	buffer[o + 3] = xform.origin.x
	buffer[o + 4] = b.x.y
	buffer[o + 5] = b.y.y
	buffer[o + 6] = b.z.y
	buffer[o + 7] = xform.origin.y
	buffer[o + 8] = b.x.z
	buffer[o + 9] = b.y.z
	buffer[o + 10] = b.z.z
	buffer[o + 11] = xform.origin.z
	buffer[o + 12] = color.r
	buffer[o + 13] = color.g
	buffer[o + 14] = color.b
	buffer[o + 15] = color.a


func _apply_changes(changes: Dictionary) -> void:
	var indices: PackedInt32Array = changes["indices"]
	var terrain: PackedInt32Array = changes["terrain"]
	var flags: PackedByteArray = changes["flags"]
	var tvals: PackedByteArray = changes["object_tvals"]
	var symbols: PackedByteArray = changes["symbols"]
	for n in indices.size():
		var i := indices[n]
		_terrain[i] = terrain[n]
		_flags[i] = flags[n]
		_tvals[i] = tvals[n]
		for k in 4:
			_symbols[i * 4 + k] = symbols[n * 4 + k]
		_update_grid(i)
	last_changed_grids = indices.size()


## 地形 ID ごとに解決した見た目 [KIND_*, 大きさの Basis, 高さ, 色, 視界外の色]
## 地形の種類は数百しかないので、グリッドごとに解決し直さない
var _terrain_visual_cache := {}


func _terrain_visual(terrain_id: int) -> Array:
	var cached: Array = _terrain_visual_cache.get(terrain_id, [])
	if not cached.is_empty():
		return cached
	var visual := catalog.resolve_terrain(terrain_id)
	var kind: String = visual["kind"]
	var color: Color = visual.get("color", Color.WHITE)
	var height: float = visual.get("height", 1.0)
	var s: float = visual.get("scale", 1.0)
	var placed_kind := KIND_PLANE
	if kind == "box":
		placed_kind = KIND_BOX
	elif kind == "none":
		placed_kind = KIND_NONE
	# 板・文字のビルボード・未対応のメッシュは、いまは床の板で代用する
	if placed_kind == KIND_PLANE:
		color = color * PLANE_BRIGHTNESS
		color.a = 1.0
	cached = [placed_kind, Basis().scaled(Vector3(s, height, s)), height, color, color.darkened(0.55)]
	_terrain_visual_cache[terrain_id] = cached
	return cached


## 1 グリッドに何を置くか: [KIND_*, 変換, 色]
func _compute_grid(i: int) -> Array:
	var visual := _terrain_visual(_terrain[i])
	var kind: int = visual[0]
	if kind == KIND_NONE:
		return [KIND_NONE, HIDDEN, Color.BLACK]
	var color: Color = visual[3] if (_flags[i] & GRID_IN_VIEW) != 0 else visual[4]
	var center := Vector3((i % _width) + 0.5, 0.0, (i / _width) + 0.5)
	if kind == KIND_BOX:
		return [KIND_BOX, Transform3D(visual[1], center + Vector3(0, float(visual[2]) / 2.0, 0)), color]
	return [KIND_PLANE, Transform3D(Basis(), center), color]


## 1 グリッドの地形と床アイテムを描き直す (差分用)
func _update_grid(i: int) -> void:
	var placed := _compute_grid(i)
	_grid_kind[i] = placed[0]
	if placed[0] == KIND_BOX:
		_box_mm.set_instance_transform(i, placed[1])
		_box_mm.set_instance_color(i, placed[2])
		_plane_mm.set_instance_transform(i, HIDDEN)
	elif placed[0] == KIND_PLANE:
		_plane_mm.set_instance_transform(i, placed[1])
		_plane_mm.set_instance_color(i, placed[2])
		_box_mm.set_instance_transform(i, HIDDEN)
	else:
		_box_mm.set_instance_transform(i, HIDDEN)
		_plane_mm.set_instance_transform(i, HIDDEN)
	_update_item(i)


func _update_item(i: int) -> void:
	var has_item := (_flags[i] & GRID_OBJECT) != 0 and (_flags[i] & GRID_KNOWN) != 0
	if has_item:
		var item_visual := catalog.resolve_item(_tvals[i], char(_symbols[i * 4 + 1]), _symbols[i * 4])
		var label: Label3D = _item_labels.get(i)
		if label == null:
			label = _make_glyph(item_visual["glyph"], item_visual["color"], 0.6)
			_item_root.add_child(label)
			_item_labels[i] = label
		label.text = item_visual["glyph"]
		label.modulate = item_visual["color"]
		label.position = InputIntents.floor_to_world(Vector2i(i % _width, i / _width), 1.0, 0.3)
	elif _item_labels.has(i):
		_item_labels[i].queue_free()
		_item_labels.erase(i)


## クリーチャーのビルボードを表示用 ID ごとに保つ。隣へ動いたものは滑らかに動かす
func _sync_creatures(creatures: Array) -> void:
	var seen := {}
	for c: Dictionary in creatures:
		var id: int = c["id"]
		seen[id] = true
		var visual := catalog.resolve_creature(c)
		var color: Color = visual["color"]
		if (int(c["flags"]) & CREATURE_IN_VIEW) == 0:
			color = color.darkened(0.4)
		var target := InputIntents.floor_to_world(Vector2i(c["x"], c["y"]), 1.0, 0.6 * float(visual["scale"]))
		var label: Label3D = _creature_labels.get(id)
		if label == null:
			label = _make_glyph(visual["glyph"], color, float(visual["scale"]))
			_creature_root.add_child(label)
			_creature_labels[id] = label
			label.position = target
		else:
			label.text = visual["glyph"]
			label.modulate = color
			if label.position.distance_to(target) <= 1.5 and is_inside_tree():
				create_tween().tween_property(label, "position", target, MOVE_TWEEN_SEC)
			else:
				label.position = target
	for id in _creature_labels.keys():
		if not seen.has(id):
			_creature_labels[id].queue_free()
			_creature_labels.erase(id)


func _on_game_events(_events: Array) -> void:
	# 近接攻撃・ボルトの演出は今後の段で行う (#8805 など)
	pass


func _update_camera() -> void:
	var focus := InputIntents.floor_to_world(_player)
	var offset := Vector3(0, _camera_distance * 0.85, _camera_distance * 0.55).rotated(Vector3.UP, -_camera_yaw)
	_camera.position = focus + offset
	_camera.look_at(focus, Vector3.UP)


func _process(_delta: float) -> void:
	if not visible or game == null:
		return
	# 1 行目 (メッセージ・プロンプト) を重ねて見せる
	var terms := get_tree().root.find_children("*", "GodotTerminal", true, false)
	if not terms.is_empty():
		_message.text = terms[0].get_row_text(0).strip_edges()


func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_MIDDLE:
			_dragging = event.pressed
		elif event.pressed and event.shift_pressed and (event.button_index == MOUSE_BUTTON_WHEEL_UP or event.button_index == MOUSE_BUTTON_WHEEL_DOWN):
			_camera_distance = clampf(_camera_distance + (-1.5 if event.button_index == MOUSE_BUTTON_WHEEL_UP else 1.5), 4.0, 60.0)
			_update_camera()
		elif event.pressed and (event.button_index == MOUSE_BUTTON_LEFT or event.button_index == MOUSE_BUTTON_RIGHT):
			var pos := pick_floor(event.position)
			if pos.x >= 0:
				if event.button_index == MOUSE_BUTTON_LEFT:
					game.travel_to(pos.x, pos.y)
				else:
					game.target_grid(pos.x, pos.y)
		accept_event()
	elif event is InputEventMouseMotion and _dragging:
		_camera_yaw += event.relative.x * 0.01
		_update_camera()
		accept_event()


## 画面上の点を床の高さ (y = 0) の平面に投影してフロアの座標を求める。外れたら (-1, -1)
func pick_floor(screen_pos: Vector2) -> Vector2i:
	var from := _camera.project_ray_origin(screen_pos)
	var dir := _camera.project_ray_normal(screen_pos)
	if absf(dir.y) < 0.0001:
		return Vector2i(-1, -1)
	var t := -from.y / dir.y
	if t < 0:
		return Vector2i(-1, -1)
	var pos := InputIntents.world_to_floor(from + dir * t)
	if pos.x < 0 or pos.y < 0 or pos.x >= _width or pos.y >= _height:
		return Vector2i(-1, -1)
	return pos


## カメラの向き (input_intents.gd の rotate_direction に渡す)
func get_camera_yaw() -> float:
	return _camera_yaw


## 確認用の数 (箱・板として置いたグリッド、クリーチャー・床アイテムのビルボード)
func count_instances() -> Dictionary:
	var boxes := 0
	var planes := 0
	for kind in _grid_kind:
		if kind == KIND_BOX:
			boxes += 1
		elif kind == KIND_PLANE:
			planes += 1
	return { "boxes": boxes, "planes": planes, "creatures": _creature_labels.size(), "items": _item_labels.size() }
