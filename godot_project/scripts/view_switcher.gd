extends Node

## 地図ビュー (3D 表示など) と端末のどちらを見せるかを、画面の状態から決める (#9294)
##
## HengbandGame の screen_state_changed を受け、地図を見ている間 (MAP / MAP_WITH_PROMPT /
## TARGETING) だけ地図ビューを見せ、メニューや全画面の文章では端末を見せる。
## 地図ビューが登録されていない、または GameState.use_map_view が false なら何もしない
## (端末だけの現状の表示と同じ)。

## 地図ビューを見せる/隠すべきときに通知する
signal map_view_requested(show: bool)

## ScreenState (src/view/screen-state.h) と同じ値
enum ScreenState { NO_GAME, MAP, MAP_WITH_PROMPT, TARGETING, OVERLAY_MENU, FULLSCREEN_TEXT }

const STATE_NAMES: Array[String] = ["NO_GAME", "MAP", "MAP_WITH_PROMPT", "TARGETING", "OVERLAY_MENU", "FULLSCREEN_TEXT"]

## 現在の画面の状態 (get_screen_state() の Dictionary)
var current: Dictionary = {}

## 地図ビュー (CanvasItem / Node3D を持つノード)。null なら切り替えない
var map_view: Node = null

var _showing_map := false


func setup(game: Node) -> void:
	game.screen_state_changed.connect(_on_screen_state_changed)


## 地図を見せてよい状態か
static func is_map_state(state: int) -> bool:
	return state == ScreenState.MAP or state == ScreenState.MAP_WITH_PROMPT or state == ScreenState.TARGETING


static func state_name(state: int) -> String:
	return STATE_NAMES[state] if state >= 0 and state < STATE_NAMES.size() else "UNKNOWN"


func _on_screen_state_changed(state: Dictionary) -> void:
	current = state
	refresh()


## 設定や地図ビューが変わったときにも呼ぶ
func refresh() -> void:
	var show := GameState.use_map_view and map_view != null and not current.is_empty() and is_map_state(current["state"])
	if show == _showing_map:
		return
	_showing_map = show
	if map_view != null and "visible" in map_view:
		map_view.visible = show
	map_view_requested.emit(show)
