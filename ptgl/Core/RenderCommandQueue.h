#ifndef PTGL_CORE_RENDERCOMMANDQUEUE_H_
#define PTGL_CORE_RENDERCOMMANDQUEUE_H_

#include "RenderCommandSender.h"
#include "GraphicsItem.h"
#include <deque>
#include <map>
#include <mutex>

namespace ptgl { namespace detail {
// Internal queue: mutex protects producers; active items belong to the view thread.
class RenderCommandQueue {
public:
    SubmitResult submit(std::string id, std::shared_ptr<const Render3DItem> frame);
    SubmitResult post(std::function<void(GraphicsView&)> task);
    bool isOpen() const;
    void close();
    void apply(GraphicsView& view);
    void release(GraphicsView& view);
private:
    struct Layer {
        std::shared_ptr<const Render3DItem> frame;
        bool dirty = true;
    };
    mutable std::mutex mutex_;
    bool open_ = true;
    std::size_t bytes_ = 0;
    std::deque<std::function<void(GraphicsView&)>> tasks_;
    std::map<std::string, Layer> layers_;
    std::map<std::string, GraphicsItemPtr> active_;
};
} }
#endif
