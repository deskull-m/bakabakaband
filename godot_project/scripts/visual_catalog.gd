extends RefCounted

## 見た目の定義 (data/visuals.jsonc) を読み、地形・クリーチャー・床アイテムの見た目を決める (#9297)
##
## 解決の順番:
##   地形:        terrain.by_tag[タグ] → terrain.by_shape[形状] → 文字のビルボード
##   クリーチャー: monsters.by_race[種族 ID] → monsters.by_symbol[文字] → 文字のビルボード
##   床アイテム:  items.by_tval[種別] → 文字のビルボード
## 見つかった定義に mesh も sprite も shape も無ければ、文字のビルボードにその他の指定 (scale 等) を重ねる。
## 定義ファイルが無い・空でも、すべて端末と同じ文字・色のビルボードで描ける。
##
## resolve_*() の戻り値 (Dictionary):
##   kind:   "mesh" / "sprite" / "box" / "plane" / "glyph" / "none"
##   mesh / sprite: リソースのパス (kind が mesh / sprite のとき)
##   glyph:  文字 (String)。glyph_color: 端末の色の番号
##   scale / height: 数値。color: Color (定義に無ければ端末の色から)
##   source: どの定義から決まったか ("by_tag" / "by_shape" / "by_race" / "by_symbol" / "by_tval" / "fallback")

const DEFAULT_PATH := "res://data/visuals.jsonc"

## ObservedTerrainShape (src/view/observable-world.h) の名前
const SHAPE_NAMES: Array[String] = ["UNKNOWN", "FLOOR", "WALL", "DOOR_CLOSED", "DOOR_OPEN", "STAIRS_UP", "STAIRS_DOWN", "LIQUID", "TREE", "ENTRANCE", "OTHER_PASSABLE", "OTHER_OBSTACLE"]

var definitions: Dictionary = {}
var load_error: String = ""

## get_terrain_catalog() の結果 (id → Dictionary)
var _terrains: Dictionary = {}
## 端末の色の番号 → Color (HengbandGame.get_term_color)
var _color_provider: Callable = Callable()


## JSONC (// と /* */ のコメント・末尾のカンマを許す JSON) を読む
static func parse_jsonc(text: String) -> Variant:
	var out := ""
	var i := 0
	var in_string := false
	while i < text.length():
		var c := text[i]
		if in_string:
			out += c
			if c == "\\" and i + 1 < text.length():
				out += text[i + 1]
				i += 2
				continue
			if c == "\"":
				in_string = false
			i += 1
			continue
		if c == "\"":
			in_string = true
			out += c
			i += 1
		elif c == "/" and i + 1 < text.length() and text[i + 1] == "/":
			while i < text.length() and text[i] != "\n":
				i += 1
		elif c == "/" and i + 1 < text.length() and text[i + 1] == "*":
			var end := text.find("*/", i + 2)
			i = text.length() if end < 0 else end + 2
		else:
			out += c
			i += 1
	# 末尾のカンマ (", }" / ", ]") を取り除く
	var re := RegEx.new()
	re.compile(",(\\s*[}\\]])")
	out = re.sub(out, "$1", true)
	return JSON.parse_string(out)


func load_definitions(path: String = DEFAULT_PATH) -> bool:
	definitions = {}
	load_error = ""
	if not FileAccess.file_exists(path):
		load_error = "not found: " + path
		return false
	var parsed = parse_jsonc(FileAccess.get_file_as_string(path))
	if not (parsed is Dictionary):
		load_error = "parse error: " + path
		return false
	definitions = parsed
	return true


## 地形の一覧 (HengbandGame.get_terrain_catalog()) を覚える
func set_terrain_catalog(catalog: Array) -> void:
	_terrains = {}
	for entry: Dictionary in catalog:
		_terrains[int(entry["id"])] = entry


## 端末の色の番号を Color にする関数を与える (HengbandGame.get_term_color など)
func set_color_provider(provider: Callable) -> void:
	_color_provider = provider


func _term_color(index: int) -> Color:
	return _color_provider.call(index) if _color_provider.is_valid() else Color.WHITE


func _section(name: String, key: String) -> Dictionary:
	var section: Variant = definitions.get(name, {})
	if not (section is Dictionary):
		return {}
	var table: Variant = section.get(key, {})
	return table if table is Dictionary else {}


## 定義 1 件とフォールバックの文字から結果を作る
func _build(entry: Dictionary, source: String, glyph: String, glyph_color: int) -> Dictionary:
	var result := {
		"kind": "glyph",
		"glyph": glyph,
		"glyph_color": glyph_color,
		"scale": float(entry.get("scale", 1.0)),
		"height": float(entry.get("height", 1.0)),
		"color": Color.html(entry["color"]) if entry.has("color") else _term_color(glyph_color),
		"source": source,
	}
	if entry.has("mesh"):
		result["kind"] = "mesh"
		result["mesh"] = entry["mesh"]
	elif entry.has("sprite"):
		result["kind"] = "sprite"
		result["sprite"] = entry["sprite"]
	elif entry.has("shape"):
		result["kind"] = entry["shape"]
	return result


## 地形の見た目を決める。terrain_id が -1 (未知) なら kind = "none"
func resolve_terrain(terrain_id: int, fallback_glyph: String = "", fallback_color: int = 1) -> Dictionary:
	if terrain_id < 0:
		return { "kind": "none", "source": "unknown" }
	var info: Dictionary = _terrains.get(terrain_id, {})
	var glyph: String = info.get("symbol", fallback_glyph)
	var color: int = info.get("symbol_color", fallback_color)
	var tag: String = info.get("tag", "")
	var by_tag := _section("terrain", "by_tag")
	if tag != "" and by_tag.has(tag):
		return _build(by_tag[tag], "by_tag", glyph, color)
	var shape: int = info.get("shape", 0)
	var shape_name: String = SHAPE_NAMES[shape] if shape < SHAPE_NAMES.size() else ""
	var by_shape := _section("terrain", "by_shape")
	if by_shape.has(shape_name):
		return _build(by_shape[shape_name], "by_shape", glyph, color)
	return _build({}, "fallback", glyph, color)


## クリーチャーの見た目を決める (creature: スナップショットの creatures の 1 件)
## 幻覚中 (race_id = -1) は種族が分からないので "?" のビルボード
func resolve_creature(creature: Dictionary) -> Dictionary:
	var race_id: int = creature.get("race_id", -1)
	if race_id < 0:
		return _build({}, "fallback", "?", 10)
	var glyph: String = creature.get("symbol", "?")
	var color: int = creature.get("symbol_color", 1)
	var by_race := _section("monsters", "by_race")
	if by_race.has(str(race_id)):
		return _build(by_race[str(race_id)], "by_race", glyph, color)
	var by_symbol := _section("monsters", "by_symbol")
	if by_symbol.has(glyph):
		return _build(by_symbol[glyph], "by_symbol", glyph, color)
	return _build({}, "fallback", glyph, color)


## 床アイテムの見た目を決める (tval: スナップショットの object_tvals。0 は不明)
func resolve_item(tval: int, fallback_glyph: String, fallback_color: int) -> Dictionary:
	var by_tval := _section("items", "by_tval")
	if tval > 0 and by_tval.has(str(tval)):
		return _build(by_tval[str(tval)], "by_tval", fallback_glyph, fallback_color)
	return _build({}, "fallback", fallback_glyph, fallback_color)


## 2D タイルセット上のタイルの範囲 (タイル表示中は symbols の色・文字がタイルの行・列になる)
static func tile_region(tile_row: int, tile_col: int, cell_size: Vector2i) -> Rect2i:
	return Rect2i((tile_col & 0x7F) * cell_size.x, (tile_row & 0x7F) * cell_size.y, cell_size.x, cell_size.y)
