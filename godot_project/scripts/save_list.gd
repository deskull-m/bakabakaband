extends RefCounted

## ロードするセーブデータの一覧 (#9320) の並べ替えと表示用の整形
##
## title.gd が使う。自動読み込み (GameState) に頼らないので、単体の確認 (tools/godot/gdscript-unit-check.gd) からも呼べる。
## 一覧の各行は HengbandGame.scan_save_files() の Dictionary
## (name / level / personality / race / class / play_time。読めなかった数値は -1、名前は空)。

## 列。並び順は表の左から
enum Column { NAME, LEVEL, PERSONALITY, RACE, CLASS, PLAY_TIME }

## 各列の見出し (日本語 / 英語)
const HEADERS_JA := ["名前", "レベル", "性格", "種族", "職業", "プレイ時間"]
const HEADERS_EN := ["Name", "Level", "Personality", "Race", "Class", "Play Time"]

## 読めなかった項目に出す文字
const UNKNOWN_TEXT := "?"

## 列ごとに並べ替えに使う値の名前と、数値の列か
const SORT_KEYS := ["name", "level", "personality", "race", "class", "play_time"]
const NUMERIC_COLUMNS := [Column.LEVEL, Column.PLAY_TIME]


static func headers(japanese: bool) -> Array:
	return HEADERS_JA if japanese else HEADERS_EN


## 最初に開いたときの並べ方 (列と昇順か)。レベルの高い順
static func default_sort() -> Array:
	return [Column.LEVEL, false]


## 見出しを押したときの並べ方。同じ列なら向きを逆に、違う列なら名前などは昇順・数値は降順から始める
static func next_sort(current_column: int, current_ascending: bool, clicked_column: int) -> Array:
	if clicked_column == current_column:
		return [clicked_column, not current_ascending]
	return [clicked_column, not NUMERIC_COLUMNS.has(clicked_column)]


## 並べ替えた写しを返す。読めなかった値 (-1 / 空) は向きによらず末尾に置き、同じ値どうしは名前順にする
static func sort_entries(entries: Array, column: int, ascending: bool) -> Array:
	var sorted := entries.duplicate()
	var key: String = SORT_KEYS[column]
	var numeric := NUMERIC_COLUMNS.has(column)
	sorted.sort_custom(func(a: Dictionary, b: Dictionary) -> bool:
		var va = a.get(key, -1 if numeric else "")
		var vb = b.get(key, -1 if numeric else "")
		var unknown_a := _is_unknown(va, numeric)
		var unknown_b := _is_unknown(vb, numeric)
		if unknown_a != unknown_b:
			return unknown_b
		if not unknown_a and va != vb:
			if numeric:
				return va < vb if ascending else va > vb
			var order := String(va).naturalnocasecmp_to(String(vb))
			return order < 0 if ascending else order > 0
		return String(a.get("name", "")).naturalnocasecmp_to(String(b.get("name", ""))) < 0
	)
	return sorted


static func _is_unknown(value, numeric: bool) -> bool:
	if numeric:
		return int(value) < 0
	return String(value).is_empty()


## 表に出す文字列
static func cell_text(entry: Dictionary, column: int) -> String:
	match column:
		Column.LEVEL:
			var level := int(entry.get("level", -1))
			return UNKNOWN_TEXT if level < 0 else str(level)
		Column.PLAY_TIME:
			return format_play_time(int(entry.get("play_time", -1)))
		_:
			var text := String(entry.get(SORT_KEYS[column], ""))
			return UNKNOWN_TEXT if text.is_empty() else text


## プレイ時間 [秒] を「時:分:秒」にする (端末版の C コマンドの画面と同じ形)。記録が無ければ "?"
static func format_play_time(seconds: int) -> String:
	if seconds < 0:
		return UNKNOWN_TEXT
	return "%d:%02d:%02d" % [seconds / 3600, (seconds / 60) % 60, seconds % 60]
