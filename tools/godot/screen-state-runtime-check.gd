extends SceneTree

## 画面の状態 (#9294) の遷移を実ゲームで確かめる
##
## 使い方:
##   HB_SAVE=<使い捨てのセーブファイル> godot --headless --path godot_project --script "$PWD"/tools/godot/screen-state-runtime-check.gd
##
## セーブを読み込み、STEPS のキーを 1 つずつ送って get_screen_state() が期待どおりかを確かめる。
## キーは元祖キー配置を前提にしている。地下でキャラクターが襲われると期待どおりにならないので、
## 安全な場所 (街など) で保存したセーブを渡すこと。

var save_path := OS.get_environment("HB_SAVE")
var game: Node = null
var ih: Node = null
var step := -1
var step_frame := 0
var ok := true

## [送るキー, 期待する状態, 説明]
const STEPS := [
	[0x1B, 1, "map after ESC"],
	[0x69, 4, "inventory list (i)"],
	[0x1B, 1, "back to map"],
	[0x43, 5, "character sheet (C)"],
	[0x1B, 1, "back to map"],
	[0x6C, 3, "look (l)"],
	[0x1B, 1, "back to map"],
	[0x7E, 5, "knowledge menu (~)"],
	[0x1B, 1, "back to map"],
	[0x76, 4, "throw: item list (v)"],
	[0x1B, 1, "back to map"],
	[0x2B, 2, "alter: direction prompt (+)"],
	[0x1B, 1, "back to map"],
	[0x43, 5, "character sheet again (C)"],
	[0x1B, 1, "back to map"],
	[0x69, 4, "inventory after sheet (i)"],
	[0x1B, 1, "back to map"],
]

func _initialize() -> void:
	var gs = root.get_node("GameState")
	gs.save_path = save_path
	# 画面の大きさの変化などで設定が保存されるので、利用者の設定ファイルを書き換えないよう別の場所へ向ける
	gs.config_path = "user://runtime-check-config.cfg"
	gs.layout_data = {}
	change_scene_to_file("res://scenes/main.tscn")

func _process(_delta: float) -> bool:
	step_frame += 1
	if game == null:
		var scene = current_scene
		if scene and scene.has_node("HengbandGame"):
			game = scene.get_node("HengbandGame")
			ih = scene.get_node("HengbandGame/InputHandler")
		return false
	if step_frame % 30 == 0:
		game.set_sub_window_size(0, 100, 36)
	if Time.get_ticks_msec() > 150000:
		print("RESULT: timeout")
		OS.kill(OS.get_process_id())
	if step < 0:
		if not game.get_player_status().is_empty() and step_frame > 120:
			step = 0
			step_frame = 0
			ih.inject_key(STEPS[0][0])
		return false
	if step_frame == 90:
		var st: Dictionary = game.get_screen_state()
		var expected: int = STEPS[step][1]
		var pass_ : bool = st["state"] == expected
		ok = ok and pass_
		print("%s %-24s state=%d expected=%d kind=%d depth=%d cursor=%s prompt=\"%s\"" % ["OK" if pass_ else "NG", STEPS[step][2], st["state"], expected, st["input_kind"], st["icky_depth"], ("(%d,%d)" % [st["cursor_x"], st["cursor_y"]]) if st["has_cursor"] else "-", st["prompt"]])
		step += 1
		step_frame = 0
		if step >= STEPS.size():
			print("RESULT: ", "OK" if ok else "NG")
			OS.kill(OS.get_process_id())
		ih.inject_key(STEPS[step][0])
	return false
