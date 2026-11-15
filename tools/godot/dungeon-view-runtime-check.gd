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
var phase_frame := 0
var ok := true
var start_player := Vector2i.ZERO


func _initialize() -> void:
	var gs = root.get_node("GameState")
	gs.save_path = save_path
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
	for i in terrain.size():
		var kind: String = vc.resolve_terrain(terrain[i])["kind"]
		if kind == "box":
			boxes += 1
		elif kind != "none":
			planes += 1
		if (flags[i] & 1) != 0 and (flags[i] & 8) != 0:
			items += 1
	return { "boxes": boxes, "planes": planes, "creatures": s["creatures"].size(), "items": items, "player": Vector2i(s["player_x"], s["player_y"]) }


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
				var got: Dictionary = view.count_instances()
				var want := expected_counts()
				check("instances match the snapshot", got["boxes"] == want["boxes"] and got["planes"] == want["planes"] and got["creatures"] == want["creatures"] and got["items"] == want["items"], "got=%s want=%s" % [got, want])
				print("full build %.1f ms" % view.last_full_build_ms)
				start_player = want["player"]
				ih.inject_key(0x69) # i: 持ち物
		2:
			pass
	if phase == 1 and phase_frame == 150:
		check("hidden in the inventory", not view.visible)
		ih.inject_key(0x1B)
	if phase == 1 and phase_frame == 210:
		check("visible again on the map", view.visible)
		game.walk(6)
	if phase == 1 and phase_frame == 300:
		var want := expected_counts()
		var got: Dictionary = view.count_instances()
		check("instances still match after moving", got["boxes"] == want["boxes"] and got["planes"] == want["planes"] and got["creatures"] == want["creatures"], "got=%s want=%s" % [got, want])
		var label_pos: Vector3 = view._player_label.position
		check("player billboard follows", Vector2i(int(floor(label_pos.x)), int(floor(label_pos.z))) == want["player"], "label=%s player=%s start=%s" % [label_pos, want["player"], start_player])
		print("last diff update %.2f ms for %d grids" % [view.last_update_ms, view.last_changed_grids])
		print("RESULT: ", "OK" if ok else "NG")
		OS.kill(OS.get_process_id())
	return false


func _next() -> void:
	phase += 1
	phase_frame = 0
