extends SceneTree

## 3D の地図ビュー (#9298) を実ゲームで確かめる
##
## 使い方:
##   HB_SAVE=<使い捨てのセーブファイル> godot --headless --path godot_project --script "$PWD"/tools/godot/dungeon-view-runtime-check.gd
##
## headless では描画はされないが、ノード・MultiMesh のインスタンスは作られるので、
## 置いた数がスナップショットと一致するか、地図以外の画面で隠れるか、移動に追従するかを確かめる。
## 全体の作り直しと差分の反映にかかった時間も表示する。

const VisualCatalog = preload("res://scripts/visual_catalog.gd")

var save_path := OS.get_environment("HB_SAVE")
var game: Node = null
var view: Control = null
var ih: Node = null
var phase := 0
var status_before := false
var phase_frame := 0
var ok := true
var start_player := Vector2i.ZERO


func _initialize() -> void:
	var gs = root.get_node("GameState")
	gs.save_path = save_path
	# 画面の大きさの変化などで設定が保存されるので、利用者の設定ファイルを書き換えないよう別の場所へ向ける
	gs.config_path = "user://runtime-check-config.cfg"
	gs.layout_data = {}
	gs.use_map_view = true
	root.size = Vector2i(1280, 800)
	change_scene_to_file("res://scenes/main.tscn")


func check(label: String, cond: bool, detail: String = "") -> void:
	ok = ok and cond
	print("%s %s %s" % ["OK" if cond else "NG", label, detail])


func expected_counts() -> Dictionary:
	var vc := VisualCatalog.new()
	vc.load_definitions()
	vc.set_terrain_catalog(game.get_terrain_catalog())
	var s: Dictionary = game.get_world_snapshot()
	var terrain: PackedInt32Array = s["terrain"]
	var flags: PackedByteArray = s["flags"]
	var boxes := 0
	var planes := 0
	var items := 0
	var terrain_glyphs := 0
	for i in terrain.size():
		var visual := vc.resolve_terrain(terrain[i])
		var kind: String = visual["kind"]
		if kind == "box":
			boxes += 1
		elif kind == "glyph":
			terrain_glyphs += 1
		elif kind != "none":
			planes += 1
		if kind != "glyph" and kind != "none" and visual.get("overlay_glyph", false):
			terrain_glyphs += 1
		# 見つけた床アイテムは地形を知らないグリッドにも描く
		if (flags[i] & 8) != 0:
			items += 1
	return { "boxes": boxes, "planes": planes, "creatures": s["creatures"].size(), "items": items, "terrain_glyphs": terrain_glyphs, "player": Vector2i(s["player_x"], s["player_y"]) }


func _process(_delta: float) -> bool:
	phase_frame += 1
	if game == null:
		var scene = current_scene
		if scene and scene.has_node("HengbandGame"):
			game = scene.get_node("HengbandGame")
			ih = scene.get_node("HengbandGame/InputHandler")
			view = scene.find_child("DungeonView3D", true, false)
		return false
	if phase_frame % 30 == 0:
		game.set_sub_window_size(0, 100, 36)
	if Time.get_ticks_msec() > 200000:
		print("RESULT: timeout")
		OS.kill(OS.get_process_id())
	match phase:
		0:
			if not game.get_player_status().is_empty() and phase_frame > 120:
				ih.inject_key(0x1B)
				ih.inject_key(0x12)
				_next()
		1:
			if phase_frame == 90:
				check("view exists", view != null)
				check("visible on the map", view.visible)
				var sidebar = current_scene.status_sidebar
				var sidebar_text := ""
				for line in sidebar._view._lines:
					sidebar_text += "".join(line["chars"]) + "\n"
				check("status sidebar shown with the map", sidebar.visible and sidebar_text.contains("HP"), sidebar_text.replace("\n", " | "))
				var got: Dictionary = view.count_instances()
				var want := expected_counts()
				check("instances match the snapshot", got["boxes"] == want["boxes"] and got["planes"] == want["planes"] and got["creatures"] == want["creatures"] and got["items"] == want["items"] and got["terrain_glyphs"] == want["terrain_glyphs"], "got=%s want=%s" % [got, want])
				print("full build %.1f ms" % view.last_full_build_ms)
				# 光源 (#9299): プレイヤーの光の届く範囲は光源の半径に合わせる
				var snap: Dictionary = game.get_world_snapshot()
				var radius: int = snap["player_light_radius"]
				# 盲目なら光は置かない (スナップショットの半径は光源の値のまま)
				var blind: bool = snap.get("player_blind", false)
				var expected_light := 0.0 if blind or radius == 0 else radius + 0.75
				check("player light follows the light radius", is_equal_approx(got["player_light"], expected_light), "radius=%d blind=%s range=%s" % [radius, blind, got["player_light"]])
				var lit_creatures := 0
				for c in snap["creatures"]:
					if c["light_radius"] > 0:
						lit_creatures += 1
				check("creature lights", got["creature_lights"] == mini(lit_creatures, 16), "lights=%d lit creatures=%d" % [got["creature_lights"], lit_creatures])
				# 明るさの段階: 視界内で明るい / 暗い / 記憶だけ が色で区別されること
				var DV = load("res://scripts/dungeon_view_3d.gd")
				var base := Color(0.8, 0.6, 0.4)
				var c_lit: Color = DV.grid_color(base, 1 | 2 | 4, false)
				var c_dark: Color = DV.grid_color(base, 1 | 2, false)
				var c_memory: Color = DV.grid_color(base, 1, false)
				var c_blind: Color = DV.grid_color(base, 1 | 2 | 4, true)
				check("brightness levels", c_lit.v > c_dark.v and c_dark.v > c_memory.v and c_blind == c_memory, "lit=%s dark=%s memory=%s" % [c_lit, c_dark, c_memory])
				start_player = want["player"]
				ih.inject_key(0x69) # i: 持ち物
		2:
			pass
	if phase == 1 and phase_frame == 150:
		# 地図の上に重ねる一覧 (持ち物) は 3D のまま、半透明の窓に写す
		check("still visible in the inventory", view.visible)
		var overlay = current_scene.status_overlay_menu
		var state: Dictionary = game.get_screen_state()
		var rect := Rect2i(state["overlay_x"], state["overlay_y"], state["overlay_w"], state["overlay_h"])
		var term = current_scene.status_sidebar._terminal
		var expected := ""
		for y in range(rect.position.y, rect.end.y):
			expected += term.get_row_text(y).substr(rect.position.x, rect.size.x).replace(" ", "")
		var mirrored := ""
		for line in overlay._view._lines:
			mirrored += "".join(line["chars"]).replace(" ", "")
		check("inventory shown in the overlay window", overlay.visible and rect.has_area() and mirrored != "" and mirrored == expected, "rect=%s mirror=\"%s\" term=\"%s\"" % [rect, mirrored, expected])
		ih.inject_key(0x1B)
		# 全画面の表示 (キャラクター情報) は従来どおり端末に戻す
		ih.inject_key(0x43)
	if phase == 1 and phase_frame == 180:
		check("hidden in the character sheet", not view.visible)
		check("status sidebar hidden in the character sheet", not current_scene.status_sidebar.visible)
		check("overlay window hidden in the character sheet", not current_scene.status_overlay_menu.visible)
		ih.inject_key(0x1B)
	if phase == 1 and phase_frame == 210:
		check("visible again on the map", view.visible)
		check("status sidebar shown again", current_scene.status_sidebar.visible)
		check("overlay window hidden on the map", not current_scene.status_overlay_menu.visible)
		# 下端の状態バーは端末の最下行と同じ中身 (空なら隠す)
		var bar = current_scene.status_bottom_bar
		var term = current_scene.status_sidebar._terminal
		var last_row: String = term.get_row_text(term.get_rows() - 1).strip_edges()
		var bar_text := ""
		for line in bar._view._lines:
			bar_text += " ".join(line["chars"])
		check("status bottom bar mirrors the last row", bar.visible == (last_row != "") and bar_text.replace(" ", "") == last_row.replace(" ", ""), "row=\"%s\" visible=%s" % [last_row, bar.visible])
		game.walk(6)
	if phase == 1 and phase_frame == 300:
		var want := expected_counts()
		var got: Dictionary = view.count_instances()
		check("instances still match after moving", got["boxes"] == want["boxes"] and got["planes"] == want["planes"] and got["creatures"] == want["creatures"], "got=%s want=%s" % [got, want])
		var label_pos: Vector3 = view._player_label.position
		check("player billboard follows", Vector2i(int(floor(label_pos.x)), int(floor(label_pos.z))) == want["player"], "label=%s player=%s start=%s" % [label_pos, want["player"], start_player])
		print("last diff update %.2f ms for %d grids" % [view.last_update_ms, view.last_changed_grids])
		# 1 行目のメッセージ: 階の雰囲気 (^F) を出して、写しが端末の 1 行目と一致するか
		ih.inject_key(0x06)
	if phase == 1 and phase_frame == 360:
		var term = current_scene.status_sidebar._terminal
		var msg = current_scene.status_message_line
		var row0: String = term.get_row_text(0).strip_edges()
		var msg_text := ""
		for line in msg._view._lines:
			msg_text += "".join(line["chars"])
		check("message line mirrors the first row", row0 != "" and msg.visible and msg_text == row0.replace(" ", ""), "row0=\"%s\" mirror=\"%s\"" % [row0, msg_text])
		# 下のバーの「3D」ボタンで切り替える (ステータスパネル・項目ウィンドウには波及しない)
		var button: Button = current_scene.map_view_button
		status_before = current_scene.get_node("StatusLayer/StatusPanel").visible
		check("3D button reflects the setting", button != null and button.button_pressed)
		ih.inject_key(0x1B)
		button.button_pressed = false
	if phase == 1 and phase_frame == 400:
		check("3D button turns the map view off", not view.visible and not root.get_node("GameState").use_map_view and not current_scene.status_sidebar.visible)
		check("3D button leaves the status panel alone", current_scene.get_node("StatusLayer/StatusPanel").visible == status_before)
		current_scene.get_node("ConfigLayer/BottomBar/ItemWindowButton0").pressed.emit()
		current_scene.map_view_button.button_pressed = true
	if phase == 1 and phase_frame == 440:
		check("3D button turns the map view on again", view.visible and root.get_node("GameState").use_map_view)
		check("M button leaves the status panel alone", current_scene.get_node("StatusLayer/StatusPanel").visible == status_before)
		current_scene.get_node("ConfigLayer/BottomBar/ItemWindowButton0").pressed.emit()
		# ターゲットの選択 (*): 3D のまま照準の輪と、プレイヤーからの射線を描く
		ih.inject_key(0x2A)
	if phase == 1 and phase_frame == 480:
		var state: Dictionary = game.get_screen_state()
		var markers: Dictionary = view.get_target_markers()
		var path: PackedInt32Array = state["target_path"]
		var reaches: bool = path.size() >= 2 and Vector2i(path[path.size() - 2], path[path.size() - 1]) == markers["cursor"]
		check("targeting keeps the map view", view.visible and state["state"] == 3, "state=%d" % state["state"])
		check("cursor ring at the target", markers["cursor_shown"] and markers["cursor"] == Vector2i(state["cursor_x"], state["cursor_y"]), "markers=%s" % markers)
		check("target path drawn to the cursor", markers["path_marks"] == path.size() / 2 and markers["path_marks"] > 0 and (reaches or state["target_path_blocked"]), "marks=%d path=%s blocked=%s" % [markers["path_marks"], path, state["target_path_blocked"]])
		ih.inject_key(0x1B)
		# 見るコマンド (l): 照準の輪だけで射線は描かない
		ih.inject_key(0x6C)
	if phase == 1 and phase_frame == 520:
		var markers: Dictionary = view.get_target_markers()
		check("look shows only the cursor", markers["cursor_shown"] and markers["path_marks"] == 0, "markers=%s" % markers)
		ih.inject_key(0x1B)
		ih.inject_key(0x1B)
	if phase == 1 and phase_frame == 560:
		var markers: Dictionary = view.get_target_markers()
		check("cursor hidden back on the map", not markers["cursor_shown"] and markers["path_marks"] == 0, "markers=%s" % markers)
		print("RESULT: ", "OK" if ok else "NG")
		OS.kill(OS.get_process_id())
	return false


func _next() -> void:
	phase += 1
	phase_frame = 0
