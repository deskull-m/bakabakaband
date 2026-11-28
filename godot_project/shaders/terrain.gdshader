// 3D の地図ビューの地形 (dungeon_view_3d.gd の MultiMesh) 用
//
// - テクスチャは Texture2DArray の 1 枚 (層) をインスタンスごとに選ぶ (INSTANCE_CUSTOM.r)。層 0 は無地
// - 色はインスタンスの色 (端末の地形の色 × 明るさの段階) をテクスチャに掛ける
// - UV はワールド座標から作る (1 グリッド = テクスチャ 1 枚)。箱の大きさ・高さに依らず模様の縮尺が揃い、
//   隣り合う壁の模様もつながる。上面・側面は法線で使い分ける
shader_type spatial;

uniform sampler2DArray terrain_textures : source_color, filter_linear_mipmap_anisotropic, repeat_enable;

varying vec3 world_pos;
varying vec3 world_normal;
varying flat float layer;

void vertex() {
	world_pos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
	world_normal = normalize((MODEL_MATRIX * vec4(NORMAL, 0.0)).xyz);
	layer = INSTANCE_CUSTOM.r;
}

void fragment() {
	vec3 n = abs(world_normal);
	vec2 uv;
	if (n.y > 0.5) {
		uv = world_pos.xz;
	} else if (n.x > n.z) {
		uv = vec2(world_pos.z, -world_pos.y);
	} else {
		uv = vec2(world_pos.x, -world_pos.y);
	}
	vec3 tex = texture(terrain_textures, vec3(uv, layer)).rgb;
	ALBEDO = COLOR.rgb * tex;
	ROUGHNESS = 0.9;
}
