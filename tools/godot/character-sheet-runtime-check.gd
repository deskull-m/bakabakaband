extends SceneTree

## ステータスパネル (C コマンドの画面のダンプ) を実ゲームで確かめる
##
## 使い方:
##   HB_SAVE=<使い捨てのセーブファイル> godot --headless --path godot_project --script "$PWD"/tools/godot/character-sheet-runtime-check.gd
##
## 1. get_character_sheet() がダンプ (5 画面ぶん) を返し、パネルに描かれること
## 2. 作っても端末の画面 (地図) が変わらないこと (画面外の端末に描いている)
## 3. C コマンドで端末に出したステータス画面の各行と、ダンプの先頭の画面が一致すること
## 4. ゲームターンが進むと作り直されること (character_sheet_changed)

var save_path := OS.get_environment("HB_SAVE")
var game: Node = null
var ih: Node = null
var terminal: Node = null
var phase := 0
var phase_frame := 0
var ok := true
var sheet_signals := 0
var map_rows_before := PackedStringArray()


func _initialize() -> void:
	var gs = root.get_node("GameState")
	gs.save_path = save_path
	gs.layout_data = {}
	gs.status_panel_visible = true
	change_scene_to_file("res://scenes/main.tscn")


func check(label: String, cond: bool, detail: String = "") -> void:
	ok = ok and cond
	print("%s %s %s" % ["OK" if cond else "NG", label, detail])


func rows() -> PackedStringArray:
	var out := PackedStringArray()
	for y in terminal.get_rows():
		out.append(terminal.get_row_text(y))
	return out


func _process(_delta: float) -> bool:
	phase_frame += 1
	if game == null:
		var scene = current_scene
		if scene and scene.has_node("HengbandGame"):
			game = scene.get_node("HengbandGame")
			ih = scene.get_node("HengbandGame/InputHandler")
			game.character_sheet_changed.connect(func(_v: int) -> void: sheet_signals += 1)
		return false
	if terminal == null:
		var found := root.find_children("*", "GodotTerminal", true, false)
		if found.is_empty():
			return false
		terminal = found[0]
	if phase_frame % 30 == 0:
		game.set_sub_window_size(0, 100, 36)
	if Time.get_ticks_msec() > 200000:
		print("RESULT: timeout")
		OS.kill(OS.get_process_id())
	match phase:
		0:
			if not game.get_player_status().is_empty() and phase_frame > 150:
				ih.inject_key(0x1B)
				_next()
		1:
			if phase_frame == 60:
				var sheet: Dictionary = game.get_character_sheet()
				var text: PackedStringArray = sheet.get("text", PackedStringArray())
				check("sheet data", text.size() >= 60, "lines=%d" % text.size())
				var name: String = game.get_player_status().get("name", "")
				check("sheet has the name", "\n".join(text).contains(name), name)
				print("--- sheet (first 24 lines) ---")
				for i in mini(24, text.size()):
					print(text[i])
				var panel = current_scene.get_node("StatusLayer/StatusPanel")
				check("panel drawn", panel.visible and panel._view._lines.size() == text.size() and panel._view.custom_minimum_size.x > 0)
				var widest := 0
				for line in text:
					widest = maxi(widest, line.length())
				check("lines fit in 79 columns", widest <= 79, "widest=%d" % widest)
				map_rows_before = rows()
				check("map shown", game.get_screen_state()["state"] == 1)
				# ゲームターンを進める (その場で休む: R & Enter は確認があるので 1 ターンの '5' 相当として s)
				sheet_signals = 0
				ih.inject_key(0x73)
			elif phase_frame == 150:
				check("rebuilt when the turn advances", sheet_signals > 0, "signals=%d" % sheet_signals)
				check("map still shown", game.get_screen_state()["state"] == 1)
				ih.inject_key(0x43) # C
			elif phase_frame == 240:
				var sheet: Dictionary = game.get_character_sheet()
				var text: PackedStringArray = sheet.get("text", PackedStringArray())
				var screen := rows()
				# ダンプの先頭の画面は C の画面の 1 行目から (start_y = 1)。端末は 80x24 より大きく C の画面は
				# 中央寄せで描かれるので、ダンプの 1 行目が出ている端末の行から比べる (左右のずれは空白を除いて比べる)
				var top := -1
				for y in screen.size():
					if screen[y].strip_edges() == text[0].strip_edges():
						top = y
						break
				check("C screen found", top >= 0, "top=%d" % top)
				var mismatches := 0
				var compared := 0
				for i in mini(20, text.size()):
					var want := text[i].strip_edges()
					var got := screen[top + i].strip_edges() if top >= 0 and top + i < screen.size() else ""
					if want == "" and got == "":
						continue
					# プレイ時間は秒単位で進むので、作った時点と C の画面を出した時点で食い違いうる
					if want.contains("Play time") or want.contains("プレイ時間"):
						continue
					compared += 1
					if want != got:
						mismatches += 1
						if mismatches <= 3:
							print("  diff row %d\n    sheet: %s\n    term:  %s" % [i, want, got])
				if mismatches > 0:
					print("--- terminal ---\n", "\n".join(screen.slice(0, 12)))
				check("matches the C screen", compared > 10 and mismatches == 0, "compared=%d mismatches=%d" % [compared, mismatches])
				ih.inject_key(0x1B)
				print("RESULT: ", "OK" if ok else "NG")
				OS.kill(OS.get_process_id())
	return false


func _next() -> void:
	phase += 1
	phase_frame = 0
