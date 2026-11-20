extends SceneTree

## メッセージ・持ち物・装備のウィンドウを実ゲームで確かめる
##
## 使い方:
##   HB_SAVE=<使い捨てのセーブファイル> godot --headless --path godot_project --script "$PWD"/tools/godot/item-windows-runtime-check.gd
##
## 1. get_inventory() が持ち物と装備を返し、ウィンドウに描かれること
## 2. 何かメッセージが出ると messages_changed が届き、メッセージのウィンドウに出ること
## 3. 持ち物を落とすと inventory_changed が届くこと

var save_path := OS.get_environment("HB_SAVE")
var game: Node = null
var ih: Node = null
var phase := 0
var phase_frame := 0
var ok := true
var inventory_signals := 0
var message_signals := 0
var pack_count := -1


func _initialize() -> void:
	var gs = root.get_node("GameState")
	gs.save_path = save_path
	# 画面の大きさの変化などで設定が保存されるので、利用者の設定ファイルを書き換えないよう別の場所へ向ける
	gs.config_path = "user://runtime-check-config.cfg"
	gs.layout_data = {}
	gs.item_windows = {}
	change_scene_to_file("res://scenes/main.tscn")


func check(label: String, cond: bool, detail: String = "") -> void:
	ok = ok and cond
	print("%s %s %s" % ["OK" if cond else "NG", label, detail])


func _process(_delta: float) -> bool:
	phase_frame += 1
	if game == null:
		var scene = current_scene
		if scene and scene.has_node("HengbandGame"):
			game = scene.get_node("HengbandGame")
			ih = scene.get_node("HengbandGame/InputHandler")
			game.inventory_changed.connect(func(_v: int) -> void: inventory_signals += 1)
			game.messages_changed.connect(func(_v: int) -> void: message_signals += 1)
		return false
	if phase_frame % 30 == 0:
		game.set_sub_window_size(0, 100, 36)
	if Time.get_ticks_msec() > 200000:
		print("RESULT: timeout")
		OS.kill(OS.get_process_id())
	var windows: Array = current_scene.item_windows
	match phase:
		0:
			if not game.get_player_status().is_empty() and phase_frame > 120:
				ih.inject_key(0x1B)
				for w in windows:
					w.visible = true
				_next()
		1:
			if phase_frame == 30:
				var inv: Dictionary = game.get_inventory()
				check("inventory data", not inv.is_empty(), "keys=%s" % [inv.keys()])
				var pack: Array = inv.get("pack", [])
				pack_count = pack.size()
				check("pack has items", pack_count > 0, "pack=%d" % pack_count)
				check("equipment slots", inv.get("equipment", []).size() >= 12, "slots=%d" % inv.get("equipment", []).size())
				check("weight limit", int(inv.get("weight_limit", 0)) > 0)
				if pack_count > 0:
					print("  first item: ", pack[0])
				var inv_text: String = windows[1]._body.get_parsed_text()
				var eq_text: String = windows[2]._body.get_parsed_text()
				check("inventory window drawn", pack_count == 0 or inv_text.contains(str(pack[0]["name"])))
				check("equipment window drawn", eq_text.split("\n").size() >= 12)
				print("--- inventory window ---\n", inv_text, "\n--- equipment window ---\n", eq_text)
				# 何かメッセージを出す: 足元を調べる
				message_signals = 0
				ih.inject_key(0x2C)
			elif phase_frame == 120:
				var msgs: Dictionary = game.get_messages()
				check("messages data", msgs.get("messages", PackedStringArray()).size() > 0, "n=%d signals=%d" % [msgs.get("messages", []).size(), message_signals])
				var text: String = windows[0]._body.get_parsed_text()
				check("message window drawn", text.length() > 0)
				print("--- message window (tail) ---\n", text.right(300))
				# 持ち物の先頭を落とす: d a (数量を聞かれたら Enter)
				inventory_signals = 0
				message_signals = 0
				ih.inject_key(0x64)
				ih.inject_key(0x61)
				ih.inject_key(0x0D)
			elif phase_frame == 240:
				ih.inject_key(0x1B)
				var inv: Dictionary = game.get_inventory()
				var pack: Array = inv.get("pack", [])
				var first_item: String = "(empty pack)" if pack.is_empty() else str(pack[0].get("name", ""))
				check("inventory_changed on drop", inventory_signals > 0, "signals=%d first item now: %s" % [inventory_signals, first_item])
				var last: PackedStringArray = game.get_messages().get("messages", PackedStringArray())
				check("drop message shown", message_signals > 0 and last.size() > 0 and windows[0]._body.get_parsed_text().contains(last[last.size() - 1]), "signals=%d last=\"%s\"" % [message_signals, last[last.size() - 1] if last.size() > 0 else ""])
				print("RESULT: ", "OK" if ok else "NG")
				OS.kill(OS.get_process_id())
	return false


func _next() -> void:
	phase += 1
	phase_frame = 0
