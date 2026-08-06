extends PanelContainer

## TerminalPane: 分割レイアウト用のターミナルペインシーン
##
## ヘッダバー（タイトル + 縦追加/横追加/閉じるボタン）と
## SubViewport を持つ GodotTerminal コンテナ。
## 分割・クローズの要求は親の main.gd にシグナルで伝達する。

signal v_split_requested(pane: Node)  ## 縦に追加（左右に並べる）
signal h_split_requested(pane: Node)  ## 横に追加（上下に並べる）
signal close_requested(pane: Node)    ## このペインを閉じる

## ゲームノードから割り当てられたターミナルインデックス
var terminal_index: int = -1
var _game_node: Node = null
## _do_split でペインが親から外れて再追加されると Godot 4.6 は _ready() を再呼び出しする。
## このフラグでシグナルの二重接続を防ぐ。
var _initialized: bool = false

## ペイン固有のフォント設定（空文字/0 = グローバル設定を使用）
var pane_font_name: String = ""
var pane_font_size: int = 0

func _ready() -> void:
	if _initialized:
		return
	_initialized = true
	$PaneVBox/Header/HBox/VSplitButton.pressed.connect(func(): v_split_requested.emit(self))
	$PaneVBox/Header/HBox/HSplitButton.pressed.connect(func(): h_split_requested.emit(self))
	$PaneVBox/Header/HBox/CloseButton.pressed.connect(func(): close_requested.emit(self))
	$PaneVBox/SubViewportContainer.resized.connect(fit_subviewport)
	add_to_group("terminal_panes")

## ターミナルを初期化してゲームノードに登録する。
## add_child() の後に呼ぶこと（ノードがシーンツリーに入ってから）。
func setup(game: Node, term_idx: int) -> void:
	terminal_index = term_idx
	_game_node = game

	$PaneVBox/Header/HBox/TitleLabel.text = "Main" if term_idx == 0 else "Sub %d" % term_idx
	$PaneVBox/Header/HBox/CloseButton.visible = (term_idx != 0)
	_apply_header_color(term_idx == 0)

	var term = $PaneVBox/SubViewportContainer/SubViewport/TerminalContainer/Terminal
	var tiles = $PaneVBox/SubViewportContainer/SubViewport/TerminalContainer/TileLayer
	game.register_terminal(term_idx, term, tiles)

func _apply_header_color(is_main: bool) -> void:
	var style := StyleBoxFlat.new()
	style.content_margin_left = 2.0
	style.content_margin_top = 0.0
	style.content_margin_right = 2.0
	style.content_margin_bottom = 0.0
	# Main: 下部バーと同色、Sub: やや薄い同系色
	style.bg_color = Color(0.16, 0.07, 0.07, 0.95) if is_main else Color(0.22, 0.10, 0.10, 0.80)
	$PaneVBox/Header.add_theme_stylebox_override("panel", style)

## このペインのターミナルにフォントを直接適用してグリッドを再フィットする
func apply_pane_font(font: Font, size: int) -> void:
	var term = $PaneVBox/SubViewportContainer/SubViewport/TerminalContainer/Terminal
	if not term:
		return
	term.set_terminal_font(font, size)
	fit_subviewport()

## SubViewport のサイズをコンテナのピクセルサイズに合わせ、
## グリッドサイズをセルサイズで割った値で更新する。
func fit_subviewport() -> void:
	var svc: SubViewportContainer = $PaneVBox/SubViewportContainer
	var term = $PaneVBox/SubViewportContainer/SubViewport/TerminalContainer/Terminal
	if not svc or not term:
		return
	var px := Vector2i(int(svc.size.x), int(svc.size.y))
	if px.x <= 0 or px.y <= 0:
		return
	# stretch=true 時は SubViewportContainer が sv.size を自動管理するため手動設定不要
	if _game_node == null or terminal_index < 0:
		return
	var cw: int = term.get_cell_width()
	var ch: int = term.get_cell_height()
	if cw <= 0 or ch <= 0:
		return
	_game_node.set_sub_window_size(terminal_index, maxi(1, px.x / cw), maxi(1, px.y / ch))
