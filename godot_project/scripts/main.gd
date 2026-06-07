extends Node

## Hengband メインエントリポイント (Phase 7)
## HengbandGame GDExtension ノードが start_game() でゲームスレッドを起動する

## lib/ ディレクトリのパス (空文字列 = 実行ファイルの隣を自動検出)
## Godot エディタでテストする場合: プロジェクトルートから見た lib/ の絶対パスを指定
@export var game_lib_path: String = ""

## フォントプリセット一覧（最後の "カスタム" は手入力用）
const FONT_PRESETS: Array[String] = [
	"MS Gothic",
	"Yu Gothic",
	"BIZ UDGothic",
	"Meiryo",
	"Noto Sans Mono CJK JP",
	"Consolas",
	"Courier New",
	"カスタム",
]

## タイルモード選択肢: インデックスが GameState.tile_mode に対応
const TILE_ITEMS: Array[String] = ["なし", "8×8.bmp", "16×16.bmp"]

## 下部ツールバーの高さ (px) — main.tscn の LayoutRoot offset_bottom と合わせること
const BOTTOM_BAR_HEIGHT: int = 45

## ウィンドウサイズ変更後の保存をデバウンスするフラグ
var _window_save_pending: bool = false

## ターミナルの最大数 (0=メイン, 1〜7=サブ)
const MAX_TERMINALS: int = 8

## TerminalPane プレハブ
const TerminalPaneScene = preload("res://scenes/terminal_pane.tscn")

## 解決済み lib/ パス（タイル読み込みなどで再利用する）
var _lib_path: String = ""

## サブターミナル番号選択ポップアップ
var _term_select_popup: PopupMenu = null
## ポップアップ表示中に保持する分割先ペインと方向
var _pending_pane: Node = null
var _pending_vertical: bool = false

## コンフィグパネルで現在選択中のペイン
var _config_target_pane: Node = null

func _ready() -> void:
	# ウィンドウ × ボタンを自前で処理するため自動終了を無効化する
	get_tree().set_auto_accept_quit(false)

	var game := $HengbandGame
	if not game:
		push_error("HengbandGame node not found")
		return

	_apply_font(game)

	# ウィンドウリサイズ時にターミナルグリッドを追従させる
	get_viewport().size_changed.connect(_on_viewport_size_changed)

	_setup_config_ui()
	_setup_term_popup()

	# lib_path が未設定の場合はプロジェクトディレクトリの隣の lib/ を使う
	_lib_path = game_lib_path
	if _lib_path.is_empty():
		_lib_path = ProjectSettings.globalize_path("res://../lib")

	# 保存済みレイアウトを復元、なければデフォルト単一ペイン
	if not GameState.layout_data.is_empty():
		var root_node := _restore_layout_node(GameState.layout_data)
		$LayoutRoot.add_child(root_node)
		_setup_panes_in_subtree(root_node, game)
		_apply_font(game)
	else:
		var main_pane := _create_terminal_pane(0)
		$LayoutRoot.add_child(main_pane)
		main_pane.setup(game, 0)

	# レイアウト確定後にグリッドをフィット（1フレーム後に実行）
	_fit_main_term.call_deferred()

	print("Hengband: lib_path = ", _lib_path)
	print("Hengband: save_path = ", GameState.save_path)
	game.start_game(_lib_path, GameState.save_path)

	# 起動時にタイル設定を適用する
	_apply_tiles(game)

	# StatusPanel に HengbandGame シグナルを接続する
	var status_panel := $StatusLayer/StatusPanel
	if status_panel and status_panel.has_method("connect_to_game"):
		status_panel.connect_to_game(game)

func _notification(what: int) -> void:
	if what == NOTIFICATION_WM_CLOSE_REQUEST:
		_save_layout_config()
		var game := $HengbandGame
		if game and game.is_game_started():
			# ゲームが起動中 → セーブ＆終了をキューに注入する。
			# ESC×2 で現在の画面から抜け、Ctrl+X で終了ダイアログを開き、
			# 'y' で確定する。
			var ih := $HengbandGame/InputHandler
			ih.inject_key(0x1B)  # ESC
			ih.inject_key(0x1B)  # ESC
			ih.inject_key(0x18)  # Ctrl+X (Save & Quit)
			ih.inject_key(0x79)  # 'y' (確認)
			# 実際の終了は Hengband の godot_quit_aux が tree->quit() を呼ぶことで行われる
		else:
			# ゲーム未起動（タイトル画面など）はそのまま終了
			get_tree().quit()

func _on_viewport_size_changed() -> void:
	_fit_main_term()
	if not _window_save_pending:
		_window_save_pending = true
		get_tree().create_timer(1.0).timeout.connect(func():
			_window_save_pending = false
			_save_layout_config()
		)

## マウスホイールでフォントサイズを拡大縮小する
func _input(event: InputEvent) -> void:
	if not (event is InputEventMouseButton) or not event.pressed:
		return
	var delta := 0
	if event.button_index == MOUSE_BUTTON_WHEEL_UP:
		delta = 1
	elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
		delta = -1
	else:
		return
	var new_size := clampi(GameState.font_size + delta, 8, 48)
	if new_size == GameState.font_size:
		return
	GameState.font_size = new_size
	# SpinBox の値をホイール操作に追従させる（signal は block して二重適用を防ぐ）
	var spin: SpinBox = $ConfigLayer/ConfigPanel/PanelVBox/FontSizeRow/FontSizeSpinBox
	spin.set_block_signals(true)
	spin.value = new_size
	spin.set_block_signals(false)
	_apply_font($HengbandGame)
	get_viewport().set_input_as_handled()

## 全ペインの SubViewport サイズとターミナルグリッドを更新する
func _fit_main_term() -> void:
	for pane in get_tree().get_nodes_in_group("terminal_panes"):
		pane.fit_subviewport()

## グローバルフォントをゲームに適用し、ペイン固有設定があれば上書き再適用する
func _apply_font(game: Node) -> void:
	var font := SystemFont.new()
	font.font_names = [GameState.font_name, "Courier New", "Monospace"]
	game.set_game_font(font, GameState.font_size)
	_fit_main_term()
	# ペイン固有フォントを再適用（set_game_font でリセットされるため）
	for pane in get_tree().get_nodes_in_group("terminal_panes"):
		if pane.pane_font_name != "" or pane.pane_font_size > 0:
			var pfn: String = pane.pane_font_name if pane.pane_font_name != "" else GameState.font_name
			var pfs: int = pane.pane_font_size if pane.pane_font_size > 0 else GameState.font_size
			var pf := SystemFont.new()
			pf.font_names = [pfn, "Courier New", "Monospace"]
			pane.apply_pane_font(pf, pfs)

## タイル設定を適用する。失敗時は tile_mode を 0 に戻して UI を更新する
func _apply_tiles(game: Node) -> void:
	match GameState.tile_mode:
		1:
			var tileset := _lib_path + "/xtra/graf/8x8.bmp"
			var mask := _lib_path + "/xtra/graf/8x8a.bmp"
			var ok: bool = game.load_tileset(tileset, mask, 8, 8, "old")
			if not ok:
				push_warning("タイルセットの読み込みに失敗しました: " + tileset)
				GameState.tile_mode = 0
				_sync_tile_ui()
			else:
				game.set_bigtile_enabled(GameState.use_bigtile)
		2:
			var tileset := _lib_path + "/xtra/graf/16x16.bmp"
			var mask := _lib_path + "/xtra/graf/mask.bmp"
			var ok: bool = game.load_tileset(tileset, mask, 16, 16, "new")
			if not ok:
				push_warning("タイルセットの読み込みに失敗しました: " + tileset)
				GameState.tile_mode = 0
				_sync_tile_ui()
			else:
				game.set_bigtile_enabled(GameState.use_bigtile)
		_:
			game.set_tile_rendering_enabled(false)
			game.set_bigtile_enabled(false)

## TileOption と BigtileCheck を GameState.tile_mode に同期する
func _sync_tile_ui() -> void:
	var tile_option: OptionButton = $ConfigLayer/ConfigPanel/PanelVBox/TileRow/TileOption
	tile_option.select(GameState.tile_mode)
	var bigtile_check: CheckBox = $ConfigLayer/ConfigPanel/PanelVBox/BigtileRow/BigtileCheck
	bigtile_check.disabled = (GameState.tile_mode == 0)

## コンフィグ UI の初期化とシグナル接続
func _setup_config_ui() -> void:
	var font_option: OptionButton = $ConfigLayer/ConfigPanel/PanelVBox/FontRow/FontOption
	for preset in FONT_PRESETS:
		font_option.add_item(preset)

	var font_edit: LineEdit = $ConfigLayer/ConfigPanel/PanelVBox/FontNameRow/FontNameEdit
	font_edit.text = GameState.font_name

	var spin: SpinBox = $ConfigLayer/ConfigPanel/PanelVBox/FontSizeRow/FontSizeSpinBox
	spin.value = GameState.font_size

	var tile_option: OptionButton = $ConfigLayer/ConfigPanel/PanelVBox/TileRow/TileOption
	for item in TILE_ITEMS:
		tile_option.add_item(item)
	tile_option.selected = GameState.tile_mode

	var bigtile_check: CheckBox = $ConfigLayer/ConfigPanel/PanelVBox/BigtileRow/BigtileCheck
	bigtile_check.button_pressed = GameState.use_bigtile
	bigtile_check.disabled = (GameState.tile_mode == 0)

	# 現在のフォント名にマッチするプリセットを選択（なければ "カスタム"）
	var idx := FONT_PRESETS.find(GameState.font_name)
	if idx < 0:
		idx = FONT_PRESETS.size() - 1
	font_option.selected = idx
	_sync_font_edit_from_preset(idx)

	$ConfigLayer/BottomBar/GearButton.pressed.connect(_on_gear_pressed)
	$ConfigLayer/BottomBar/StatusButton.pressed.connect(_on_status_toggle_pressed)
	var window_tabs: TabBar = $ConfigLayer/ConfigPanel/PanelVBox/WindowTabBar
	window_tabs.tab_changed.connect(_on_window_tab_changed)
	font_option.item_selected.connect(_on_font_preset_selected)
	font_edit.text_changed.connect(_on_font_name_changed)
	spin.value_changed.connect(_on_font_size_changed)
	tile_option.item_selected.connect(_on_tile_option_selected)
	bigtile_check.toggled.connect(_on_bigtile_check_toggled)
	$ConfigLayer/ConfigPanel/PanelVBox/ButtonRow/ApplyButton.pressed.connect(_on_apply_config)
	$ConfigLayer/ConfigPanel/PanelVBox/ButtonRow/CloseButton.pressed.connect(_on_close_config)

func _on_gear_pressed() -> void:
	var panel: Panel = $ConfigLayer/ConfigPanel
	if not panel.visible:
		_sync_window_tabs()
	panel.visible = not panel.visible

func _on_status_toggle_pressed() -> void:
	var sp := $StatusLayer/StatusPanel
	if sp:
		sp.visible = not sp.visible
		GameState.status_panel_visible = sp.visible
		GameState.save_config()

## プリセット選択時: FontNameEdit に反映してリアルタイム適用
func _on_font_preset_selected(idx: int) -> void:
	_sync_font_edit_from_preset(idx)
	_apply_config_live()

## カスタム以外はプリセット名を FontNameEdit に設定して読み取り専用にする
func _sync_font_edit_from_preset(idx: int) -> void:
	var font_edit: LineEdit = $ConfigLayer/ConfigPanel/PanelVBox/FontNameRow/FontNameEdit
	if idx < FONT_PRESETS.size() - 1:
		font_edit.text = FONT_PRESETS[idx]
		font_edit.editable = false
	else:
		font_edit.editable = true

## フォント名を直接編集したときにリアルタイム適用
func _on_font_name_changed(_text: String) -> void:
	_apply_config_live()

## フォントサイズを変更したときにリアルタイム適用
func _on_font_size_changed(_value: float) -> void:
	_apply_config_live()

## タイル選択ドロップダウンが変化したときにリアルタイム適用
func _on_tile_option_selected(idx: int) -> void:
	GameState.tile_mode = idx
	var bigtile_check: CheckBox = $ConfigLayer/ConfigPanel/PanelVBox/BigtileRow/BigtileCheck
	bigtile_check.disabled = (idx == 0)
	_apply_tiles($HengbandGame)
	if $HengbandGame.is_game_started():
		$HengbandGame/InputHandler.inject_key(0x12)  # Ctrl+R: 画面再描画

func _on_bigtile_check_toggled(toggled: bool) -> void:
	GameState.use_bigtile = toggled
	$HengbandGame.set_bigtile_enabled(toggled)
	if $HengbandGame.is_game_started():
		$HengbandGame/InputHandler.inject_key(0x12)  # Ctrl+R: 再描画で bigtile 反映

## UI の現在値を選択ペインに反映してフォントを即時適用する（保存はしない）
func _apply_config_live() -> void:
	if _config_target_pane == null:
		return
	var font_edit: LineEdit = $ConfigLayer/ConfigPanel/PanelVBox/FontNameRow/FontNameEdit
	var spin: SpinBox = $ConfigLayer/ConfigPanel/PanelVBox/FontSizeRow/FontSizeSpinBox
	var fn := font_edit.text
	var fs := int(spin.value)
	# Main (idx=0) はグローバルデフォルトも更新する
	if _config_target_pane.terminal_index == 0:
		GameState.font_name = fn
		GameState.font_size = fs
	_config_target_pane.pane_font_name = fn
	_config_target_pane.pane_font_size = fs
	# set_game_font 経由で呼ぶことで、タイルレイヤーのセルサイズも更新される
	_apply_font($HengbandGame)

## 設定を保存してパネルを閉じる
func _on_apply_config() -> void:
	_apply_config_live()
	_save_layout_config()
	$ConfigLayer/ConfigPanel.visible = false

func _on_close_config() -> void:
	$ConfigLayer/ConfigPanel.visible = false

# ---------------------------------------------------------------------------
# ウィンドウ・レイアウト保存/復元
# ---------------------------------------------------------------------------

## 現在のウィンドウ位置・サイズとレイアウトを設定に書き込む
func _save_layout_config() -> void:
	GameState.window_rect = Rect2i(
		DisplayServer.window_get_position(),
		DisplayServer.window_get_size()
	)
	var children := $LayoutRoot.get_children()
	GameState.layout_data = _serialize_layout_node(children[0]) if not children.is_empty() else {}
	GameState.save_config()

## レイアウトノードをシリアライズして辞書に変換する（再帰）
func _serialize_layout_node(node: Node) -> Dictionary:
	if node is SplitContainer:
		var children_data: Array = []
		for child in node.get_children():
			children_data.append(_serialize_layout_node(child))
		return {
			"type": "split",
			"dir": "v" if node is VSplitContainer else "h",
			"offset": node.split_offset,
			"children": children_data
		}
	return {
		"type": "pane",
		"index": node.terminal_index,
		"font_name": node.pane_font_name,
		"font_size": node.pane_font_size
	}

## 辞書からレイアウトノードを再構築する（再帰）
func _restore_layout_node(data: Dictionary) -> Node:
	if data.get("type", "") == "split":
		var split: SplitContainer
		if data.get("dir", "h") == "v":
			split = VSplitContainer.new()
		else:
			split = HSplitContainer.new()
		split.size_flags_horizontal = Control.SIZE_FILL | Control.SIZE_EXPAND
		split.size_flags_vertical = Control.SIZE_FILL | Control.SIZE_EXPAND
		split.add_theme_constant_override("separation", 8)
		split.focus_mode = Control.FOCUS_NONE
		for child_data in data.get("children", []):
			split.add_child(_restore_layout_node(child_data))
		var offset: int = data.get("offset", 0)
		if offset != 0:
			split.set_deferred("split_offset", offset)
		return split
	var idx: int = data.get("index", 0)
	var pane := _create_terminal_pane(idx)
	pane.terminal_index = idx
	pane.pane_font_name = data.get("font_name", "")
	pane.pane_font_size = data.get("font_size", 0)
	return pane

## ツリー内の全 TerminalPane に setup() を呼ぶ（add_child 後に実行）
func _setup_panes_in_subtree(node: Node, game: Node) -> void:
	if node.has_method("setup"):
		node.setup(game, node.terminal_index)
	for child in node.get_children():
		_setup_panes_in_subtree(child, game)

## 開いているペインをターミナルインデックス順に返す
func _get_sorted_panes() -> Array:
	var panes: Array = get_tree().get_nodes_in_group("terminal_panes")
	panes.sort_custom(func(a, b): return a.terminal_index < b.terminal_index)
	return panes

## 開いているペインで WindowTabBar を更新し、先頭タブを選択する
func _sync_window_tabs() -> void:
	var tabs: TabBar = $ConfigLayer/ConfigPanel/PanelVBox/WindowTabBar
	tabs.set_block_signals(true)
	while tabs.tab_count > 0:
		tabs.remove_tab(0)
	for pane in _get_sorted_panes():
		var label := "Main" if pane.terminal_index == 0 else "Sub %d" % pane.terminal_index
		tabs.add_tab(label)
	tabs.set_block_signals(false)
	if tabs.tab_count > 0:
		tabs.current_tab = 0
		_on_window_tab_changed(0)

## タブが切り替わったらそのペインのフォント設定を UI に反映する
func _on_window_tab_changed(tab_idx: int) -> void:
	var panes := _get_sorted_panes()
	if tab_idx < 0 or tab_idx >= panes.size():
		return
	_config_target_pane = panes[tab_idx]
	var fn: String = _config_target_pane.pane_font_name
	var fs: int = _config_target_pane.pane_font_size
	if fn == "":
		fn = GameState.font_name
	if fs == 0:
		fs = GameState.font_size
	_load_font_to_ui(fn, fs)

## フォント名・サイズを UI コントロールに反映する（signal はブロックして適用を抑制）
func _load_font_to_ui(fn: String, fs: int) -> void:
	var font_option: OptionButton = $ConfigLayer/ConfigPanel/PanelVBox/FontRow/FontOption
	var font_edit: LineEdit = $ConfigLayer/ConfigPanel/PanelVBox/FontNameRow/FontNameEdit
	var spin: SpinBox = $ConfigLayer/ConfigPanel/PanelVBox/FontSizeRow/FontSizeSpinBox
	font_option.set_block_signals(true)
	font_edit.set_block_signals(true)
	spin.set_block_signals(true)
	var idx := FONT_PRESETS.find(fn)
	if idx < 0:
		idx = FONT_PRESETS.size() - 1
	font_option.selected = idx
	_sync_font_edit_from_preset(idx)
	if idx == FONT_PRESETS.size() - 1:
		font_edit.text = fn  # カスタムフォントの場合は手入力値を復元
	spin.value = fs
	font_option.set_block_signals(false)
	font_edit.set_block_signals(false)
	spin.set_block_signals(false)

# ---------------------------------------------------------------------------
# 分割レイアウト管理
# ---------------------------------------------------------------------------

## TerminalPane インスタンスを生成してシグナルを接続する（add_child前に呼ぶ）
func _create_terminal_pane(_term_idx: int) -> Node:
	var pane = TerminalPaneScene.instantiate()
	pane.v_split_requested.connect(_on_v_split_requested)
	pane.h_split_requested.connect(_on_h_split_requested)
	pane.close_requested.connect(_on_close_requested)
	return pane

## サブターミナル選択ポップアップを初期化する
func _setup_term_popup() -> void:
	_term_select_popup = PopupMenu.new()
	add_child(_term_select_popup)
	_term_select_popup.id_pressed.connect(_on_term_popup_id_pressed)

## 現在使用中でないサブターミナル番号 (1〜7) を返す
func _get_available_term_indices() -> Array[int]:
	var used: Array[int] = []
	for pane in get_tree().get_nodes_in_group("terminal_panes"):
		used.append(pane.terminal_index)
	var available: Array[int] = []
	for i in range(1, MAX_TERMINALS):
		if i not in used:
			available.append(i)
	return available

## 左右分割ボタン → ポップアップでサブ番号選択
func _on_v_split_requested(pane: Node) -> void:
	_show_term_select_popup(pane, false)

## 上下分割ボタン → ポップアップでサブ番号選択
func _on_h_split_requested(pane: Node) -> void:
	_show_term_select_popup(pane, true)

## 分割方向と待機ペインを保存してサブ番号選択ポップアップを表示する
func _show_term_select_popup(pane: Node, vertical: bool) -> void:
	var available := _get_available_term_indices()
	if available.is_empty():
		return  # 空きスロットなし

	_pending_pane = pane
	_pending_vertical = vertical

	_term_select_popup.clear()
	for idx in available:
		_term_select_popup.add_item("Sub %d" % idx, idx)

	# ▶/▼ ボタン二つの直下にポップアップを配置する
	var v_btn := pane.get_node("PaneVBox/Header/HBox/VSplitButton") as Control
	var h_btn := pane.get_node("PaneVBox/Header/HBox/HSplitButton") as Control
	var vp_pos := Vector2(
		v_btn.get_global_rect().position.x,
		v_btn.get_global_rect().end.y
	)
	var screen_pos := pane.get_viewport().get_screen_transform() * vp_pos
	_term_select_popup.position = Vector2i(screen_pos)
	_term_select_popup.popup()

## ポップアップで番号が選ばれたら実際に分割する
func _on_term_popup_id_pressed(id: int) -> void:
	if _pending_pane == null:
		return
	_do_split(_pending_pane, _pending_vertical, id)
	_pending_pane = null

## pane を分割して指定インデックスの新しいターミナルペインを追加する
## vertical=true  → 上下分割 (VSplitContainer)
## vertical=false → 左右分割 (HSplitContainer)
func _do_split(pane: Node, vertical: bool, new_idx: int) -> void:
	var parent := pane.get_parent()
	var pane_pos := pane.get_index()

	# Godot 4: HSplitContainer = 左右並び, VSplitContainer = 上下積み
	var split: SplitContainer
	if vertical:
		split = VSplitContainer.new()
	else:
		split = HSplitContainer.new()
	split.size_flags_horizontal = Control.SIZE_FILL | Control.SIZE_EXPAND
	split.size_flags_vertical = Control.SIZE_FILL | Control.SIZE_EXPAND
	split.add_theme_constant_override("separation", 8)
	# フォーカスを無効化してゲームへの矢印キー入力を横取りしないようにする
	split.focus_mode = Control.FOCUS_NONE

	# 既存ペインを現在の親から外し、split を元の位置に差し込んでからペインを移す。
	parent.remove_child(pane)
	parent.add_child(split)
	parent.move_child(split, pane_pos)
	split.add_child(pane)

	var new_pane := _create_terminal_pane(new_idx)
	split.add_child(new_pane)

	new_pane.setup($HengbandGame, new_idx)
	# 両ペインの SubViewport サイズをレイアウト確定後に更新する
	pane.fit_subviewport.call_deferred()
	new_pane.fit_subviewport.call_deferred()
	_save_layout_config.call_deferred()

## ペインを閉じる（メインペインは閉じない）
func _on_close_requested(pane: Node) -> void:
	if pane.terminal_index == 0:
		return  # メインターミナルは閉じない

	# C++ 側のターミナル参照を解除する（ダングリングポインタ防止）
	$HengbandGame.unregister_terminal(pane.terminal_index)

	var parent := pane.get_parent()
	if parent is SplitContainer:
		# 兄弟ノードを探す
		var sibling: Node = null
		for child in parent.get_children():
			if child != pane:
				sibling = child
				break

		if sibling:
			var grandparent := parent.get_parent()
			var parent_pos := parent.get_index()

			# sibling を parent から外してから parent を grandparent から外す。
			# この順序により grandparent の子数が一時的に増えず、
			# SplitContainer が3子持ちになるレイアウト崩れを防ぐ。
			parent.remove_child(sibling)
			grandparent.remove_child(parent)
			grandparent.add_child(sibling)
			grandparent.move_child(sibling, parent_pos)

		parent.queue_free()  # parent ごと pane を解放
	else:
		pane.queue_free()
	_save_layout_config.call_deferred()
