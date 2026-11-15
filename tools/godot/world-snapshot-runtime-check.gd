extends SceneTree

## main.tscn を実際に起動してセーブデータを読み込み、ワールドスナップショットを確かめる。
##
## 使い方 (docs/godot-interface.md「実ゲームでの確認」参照):
##   HB_SAVE=<セーブファイル> godot --headless --path godot_project --script "$PWD"/tools/godot/world-snapshot-runtime-check.gd
##
## 1. セーブを読み込み、ワールドスナップショットと検証モードを有効にする
## 2. 全体スナップショットの概要と画面を表示する
## 3. 移動キーを送り、その間 world_snapshot_changed ごとに差分を手元の配列へ反映する
## 4. 手元の配列が全体スナップショットと一致し、検証モードの食い違いが 0 なら OK
##
## headless の Godot はビューポートが極小になり端末が 11x1 ("Please wait") になるため、
## 端末の大きさを set_sub_window_size() で直接与えている。
## 移動中にキャラクターが死ぬこともあるので、使い捨てのセーブを渡すこと。

var save_path := OS.get_environment("HB_SAVE")
var frame := 0
var game: Node = null
var ih: Node = null
var phase := 0
var phase_frame := 0
var last_version := 0
var local_shapes := PackedByteArray()
var changed_total := 0
var pulls := 0
var ids_seen := {}

func _initialize() -> void:
	var gs = root.get_node("GameState")
	gs.save_path = save_path
	gs.layout_data = {}
	root.size = Vector2i(1280, 800)
	change_scene_to_file("res://scenes/main.tscn")

func _process(_delta: float) -> bool:
	frame += 1
	phase_frame += 1
	if game == null:
		var scene = current_scene
		if scene and scene.has_node("HengbandGame"):
			game = scene.get_node("HengbandGame")
			ih = scene.get_node("HengbandGame/InputHandler")
			game.world_snapshot_changed.connect(_on_changed)
		return false
	match phase:
		0:
			if phase_frame % 60 == 0:
				print("waiting... status=", not game.get_player_status().is_empty(), " t=", Time.get_ticks_msec())
			if Time.get_ticks_msec() > 120000:
				print("RESULT: timeout waiting for game")
				_finish(1)
				return true
			if phase_frame % 30 == 0:
				game.set_sub_window_size(0, 100, 36)
			if not game.get_player_status().is_empty() and phase_frame > 90:
				# 起動時の「何かキーを」を抜ける
				ih.inject_key(0x1B)
				game.set_world_snapshot_enabled(true)
				game.set_world_snapshot_verify(true)
				ih.inject_key(0x12) # Ctrl+R 再描画
				_next()
		1:
			if phase_frame > 120:
				var full: Dictionary = game.get_world_snapshot()
				if full.is_empty():
					print("RESULT: no snapshot")
					_finish(1)
					return true
				var known := 0
				var shapes: PackedByteArray = full["shapes"]
				var hist := {}
				for s in shapes:
					if s != 0:
						known += 1
					hist[s] = hist.get(s, 0) + 1
				print("FULL v=%d floor_v=%d size=%dx%d dlv=%d player=(%d,%d) known=%d creatures=%d shapes=%s" % [full["version"], full["floor_version"], full["width"], full["height"], full["dun_level"], full["player_x"], full["player_y"], known, full["creatures"].size(), hist])
				var symbols: PackedByteArray = full["symbols"]
				var pidx: int = full["player_y"] * full["width"] + full["player_x"]
				dump_screen("after load")
				print("player grid flags=%d fg_char=%s" % [full["flags"][pidx], char(symbols[pidx * 4 + 1])])
				_next()
		2:
			# 移動 (テンキー方向)。1 歩ずつ間を空ける
			if phase_frame % 40 == 0:
				var keys := [0x1B, 0x36, 0x1B, 0x36, 0x1B, 0x38, 0x1B, 0x38, 0x1B, 0x34, 0x1B, 0x34, 0x1B, 0x32, 0x1B, 0x32, 0x1B, 0x36, 0x1B, 0x36, 0x1B, 0x36, 0x1B, 0x38, 0x1B, 0x38, 0x1B, 0x38]
				var i: int = phase_frame / 40 - 1
				if i < keys.size():
					ih.inject_key(keys[i])
				else:
					_next()
		3:
			if phase_frame > 120:
				var full: Dictionary = game.get_world_snapshot()
				var same: bool = full["shapes"] == local_shapes
				dump_screen("after moves")
				print("creatures now: ", full["creatures"])
				print("AFTER MOVES v=%d player=(%d,%d) pulls=%d changed_total=%d diff_applied_matches_full=%s verify_mismatches=%d creature_ids=%d" % [full["version"], full["player_x"], full["player_y"], pulls, changed_total, same, game.get_world_snapshot_verify_mismatches(), ids_seen.size()])
				_finish(0 if same and game.get_world_snapshot_verify_mismatches() == 0 else 1)
				return true
	return false

func _on_changed(_v: int) -> void:
	var ch: Dictionary = game.get_world_snapshot_changes(last_version)
	if ch.is_empty():
		return
	pulls += 1
	if ch["full"]:
		local_shapes = ch["shapes"]
	else:
		var idx: PackedInt32Array = ch["indices"]
		var shapes: PackedByteArray = ch["shapes"]
		for i in idx.size():
			local_shapes[idx[i]] = shapes[i]
		changed_total += idx.size()
	for c in ch["creatures"]:
		ids_seen[c["id"]] = true
	last_version = ch["version"]

func dump_screen(label: String) -> void:
	var terms := root.find_children("*", "GodotTerminal", true, false)
	if terms.is_empty():
		print("no terminal")
		return
	var t = terms[0]
	print("---- screen (", label, ") ", t.get_cols(), "x", t.get_rows())
	for y in t.get_rows():
		var line: String = t.get_row_text(y).strip_edges(false, true)
		if not line.is_empty():
			print("|", line)

func _next() -> void:
	phase += 1
	phase_frame = 0

func _finish(code: int) -> void:
	print("RESULT: ", "OK" if code == 0 else "NG")
	OS.kill(OS.get_process_id())
