extends SceneTree

## GDScript 側の純粋な補助関数の確認 (セーブ不要。CI でも実行する)
## 使い方: godot --headless --path godot_project --script "$PWD"/tools/godot/gdscript-unit-check.gd
const I = preload("res://scripts/input_intents.gd")
func _initialize() -> void:
	var ok := true
	var cases := [
		[I.rotate_direction(8, 0.0), 8], [I.rotate_direction(8, PI / 2), 6], [I.rotate_direction(8, PI), 2],
		[I.rotate_direction(6, -PI / 2), 8], [I.rotate_direction(7, PI / 4), 8], [I.rotate_direction(5, 1.0), 5],
		[I.direction_from_vector(Vector2(0, -1)), 8], [I.direction_from_vector(Vector2(1, 1)), 3],
		[I.direction_from_vector(Vector2(-1, 0)), 4], [I.direction_from_vector(Vector2.ZERO), 0],
		[I.world_to_floor(I.floor_to_world(Vector2i(12, 34))), Vector2i(12, 34)],
	]
	for c in cases:
		if c[0] != c[1]:
			ok = false
			print("NG ", c)
	print("RESULT: ", "OK" if ok else "NG")
	quit(0 if ok else 1)
