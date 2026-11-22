extends RefCounted

## タイトル画面の配置 (#9317)
##
## title.gd が使う。自動読み込み (GameState) に頼らないので、単体の確認 (tools/godot/gdscript-unit-check.gd) からも呼べる。

## 画面の端からの余白
const MARGIN := 24.0
## ボタンの大きさと、ボタン同士の間
const BUTTON_SIZE := Vector2(300, 48)
const BUTTON_SEPARATION := 12.0


## タイトル画像の置き場所: 左下、横幅は画面の 50% (余白を除く)。高すぎるときは画面の高さの 60% に縮める
static func title_image_rect(vp_size: Vector2, aspect: float) -> Rect2:
	var width := vp_size.x * 0.5 - MARGIN * 1.5
	var height := width / maxf(aspect, 0.01)
	var max_height := vp_size.y * 0.6
	if height > max_height:
		height = max_height
		width = height * aspect
	return Rect2(MARGIN, vp_size.y - MARGIN - height, maxf(width, 0.0), maxf(height, 0.0))


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


## news の置き場所: 左上から、タイトル画像の上端の手前まで
static func news_area_rect(vp_size: Vector2, title_rect: Rect2) -> Rect2:
	var bottom := title_rect.position.y - MARGIN
	return Rect2(MARGIN, MARGIN, vp_size.x - MARGIN * 2.0, maxf(bottom - MARGIN, 0.0))
