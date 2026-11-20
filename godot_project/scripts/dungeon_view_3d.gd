extends SubViewportContainer

## 3D の地図ビュー (最小構成、#9298)
##
## ワールドスナップショット (#9292/#9293) を受け、地形を MultiMesh、クリーチャー・プレイヤーを立てた
## 立体の文字、床アイテムを床に寝かせた立体の文字で描く。見た目は visual_catalog.gd (#9297) で決める。
## 地図を見ている間だけ view_switcher.gd (#9294) が見せる。
##
## - 地形はグリッドごとに MultiMesh のインスタンスを 1 つ割り当てる (箱用と板用の 2 つ)。
##   差分では変わったグリッドのインスタンスだけを書き換える
## - 座標は X 軸 = 東、Z 軸 = 南、1 グリッド = 1 (input_intents.gd と同じ)
## - 左クリックでトラベル、右クリックでターゲット (#9296)。中ボタンのドラッグでカメラを回し、
##   Shift + ホイールで寄る・引く
## - ターゲットの選択・場所の指定 (画面の状態が TARGETING) の間は、カーソルのマスに照準の輪を置き、
##   ターゲットの選択ではプレイヤーからの射線 (ゲーム側の observe_target_path()) をマスごとの印で描く。
##   カメラはプレイヤーとカーソルの中間を見る
## - クリーチャーとプレイヤーは文字を押し出した立体 (TextMesh) で立て、平行光で床に影を落とす。
##   足元にはグラデーションの円を敷き、どのグリッドにいるかを示す。文字はカメラの向きに合わせて回す
## - 視界・光源・記憶 (#9299): グリッドは「視界内で明るい」「視界内だが暗い」「記憶だけ」の 3 段階の
##   明るさで描き、未知のグリッドは描かない。盲目なら全て記憶だけとして描く。プレイヤーの光源と
##   光を持つモンスターには OmniLight3D を置く。見え方の判定はすべてゲーム側の値 (スナップショット)
##   に従い、3D の光の届き方で見えるかどうかを決めることはしない

const VisualCatalog = preload("res://scripts/visual_catalog.gd")
const TerrainShader = preload("res://shaders/terrain.gdshader")
## 地形のテクスチャの置き場所 (<名前>.png、tools/godot/gen-terrain-textures.py で生成)
const TERRAIN_TEXTURE_DIR := "res://assets/textures/terrain/"
const TERRAIN_TEXTURE_SIZE := 128
const InputIntents = preload("res://scripts/input_intents.gd")

## ObservedGridFlag / ObservedCreatureFlag
const GRID_KNOWN := 1 << 0
const GRID_IN_VIEW := 1 << 1
const GRID_LIT := 1 << 2
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
var _player_token: Node3D

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
## 箱も板も置かず、端末の文字のビルボードだけを立てる (定義が無い地形)
const KIND_GLYPH := 3
var _terrain_labels := {} ## グリッドの添字 → Label3D (文字で描く地形・文字を重ねる地形)
var _creature_tokens := {} ## id → Node3D (立体の文字と足元の円、_make_token)
var _creature_tweens := {} ## id → Tween (隣へ動く途中のもの)
var _item_labels := {} ## グリッドの添字 → Node3D (床に寝かせた立体の文字、_make_item_token)
var _player := Vector2i.ZERO
var _camera_yaw := 0.0
var _camera_distance := 22.0

## 床の板は端末の色そのままだと明るすぎて上のビルボードが見づらいので落とす
const PLANE_BRIGHTNESS := 0.35
## テクスチャを貼った板の明るさ (テクスチャ自体が暗い分を戻す)
const TEXTURED_PLANE_BRIGHTNESS := 0.6
## 視界内だが暗いグリッドの明るさ
const UNLIT_BRIGHTNESS := 0.55
## 記憶だけのグリッドの明るさと彩度
const MEMORY_BRIGHTNESS := 0.3
const MEMORY_SATURATION := 0.25
## 光を持つモンスターに置く光の上限 (多すぎると重い)
const MAX_CREATURE_LIGHTS := 16

var _blind := false
var _player_light: OmniLight3D
var _creature_lights := {} ## id → OmniLight3D
var _dragging := false

## テクスチャの名前 → Texture2DArray の層 (0 は無地)
var _texture_layers := {}
var _terrain_material: ShaderMaterial

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
	env.environment.ambient_light_color = Color(AMBIENT_LEVEL, AMBIENT_LEVEL, AMBIENT_LEVEL)
	_world.add_child(env)
	# 箱の面を見分けるための弱い平行光。明るさの段階はグリッドの色で表し、光源は OmniLight3D で添える
	# クリーチャーの立体の文字の影を床に落とす (地形は影を落とさない。_make_multimesh)
	var sun := DirectionalLight3D.new()
	sun.light_energy = SUN_ENERGY
	sun.rotation_degrees = Vector3(-60, 30, 0)
	sun.shadow_enabled = true
	sun.shadow_opacity = SHADOW_OPACITY
	sun.directional_shadow_mode = DirectionalLight3D.SHADOW_ORTHOGONAL
	sun.directional_shadow_max_distance = 60.0
	_world.add_child(sun)

	_camera = Camera3D.new()
	_camera.fov = 50.0
	_world.add_child(_camera)

	_terrain_material = ShaderMaterial.new()
	_terrain_material.shader = TerrainShader
	_box_mm = _make_multimesh(BoxMesh.new())
	_plane_mm = _make_multimesh(_make_plane_mesh())
	_creature_root = Node3D.new()
	_world.add_child(_creature_root)
	_item_root = Node3D.new()
	_world.add_child(_item_root)
	_disc_texture = _make_disc_texture()
	_token_outline_material = _make_token_outline_material()
	_player_token = _make_token("@", Color.WHITE, 1.0)
	_world.add_child(_player_token)
	_player_light = _make_light(Color(1.0, 0.85, 0.6))
	# プレイヤーの光源は周りのクリーチャーの影を落とす (地形は影を落とさないので軽い)
	_player_light.shadow_enabled = true
	_world.add_child(_player_light)
	_setup_target_markers()



func setup(game_node: Node) -> void:
	game = game_node
	catalog.load_definitions()
	catalog.set_color_provider(game.get_term_color)
	_load_terrain_textures()
	game.world_snapshot_changed.connect(_on_world_snapshot_changed)
	game.game_events.connect(_on_game_events)
	game.screen_state_changed.connect(_on_screen_state_changed)
	visibility_changed.connect(_on_visibility_changed)


func _make_plane_mesh() -> Mesh:
	var plane := PlaneMesh.new()
	plane.size = Vector2(1, 1)
	return plane


func _make_multimesh(mesh: Mesh) -> MultiMesh:
	var mm := MultiMesh.new()
	mm.transform_format = MultiMesh.TRANSFORM_3D
	mm.use_colors = true
	mm.use_custom_data = true
	mm.mesh = mesh
	mesh.surface_set_material(0, _terrain_material)
	var instance := MultiMeshInstance3D.new()
	instance.multimesh = mm
	# 壁が床に影を落とすと明るさの段階 (grid_color) が読みにくくなるので、影は受けるだけにする
	instance.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	_world.add_child(instance)
	return mm


## 見た目の定義で使われているテクスチャを読み、Texture2DArray にまとめてシェーダに渡す
## 層 0 は無地 (白)。読めない・大きさの違うテクスチャは無地で描く
func _load_terrain_textures() -> void:
	_texture_layers = {}
	var white := Image.create(TERRAIN_TEXTURE_SIZE, TERRAIN_TEXTURE_SIZE, false, Image.FORMAT_RGB8)
	white.fill(Color.WHITE)
	white.generate_mipmaps()
	var images: Array[Image] = [white]
	for name in collect_texture_names(catalog.definitions):
		var image := load_texture_image(TERRAIN_TEXTURE_DIR + name + ".png")
		if image == null or image.get_size() != Vector2i(TERRAIN_TEXTURE_SIZE, TERRAIN_TEXTURE_SIZE):
			push_warning("terrain texture not usable: " + name)
			continue
		_texture_layers[name] = images.size()
		images.append(image)
	var array := Texture2DArray.new()
	array.create_from_images(images)
	_terrain_material.set_shader_parameter("terrain_textures", array)


## 見た目の定義 (terrain.by_shape / by_tag) で使われているテクスチャの名前 (重複なし、出てきた順)
static func collect_texture_names(definitions: Dictionary) -> Array[String]:
	var names: Array[String] = []
	var terrain: Variant = definitions.get("terrain", {})
	if not (terrain is Dictionary):
		return names
	for section in ["by_shape", "by_tag"]:
		var table: Variant = terrain.get(section, {})
		if not (table is Dictionary):
			continue
		for entry in table.values():
			if entry is Dictionary and entry.has("texture"):
				var name := str(entry["texture"])
				if name != "" and not names.has(name):
					names.append(name)
	return names


## PNG を読む (インポートを通さないので、書き出し時は assets/textures を含めること)
static func load_texture_image(path: String) -> Image:
	if not FileAccess.file_exists(path):
		return null
	var image := Image.new()
	if image.load_png_from_buffer(FileAccess.get_file_as_bytes(path)) != OK:
		return null
	image.convert(Image.FORMAT_RGB8)
	image.generate_mipmaps()
	return image


func _make_light(color: Color) -> OmniLight3D:
	var light := OmniLight3D.new()
	light.light_color = color
	light.light_energy = 1.6
	light.omni_attenuation = 1.2
	light.shadow_enabled = false
	return light


## 光の半径 (グリッド) に合わせて光を置く。0 なら消す
static func _place_light(light: OmniLight3D, pos: Vector2i, radius: int) -> void:
	light.visible = radius > 0
	light.omni_range = radius + 0.75
	light.position = Vector3(pos.x + 0.5, 0.9, pos.y + 0.5)


## グリッドの明るさの段階に合わせて色を変える
static func grid_color(base: Color, flags: int, blind: bool) -> Color:
	var in_view := (flags & GRID_IN_VIEW) != 0 and not blind
	if in_view and (flags & GRID_LIT) != 0:
		return base
	if in_view:
		var dim := base * UNLIT_BRIGHTNESS
		dim.a = 1.0
		return dim
	var memory := Color.from_hsv(base.h, base.s * MEMORY_SATURATION, base.v * MEMORY_BRIGHTNESS)
	return memory


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


# ---- クリーチャーとプレイヤーの立体の文字・影・足元の円 ----

## 平行光と環境光の強さ。平行光を強めにして影が見えるようにし、そのぶん環境光を落とす
const SUN_ENERGY := 0.7
const AMBIENT_LEVEL := 0.38
const SHADOW_OPACITY := 1.0
## 立体の文字の大きさ (scale 1.0 で高さ約 0.8 グリッド)・押し出しの厚み・カメラ側から見た後ろへの傾き
const TOKEN_PIXEL_SIZE := 0.017
const TOKEN_FONT_SIZE := 48
const TOKEN_DEPTH := 0.12
const TOKEN_TILT_DEG := 15.0
## 文字が暗い所で沈まないよう、色の一部を自己発光させる
const TOKEN_EMISSION := 0.35
## 足元の円の直径 (グリッド = 1.0)・中心の濃さ・高さ (床の板より上、射線の印より下)
const DISC_SIZE := 0.95
const DISC_ALPHA := 0.35
const DISC_HEIGHT := 0.02

## 立体の文字の縁取り: 法線方向へ膨らませた裏面を黒で描く (背面法)。太さはグリッド = 1.0 の単位
const TOKEN_OUTLINE_WIDTH := 0.035
const TOKEN_OUTLINE_COLOR := Color(0, 0, 0)

var _disc_texture: Texture2D
var _token_outline_material: StandardMaterial3D


## 足元の円のテクスチャ: 中心から外へ透明になる放射状のグラデーション
static func _make_disc_texture() -> Texture2D:
	var gradient := Gradient.new()
	gradient.set_offset(0, 0.0)
	gradient.set_color(0, Color(1, 1, 1, 1))
	gradient.set_offset(1, 1.0)
	gradient.set_color(1, Color(1, 1, 1, 0))
	gradient.add_point(0.55, Color(1, 1, 1, 0.55))
	var texture := GradientTexture2D.new()
	texture.gradient = gradient
	texture.fill = GradientTexture2D.FILL_RADIAL
	texture.fill_from = Vector2(0.5, 0.5)
	texture.fill_to = Vector2(0.5, 0.0)
	texture.width = 64
	texture.height = 64
	return texture


## 縁取りの材質: 表を描かずに裏面だけを法線方向へ膨らませて描くので、文字の輪郭の外側に縁が出る
static func _make_token_outline_material() -> StandardMaterial3D:
	var outline := StandardMaterial3D.new()
	outline.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	outline.albedo_color = TOKEN_OUTLINE_COLOR
	outline.cull_mode = BaseMaterial3D.CULL_FRONT
	outline.grow = true
	outline.grow_amount = TOKEN_OUTLINE_WIDTH
	_draw_after_markers(outline)
	return outline


## 照準の輪・射線の印 (奥行きを無視する半透明、優先度 -1) より後に描き、印が文字を隠さないようにする。
## 不透明のままだと半透明より先に描かれ、奥行きを無視する印に上から塗られてしまう。
## 奥行きの先描き (depth pre-pass) なら半透明の扱いでも影を落とし、前後関係も保てる
static func _draw_after_markers(material: StandardMaterial3D) -> void:
	material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA_DEPTH_PRE_PASS
	material.render_priority = 0


## クリーチャー・プレイヤー 1 体分の節: 子 0 = 立体の文字 (Glyph)、子 1 = 足元の円 (Disc)。
## 節の位置はグリッドの床の中心 (高さ 0)
func _make_token(text: String, color: Color, scale_factor: float) -> Node3D:
	var token := Node3D.new()
	var glyph := MeshInstance3D.new()
	glyph.name = "Glyph"
	var mesh := TextMesh.new()
	mesh.font_size = TOKEN_FONT_SIZE
	mesh.depth = TOKEN_DEPTH
	mesh.vertical_alignment = VERTICAL_ALIGNMENT_BOTTOM
	glyph.mesh = mesh
	var material := StandardMaterial3D.new()
	material.roughness = 0.6
	material.emission_enabled = true
	_draw_after_markers(material)
	material.next_pass = _token_outline_material
	glyph.material_override = material
	glyph.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_ON
	token.add_child(glyph)

	var disc := MeshInstance3D.new()
	disc.name = "Disc"
	var plane := PlaneMesh.new()
	plane.size = Vector2(DISC_SIZE, DISC_SIZE)
	disc.mesh = plane
	var disc_material := StandardMaterial3D.new()
	disc_material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	disc_material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	disc_material.albedo_texture = _disc_texture
	disc.material_override = disc_material
	disc.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	disc.position = Vector3(0, DISC_HEIGHT, 0)
	token.add_child(disc)
	_set_token(token, text, color, scale_factor)
	_face_token_to_camera(token)
	return token


func _set_token(token: Node3D, text: String, color: Color, scale_factor: float) -> void:
	var glyph: MeshInstance3D = token.get_child(0)
	var mesh: TextMesh = glyph.mesh
	mesh.text = text
	mesh.pixel_size = TOKEN_PIXEL_SIZE * scale_factor
	var material: StandardMaterial3D = glyph.material_override
	material.albedo_color = color
	material.emission = color
	material.emission_energy_multiplier = TOKEN_EMISSION
	if token.get_child_count() > 1:
		var disc: MeshInstance3D = token.get_child(1)
		(disc.material_override as StandardMaterial3D).albedo_color = disc_color(color)


## 足元の円の色: クリーチャーの色を明るめに寄せ、中心の濃さを DISC_ALPHA にする
static func disc_color(color: Color) -> Color:
	var tint := color.lerp(Color.WHITE, 0.3)
	tint.a = DISC_ALPHA
	return tint


## 立体の文字をカメラの水平方向へ向け、少し後ろへ傾けて上から読みやすくする
func _face_token_to_camera(token: Node3D) -> void:
	var glyph: Node3D = token.get_child(0)
	glyph.rotation = Vector3(-deg_to_rad(TOKEN_TILT_DEG), -_camera_yaw, 0.0)


## 床アイテムの大きさ (クリーチャーの文字を 1.0 として) と、寝かせた文字の中心の高さ (厚みの半分だけ床から浮かせる)
const ITEM_TOKEN_SCALE := 0.75
const ITEM_TOKEN_HEIGHT := TOKEN_DEPTH * ITEM_TOKEN_SCALE * 0.5 + 0.01


## 床アイテム 1 つ分の節: 子 0 = 床に寝かせた立体の文字 (Glyph)。足元の円は付けない。
## 文字の上端がカメラの奥を向くよう寝かせるので、カメラから見て正しい向きで読める
func _make_item_token(text: String, color: Color) -> Node3D:
	var token := Node3D.new()
	var glyph := MeshInstance3D.new()
	glyph.name = "Glyph"
	var mesh := TextMesh.new()
	mesh.font_size = TOKEN_FONT_SIZE
	mesh.depth = TOKEN_DEPTH * ITEM_TOKEN_SCALE
	glyph.mesh = mesh
	var material := StandardMaterial3D.new()
	material.roughness = 0.6
	material.emission_enabled = true
	_draw_after_markers(material)
	material.next_pass = _token_outline_material
	glyph.material_override = material
	glyph.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_ON
	token.add_child(glyph)
	_set_token(token, text, color, ITEM_TOKEN_SCALE)
	_lay_item_token(token)
	return token


## 床アイテムの文字を床に寝かせ (表を上へ)、カメラの水平の向きに合わせて回す
func _lay_item_token(token: Node3D) -> void:
	var glyph: Node3D = token.get_child(0)
	glyph.rotation = Vector3(-PI / 2.0, -_camera_yaw, 0.0)


func _face_tokens_to_camera() -> void:
	if _player_token != null:
		_face_token_to_camera(_player_token)
	for token: Node3D in _creature_tokens.values():
		_face_token_to_camera(token)
	for token: Node3D in _item_labels.values():
		_lay_item_token(token)


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
	var blind: bool = changes.get("player_blind", false)
	var blind_changed := blind != _blind
	_blind = blind
	if changes["full"]:
		_rebuild(changes)
		last_full_build_ms = (Time.get_ticks_usec() - start) / 1000.0
	else:
		_apply_changes(changes)
		if blind_changed:
			# 盲目になった・治ったら、全グリッドの明るさが変わる
			for i in _width * _height:
				_update_grid(i)
		last_update_ms = (Time.get_ticks_usec() - start) / 1000.0
	_sync_creatures(changes["creatures"])
	_player = Vector2i(changes["player_x"], changes["player_y"])
	_player_token.position = InputIntents.floor_to_world(_player)
	_place_light(_player_light, _player, int(changes.get("player_light_radius", 0)) if not _blind else 0)
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
	for label in _terrain_labels.values():
		label.queue_free()
	_terrain_labels.clear()

	# 全体はインスタンスを 1 つずつ設定せず、バッファ (変換 12 + 色 4 の float) をまとめて渡す
	var box_buffer := PackedFloat32Array()
	var plane_buffer := PackedFloat32Array()
	box_buffer.resize(count * INSTANCE_STRIDE)
	plane_buffer.resize(count * INSTANCE_STRIDE)
	for i in count:
		var placed := _compute_grid(i)
		_grid_kind[i] = placed[0]
		var target := box_buffer if placed[0] == KIND_BOX else plane_buffer
		if placed[0] == KIND_BOX or placed[0] == KIND_PLANE:
			_write_instance(target, i, placed[1], placed[2], placed[3])
		else:
			# 置かないインスタンスは大きさ 0 にする (バッファの既定の 0 は変換も 0 だが、明示しておく)
			_write_instance(target, i, HIDDEN, Color.BLACK, 0)
		_update_terrain_label(i, placed)
		_update_item(i)
	_box_mm.instance_count = count
	_plane_mm.instance_count = count
	_box_mm.buffer = box_buffer
	_plane_mm.buffer = plane_buffer
	last_changed_grids = count


## MultiMesh の 1 インスタンスの float の数 (変換 12 + 色 4 + カスタムデータ 4)
const INSTANCE_STRIDE := 20

## バッファの i 番目に変換・色・テクスチャの層を書く
## (MultiMesh の TRANSFORM_3D + 色 + カスタムデータの並び: 3x4 の行優先 + RGBA + RGBA)
static func _write_instance(buffer: PackedFloat32Array, i: int, xform: Transform3D, color: Color, layer: int) -> void:
	var o := i * INSTANCE_STRIDE
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
	buffer[o + 16] = float(layer)
	buffer[o + 17] = 0.0
	buffer[o + 18] = 0.0
	buffer[o + 19] = 0.0


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


## 地形 ID ごとに解決した見た目 [KIND_*, 大きさの Basis, 高さ, 色, テクスチャの層, 文字, 文字の色]
## 文字は KIND_GLYPH の地形と、定義で "overlay_glyph" を指定した地形 (罠など) だけが持つ ("" なら立てない)
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
	elif kind == "glyph":
		placed_kind = KIND_GLYPH
	var glyph := ""
	if placed_kind == KIND_GLYPH or visual.get("overlay_glyph", false):
		glyph = visual.get("glyph", "")
	var glyph_color := color
	var layer: int = _texture_layers.get(visual.get("texture", ""), 0)
	# 未対応のメッシュ・スプライトは、いまは床の板で代用する
	if placed_kind == KIND_PLANE:
		color = color * (TEXTURED_PLANE_BRIGHTNESS if layer > 0 else PLANE_BRIGHTNESS)
		color.a = 1.0
	cached = [placed_kind, Basis().scaled(Vector3(s, height, s)), height, color, layer, glyph, glyph_color]
	_terrain_visual_cache[terrain_id] = cached
	return cached


## 1 グリッドに何を置くか: [KIND_*, 変換, 色, テクスチャの層, 文字, 文字の色, 文字の高さ]
func _compute_grid(i: int) -> Array:
	var visual := _terrain_visual(_terrain[i])
	var kind: int = visual[0]
	if kind == KIND_NONE:
		return [KIND_NONE, HIDDEN, Color.BLACK, 0, "", Color.BLACK, 0.0]
	var color := grid_color(visual[3], _flags[i], _blind)
	var glyph_color := grid_color(visual[6], _flags[i], _blind)
	var center := Vector3((i % _width) + 0.5, 0.0, (i / _width) + 0.5)
	if kind == KIND_BOX:
		return [KIND_BOX, Transform3D(visual[1], center + Vector3(0, float(visual[2]) / 2.0, 0)), color, visual[4], visual[5], glyph_color, float(visual[2]) + TERRAIN_GLYPH_HEIGHT]
	if kind == KIND_GLYPH:
		return [KIND_GLYPH, HIDDEN, color, 0, visual[5], glyph_color, TERRAIN_GLYPH_HEIGHT]
	return [KIND_PLANE, Transform3D(Basis(), center), color, visual[4], visual[5], glyph_color, TERRAIN_GLYPH_HEIGHT]


## 地形の文字のビルボードの高さ (床アイテムの文字より低くし、同じグリッドで重ならないようにする)
const TERRAIN_GLYPH_HEIGHT := 0.15


## 地形の文字のビルボードを立てる・外す (_compute_grid の結果に従う)
func _update_terrain_label(i: int, placed: Array) -> void:
	var glyph: String = placed[4]
	if glyph == "":
		if _terrain_labels.has(i):
			_terrain_labels[i].queue_free()
			_terrain_labels.erase(i)
		return
	var label: Label3D = _terrain_labels.get(i)
	if label == null:
		label = _make_glyph(glyph, placed[5], 0.6)
		_item_root.add_child(label)
		_terrain_labels[i] = label
	label.text = glyph
	label.modulate = placed[5]
	label.position = InputIntents.floor_to_world(Vector2i(i % _width, i / _width), 1.0, placed[6])


## 1 グリッドの地形と床アイテムを描き直す (差分用)
func _update_grid(i: int) -> void:
	var placed := _compute_grid(i)
	_grid_kind[i] = placed[0]
	if placed[0] == KIND_BOX:
		_box_mm.set_instance_transform(i, placed[1])
		_box_mm.set_instance_color(i, placed[2])
		_box_mm.set_instance_custom_data(i, Color(float(placed[3]), 0, 0, 0))
		_plane_mm.set_instance_transform(i, HIDDEN)
	elif placed[0] == KIND_PLANE:
		_plane_mm.set_instance_transform(i, placed[1])
		_plane_mm.set_instance_color(i, placed[2])
		_plane_mm.set_instance_custom_data(i, Color(float(placed[3]), 0, 0, 0))
		_box_mm.set_instance_transform(i, HIDDEN)
	else:
		_box_mm.set_instance_transform(i, HIDDEN)
		_plane_mm.set_instance_transform(i, HIDDEN)
	_update_terrain_label(i, placed)
	_update_item(i)


## タイル表示中に symbols の代わりに使う床アイテムの文字と色 (色は端末の白)
const ITEM_TILE_FALLBACK_GLYPH := "&"
const ITEM_TILE_FALLBACK_COLOR := 1


## symbols の色・文字の組がタイルの行・列か (タイル表示中は両方の最上位ビットが立つ)
static func is_tile_symbol(color_byte: int, char_byte: int) -> bool:
	return (color_byte & 0x80) != 0 and (char_byte & 0x80) != 0


## 床アイテムの文字と色 (symbols の前景)。タイル表示中は symbols がタイルの位置なので使わない
static func item_fallback_symbol(color_byte: int, char_byte: int) -> Array:
	if is_tile_symbol(color_byte, char_byte):
		return [ITEM_TILE_FALLBACK_GLYPH, ITEM_TILE_FALLBACK_COLOR]
	return [char(char_byte), color_byte]


func _update_item(i: int) -> void:
	# 見つけた床アイテムは地形を知らないグリッドにも在りうる (感知など) ので、地形の既知は問わない
	var has_item := (_flags[i] & GRID_OBJECT) != 0
	if has_item:
		var fallback := item_fallback_symbol(_symbols[i * 4], _symbols[i * 4 + 1])
		var item_visual := catalog.resolve_item(_tvals[i], fallback[0], fallback[1])
		var label: Node3D = _item_labels.get(i)
		if label == null:
			label = _make_item_token(item_visual["glyph"], item_visual["color"])
			_item_root.add_child(label)
			_item_labels[i] = label
		else:
			_set_token(label, item_visual["glyph"], item_visual["color"], ITEM_TOKEN_SCALE)
		label.position = InputIntents.floor_to_world(Vector2i(i % _width, i / _width), 1.0, ITEM_TOKEN_HEIGHT)
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
		var target := InputIntents.floor_to_world(Vector2i(c["x"], c["y"]))
		var label: Node3D = _creature_tokens.get(id)
		if label == null:
			label = _make_token(visual["glyph"], color, float(visual["scale"]))
			_creature_root.add_child(label)
			_creature_tokens[id] = label
			label.position = target
		else:
			_set_token(label, visual["glyph"], color, float(visual["scale"]))
			# 前の移動の途中なら止める (止めないと、テレポート後に古い行き先へ戻される)
			var previous: Tween = _creature_tweens.get(id)
			if previous != null:
				previous.kill()
				_creature_tweens.erase(id)
			if label.position.distance_to(target) <= 1.5 and is_inside_tree():
				var tween := create_tween()
				tween.tween_property(label, "position", target, MOVE_TWEEN_SEC)
				_creature_tweens[id] = tween
			else:
				label.position = target
	for id in _creature_tokens.keys():
		if not seen.has(id):
			var tween: Tween = _creature_tweens.get(id)
			if tween != null:
				tween.kill()
				_creature_tweens.erase(id)
			_creature_tokens[id].queue_free()
			_creature_tokens.erase(id)
	_sync_creature_lights(creatures)


## 光を持つ見えているモンスターに光を置く (上限 MAX_CREATURE_LIGHTS)
func _sync_creature_lights(creatures: Array) -> void:
	var lit := {}
	for c: Dictionary in creatures:
		var radius: int = c.get("light_radius", 0)
		if radius <= 0 or lit.size() >= MAX_CREATURE_LIGHTS:
			continue
		var id: int = c["id"]
		lit[id] = true
		var light: OmniLight3D = _creature_lights.get(id)
		if light == null:
			light = _make_light(Color(1.0, 0.9, 0.7))
			_world.add_child(light)
			_creature_lights[id] = light
		_place_light(light, Vector2i(c["x"], c["y"]), radius)
	for id in _creature_lights.keys():
		if not lit.has(id):
			_creature_lights[id].queue_free()
			_creature_lights.erase(id)


func _on_game_events(_events: Array) -> void:
	# 近接攻撃・ボルトの演出は今後の段で行う (#8805 など)
	pass


## カメラの位置: 注視点からの向き (縦・奥行きの比)。距離はこれに _camera_distance を掛ける
const CAMERA_OFFSET_DIR := Vector2(0.85, 0.55)


func _update_camera() -> void:
	var player := InputIntents.floor_to_world(_player)
	var cursor := InputIntents.floor_to_world(_cursor)
	var focus := camera_focus(player, cursor, _cursor_shown)
	# カーソルを出している間は、プレイヤーとカーソルの両方が画面に収まるまで引く。
	# _camera_distance (利用者が決めた寄り具合) は変えないので、選び終えると元の寄り具合に戻る
	var distance := _camera_distance
	if _cursor_shown and size.y > 0.0:
		distance = targeting_camera_distance(_camera_distance, player, cursor, _camera_yaw, _camera.fov, size.x / size.y)
	var offset := Vector3(0, distance * CAMERA_OFFSET_DIR.x, distance * CAMERA_OFFSET_DIR.y).rotated(Vector3.UP, -_camera_yaw)
	_camera.position = focus + offset
	_camera.look_at(focus, Vector3.UP)
	_face_tokens_to_camera()



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


## 確認用の数 (箱・板として置いたグリッド、クリーチャー・床アイテムの立体の文字)
func count_instances() -> Dictionary:
	var boxes := 0
	var planes := 0
	for kind in _grid_kind:
		if kind == KIND_BOX:
			boxes += 1
		elif kind == KIND_PLANE:
			planes += 1
	return {
		"boxes": boxes, "planes": planes, "creatures": _creature_tokens.size(), "items": _item_labels.size(), "terrain_glyphs": _terrain_labels.size(),
		"player_light": _player_light.omni_range if _player_light.visible else 0.0, "creature_lights": _creature_lights.size(),
	}


# ---- ターゲットの選択・場所の指定のカーソルと射線 ----

## InputKind (src/io/input-context.h) の値
const INPUT_KIND_TARGET := 8
const INPUT_KIND_POINT := 9
## ScreenState の TARGETING
const SCREEN_TARGETING := 3
## 照準の輪の色: ターゲットの選択 / 場所の指定
const CURSOR_TARGET_COLOR := Color(1.0, 0.85, 0.2)
const CURSOR_POINT_COLOR := Color(0.35, 1.0, 0.5)
## 射線の印の色: 届く区間 / 見えているクリーチャーの位置 (ボルトはここで当たる) / その先 / 壁などで止まる位置
const PATH_COLOR := Color(1.0, 0.9, 0.35, 0.8)
const PATH_CREATURE_COLOR := Color(1.0, 0.45, 0.15, 0.95)
const PATH_BEYOND_COLOR := Color(0.75, 0.65, 0.4, 0.55)
const PATH_BLOCKED_COLOR := Color(1.0, 0.15, 0.15, 0.95)
## 射線の印の大きさ (グリッド = 1.0) と高さ (床の板・テクスチャより少し上)
const PATH_MARK_SIZE := 0.32
const MARKER_HEIGHT := 0.04
## 射線の印の上限 (ゲームの射程 MAX_RANGE より十分大きい)
const MAX_PATH_MARKS := 64

var _cursor := Vector2i.ZERO
var _cursor_shown := false
var _cursor_ring: MeshInstance3D
var _cursor_material: StandardMaterial3D
var _path_mm: MultiMesh
var _marker_time := 0.0


func _setup_target_markers() -> void:
	var torus := TorusMesh.new()
	torus.inner_radius = 0.34
	torus.outer_radius = 0.48
	torus.rings = 32
	torus.ring_segments = 8
	_cursor_material = StandardMaterial3D.new()
	_cursor_material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	_cursor_material.albedo_color = CURSOR_TARGET_COLOR
	# 壁や床に埋もれないよう奥行きを無視して描く。文字のビルボード (優先度 0) より先に描き、
	# カーソルのマスにいるクリーチャーの文字を隠さない
	_cursor_material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	_cursor_material.no_depth_test = true
	_cursor_material.render_priority = -1
	torus.surface_set_material(0, _cursor_material)
	_cursor_ring = MeshInstance3D.new()
	_cursor_ring.mesh = torus
	_cursor_ring.scale = Vector3(1.0, 0.25, 1.0)
	_cursor_ring.visible = false
	_world.add_child(_cursor_ring)

	var mark := PlaneMesh.new()
	mark.size = Vector2(PATH_MARK_SIZE, PATH_MARK_SIZE)
	var mark_material := StandardMaterial3D.new()
	mark_material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	mark_material.vertex_color_use_as_albedo = true
	mark_material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	mark_material.no_depth_test = true
	mark_material.render_priority = -1
	mark.surface_set_material(0, mark_material)
	_path_mm = MultiMesh.new()
	_path_mm.transform_format = MultiMesh.TRANSFORM_3D
	_path_mm.use_colors = true
	_path_mm.mesh = mark
	_path_mm.instance_count = MAX_PATH_MARKS
	_path_mm.visible_instance_count = 0
	var path_instance := MultiMeshInstance3D.new()
	path_instance.multimesh = _path_mm
	_world.add_child(path_instance)


func _on_screen_state_changed(state: Dictionary) -> void:
	apply_target_state(state)


## 画面の状態からカーソルと射線を置き直す (TARGETING でなければ隠す)
func apply_target_state(state: Dictionary) -> void:
	if _cursor_ring == null:
		return
	var targeting := int(state.get("state", 0)) == SCREEN_TARGETING and bool(state.get("has_cursor", false))
	_cursor_shown = targeting
	_cursor_ring.visible = targeting
	if not targeting:
		_path_mm.visible_instance_count = 0
		_update_camera()
		return
	_cursor = Vector2i(int(state["cursor_x"]), int(state["cursor_y"]))
	_cursor_material.albedo_color = CURSOR_POINT_COLOR if int(state.get("input_kind", -1)) == INPUT_KIND_POINT else CURSOR_TARGET_COLOR
	_cursor_ring.position = InputIntents.floor_to_world(_cursor, 1.0, MARKER_HEIGHT)

	var path: PackedInt32Array = state.get("target_path", PackedInt32Array())
	var count := mini(path.size() / 2, MAX_PATH_MARKS)
	var colors := target_path_colors(count, int(state.get("target_path_creature_index", -1)), bool(state.get("target_path_blocked", false)))
	for i in count:
		var pos := Vector2i(path[i * 2], path[i * 2 + 1])
		_path_mm.set_instance_transform(i, Transform3D(Basis(), InputIntents.floor_to_world(pos, 1.0, MARKER_HEIGHT)))
		_path_mm.set_instance_color(i, colors[i])
	_path_mm.visible_instance_count = count
	_update_camera()


## 射線の印の色。見えているクリーチャー (creature_index) までが届く区間で、その先は薄く描く。
## 記憶している壁などで止まる (blocked) なら最後の印を赤にする
static func target_path_colors(count: int, creature_index: int, blocked: bool) -> Array[Color]:
	var colors: Array[Color] = []
	for i in count:
		var color := PATH_COLOR
		if creature_index >= 0 and i == creature_index:
			color = PATH_CREATURE_COLOR
		elif creature_index >= 0 and i > creature_index:
			color = PATH_BEYOND_COLOR
		if blocked and i == count - 1:
			color = PATH_BLOCKED_COLOR
		colors.append(color)
	return colors


## ターゲットの選択中のカメラの距離: プレイヤーとカーソルの両方 (と周り TARGET_VIEW_MARGIN グリッド) が
## 画面に収まる距離と、利用者の距離 base の大きい方。注視点は両者の中間 (camera_focus)
const TARGET_VIEW_MARGIN := 1.5


static func targeting_camera_distance(base: float, player: Vector3, cursor: Vector3, yaw: float, fov_deg: float, aspect: float) -> float:
	# カメラから見た左右 (x) と奥行き (z) に直す
	var delta := (cursor - player).rotated(Vector3.UP, yaw)
	var half_x := absf(delta.x) * 0.5 + TARGET_VIEW_MARGIN
	var half_z := absf(delta.z) * 0.5 + TARGET_VIEW_MARGIN
	var pitch := atan2(CAMERA_OFFSET_DIR.x, CAMERA_OFFSET_DIR.y)
	var tan_v := tan(deg_to_rad(fov_deg) * 0.5)
	var tan_h := tan_v * maxf(aspect, 0.1)
	# 手前側の点はカメラに近く見える範囲が狭いので、近い側で収まる距離 (注視点からカメラまで) を求める
	var near := half_z * cos(pitch)
	var need := maxf(near + half_z * sin(pitch) / tan_v, near + half_x / tan_h)
	return maxf(base, need / CAMERA_OFFSET_DIR.length())


## カメラの注視点: カーソルを出している間はプレイヤーとカーソルの中間 (遠くのマスを選んでも画面に収まる)
static func camera_focus(player: Vector3, cursor: Vector3, cursor_shown: bool) -> Vector3:
	return (player + cursor) * 0.5 if cursor_shown else player


func _process(delta: float) -> void:
	if _cursor_ring == null or not _cursor_ring.visible:
		return
	# 照準の輪をゆっくり回して脈打たせ、どこにあるか目に付くようにする
	_marker_time += delta
	_cursor_ring.rotation.y = _marker_time * 1.5
	var pulse := 1.0 + 0.12 * sin(_marker_time * 6.0)
	_cursor_ring.scale = Vector3(pulse, 0.25, pulse)


## 確認用: 照準の輪と射線の印の状態
func get_target_markers() -> Dictionary:
	return {
		"cursor_shown": _cursor_ring != null and _cursor_ring.visible,
		"cursor": _cursor,
		"path_marks": _path_mm.visible_instance_count if _path_mm else 0,
	}
