extends SceneTree

## 表示用のゲームイベント (#9295) を実ゲームで確かめる
##
## 使い方:
##   HB_SAVE=<使い捨てのセーブファイル> godot --headless --path godot_project --script "$PWD"/tools/godot/game-events-runtime-check.gd
##
## セーブを読み込み、ワールドスナップショットとゲームイベントを有効にして移動キーを送り、
## 受け取ったイベントを種類ごとに数えて表示する。モンスターのいる階で保存したセーブを渡すと
## 近接攻撃・被弾も出る (キャラクターが死ぬこともあるので使い捨てのセーブにすること)。

const EVENT_NAMES := ["?", "MELEE", "PROJECTION", "DAMAGE", "DEATH", "MOVE", "TELEPORT", "APPEAR", "DISAPPEAR"]
const DebugMinimap = preload("res://scripts/world_debug_minimap.gd")

var save_path := OS.get_environment("HB_SAVE")
var game: Node = null
var ih: Node = null
var phase := 0
var phase_frame := 0
var counts := {}
var samples := {}
var player_id := -1


func _initialize() -> void:
	var gs = root.get_node("GameState")
	gs.save_path = save_path
	gs.layout_data = {}
	change_scene_to_file("res://scenes/main.tscn")


func _process(_delta: float) -> bool:
	phase_frame += 1
	if game == null:
		var scene = current_scene
		if scene and scene.has_node("HengbandGame"):
			game = scene.get_node("HengbandGame")
			ih = scene.get_node("HengbandGame/InputHandler")
			game.game_events.connect(_on_events)
		return false
	if phase_frame % 30 == 0:
		game.set_sub_window_size(0, 100, 36)
	if Time.get_ticks_msec() > 200000:
		_finish()
	match phase:
		0:
			if not game.get_player_status().is_empty() and phase_frame > 120:
				game.set_world_snapshot_enabled(true)
				game.set_game_events_enabled(true)
				ih.inject_key(0x1B)
				phase = 1
				phase_frame = 0
		1:
			if phase_frame % 40 == 0:
				var keys := [0x36, 0x36, 0x38, 0x38, 0x34, 0x34, 0x32, 0x32, 0x36, 0x36, 0x36, 0x38, 0x38, 0x38, 0x34, 0x32]
				var i: int = phase_frame / 40 - 1
				if i < keys.size():
					ih.inject_key(0x1B)
					ih.inject_key(keys[i])
				elif phase_frame > keys.size() * 40 + 120:
					_finish()
	return false


func _on_events(events: Array) -> void:
	for event: Dictionary in events:
		var name: String = EVENT_NAMES[event["type"]]
		counts[name] = counts.get(name, 0) + 1
		if not samples.has(name):
			samples[name] = DebugMinimap.describe_event(event)


func _finish() -> void:
	print("EVENT COUNTS: ", counts)
	for name in samples:
		print("  sample ", samples[name])
	print("RESULT: ", "OK" if counts.has("MOVE") else "NG")
	OS.kill(OS.get_process_id())
