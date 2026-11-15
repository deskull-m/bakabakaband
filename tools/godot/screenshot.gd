extends SceneTree

## 3D の地図ビューを有効にしてセーブを読み込み、画面を PNG に保存する (目視確認用)
##
## 使い方 (描画が要るので headless ではなく仮想ディスプレイ + ソフトウェア OpenGL で動かす):
##   HB_SAVE=<セーブ> HB_SHOT=<出力 PNG> LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -s "-screen 0 1280x800x24" \
##     godot --rendering-driver opengl3 --path godot_project --script "$PWD"/tools/godot/screenshot.gd
## HB_ITEM_WINDOWS=1 ならメッセージ・持ち物・装備のウィンドウも開く
var save_path := OS.get_environment("HB_SAVE")
var out := OS.get_environment("HB_SHOT")
var game: Node = null
var frame := 0
func _initialize() -> void:
	var gs = root.get_node("GameState")
	gs.save_path = save_path
	gs.layout_data = {}
	gs.use_map_view = true
	if OS.get_environment("HB_ITEM_WINDOWS") == "1":
		gs.item_windows = {
			0: { "visible": true, "x": 440, "y": 520 },
			1: { "visible": true, "x": 850, "y": 280 },
			2: { "visible": true, "x": 8, "y": 40 },
		}
	change_scene_to_file("res://scenes/main.tscn")
func _process(_d: float) -> bool:
	frame += 1
	if game == null:
		var scene = current_scene
		if scene and scene.has_node("HengbandGame"):
			game = scene.get_node("HengbandGame")
		return false
	if Time.get_ticks_msec() > 150000:
		OS.kill(OS.get_process_id())
	if frame == 400:
		game.get_node("InputHandler").inject_key(0x1B)
		game.get_node("InputHandler").inject_key(0x12)
	# ウィンドウを開くときは、食料を落として拾い直して新しいメッセージを出す
	if OS.get_environment("HB_ITEM_WINDOWS") == "1" and frame > 400 and frame < 580 and frame % 30 == 0:
		var ih = game.get_node("InputHandler")
		if (frame / 30) % 2 == 0:
			for k in [0x64, 0x61, 0x0D]:
				ih.inject_key(k)
		else:
			ih.inject_key(0x67)
	if frame == 600:
		var v = current_scene.find_child("DungeonView3D", true, false)
		print("view visible=", v.visible, " size=", v.size, " counts=", v.count_instances(), " state=", game.get_screen_state()["state"], " cam=", v._camera.global_position, " vp=", v._viewport.size)
		var img := root.get_texture().get_image()
		img.save_png(out)
		print("saved ", out, " ", img.get_size())
		OS.kill(OS.get_process_id())
	return false
