extends SceneTree

## GDScript 側の純粋な補助関数の確認 (セーブ不要。CI でも実行する)
## 使い方: godot --headless --path godot_project --script "$PWD"/tools/godot/gdscript-unit-check.gd
const I = preload("res://scripts/input_intents.gd")
const VisualCatalog = preload("res://scripts/visual_catalog.gd")
func _initialize() -> void:
	var ok := true
	var cases := [
		[I.rotate_direction(8, 0.0), 8], [I.rotate_direction(8, PI / 2), 6], [I.rotate_direction(8, PI), 2],
		[I.rotate_direction(6, -PI / 2), 8], [I.rotate_direction(7, PI / 4), 8], [I.rotate_direction(5, 1.0), 5],
		[I.direction_from_vector(Vector2(0, -1)), 8], [I.direction_from_vector(Vector2(1, 1)), 3],
		[I.direction_from_vector(Vector2(-1, 0)), 4], [I.direction_from_vector(Vector2.ZERO), 0],
		[I.world_to_floor(I.floor_to_world(Vector2i(12, 34))), Vector2i(12, 34)],
	]
	# 見た目の定義 (#9297)
	var parsed = VisualCatalog.parse_jsonc('{ // c\n "a": "x//y", /* b */ "b": [1, 2,], }')
	cases.append([parsed, { "a": "x//y", "b": [1.0, 2.0] }])
	var vc := VisualCatalog.new()
	cases.append([vc.load_definitions(), true])
	vc.set_terrain_catalog([
		{ "id": 1, "tag": "FLOOR", "shape": 1, "symbol": ".", "symbol_color": 1 },
		{ "id": 2, "tag": "PERMANENT", "shape": 2, "symbol": "#", "symbol_color": 1 },
		{ "id": 3, "tag": "NO_SUCH", "shape": 0, "symbol": "x", "symbol_color": 3 },
	])
	cases.append([vc.resolve_terrain(1)["source"], "by_shape"])
	cases.append([vc.resolve_terrain(1)["kind"], "plane"])
	cases.append([vc.resolve_terrain(2)["source"], "by_tag"])
	cases.append([vc.resolve_terrain(2)["height"], 1.2])
	cases.append([vc.resolve_terrain(3)["kind"], "glyph"])
	cases.append([vc.resolve_terrain(3)["glyph"], "x"])
	cases.append([vc.resolve_terrain(-1)["kind"], "none"])
	cases.append([vc.resolve_creature({ "race_id": 5, "symbol": "D", "symbol_color": 4 })["source"], "by_symbol"])
	cases.append([vc.resolve_creature({ "race_id": 5, "symbol": "D", "symbol_color": 4 })["scale"], 1.8])
	cases.append([vc.resolve_creature({ "race_id": 5, "symbol": "k", "symbol_color": 4 })["kind"], "glyph"])
	cases.append([vc.resolve_creature({ "race_id": -1 })["glyph"], "?"])
	cases.append([VisualCatalog.tile_region(0x83, 0x82, Vector2i(16, 16)), Rect2i(32, 48, 16, 16)])
	for c in cases:
		if c[0] != c[1]:
			ok = false
			print("NG ", c)
	print("RESULT: ", "OK" if ok else "NG")
	quit(0 if ok else 1)
