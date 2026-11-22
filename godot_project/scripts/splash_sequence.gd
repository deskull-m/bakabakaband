extends RefCounted

## 起動時の演出 (#9319) の段取りと配置
##
## メーカーロゴ → 標語 → タイトル画面 の順に見せる。splash.gd が使う。
## 自動読み込み (GameState) に頼らないので、単体の確認 (tools/godot/gdscript-unit-check.gd) からも呼べる。

enum Phase { LOGO, MOTTO, TITLE, DONE }

## ロゴの横幅 (画面の横幅に対する割合)
const LOGO_WIDTH_RATIO := 0.33
## ロゴの高さの上限 (画面の高さに対する割合)。横長の画面でもはみ出さないように
const LOGO_MAX_HEIGHT_RATIO := 0.6

## 各段のフェードインの時間・表示し続ける時間・フェードアウトの時間 (秒)
const LOGO_FADE_IN := 1.0
const LOGO_HOLD := 1.5
const LOGO_FADE_OUT := 0.8
const MOTTO_FADE_IN := 1.0
const MOTTO_HOLD := 2.0
const MOTTO_FADE_OUT := 0.8
const TITLE_FADE_IN := 1.0

## 標語 (Issue #9319 の画像の文字)
const MOTTO_JA := "この門をくぐる者は一切の良識を捨てよ"
const MOTTO_EN := "Abandon all decency, ye who dare enter here."


## ロゴの置き場所: 画面の中央、横幅は画面の 33%
static func logo_rect(vp_size: Vector2, aspect: float) -> Rect2:
	var width := vp_size.x * LOGO_WIDTH_RATIO
	var height := width / maxf(aspect, 0.01)
	var max_height := vp_size.y * LOGO_MAX_HEIGHT_RATIO
	if height > max_height:
		height = max_height
		width = height * aspect
	return Rect2((vp_size - Vector2(width, height)) * 0.5, Vector2(width, height))


## 標語の左右に空ける余白
const MOTTO_MARGIN := 24.0


## 標語の文字の縮め方 (1.0 = 縮めない)。既定の大きさでの最も長い行の幅が、画面の横幅から余白を除いた幅に
## 収まらないときだけ、両方の行を同じ割合で縮める (日本語と英語の行の大きさの比を保つ)
static func motto_font_scale(widest_line_width: float, viewport_width: float) -> float:
	var available := viewport_width - MOTTO_MARGIN * 2.0
	if widest_line_width <= 0.0 or widest_line_width <= available:
		return 1.0
	return maxf(available, 0.0) / widest_line_width


## キーやマウスのボタンを押したときに飛ぶ先。ロゴからは標語へ、標語 (とタイトルのフェードイン) からはタイトルへ
static func skip_target(phase: Phase) -> Phase:
	match phase:
		Phase.LOGO:
			return Phase.MOTTO
		Phase.MOTTO, Phase.TITLE:
			return Phase.DONE
	return Phase.DONE
