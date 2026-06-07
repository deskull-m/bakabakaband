extends PanelContainer

## ステータスパネル (GODOT_RICH_UI)
##
## HengbandGame の player_status_changed シグナル (Dictionary) を受信し、
## プレイヤーの主要ステータスを画面右上に常時表示する。
## main.gd の _ready() から connect_to_game(game) で接続される。

## 能力値の内部キー (snapshot_to_dict の stat_<key>) と短縮表示名
const STAT_KEYS := ["str", "int", "wis", "dex", "con", "chr"]
const STAT_NAMES := ["筋", "知", "賢", "器", "耐", "魅"]

@onready var _name_label: Label = $VBox/NameLabel
@onready var _race_class_label: Label = $VBox/RaceClassLabel
@onready var _level_label: Label = $VBox/LevelLabel
@onready var _hp_label: Label = $VBox/HpLabel
@onready var _sp_label: Label = $VBox/SpLabel
@onready var _ac_label: Label = $VBox/AcLabel
@onready var _speed_label: Label = $VBox/SpeedLabel
@onready var _gold_label: Label = $VBox/GoldLabel
@onready var _depth_label: Label = $VBox/DepthLabel
@onready var _stats_label: Label = $VBox/StatsLabel

func _ready() -> void:
	visible = GameState.status_panel_visible

## HengbandGame ノードのステータス更新シグナルに接続する。
## 既に接続済みの場合は二重接続しない。
func connect_to_game(game: Node) -> void:
	if game == null:
		return
	if not game.is_connected("player_status_changed", Callable(self, "_on_status_changed")):
		game.connect("player_status_changed", Callable(self, "_on_status_changed"))
	# 初期表示を即時反映する (シグナルは変化時のみ発火するため)
	if game.has_method("get_player_status"):
		var snapshot: Dictionary = game.get_player_status()
		if not snapshot.is_empty():
			_on_status_changed(snapshot)

func _on_status_changed(status: Dictionary) -> void:
	_name_label.text = str(status.get("name", ""))
	_race_class_label.text = "%s %s" % [status.get("race", ""), status.get("class", "")]
	_level_label.text = "Lv %d" % int(status.get("level", 0))
	_hp_label.text = "HP %d / %d" % [int(status.get("hp", 0)), int(status.get("max_hp", 0))]
	_sp_label.text = "SP %d / %d" % [int(status.get("sp", 0)), int(status.get("max_sp", 0))]
	_ac_label.text = "AC %d" % int(status.get("ac", 0))
	_speed_label.text = "速度 %+d" % int(status.get("speed", 0))
	_gold_label.text = "$ %d" % int(status.get("gold", 0))
	_depth_label.text = _format_depth(int(status.get("dun_level", 0)))

	var parts: Array[String] = []
	for i in STAT_KEYS.size():
		var raw := int(status.get("stat_" + STAT_KEYS[i], 0))
		parts.append("%s%s" % [STAT_NAMES[i], _format_stat(raw)])
	_stats_label.text = "  ".join(parts)

## 内部能力値 (18 超は 18 + xx で格納) を "18/xxx" 形式に整形する
func _format_stat(value: int) -> String:
	if value <= 18:
		return str(value)
	var bonus := value - 18
	if bonus >= 220:
		return "18/***"
	return "18/%d" % bonus

## ダンジョン深度を表示用文字列に整形する
func _format_depth(dun_level: int) -> String:
	if dun_level <= 0:
		return "地上"
	return "%d F (%d')" % [dun_level, dun_level * 50]
