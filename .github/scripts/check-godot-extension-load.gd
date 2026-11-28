extends SceneTree

## GDExtension (bakabakaband.gdextension) が読み込まれ、
## 拡張側で登録したクラスが ClassDB に存在するかを確認する。
## 使い方: godot --headless --path godot_project --script <このファイル>

const REQUIRED_CLASSES: Array[String] = [
	"HengbandGame",
	"GodotTerminal",
	"GodotTileLayer",
	"GodotInputHandler",
	"GodotAudioManager",
]


func _initialize() -> void:
	var missing: Array[String] = []
	for name in REQUIRED_CLASSES:
		if not ClassDB.class_exists(name):
			missing.append(name)

	if missing.is_empty():
		print("GDExtension loaded: ", ", ".join(REQUIRED_CLASSES))
		quit(0)
	else:
		printerr("GDExtension classes not registered: ", ", ".join(missing))
		quit(1)
