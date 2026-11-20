extends SceneTree

## 座標・方向で指定する入力 (#9296) を実ゲームで確かめる
##
## 使い方:
##   HB_SAVE=<使い捨てのセーブファイル> godot --headless --path godot_project --script "$PWD"/tools/godot/input-intents-runtime-check.gd
##
## 1. walk(6) で東へ 1 歩歩けること
## 2. 端末の '@' のセルを cell_to_floor() で直すとプレイヤーの座標になること
## 3. travel_to() で数グリッド先の床へトラベルできること
## 4. target_grid() の後、画面が地図に戻ること (ターゲットの選択で止まらないこと)
## 安全な場所 (街など) で保存したセーブを渡すこと。

const InputIntents = preload("res://scripts/input_intents.gd")

var save_path := OS.get_environment("HB_SAVE")
var game: Node = null
var phase := 0
var phase_frame := 0
var start_pos := Vector2i.ZERO
var goal := Vector2i(-1, -1)
var ok := true


func _initialize() -> void:
	var gs = root.get_node("GameState")
	gs.save_path = save_path
	# 画面の大きさの変化などで設定が保存されるので、利用者の設定ファイルを書き換えないよう別の場所へ向ける
	gs.config_path = "user://runtime-check-config.cfg"
	gs.layout_data = {}
	change_scene_to_file("res://scenes/main.tscn")


func snapshot() -> Dictionary:
	return game.get_world_snapshot()


func player_pos() -> Vector2i:
	var s := snapshot()
	return Vector2i(s["player_x"], s["player_y"])


func check(label: String, cond: bool, detail: String = "") -> void:
	ok = ok and cond
	print("%s %s %s" % ["OK" if cond else "NG", label, detail])


func find_player_cell() -> Vector2i:
	var t = root.find_children("*", "GodotTerminal", true, false)[0]
	for y in t.get_rows():
		var line: String = t.get_row_text(y)
		var x := line.find("@", 12)
		if x >= 0:
			return Vector2i(column_of(line, x), y)
	return Vector2i(-1, -1)


## get_row_text() の文字の位置を端末の桁に直す。全角の字は 2 桁を占める (右半分は行の文字列に入らない)
static func column_of(line: String, index: int) -> int:
	var col := 0
	for i in index:
		var c := line.unicode_at(i)
		col += 1 if c < 0x80 or (c >= 0xFF61 and c <= 0xFF9F) else 2
	return col


## 東西に並んだ既知の床のうち、プレイヤーから 3〜6 グリッド先のものを探す
func find_travel_goal() -> Vector2i:
	var s := snapshot()
	var w: int = s["width"]
	var shapes: PackedByteArray = s["shapes"]
	var p := player_pos()
	for dx in [5, -5, 4, -4, 6, -6, 3, -3]:
		var q := Vector2i(p.x + dx, p.y)
		if q.x >= 0 and q.x < w and shapes[q.y * w + q.x] == 1:
			return q
	return Vector2i(-1, -1)


func _process(_delta: float) -> bool:
	phase_frame += 1
	if game == null:
		var scene = current_scene
		if scene and scene.has_node("HengbandGame"):
			game = scene.get_node("HengbandGame")
		return false
	if phase_frame % 30 == 0:
		game.set_sub_window_size(0, 100, 36)
	if Time.get_ticks_msec() > 200000:
		print("RESULT: timeout")
		OS.kill(OS.get_process_id())
	match phase:
		0:
			if not game.get_player_status().is_empty() and phase_frame > 120:
				game.set_world_snapshot_enabled(true)
				game.get_node("InputHandler").inject_key(0x12)
				_next()
		1:
			if phase_frame == 60:
				start_pos = player_pos()
				check("walk accepted", game.walk(6))
			elif phase_frame == 120:
				var p := player_pos()
				check("walk east", p == start_pos + Vector2i(1, 0), "%s -> %s" % [start_pos, p])
				_next()
		2:
			if phase_frame == 30:
				var cell := find_player_cell()
				var pos := InputIntents.cell_to_floor(cell, game.get_screen_state(), Vector2i(1 << 30, 1 << 30))
				check("cell_to_floor('@')", pos == player_pos(), "cell=%s floor=%s player=%s" % [cell, pos, player_pos()])
				goal = find_travel_goal()
				check("travel accepted", goal.x >= 0 and game.travel_to(goal.x, goal.y), "goal=%s" % goal)
			elif phase_frame == 300:
				check("travel reached", player_pos() == goal, "player=%s goal=%s" % [player_pos(), goal])
				_next()
		3:
			if phase_frame == 30:
				check("target accepted", game.target_grid(start_pos.x, start_pos.y))
			elif phase_frame == 120:
				var st: Dictionary = game.get_screen_state()
				check("back to map after target", st["state"] == 1, "state=%d prompt=\"%s\"" % [st["state"], st["prompt"]])
				# 積んだ座標がターゲットの選択で取り出されたこと ('*' が何もしなくても上の確認は通るため)
				check("target intent consumed", not game.has_pending_point_intent())
				print("RESULT: ", "OK" if ok else "NG")
				OS.kill(OS.get_process_id())
	return false


func _next() -> void:
	phase += 1
	phase_frame = 0
