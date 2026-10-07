#ifndef PTGL_CORE_RENDERCOMMANDSENDER_H_
#define PTGL_CORE_RENDERCOMMANDSENDER_H_

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include "RenderItem.h"

namespace ptgl {
class GraphicsView;
namespace detail { class RenderCommandQueue; }

enum class SubmitResult { Accepted, Closed, QueueFull, TooLarge, InvalidArgument };

// Copyable, multiple-producer endpoint. Does not own or call the GraphicsView.
// Obtain before starting workers; copies remain safe after the view is destroyed.
class RenderCommandSender {
public:
    RenderCommandSender() = default;
    static constexpr std::size_t MaxQueuedTasks = 1024;
    static constexpr std::size_t MaxLayers = 128;
    static constexpr std::size_t MaxFrameBytes = 16 * 1024 * 1024;
    static constexpr std::size_t MaxBufferedBytes = 64 * 1024 * 1024;
    static constexpr std::size_t MaxLayerIdBytes = 256;

    // Latest accepted update wins for an ID. Retained until replaced/cleared.
    // A value copy (or move) separates the recording from the producer.
    SubmitResult submitFrame(std::string layerId, Render3DItem commands) const;
    SubmitResult clearFrame(std::string layerId) const;
    // FIFO by acceptance order. Executed once on the view thread, before a frame.
    // Capture values/owned data; callbacks must be short and must not destroy the view.
    // Accepted means queued, not completed; shutdown cancels work not yet started.
    SubmitResult post(std::function<void(GraphicsView&)> task) const;
    bool isOpen() const;

private:
    friend class GraphicsView;
    explicit RenderCommandSender(std::shared_ptr<detail::RenderCommandQueue> queue);
    std::shared_ptr<detail::RenderCommandQueue> queue_;
};
} // namespace ptgl
#endif
