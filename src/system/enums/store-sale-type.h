#pragma once

enum class StoreSaleType : int {
    NONE = -1, //!< 店舗種別未定義
    GENERAL = 0, //!< 店舗の種類 雑貨屋
    ARMOURY = 1, //!< 店舗の種類 防具屋
    WEAPON = 2, //!< 店舗の種類 武器屋
    TEMPLE = 3, //!< 店舗の種類 寺院
    ALCHEMIST = 4, //!< 店舗の種類 錬金術の店
    MAGIC = 5, //!< 店舗の種類 魔道具屋
    BLACK = 6, //!< 店舗の種類 ブラック・マーケット
    HOME = 7, //!< 店舗の種類 我が家
    BOOK = 8, //!< 店舗の種類 書店
    MUSEUM = 9, //!< 店舗の種類 博物館
    MELINLAITO = 10, //!< 店舗の種類 メリンライトの薬品庫
    HUNAHYANDA = 11, //!< 店舗の種類 フナヒヤンダの武器蔵
    MAX,
};
