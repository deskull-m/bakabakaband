extends Node

## 地図ビュー (3D 表示など) と端末のどちらを見せるかを、画面の状態から決める (#9294)
##
## HengbandGame の screen_state_changed を受け、地図を見ている間 (MAP / MAP_WITH_PROMPT /
## TARGETING) と、地図の上に一覧を重ねている間 (OVERLAY_MENU。持ち物・装備の一覧など) は
## 地図ビューを見せ、全画面の文章 (店・知識・キャラクター情報など) では端末を見せる。
## 重ねた一覧は main.gd が地図ビューの上の半透明の窓に写す (overlay_menus_on_map が false なら
## 従来どおり端末に戻す)。
## 地図ビューが登録されていない、または GameState.use_map_view が false なら何もしない
## (端末だけの現状の表示と同じ)。

## 地図ビューを見せる/隠すべきときに通知する
signal map_view_requested(show: bool)

const ScreenOverlay = preload("res://scripts/screen_overlay.gd")

## ScreenState (src/view/screen-state.h) と同じ値
enum ScreenState { NO_GAME, MAP, MAP_WITH_PROMPT, TARGETING, OVERLAY_MENU, FULLSCREEN_TEXT }

const STATE_NAMES: Array[String] = ["NO_GAME", "MAP", "MAP_WITH_PROMPT", "TARGETING", "OVERLAY_MENU", "FULLSCREEN_TEXT"]

## 現在の画面の状態 (get_screen_state() の Dictionary)
var current: Dictionary = {}

## 地図ビュー (CanvasItem / Node3D を持つノード)。null なら切り替えない
var map_view: Node = null

var _showing_map := false

## 地図の上に重ねた一覧 (OVERLAY_MENU) の間も地図ビューを見せるか (一覧は半透明の窓に写す)
var overlay_menus_on_map := true


func setup(game: Node) -> void:
	game.screen_state_changed.connect(_on_screen_state_changed)


## 地図を見せてよい状態か
static func is_map_state(state: int) -> bool:
	return ScreenOverlay.is_map_state(state)


static func state_name(state: int) -> String:
	return STATE_NAMES[state] if state >= 0 and state < STATE_NAMES.size() else "UNKNOWN"


func _on_screen_state_changed(state: Dictionary) -> void:
	current = state
	refresh()


## 設定や地図ビューが変わったときにも呼ぶ。端末の大きさが変わると一覧の範囲も変わり、
## ゲーム側が screen_state_changed を出し直すので、ここで選び直される
func refresh() -> void:
	var show := GameState.use_map_view and map_view != null and ScreenOverlay.shows_map_view(current, overlay_menus_on_map)
	if show == _showing_map:
		return
	_showing_map = show
	if map_view != null and "visible" in map_view:
		map_view.visible = show
	map_view_requested.emit(show)
