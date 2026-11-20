#pragma once
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <memory>

// All drawing resources belong to the menu dialog, not the SDL runtime.
class MenuSkin {
    ULONG_PTR token{};
    IStream *stream{};
    std::unique_ptr<Gdiplus::Image> cover;
    HFONT normal{}, bold{};
    HBRUSH night{}, surface{}, gold{};

  public:
    void open(HWND window);
    ~MenuSkin();
    void background(HWND window, HDC dc) const;
    INT_PTR colors(HWND control, HDC dc, UINT message) const;
    void draw(const DRAWITEMSTRUCT &item, int page) const;
};
