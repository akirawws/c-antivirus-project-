#pragma once
#include <windows.h>

namespace Colors {
    // Бардовая палитра в стиле Kaspersky
    constexpr COLORREF BURGUNDY_PRIMARY   = RGB(163, 38, 56);   // основной бардовый
    constexpr COLORREF BURGUNDY_DARK      = RGB(120, 20, 35);   // темный бардовый (хедер)
    constexpr COLORREF BURGUNDY_MID       = RGB(200, 50, 70);   // средний бардовый (hover)
    constexpr COLORREF BURGUNDY_LIGHT     = RGB(240, 220, 225); // светлый бардовый (фон)
    constexpr COLORREF BURGUNDY_HOVER     = RGB(180, 40, 55);   // hover эффект
    
    // Темные тона для фона (стиль Kaspersky)
    constexpr COLORREF DARK_BG            = RGB(30, 30, 35);    // темный фон
    constexpr COLORREF DARK_PANEL         = RGB(40, 40, 45);    // темная панель
    constexpr COLORREF DARK_BORDER        = RGB(60, 60, 65);     // темная граница
    
    // Нейтральные тона
    constexpr COLORREF GRAY_BG            = RGB(245, 247, 250);
    constexpr COLORREF GRAY_BORDER        = RGB(220, 225, 232);
    constexpr COLORREF GRAY_TEXT          = RGB(180, 180, 185);  // светлый текст на темном
    constexpr COLORREF GRAY_DARK_TEXT     = RGB(54, 62, 73);
    constexpr COLORREF GRAY_LIGHT_TEXT    = RGB(220, 220, 225); // светлый текст

    // Состояния
    constexpr COLORREF ORANGE_ALERT       = RGB(255, 140, 0);
    constexpr COLORREF ORANGE_LIGHT       = RGB(255, 244, 230);
    constexpr COLORREF WHITE              = RGB(255, 255, 255);
    constexpr COLORREF BLACK              = RGB(0, 0, 0);
}