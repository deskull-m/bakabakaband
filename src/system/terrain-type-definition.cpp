/*
 * @brief 地形特性定義
 * @author Hourier
 * @date 2022/10/15
 */

#include "system/terrain-type-definition.h"
#include "grid/feature.h" // 暫定、is_ascii_graphics() は別ファイルに移す.
#include "grid/lighting-colors-table.h"

TerrainType::TerrainType()
    : symbol_definitions(DEFAULT_SYMBOLS)
    , symbol_configs(DEFAULT_SYMBOLS)
{
}

bool TerrainType::is_permanent_wall() const
{
    return this->flags.has_all_of({ TerrainCharacteristics::WALL, TerrainCharacteristics::PERMANENT });
}

/*!
 * @brief 地形のライティング状況をリセットする
 * @param is_config 設定値ならばtrue、定義値ならばfalse (定義値が入るのは初期化時のみ)
 */
void TerrainType::reset_lighting(bool is_config)
{
    auto &symbols = is_config ? this->symbol_configs : this->symbol_definitions;
    if (is_ascii_graphics(symbols[F_LIT_STANDARD].color)) {
        this->reset_lighting_ascii(symbols);
        return;
    }

    this->reset_lighting_graphics(symbols);
}

void TerrainType::reset_lighting_ascii(std::map<int, DisplaySymbol> &symbols)
{
    const auto color_standard = symbols[F_LIT_STANDARD].color;
    const auto character_standard = symbols[F_LIT_STANDARD].character;
    symbols[F_LIT_LITE].color = lighting_colours[color_standard & 0x0f][0];
    symbols[F_LIT_DARK].color = lighting_colours[color_standard & 0x0f][1];
    for (int i = F_LIT_NS_BEGIN; i < F_LIT_MAX; i++) {
        symbols[i].character = character_standard;
    }
}

void TerrainType::reset_lighting_graphics(std::map<int, DisplaySymbol> &symbols)
{
    const auto color_standard = symbols[F_LIT_STANDARD].color;
    const auto character_standard = symbols[F_LIT_STANDARD].character;
    for (auto i = F_LIT_NS_BEGIN; i < F_LIT_MAX; i++) {
        symbols[i].color = color_standard;
    }

    symbols[F_LIT_LITE].character = character_standard + 2;
    symbols[F_LIT_DARK].character = character_standard + 1;
}

TerrainList TerrainList::instance{};

TerrainList &TerrainList::get_instance()
{
    return instance;
}

std::vector<TerrainType> &TerrainList::get_raw_vector()
{
    return this->terrains;
}

TerrainType &TerrainList::get_terrain(short terrain_id)
{
    return this->terrains.at(terrain_id);
}

const TerrainType &TerrainList::get_terrain(short terrain_id) const
{
    return this->terrains.at(terrain_id);
}

std::vector<TerrainType>::iterator TerrainList::begin()
{
    return this->terrains.begin();
}

std::vector<TerrainType>::const_iterator TerrainList::begin() const
{
    return this->terrains.cbegin();
}

std::vector<TerrainType>::reverse_iterator TerrainList::rbegin()
{
    return this->terrains.rbegin();
}

std::vector<TerrainType>::const_reverse_iterator TerrainList::rbegin() const
{
    return this->terrains.crbegin();
}

std::vector<TerrainType>::iterator TerrainList::end()
{
    return this->terrains.end();
}

std::vector<TerrainType>::const_iterator TerrainList::end() const
{
    return this->terrains.cend();
}

std::vector<TerrainType>::reverse_iterator TerrainList::rend()
{
    return this->terrains.rend();
}

std::vector<TerrainType>::const_reverse_iterator TerrainList::rend() const
{
    return this->terrains.crend();
}

size_t TerrainList::size() const
{
    return this->terrains.size();
}

bool TerrainList::empty() const
{
    return this->terrains.empty();
}

void TerrainList::resize(size_t new_size)
{
    this->terrains.resize(new_size);
}
