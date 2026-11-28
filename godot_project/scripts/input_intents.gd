extends RefCounted

## 座標・方向で指定する入力の補助 (#9296)
##
## - カメラが回ると画面の上と北が一致しなくなるので、画面上の方向をゲームの方向 (テンキー配置 1-9) に直す
## - 端末のセル・3D 空間の座標をフロアの座標に直す
## 実際に送るのは HengbandGame.travel_to() / target_grid() / walk()。


## 北から時計回りに並べたゲームの方向 (テンキー配置): 北, 北東, 東, 南東, 南, 南西, 西, 北西
const DIRS_CLOCKWISE: Array[int] = [8, 9, 6, 3, 2, 1, 4, 7]


## 画面上の方向 (テンキー配置。8 = 画面の上) を、カメラの向きに合わせてゲームの方向に直す
## camera_yaw: カメラが北を向いているときを 0 とし、時計回りを正とした回転角 (ラジアン)
static func rotate_direction(screen_dir: int, camera_yaw: float) -> int:
	var index := DIRS_CLOCKWISE.find(screen_dir)
	if index < 0:
		return screen_dir
	var steps := int(round(camera_yaw / (PI / 4.0)))
	return DIRS_CLOCKWISE[posmod(index + steps, DIRS_CLOCKWISE.size())]


## 平面上のベクトル (x = 東, y = 南) を最も近い 8 方向に丸める。長さ 0 なら 0
static func direction_from_vector(v: Vector2) -> int:
	if v.length_squared() < 0.0001:
		return 0
	# 北 (y が負) を 0 とし時計回りの角度
	var angle := atan2(v.x, -v.y)
	var index := posmod(int(round(angle / (PI / 4.0))), 8)
	return DIRS_CLOCKWISE[index]


## メイン端末のセル (列, 行) をフロアの座標に直す。地図の外なら Vector2i(-1, -1)
## screen_state: HengbandGame.get_screen_state() の Dictionary
static func cell_to_floor(cell: Vector2i, screen_state: Dictionary, floor_size: Vector2i) -> Vector2i:
	if screen_state.is_empty():
		return Vector2i(-1, -1)
	var col0: int = screen_state["map_col0"]
	if cell.x < col0:
		return Vector2i(-1, -1)
	var width_per_grid := 2 if screen_state["bigtile"] else 1
	var x: int = (cell.x - col0) / width_per_grid + int(screen_state["panel_col_min"])
	var y: int = cell.y + int(screen_state["panel_row_prt"])
	if x < 0 or y < 0 or x >= floor_size.x or y >= floor_size.y:
		return Vector2i(-1, -1)
	return Vector2i(x, y)


## 3D 空間の座標をフロアの座標に直す (1 グリッド = grid_size、X 軸 = 東、Z 軸 = 南。グリッドの中心が整数 + 0.5)
static func world_to_floor(position: Vector3, grid_size: float = 1.0) -> Vector2i:
	return Vector2i(int(floor(position.x / grid_size)), int(floor(position.z / grid_size)))


## フロアの座標のグリッドの中心を 3D 空間の座標に直す (world_to_floor の逆)
static func floor_to_world(pos: Vector2i, grid_size: float = 1.0, height: float = 0.0) -> Vector3:
	return Vector3((pos.x + 0.5) * grid_size, height, (pos.y + 0.5) * grid_size)
