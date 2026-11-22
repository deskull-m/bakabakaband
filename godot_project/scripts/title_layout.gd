extends RefCounted

## タイトル画面の配置 (#9317)
##
## title.gd が使う。自動読み込み (GameState) に頼らないので、単体の確認 (tools/godot/gdscript-unit-check.gd) からも呼べる。

## 画面の端からの余白
const MARGIN := 24.0
## ボタンの大きさと、ボタン同士の間
const BUTTON_SIZE := Vector2(300, 48)
const BUTTON_SEPARATION := 12.0
## タイトル画像 (image/BakabakabandTitle.png、1560x1393) の中の位置 (画像の高さに対する割合)。
## 絵の上端 (その上は透明) と、サブタイトル「ALL WILL BE HACKED.」の下端
const TITLE_CONTENT_TOP := 239.0 / 1393.0
const TITLE_SUBTITLE_BOTTOM := 880.0 / 1393.0
## サブタイトルの下に出すバージョンの表記の高さと、サブタイトルとの間
const VERSION_HEIGHT := 28.0
const VERSION_GAP := 4.0


## タイトル画像の置き場所: 左下、横幅は画面の 50% (余白を除く)。高すぎるときは画面の高さの 60% に縮める。
## サブタイトルとその下のバージョンの表記が画面の下端 (余白) に届くまで下げる。
## サブタイトルより下の光の残りは画面の外へはみ出す
static func title_image_rect(vp_size: Vector2, aspect: float) -> Rect2:
	var width := vp_size.x * 0.5 - MARGIN * 1.5
	var height := width / maxf(aspect, 0.01)
	var max_height := vp_size.y * 0.6
	if height > max_height:
		height = max_height
		width = height * aspect
	var subtitle_bottom := vp_size.y - MARGIN - VERSION_HEIGHT - VERSION_GAP
	return Rect2(MARGIN, subtitle_bottom - height * TITLE_SUBTITLE_BOTTOM, maxf(width, 0.0), maxf(height, 0.0))


## タイトル画像の絵の上端 (画面の座標)
static func title_content_top(title_rect: Rect2) -> float:
	return title_rect.position.y + title_rect.size.y * TITLE_CONTENT_TOP


## バージョンの表記の置き場所: サブタイトルの真下、画像と同じ幅で中央揃え
static func version_rect(title_rect: Rect2) -> Rect2:
	var top := title_rect.position.y + title_rect.size.y * TITLE_SUBTITLE_BOTTOM + VERSION_GAP
	return Rect2(title_rect.position.x, top, title_rect.size.x, VERSION_HEIGHT)


## ボタンの置き場所: 右下の端 (右と下の余白に寄せる)。ボタンの文字は左揃えで縦に並べ、
## 画面の右半分に収まらない幅は縮める
static func button_area_rect(vp_size: Vector2, button_size: Vector2, count: int, separation: float) -> Rect2:
	var width := minf(button_size.x, vp_size.x * 0.5 - MARGIN * 1.5)
	var left := vp_size.x - MARGIN - width
	var height := button_size.y * count + separation * maxi(count - 1, 0)
	return Rect2(left, vp_size.y - MARGIN - height, maxf(width, 0.0), height)


## news_j.txt / news.txt のうちタイトル画面に出す部分: 区切りの「*」だけの行より後ろ (先頭の空行は除く)。
## 区切りの行が無ければ全体
static func news_body(text: String) -> String:
	var lines := text.replace("\r", "").split("\n")
	var start := 0
	for i in lines.size():
		var stripped := lines[i].strip_edges()
		if not stripped.is_empty() and stripped.replace("*", "").is_empty():
			start = i + 1
	while start < lines.size() and lines[start].strip_edges().is_empty():
		start += 1
	return "\n".join(lines.slice(start)).strip_edges(false, true)


## news のうち、高さ area_height に入る行数 (1 行の高さ line_height、全 total 行)
static func news_visible_lines(area_height: float, line_height: float, total: int) -> int:
	if line_height <= 0.0:
		return total
	return clampi(floori(area_height / line_height), 0, total)


## news の置き場所: 左上から、タイトル画像の絵の上端の手前まで
static func news_area_rect(vp_size: Vector2, title_rect: Rect2) -> Rect2:
	var bottom := title_content_top(title_rect) - MARGIN
	return Rect2(MARGIN, MARGIN, vp_size.x - MARGIN * 2.0, maxf(bottom - MARGIN, 0.0))
