extends SceneTree

## 3D の地図ビューを有効にしてセーブを読み込み、画面を PNG に保存する (目視確認用)
##
## 使い方 (描画が要るので headless ではなく仮想ディスプレイ + ソフトウェア OpenGL で動かす):
##   HB_SAVE=<セーブ> HB_SHOT=<出力 PNG> LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -s "-screen 0 1280x800x24" \
##     godot --rendering-driver opengl3 --path godot_project --script "$PWD"/tools/godot/screenshot.gd
## HB_MAP_VIEW=0 なら 3D の地図ビューを使わず端末の表示のまま撮る
## HB_CAMERA_DISTANCE=<数値> なら 3D の地図ビューのカメラをその距離に寄せる (既定 22)
## HB_FONT=<フォント名> / HB_FONT_SIZE=<大きさ> なら端末・パネルのフォントを変える (保存済みの設定より優先)
## HB_STATUS_PANEL=0 ならステータスパネル (C の画面のダンプ) を閉じて撮る
## HB_KEYS=<キーコード,...> (10 進) なら撮る前にそのキーを送る (メッセージを出すとき等。例: 6 = ^F)
## HB_ITEM_WINDOWS=1 ならメッセージ・持ち物・装備のウィンドウも開く
var save_path := OS.get_environment("HB_SAVE")
var out := OS.get_environment("HB_SHOT")
var game: Node = null
var frame := 0
func _initialize() -> void:
	var gs = root.get_node("GameState")
	gs.save_path = save_path
	# 画面の大きさの変化などで設定が保存されるので、利用者の設定ファイルを書き換えないよう別の場所へ向ける
	gs.config_path = "user://runtime-check-config.cfg"
	gs.layout_data = {}
	gs.use_map_view = OS.get_environment("HB_MAP_VIEW") != "0"
	if OS.get_environment("HB_STATUS_PANEL") == "0":
		gs.status_panel_visible = false
	if OS.get_environment("HB_FONT") != "":
		gs.font_name = OS.get_environment("HB_FONT")
	if OS.get_environment("HB_FONT_SIZE") != "":
		gs.font_size = int(OS.get_environment("HB_FONT_SIZE"))
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
	if frame == 390 and OS.get_environment("HB_CAMERA_DISTANCE") != "":
		var view = current_scene.find_child("DungeonView3D", true, false)
		if view:
			view._camera_distance = float(OS.get_environment("HB_CAMERA_DISTANCE"))
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
	# 設定から開いた状態で始まることがあるので、撮る前にも閉じておく
	if frame == 470 and OS.get_environment("HB_STATUS_PANEL") == "0" and current_scene.has_node("StatusLayer/StatusPanel"):
		current_scene.get_node("StatusLayer/StatusPanel").visible = false
	if frame == 480 and OS.get_environment("HB_KEYS") != "":
		for k in OS.get_environment("HB_KEYS").split(","):
			game.get_node("InputHandler").inject_key(int(k))
	if frame == 600:
		var v = current_scene.find_child("DungeonView3D", true, false)
		if v and v.visible:
			print("view visible=", v.visible, " size=", v.size, " counts=", v.count_instances(), " state=", game.get_screen_state()["state"], " cam=", v._camera.global_position, " vp=", v._viewport.size)
		var img := root.get_texture().get_image()
		var err := img.save_png(out)
		if err == OK:
			print("saved ", out, " ", img.get_size())
		else:
			print("FAILED to save ", out, ": ", error_string(err))
		OS.kill(OS.get_process_id())
	return false
