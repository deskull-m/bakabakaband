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


## 地図ビューを見せる状態か (重ねた一覧を窓に写すなら OVERLAY_MENU も含む)
static func shows_map_view(state: int, overlay_menus: bool) -> bool:
	return is_map_state(state) or (overlay_menus and state == STATE_OVERLAY_MENU)


## 地図の上に重ねている一覧の範囲 (端末の桁・行、get_screen_state() の overlay_*)。重ねていなければ大きさ 0
static func overlay_rect(state: Dictionary) -> Rect2i:
	if state.is_empty() or int(state.get("state", 0)) != STATE_OVERLAY_MENU:
		return Rect2i()
	return Rect2i(int(state.get("overlay_x", 0)), int(state.get("overlay_y", 0)), int(state.get("overlay_w", 0)), int(state.get("overlay_h", 0)))
