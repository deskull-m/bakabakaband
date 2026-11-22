extends Control

## 起動時の演出 (#9319): メーカーロゴ → 標語 → タイトル画面
##
## タイトル画面の一番手前に重ねる。黒の幕の上でロゴをフェードイン・アウトし、続けて標語をフェードイン・アウトし、
## 最後に幕を消してタイトル画面をフェードインさせる。
## キーかマウスのボタンを押すと、ロゴからは標語へ、標語からはタイトル画面へすぐに飛ぶ。

signal finished

const Sequence = preload("res://scripts/splash_sequence.gd")
const LOGO_PATH := "res://../image/SikabaneWorksLogo.png"
## 標語の文字の大きさ (日本語の行 / 英語の行) と書体
const MOTTO_JA_FONT_SIZE := 30
const MOTTO_EN_FONT_SIZE := 22
## 狭い画面で標語を縮めるときの文字の大きさの下限
const MOTTO_MIN_FONT_SIZE := 8
const MOTTO_FONT_NAMES: PackedStringArray = ["Yu Mincho", "Hiragino Mincho ProN", "Noto Serif CJK JP", "IPAMincho", "Times New Roman", "Liberation Serif", "Serif"]

var _phase: Sequence.Phase = Sequence.Phase.LOGO
var _tween: Tween

@onready var _curtain: ColorRect = $Curtain
@onready var _logo: TextureRect = $Logo
@onready var _motto: VBoxContainer = $Motto


func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	_load_logo()
	var font := SystemFont.new()
	font.font_names = MOTTO_FONT_NAMES
	var ja: Label = $Motto/Ja
	var en: Label = $Motto/En
	ja.text = Sequence.MOTTO_JA
	en.text = Sequence.MOTTO_EN
	for label: Label in [ja, en]:
		label.add_theme_font_override("font", font)
	_curtain.color.a = 1.0
	_logo.modulate.a = 0.0
	_motto.modulate.a = 0.0
	get_viewport().size_changed.connect(_fit_layout)
	_fit_layout()
	_enter(Sequence.Phase.LOGO)


func _load_logo() -> void:
	var path := ProjectSettings.globalize_path(LOGO_PATH)
	var img := Image.load_from_file(path) if FileAccess.file_exists(path) else null
	if img == null:
		push_warning("SikabaneWorksLogo.png not found: " + path)
		return
	_logo.texture = ImageTexture.create_from_image(img)


func _fit_layout() -> void:
	var vp_size := get_viewport().get_visible_rect().size
	var aspect := 1.0
	if _logo.texture and _logo.texture.get_height() > 0:
		aspect = float(_logo.texture.get_width()) / float(_logo.texture.get_height())
	var rect := Sequence.logo_rect(vp_size, aspect)
	_logo.set_position(rect.position)
	_logo.set_size(rect.size)
	_fit_motto(vp_size.x)
	_motto.set_position(Vector2.ZERO)
	_motto.set_size(vp_size)


## 標語が画面の横幅に収まるよう文字の大きさを決める。収まるときは既定の大きさのまま
## (Label は折り返さず、中身の幅が最小の幅になるので、縮めないと右へはみ出して中央からずれる)
func _fit_motto(viewport_width: float) -> void:
	var ja: Label = $Motto/Ja
	var en: Label = $Motto/En
	var widest := 0.0
	for pair in [[ja, MOTTO_JA_FONT_SIZE], [en, MOTTO_EN_FONT_SIZE]]:
		var label: Label = pair[0]
		var font: Font = label.get_theme_font("font")
		widest = maxf(widest, font.get_string_size(label.text, HORIZONTAL_ALIGNMENT_LEFT, -1, pair[1]).x)
	var motto_scale := Sequence.motto_font_scale(widest, viewport_width)
	ja.add_theme_font_size_override("font_size", maxi(MOTTO_MIN_FONT_SIZE, floori(MOTTO_JA_FONT_SIZE * motto_scale)))
	en.add_theme_font_size_override("font_size", maxi(MOTTO_MIN_FONT_SIZE, floori(MOTTO_EN_FONT_SIZE * motto_scale)))


func _input(event: InputEvent) -> void:
	if _phase == Sequence.Phase.DONE:
		return
	var pressed: bool = (event is InputEventKey and event.pressed and not event.echo) \
		or (event is InputEventMouseButton and event.pressed) \
		or (event is InputEventJoypadButton and event.pressed)
	if not pressed:
		return
	# 押したキーやクリックがタイトル画面のボタンに届かないようにする
	get_viewport().set_input_as_handled()
	_enter(Sequence.skip_target(_phase))


## 段を切り替える。途中の演出は打ち切り、飛ばした段の表示は消す
func _enter(phase: Sequence.Phase) -> void:
	if _tween:
		_tween.kill()
	_phase = phase
	match phase:
		Sequence.Phase.LOGO:
			_tween = create_tween()
			_tween.tween_property(_logo, "modulate:a", 1.0, Sequence.LOGO_FADE_IN)
			_tween.tween_interval(Sequence.LOGO_HOLD)
			_tween.tween_property(_logo, "modulate:a", 0.0, Sequence.LOGO_FADE_OUT)
			_tween.tween_callback(_enter.bind(Sequence.Phase.MOTTO))
		Sequence.Phase.MOTTO:
			_logo.modulate.a = 0.0
			_tween = create_tween()
			_tween.tween_property(_motto, "modulate:a", 1.0, Sequence.MOTTO_FADE_IN)
			_tween.tween_interval(Sequence.MOTTO_HOLD)
			_tween.tween_property(_motto, "modulate:a", 0.0, Sequence.MOTTO_FADE_OUT)
			_tween.tween_callback(_enter.bind(Sequence.Phase.TITLE))
		Sequence.Phase.TITLE:
			_motto.modulate.a = 0.0
			# 幕を薄くしてタイトル画面をフェードインさせる。フェードイン中もボタンはまだ押せない
			_tween = create_tween()
			_tween.tween_property(_curtain, "color:a", 0.0, Sequence.TITLE_FADE_IN)
			_tween.tween_callback(_enter.bind(Sequence.Phase.DONE))
		Sequence.Phase.DONE:
			_tween = null
			visible = false
			mouse_filter = Control.MOUSE_FILTER_IGNORE
			finished.emit()
