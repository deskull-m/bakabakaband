extends SceneTree

## GDScript 側の純粋な補助関数の確認 (セーブ不要。CI でも実行する)
## 使い方: godot --headless --path godot_project --script "$PWD"/tools/godot/gdscript-unit-check.gd
const I = preload("res://scripts/input_intents.gd")
const VisualCatalog = preload("res://scripts/visual_catalog.gd")
const ItemWindow = preload("res://scripts/item_window.gd")
const DungeonView = preload("res://scripts/dungeon_view_3d.gd")
const SheetView = preload("res://scripts/character_sheet_view.gd")
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
	# 地形のテクスチャ
	cases.append([vc.resolve_terrain(1)["texture"], "floor"])
	# 末尾のカンマは取り除くが、文字列の中のカンマは残す
	cases.append([VisualCatalog.parse_jsonc('{ "mesh": "res://a,}.tres", "b": [1, ], }'), { "mesh": "res://a,}.tres", "b": [1.0] }])
	# 見つけた罠 (TRAP) は板に端末の文字を重ねる
	var trap_vc := VisualCatalog.new()
	trap_vc.definitions = VisualCatalog.parse_jsonc('{ "terrain": { "by_shape": { "TRAP": { "shape": "plane", "overlay_glyph": true } } } }')
	trap_vc.set_terrain_catalog([{ "id": 7, "tag": "TRAP_X", "shape": VisualCatalog.SHAPE_NAMES.find("TRAP"), "symbol": "^", "symbol_color": 3 }])
	var trap_visual := trap_vc.resolve_terrain(7)
	cases.append([[trap_visual["kind"], trap_visual["overlay_glyph"], trap_visual["glyph"]], ["plane", true, "^"]])
	cases.append([VisualCatalog.SHAPE_NAMES.find("TRAP"), 12])
	# タイル表示中の symbols (最上位ビット付きのタイルの行・列) は床アイテムの文字に使わない
	cases.append([DungeonView.item_fallback_symbol(0x85, 0x90), [DungeonView.ITEM_TILE_FALLBACK_GLYPH, DungeonView.ITEM_TILE_FALLBACK_COLOR]])
	cases.append([DungeonView.item_fallback_symbol(11, 0x21), ["!", 11]])
	cases.append([DungeonView.collect_texture_names({ "terrain": { "by_shape": { "A": { "texture": "x" }, "B": {} }, "by_tag": { "C": { "texture": "y" }, "D": { "texture": "x" } } } }), ["x", "y"]])
	var texture_names := DungeonView.collect_texture_names(vc.definitions)
	for name in texture_names:
		var image := DungeonView.load_texture_image(DungeonView.TERRAIN_TEXTURE_DIR + name + ".png")
		cases.append([[name, image != null and image.get_size() == Vector2i(DungeonView.TERRAIN_TEXTURE_SIZE, DungeonView.TERRAIN_TEXTURE_SIZE)], [name, true]])
	cases.append([texture_names.size() >= 10, true])
	# 端末の矩形を格子の行に直す (全角の右半分 0 は前の字の幅、色は下位 4 ビット)
	var region_lines := SheetView.region_to_lines({ "cols": 4, "rows": 2,
		"chars": PackedInt32Array([0x8150, 0, 65, 32, 32, 32, 32, 32]), "colors": PackedByteArray([0x11, 0x21, 3, 0, 0, 0, 0, 0]) })
	cases.append([region_lines.size(), 2])
	cases.append([region_lines[0]["cols"], PackedInt32Array([0, 2])])
	cases.append([region_lines[0]["widths"], PackedByteArray([2, 1])])
	cases.append([region_lines[0]["colors"], PackedByteArray([1, 3])])
	cases.append([region_lines[1]["chars"].size(), 0])
	# メッセージ・持ち物・装備のウィンドウ
	var white := func(_c: int) -> Color: return Color(1, 1, 1)
	cases.append([ItemWindow.escape_bbcode("a[b]c"), "a[lb]b]c"])
	cases.append([ItemWindow.format_messages({ "messages": PackedStringArray(["x", "[y]"]) }), "x\n[lb]y]"])
	cases.append([ItemWindow.clamp_position(Vector2(-5, 900), Vector2(100, 50), Vector2(400, 300)), Vector2(0, 276)])
	cases.append([ItemWindow.clamp_position(Vector2(380, 10), Vector2(100, 50), Vector2(400, 300)), Vector2(300, 10)])
	var item := { "label": "a", "name": "Dagger", "symbol": "|", "symbol_color": 1, "color": 1, "weight_text": "1.2 lb" }
	cases.append([ItemWindow.format_item_line("a", item, white), "a) [color=#ffffff]|[/color] [color=#ffffff]Dagger[/color]  [color=#a0a0a0]1.2 lb[/color]"])
	cases.append([ItemWindow.format_weight_footer({ "total_weight": 123, "weight_limit": 1500 }).contains("12.3 lb / 150.0 lb"), true])
	cases.append([ItemWindow.format_weight_footer({ "total_weight": 123, "weight_limit": 1500, "total_weight_text": "  5.6 kg", "weight_limit_text": " 68.0 kg" }).contains("5.6 kg / 68.0 kg"), true])
	cases.append([ItemWindow.format_inventory({}, white), ""])
	cases.append([ItemWindow.format_inventory({ "pack": [item], "total_weight": 12, "weight_limit": 100 }, white).contains("Dagger"), true])
	var equip := ItemWindow.format_equipment({ "equipment": [
		{ "label": "a", "slot_name": "Wielding", "can_equip": true, "item": item },
		{ "label": "b", "slot_name": "Shooting", "can_equip": true, "item": null },
		{ "label": "c", "slot_name": "Tail", "can_equip": false, "item": null },
	] }, white)
	cases.append([equip.split("\n").size(), 3])
	cases.append([equip.contains("(なし: Shooting)") and equip.contains("装備できない部位: Tail"), true])
	for c in cases:
		if c[0] != c[1]:
			ok = false
			print("NG ", c)
	print("RESULT: ", "OK" if ok else "NG")
	quit(0 if ok else 1)
