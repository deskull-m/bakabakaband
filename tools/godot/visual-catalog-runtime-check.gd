extends SceneTree

## 見た目の定義 (#9297) を実ゲームのフロアで確かめる
##
## 使い方:
##   HB_SAVE=<使い捨てのセーブファイル> godot --headless --path godot_project --script "$PWD"/tools/godot/visual-catalog-runtime-check.gd
##
## 地形の一覧を取り、ワールドスナップショットの既知のグリッド・見えているクリーチャー・床アイテムの
## すべてについて見た目が決まること (どれも "none" や空の文字にならないこと) を確かめ、
## 定義のどこから決まったかを数える。

const VisualCatalog = preload("res://scripts/visual_catalog.gd")

var save_path := OS.get_environment("HB_SAVE")
var game: Node = null
var frame := 0


func _initialize() -> void:
	var gs = root.get_node("GameState")
	gs.save_path = save_path
	# 画面の大きさの変化などで設定が保存されるので、利用者の設定ファイルを書き換えないよう別の場所へ向ける
	gs.config_path = "user://runtime-check-config.cfg"
	gs.layout_data = {}
	change_scene_to_file("res://scenes/main.tscn")


var observing := false


func _process(_delta: float) -> bool:
	frame += 1
	if game == null:
		var scene = current_scene
		if scene and scene.has_node("HengbandGame"):
			game = scene.get_node("HengbandGame")
		return false
	if frame % 30 == 0:
		game.set_sub_window_size(0, 100, 36)
	if Time.get_ticks_msec() > 150000:
		print("RESULT: timeout")
		OS.kill(OS.get_process_id())
	if game.get_player_status().is_empty() or frame < 150:
		return false
	# プレイヤーが揃ってから 1 度だけ観測を始め、最初のスナップショットができるまで待つ
	if not observing:
		observing = true
		game.set_world_snapshot_enabled(true)
		game.get_node("InputHandler").inject_key(0x12)
		return false
	if game.get_world_snapshot().is_empty():
		return false

	var catalog: Array = game.get_terrain_catalog()
	var vc := VisualCatalog.new()
	var loaded := vc.load_definitions()
	vc.set_terrain_catalog(catalog)
	vc.set_color_provider(game.get_term_color)
	var s: Dictionary = game.get_world_snapshot()
	var terrain: PackedInt32Array = s["terrain"]
	var tvals: PackedByteArray = s["object_tvals"]
	var symbols: PackedByteArray = s["symbols"]
	var sources := {}
	var kinds := {}
	var bad := 0
	for i in terrain.size():
		if terrain[i] < 0:
			continue
		var v := vc.resolve_terrain(terrain[i])
		sources[v["source"]] = sources.get(v["source"], 0) + 1
		kinds[v["kind"]] = kinds.get(v["kind"], 0) + 1
		if v["kind"] == "none" or (v["kind"] == "glyph" and v["glyph"] == ""):
			bad += 1
	var creature_sources := {}
	for c: Dictionary in s["creatures"]:
		var v := vc.resolve_creature(c)
		creature_sources[v["source"]] = creature_sources.get(v["source"], 0) + 1
		if v["glyph"] == "":
			bad += 1
	var items := 0
	for i in tvals.size():
		if tvals[i] != 0:
			var v := vc.resolve_item(tvals[i], char(symbols[i * 4 + 1]), symbols[i * 4])
			items += 1
			if v["glyph"] == "":
				bad += 1
	print("definitions loaded=%s terrains in catalog=%d" % [loaded, catalog.size()])
	print("terrain sources=%s kinds=%s" % [sources, kinds])
	print("creatures=%d sources=%s  floor items=%d" % [s["creatures"].size(), creature_sources, items])
	print("sample terrain[1]=%s  color(4)=%s" % [catalog[1], game.get_term_color(4)])
	print("RESULT: ", "OK" if loaded and bad == 0 and catalog.size() > 0 else "NG (%d unresolved)" % bad)
	OS.kill(OS.get_process_id())
	return true
