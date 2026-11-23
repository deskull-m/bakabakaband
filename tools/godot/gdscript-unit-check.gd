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
	# 地図の上に重ねた一覧 (OVERLAY_MENU) の間も 3D を見せ、範囲は get_screen_state() の overlay_* から作る
	const VS = preload("res://scripts/screen_overlay.gd")
	var overlay_state := { "state": 4, "overlay_x": 2, "overlay_y": 1, "overlay_w": 30, "overlay_h": 8 }
	cases.append([[VS.shows_map_view(overlay_state, true), VS.shows_map_view(overlay_state, false), VS.shows_map_view({ "state": 5 }, true), VS.shows_map_view({ "state": 1 }, false), VS.shows_map_view({}, true)], [true, false, false, true, false]])
	# 一覧の範囲が求まらない (大きさ 0) なら、窓に写せないので端末を見せる
	cases.append([VS.shows_map_view({ "state": 4, "overlay_w": 0, "overlay_h": 0 }, true), false])
	cases.append([VS.overlay_rect({ "state": 4, "overlay_x": 13, "overlay_y": 1, "overlay_w": 40, "overlay_h": 8 }), Rect2i(13, 1, 40, 8)])
	cases.append([VS.overlay_rect({ "state": 1, "overlay_x": 13, "overlay_y": 1, "overlay_w": 40, "overlay_h": 8 }), Rect2i()])
	# 一覧の窓は端末と同じ位置に置き、画面の右端・下端からはみ出さないよう寄せる
	const SB = preload("res://scripts/status_sidebar.gd")
	cases.append([SB.overlay_position(Rect2i(13, 1, 40, 8), Vector2(10, 20), Vector2(400, 160), Vector2(1280, 800)), Vector2(130, 28)])
	cases.append([SB.overlay_position(Rect2i(100, 30, 40, 8), Vector2(10, 20), Vector2(400, 160), Vector2(1280, 800)), Vector2(876, 800 - SB.BOTTOM_MARGIN - 160)])
	# 射線の印の色: クリーチャーの位置で当たり、その先は薄く、壁で止まる最後の印は赤
	var path_colors := DungeonView.target_path_colors(4, 1, true)
	cases.append([path_colors.map(func(c): return c), [DungeonView.PATH_COLOR, DungeonView.PATH_CREATURE_COLOR, DungeonView.PATH_BEYOND_COLOR, DungeonView.PATH_BLOCKED_COLOR]])
	cases.append([DungeonView.target_path_colors(2, -1, false).map(func(c): return c), [DungeonView.PATH_COLOR, DungeonView.PATH_COLOR]])
	# カーソルを出している間のカメラはプレイヤーとカーソルの中間を見る
	cases.append([DungeonView.camera_focus(Vector3(0, 0, 0), Vector3(10, 0, 4), true), Vector3(5, 0, 2)])
	cases.append([DungeonView.camera_focus(Vector3(1, 0, 1), Vector3(10, 0, 4), false), Vector3(1, 0, 1)])
	# 照準中は両方が収まるまで引く (寄っていても 10 マス先のカーソルが画面に入る)。近ければ利用者の距離のまま
	cases.append([DungeonView.targeting_camera_distance(4.0, Vector3(0, 0, 0), Vector3(0, 0, 10), 0.0, 50.0, 1.6) > 10.0, true])
	cases.append([DungeonView.targeting_camera_distance(22.0, Vector3(0, 0, 0), Vector3(1, 0, 0), 0.0, 50.0, 1.6), 22.0])
	cases.append([is_equal_approx(DungeonView.disc_color(Color(1, 0, 0)).a, DungeonView.DISC_ALPHA), true])
	cases.append([DungeonView.disc_color(Color(0, 0, 0)).r > 0.0, true])
	# タイトル画面 (#9317): 画像は左下の左半分、ボタンは右下の右半分、news は画像の上で終わる
	const Title = preload("res://scripts/title_layout.gd")
	var title_vp := Vector2(1494, 885)
	var title_rect: Rect2 = Title.title_image_rect(title_vp, 1560.0 / 1393.0)
	var buttons_rect: Rect2 = Title.button_area_rect(title_vp, Title.BUTTON_SIZE, 2, Title.BUTTON_SEPARATION)
	var news_rect: Rect2 = Title.news_area_rect(title_vp, title_rect)
	cases.append([title_rect.end.x <= title_vp.x * 0.5, true])
	# サブタイトルの下のバージョンの表記が画面の下端 (余白) に届く
	cases.append([is_equal_approx(Title.version_rect(title_rect).end.y, title_vp.y - Title.MARGIN), true])
	cases.append([buttons_rect.position.x >= title_vp.x * 0.5 and is_equal_approx(buttons_rect.end.x, title_vp.x - Title.MARGIN) and is_equal_approx(buttons_rect.end.y, title_vp.y - Title.MARGIN), true])
	cases.append([news_rect.end.y < Title.title_content_top(title_rect) and news_rect.position.y >= 0.0, true])
	# news は区切りの「*」の行より後ろだけを出す (先頭の空行も除く)
	cases.append([Title.news_body("\n  ****\n\n  ****\n\n  Based on Moria\n\n  URL\n"), "  Based on Moria\n\n  URL"])
	cases.append([Title.news_body("no stars\n"), "no stars"])
	# 狭い画面で news が入りきらなければ入る行数までにする (タイトル画像に被せない)
	cases.append([[Title.news_visible_lines(48.0, 12.0, 18), Title.news_visible_lines(400.0, 12.0, 18), Title.news_visible_lines(-5.0, 12.0, 18)], [4, 18, 0]])
	# 狭い画面でもボタンは右半分からはみ出さない
	cases.append([Title.button_area_rect(Vector2(800, 600), Title.BUTTON_SIZE, 2, Title.BUTTON_SEPARATION).end.x <= 800.0 - Title.MARGIN + 0.01, true])
	# 起動時の演出 (#9319): ロゴは画面の中央で横幅 33%。キーで飛ぶ先はロゴ → 標語 → タイトル
	const Splash = preload("res://scripts/splash_sequence.gd")
	var logo_rect: Rect2 = Splash.logo_rect(title_vp, 1200.0 / 525.0)
	cases.append([is_equal_approx(logo_rect.size.x, title_vp.x * 0.33) and logo_rect.get_center().is_equal_approx(title_vp * 0.5), true])
	cases.append([Splash.logo_rect(Vector2(2000, 300), 1200.0 / 525.0).size.y <= 300.0 * Splash.LOGO_MAX_HEIGHT_RATIO + 0.01, true])
	cases.append([[Splash.skip_target(Splash.Phase.LOGO), Splash.skip_target(Splash.Phase.MOTTO), Splash.skip_target(Splash.Phase.TITLE)], [Splash.Phase.MOTTO, Splash.Phase.DONE, Splash.Phase.DONE]])
	# 標語は収まるときは縮めず、収まらないときは画面の横幅 (余白を除く) に合わせて縮める
	cases.append([[Splash.motto_font_scale(500.0, 1280.0), is_equal_approx(Splash.motto_font_scale(704.0, 400.0), (400.0 - Splash.MOTTO_MARGIN * 2.0) / 704.0), Splash.motto_font_scale(0.0, 100.0)], [1.0, true, 1.0]])
	# ロード一覧 (#9320): 並べ替え・見出しを押したときの向き・プレイ時間の表示
	const SaveListHelper = preload("res://scripts/save_list.gd")
	var saves := [
		{ "name": "b", "level": 16, "race": "ドラコニアン", "play_time": 64 },
		{ "name": "a", "level": 1, "race": "人間", "play_time": -1 },
		{ "name": "c", "level": -1, "race": "", "play_time": 7200 },
		{ "name": "d", "level": 16, "race": "エルフ", "play_time": 5 },
	]
	var names_of := func(list: Array) -> Array: return list.map(func(e: Dictionary) -> String: return e["name"])
	cases.append([names_of.call(SaveListHelper.sort_entries(saves, SaveListHelper.Column.LEVEL, false)), ["b", "d", "a", "c"]])
	cases.append([names_of.call(SaveListHelper.sort_entries(saves, SaveListHelper.Column.LEVEL, true)), ["a", "b", "d", "c"]])
	cases.append([names_of.call(SaveListHelper.sort_entries(saves, SaveListHelper.Column.PLAY_TIME, true)), ["d", "b", "c", "a"]])
	cases.append([names_of.call(SaveListHelper.sort_entries(saves, SaveListHelper.Column.NAME, false)), ["d", "c", "b", "a"]])
	cases.append([SaveListHelper.next_sort(SaveListHelper.Column.LEVEL, false, SaveListHelper.Column.LEVEL), [SaveListHelper.Column.LEVEL, true]])
	cases.append([SaveListHelper.next_sort(SaveListHelper.Column.LEVEL, false, SaveListHelper.Column.NAME), [SaveListHelper.Column.NAME, true]])
	cases.append([SaveListHelper.next_sort(SaveListHelper.Column.NAME, true, SaveListHelper.Column.PLAY_TIME), [SaveListHelper.Column.PLAY_TIME, false]])
	cases.append([[SaveListHelper.format_play_time(64), SaveListHelper.format_play_time(36125), SaveListHelper.format_play_time(-1)], ["0:01:04", "10:02:05", "?"]])
	cases.append([[SaveListHelper.cell_text(saves[2], SaveListHelper.Column.LEVEL), SaveListHelper.cell_text(saves[2], SaveListHelper.Column.RACE)], ["?", "?"]])
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
	cases.append([DungeonView.item_fallback_symbol(0, 0), [DungeonView.ITEM_TILE_FALLBACK_GLYPH, DungeonView.ITEM_TILE_FALLBACK_COLOR]])
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
