extends Node

## ゲーム状態の Autoload シングルトン
## タイトル画面からゲーム画面へのデータ受け渡し、および設定の永続化に使用する

## セーブファイルのパス（空文字列 = 新規ゲーム）
var save_path: String = ""

## 起動時の演出 (ロゴと標語、#9319) を見せたか。起動ごとに 1 度だけ見せる
var splash_shown: bool = false

## フォント名（SystemFont の font_names[0] に使用）
var font_name: String = "MS Gothic"

## フォントサイズ (pt)
var font_size: int = 16

## タイル表示モード: 0=なし / 1=8×8 / 2=16×16
var tile_mode: int = 0

## タイルを2倍幅で表示するかどうか
var use_bigtile: bool = false

## ウィンドウ配置 (size が 0,0 = 未保存、OS デフォルトを使用)
var window_rect: Rect2i = Rect2i(0, 0, 0, 0)

## サブウィンドウレイアウトツリー（空辞書 = デフォルト単一ペイン）
var layout_data: Dictionary = {}

## ステータスパネルの位置 (x=-1 = 未保存、初回は右上に自動配置)
var status_panel_pos: Vector2i = Vector2i(-1, -1)
## ステータスパネルの表示状態
var status_panel_visible: bool = true

## 地図を見ている間、端末の代わりに地図ビュー (3D 表示など) を見せるか (#9294)
## 地図ビューが無い現状では効果が無い
var use_map_view: bool = false

## メッセージ・持ち物・装備のウィンドウの表示状態と位置。
## キーはウィンドウの種類 (item_window.gd の Kind)、値は {"visible": bool, "x": int, "y": int} (x<0 は未保存)
var item_windows: Dictionary = {}

const _CONFIG_PATH := "user://hengband_config.cfg"
## 設定ファイルの場所。確認用のスクリプトは利用者の設定を書き換えないよう別の場所へ向ける
## (環境変数 HB_CONFIG_PATH でも変えられる)
var config_path := _CONFIG_PATH

func _ready() -> void:
	var override := OS.get_environment("HB_CONFIG_PATH")
	if override != "":
		config_path = override
	load_config()
	_restore_window()

## 保存済みウィンドウ位置・サイズを OS ウィンドウに適用する
func _restore_window() -> void:
	if window_rect.size.x >= 400 and window_rect.size.y >= 300:
		DisplayServer.window_set_size(window_rect.size)
		DisplayServer.window_set_position(window_rect.position)

func save_config() -> void:
	var cfg := ConfigFile.new()
	cfg.set_value("display", "font_name", font_name)
	cfg.set_value("display", "font_size", font_size)
	cfg.set_value("display", "tile_mode", tile_mode)
	cfg.set_value("display", "use_bigtile", use_bigtile)
	cfg.set_value("window", "x", window_rect.position.x)
	cfg.set_value("window", "y", window_rect.position.y)
	cfg.set_value("window", "w", window_rect.size.x)
	cfg.set_value("window", "h", window_rect.size.y)
	cfg.set_value("layout", "tree", layout_data)
	cfg.set_value("status_panel", "x", status_panel_pos.x)
	cfg.set_value("status_panel", "y", status_panel_pos.y)
	cfg.set_value("status_panel", "visible", status_panel_visible)
	cfg.set_value("display", "use_map_view", use_map_view)
	cfg.set_value("item_windows", "state", item_windows)
	cfg.save(config_path)

func load_config() -> void:
	var cfg := ConfigFile.new()
	if cfg.load(config_path) != OK:
		return
	font_name = cfg.get_value("display", "font_name", font_name)
	font_size = cfg.get_value("display", "font_size", font_size)
	# 旧設定 use_tiles (bool) からの移行
	var old_use_tiles: bool = cfg.get_value("display", "use_tiles", false)
	tile_mode = cfg.get_value("display", "tile_mode", 1 if old_use_tiles else 0)
	use_bigtile = cfg.get_value("display", "use_bigtile", use_bigtile)
	var wx: int = cfg.get_value("window", "x", 0)
	var wy: int = cfg.get_value("window", "y", 0)
	var ww: int = cfg.get_value("window", "w", 0)
	var wh: int = cfg.get_value("window", "h", 0)
	window_rect = Rect2i(wx, wy, ww, wh)
	layout_data = cfg.get_value("layout", "tree", {})
	var spx: int = cfg.get_value("status_panel", "x", -1)
	var spy: int = cfg.get_value("status_panel", "y", -1)
	status_panel_pos = Vector2i(spx, spy)
	status_panel_visible = cfg.get_value("status_panel", "visible", true)
	use_map_view = cfg.get_value("display", "use_map_view", false)
	item_windows = cfg.get_value("item_windows", "state", {})
