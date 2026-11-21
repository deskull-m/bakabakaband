extends RefCounted

## 地図の上に重ねた一覧 (持ち物・装備など、OVERLAY_MENU) の扱いの補助
##
## view_switcher.gd (地図ビューを見せるか) と main.gd (一覧を写す窓) が使う。
## 自動読み込み (GameState) に頼らないので、単体の確認 (tools/godot/gdscript-unit-check.gd) からも呼べる。

## ScreenState (src/view/screen-state.h) の値
const STATE_MAP := 1
const STATE_MAP_WITH_PROMPT := 2
const STATE_TARGETING := 3
const STATE_OVERLAY_MENU := 4


## 地図を見ている状態か (MAP / MAP_WITH_PROMPT / TARGETING)
static func is_map_state(state: int) -> bool:
	return state == STATE_MAP or state == STATE_MAP_WITH_PROMPT or state == STATE_TARGETING


## 地図ビューを見せる状態か (get_screen_state() の Dictionary)。
## 重ねた一覧を窓に写すなら OVERLAY_MENU も含めるが、写す範囲が求まらないとき (一覧を開いている間に
## 端末の大きさが変わり、比べる基準の画面と大きさが合わなくなった等) は窓に何も写せないので端末を見せる
static func shows_map_view(state: Dictionary, overlay_menus: bool) -> bool:
	if state.is_empty():
		return false
	var value := int(state.get("state", 0))
	if is_map_state(value):
		return true
	return overlay_menus and value == STATE_OVERLAY_MENU and overlay_rect(state).has_area()


## 地図の上に重ねている一覧の範囲 (端末の桁・行、get_screen_state() の overlay_*)。重ねていなければ大きさ 0
static func overlay_rect(state: Dictionary) -> Rect2i:
	if state.is_empty() or int(state.get("state", 0)) != STATE_OVERLAY_MENU:
		return Rect2i()
	return Rect2i(int(state.get("overlay_x", 0)), int(state.get("overlay_y", 0)), int(state.get("overlay_w", 0)), int(state.get("overlay_h", 0)))
