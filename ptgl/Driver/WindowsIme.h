#ifndef PTGL_DRIVER_WINDOWSIME_H_
#define PTGL_DRIVER_WINDOWSIME_H_
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <imm.h>
#include <functional>
#include <string>
#include <algorithm>
#ifdef _MSC_VER
#pragma comment(lib, "imm32.lib")
#endif
namespace ptgl::detail
{
// Subclasses only this driver's HWND; the original GLFW procedure remains intact.
class WindowsIme
{
  public:
    WindowsIme(HWND window, std::function<void(std::string)> commit,
               std::function<void(std::string, int)> composition)
        : window_(window), commit_(std::move(commit)), composition_(std::move(composition))
    {
        SetPropW(window_, L"ptgl.ime", this);
        previous_ = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrW(window_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(windowProc)));
    }
    ~WindowsIme()
    {
        if (IsWindow(window_)) {
            if (previous_)
                SetWindowLongPtrW(window_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(previous_));
            RemovePropW(window_, L"ptgl.ime");
        }
    }
    bool composing() const { return composing_; }
    void cancel()
    {
        if (auto context = ImmGetContext(window_)) {
            ImmNotifyIME(context, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
            ImmReleaseContext(window_, context);
        }
        composing_ = false;
    }
    void setCaret(int x, int y, int height)
    {
        if (auto context = ImmGetContext(window_)) {
            CANDIDATEFORM candidate{};
            candidate.dwStyle = CFS_EXCLUDE;
            candidate.ptCurrentPos = {x, y + height};
            candidate.rcArea = {x, y, x + 1, y + height};
            ImmSetCandidateWindow(context, &candidate);
            COMPOSITIONFORM form{};
            form.dwStyle = CFS_POINT;
            form.ptCurrentPos = {x, y};
            ImmSetCompositionWindow(context, &form);
            ImmReleaseContext(window_, context);
        }
    }

  private:
    static std::string utf8(const std::wstring &text)
    {
        if (text.empty())
            return {};
        int n = WideCharToMultiByte(CP_UTF8, 0, text.data(), int(text.size()), nullptr, 0, nullptr, nullptr);
        std::string result(n, '\0');
        WideCharToMultiByte(CP_UTF8, 0, text.data(), int(text.size()), result.data(), n, nullptr, nullptr);
        return result;
    }
    static std::wstring read(HIMC context, DWORD kind)
    {
        LONG bytes = ImmGetCompositionStringW(context, kind, nullptr, 0);
        if (bytes <= 0)
            return {};
        std::wstring text(size_t(bytes) / sizeof(wchar_t), L'\0');
        ImmGetCompositionStringW(context, kind, text.data(), bytes);
        return text;
    }
    static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto self = static_cast<WindowsIme *>(GetPropW(window, L"ptgl.ime"));
        if (!self || !self->previous_)
            return DefWindowProcW(window, message, wParam, lParam);
        if (message == WM_IME_STARTCOMPOSITION)
            self->composing_ = true;
        if (message == WM_IME_ENDCOMPOSITION) {
            self->composing_ = false;
            self->composition_({}, 0);
        }
        if (message == WM_IME_SETCONTEXT)
            lParam &= ~ISC_SHOWUICOMPOSITIONWINDOW;
        if (message == WM_IME_COMPOSITION) {
            if (auto context = ImmGetContext(window)) {
                if (lParam & GCS_RESULTSTR) {
                    self->commit_(utf8(read(context, GCS_RESULTSTR)));
                    self->composition_({}, 0);
                }
                if (lParam & (GCS_COMPSTR | GCS_CURSORPOS)) {
                    auto text = read(context, GCS_COMPSTR);
                    LONG cursor = ImmGetCompositionStringW(context, GCS_CURSORPOS, nullptr, 0);
                    cursor = std::clamp<LONG>(cursor, 0, LONG(text.size()));
                    self->composing_ = !text.empty();
                    self->composition_(utf8(text), int(utf8(text.substr(0, cursor)).size()));
                } else if (!lParam)
                    self->composition_({}, 0);
                ImmReleaseContext(window, context);
            }
            // We commit the result ourselves. Do not also generate WM_CHAR for it.
            lParam &= ~(GCS_RESULTSTR | GCS_RESULTCLAUSE | GCS_RESULTREADSTR | GCS_RESULTREADCLAUSE);
            if (!lParam)
                return 0;
        }
        return CallWindowProcW(self->previous_, window, message, wParam, lParam);
    }
    HWND window_;
    WNDPROC previous_ = nullptr;
    bool composing_ = false;
    std::function<void(std::string)> commit_;
    std::function<void(std::string, int)> composition_;
};
} // namespace ptgl::detail
#endif
#endif
