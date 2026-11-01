#include "PluginEditor.h"
#include "UI/LookAndFeel/GranoTheme.h"

#if JUCE_WINDOWS
 // JUCE public headers don't pull windows.h into user TUs (only unity builds get it).
 // Include it explicitly so shellapi.h has the types it needs.
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #include <windows.h>
 #include <shellapi.h> // DragQueryFileW
 #include <ole2.h>     // RegisterDragDrop / RevokeDragDrop / IDropTarget
 #include <shlobj.h>   // CF_HDROP
#endif

#if JUCE_WINDOWS
namespace
{
    // Minimal OLE IDropTarget. Windows Explorer drags are OLE-based, and the
    // drag cursor (copy vs. no-drop) is decided entirely by the DROPEFFECT a
    // registered IDropTarget returns — WM_DROPFILES does not drive it on modern
    // Windows. We register one of these so the cursor shows "copy" and the drop
    // routes files to the editor. Lives on the STA message thread.
    class GranoDropTarget final : public IDropTarget
    {
    public:
        explicit GranoDropTarget(GranoAudioProcessorEditor& e) : editor_(e) {}

        // ── IUnknown ──────────────────────────────────────────────────────────
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
        {
            if (riid == IID_IUnknown || riid == IID_IDropTarget)
            {
                *ppv = static_cast<IDropTarget*>(this);
                AddRef();
                return S_OK;
            }
            *ppv = nullptr;
            return E_NOINTERFACE;
        }
        ULONG STDMETHODCALLTYPE AddRef() override { return (ULONG) ++refCount_; }
        ULONG STDMETHODCALLTYPE Release() override
        {
            const auto n = --refCount_;
            if (n == 0) delete this;
            return (ULONG) n;
        }

        // ── IDropTarget ───────────────────────────────────────────────────────
        HRESULT STDMETHODCALLTYPE DragEnter(IDataObject* obj, DWORD, POINTL, DWORD* effect) override
        {
            hasFiles_ = dataHasFiles(obj);
            *effect = hasFiles_ ? DROPEFFECT_COPY : DROPEFFECT_NONE;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE DragOver(DWORD, POINTL, DWORD* effect) override
        {
            *effect = hasFiles_ ? DROPEFFECT_COPY : DROPEFFECT_NONE;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE DragLeave() override { hasFiles_ = false; return S_OK; }
        HRESULT STDMETHODCALLTYPE Drop(IDataObject* obj, DWORD, POINTL, DWORD* effect) override
        {
            const juce::StringArray files = extractFiles(obj);
            *effect = DROPEFFECT_COPY;
            if (! files.isEmpty() && editor_.isInterestedInFileDrag(files))
                editor_.filesDropped(files, 0, 0);
            return S_OK;
        }

    private:
        static bool dataHasFiles(IDataObject* obj)
        {
            FORMATETC fmt { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
            return obj != nullptr && obj->QueryGetData(&fmt) == S_OK;
        }
        static juce::StringArray extractFiles(IDataObject* obj)
        {
            juce::StringArray out;
            FORMATETC fmt { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
            STGMEDIUM med {};
            if (obj != nullptr && obj->GetData(&fmt, &med) == S_OK)
            {
                if (auto* hDrop = static_cast<HDROP>(::GlobalLock(med.hGlobal)))
                {
                    const UINT n = ::DragQueryFileW(hDrop, 0xFFFFFFFFu, nullptr, 0);
                    for (UINT i = 0; i < n; ++i)
                    {
                        WCHAR path[MAX_PATH] = {};
                        if (::DragQueryFileW(hDrop, i, path, MAX_PATH) > 0)
                            out.add(juce::String(path));
                    }
                    ::GlobalUnlock(med.hGlobal);
                }
                ::ReleaseStgMedium(&med);
            }
            return out;
        }

        GranoAudioProcessorEditor& editor_;
        int  refCount_ { 1 };
        bool hasFiles_ { false };
    };
}
#endif

GranoAudioProcessorEditor::GranoAudioProcessorEditor(GranoAudioProcessor& p)
    : AudioProcessorEditor(&p), view_(p)
{
    setLookAndFeel(&laf_);
    addAndMakeVisible(view_);
    tooltips_.setLookAndFeel(&laf_);

    // Fixed-ratio canvas, scaled uniformly (Theme::kMinScale..kMaxScale).
    setResizable(true, true);
    setResizeLimits(juce::roundToInt(Theme::kBaseWidth * Theme::kMinScale),
                    juce::roundToInt(Theme::kBaseHeight * Theme::kMinScale),
                    juce::roundToInt(Theme::kBaseWidth * Theme::kMaxScale),
                    juce::roundToInt(Theme::kBaseHeight * Theme::kMaxScale));
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio((double)Theme::kBaseWidth / (double)Theme::kBaseHeight);
    setSize(Theme::kBaseWidth, Theme::kBaseHeight);

#if JUCE_WINDOWS
    startTimerHz(4); // retries OLE drop registration until the peer exists
#endif
}

GranoAudioProcessorEditor::~GranoAudioProcessorEditor()
{
    stopTimer();
#if JUCE_WINDOWS
    if (droppedHwnd_ != nullptr)
        ::RevokeDragDrop(static_cast<HWND>(droppedHwnd_));
    if (dropTarget_ != nullptr)
        static_cast<IDropTarget*>(dropTarget_)->Release();
#endif
    tooltips_.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}

void GranoAudioProcessorEditor::timerCallback()
{
#if JUCE_WINDOWS
    tryRegisterWindowDrop(); // no-op once dragAcceptRegistered_ = true
    if (dragAcceptRegistered_)
        stopTimer();
#endif
}

void GranoAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(Theme::C::desk);
}

void GranoAudioProcessorEditor::resized()
{
    const float scale = (float)getWidth() / (float)Theme::kBaseWidth;
    view_.setTransform(juce::AffineTransform::scale(scale));
    view_.setBounds(0, 0, Theme::kBaseWidth, Theme::kBaseHeight);
}

void GranoAudioProcessorEditor::parentHierarchyChanged()
{
#if JUCE_WINDOWS
    tryRegisterWindowDrop();
#endif
}

#if JUCE_WINDOWS
void GranoAudioProcessorEditor::tryRegisterWindowDrop()
{
    if (dragAcceptRegistered_)
        return;
    auto* peer = getPeer();
    if (peer == nullptr)
        return;
    auto* hwnd = static_cast<HWND>(peer->getNativeHandle());
    if (hwnd == nullptr)
        return;

    // JUCE registers its own OLE IDropTarget on the peer, but it routes through
    // the component hit-test and was not delivering drops here. Replace it with
    // our own target so the cursor shows "copy" and dropped files reach us.
    // RegisterDragDrop needs an STA thread — JUCE's message thread is STA, and
    // this runs on it (timer / parentHierarchyChanged callbacks).
    ::RevokeDragDrop(hwnd); // remove JUCE's target (ignored if none)
    auto* target = new GranoDropTarget(*this);
    if (::RegisterDragDrop(hwnd, target) != S_OK)
    {
        target->Release(); // registration failed — drop our ref; timer retries next tick
        return;
    }
    dropTarget_  = target; // system holds its own ref; we keep ours for cleanup
    droppedHwnd_ = hwnd;
    dragAcceptRegistered_ = true;
}
#endif

bool GranoAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& path : files)
    {
        const juce::String ext = juce::File(path).getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".aif" || ext == ".aiff" ||
            ext == ".flac" || ext == ".ogg" || ext == ".mp3")
            return true;
    }
    return false;
}

void GranoAudioProcessorEditor::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    // JUCE delivers filesDropped without a trailing fileDragExit — clear the
    // drag highlight here or it stays painted after a successful drop.
    view_.setDragHighlight(false);
    for (const auto& path : files)
        if (isInterestedInFileDrag(juce::StringArray(path)))
        {
            view_.loadIntoShownSlot(juce::File(path));
            return;
        }
}

void GranoAudioProcessorEditor::fileDragEnter(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    if (isInterestedInFileDrag(files))
        view_.setDragHighlight(true);
}

void GranoAudioProcessorEditor::fileDragExit(const juce::StringArray& /*files*/)
{
    view_.setDragHighlight(false);
}
