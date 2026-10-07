#include "RenderCommandSender.h"
#include "RenderCommandQueue.h"
#include "GraphicsView.h"
#include <iostream>
#include <utility>

namespace ptgl {
RenderCommandSender::RenderCommandSender(std::shared_ptr<detail::RenderCommandQueue> queue)
    : queue_(std::move(queue)) {}

SubmitResult RenderCommandSender::submitFrame(std::string id, Render3DItem commands) const
{
    if (!isOpen()) return SubmitResult::Closed;
    if (id.empty() || id.size() > MaxLayerIdBytes) return SubmitResult::InvalidArgument;
    if (commands.byteSize() > MaxFrameBytes) return SubmitResult::TooLarge;
    if (!commands.valid()) return SubmitResult::InvalidArgument;
    return queue_->submit(std::move(id), std::make_shared<const Render3DItem>(std::move(commands)));
}

SubmitResult RenderCommandSender::clearFrame(std::string id) const
{
    if (!isOpen()) return SubmitResult::Closed;
    if (id.empty() || id.size() > MaxLayerIdBytes) return SubmitResult::InvalidArgument;
    return queue_->submit(std::move(id), nullptr);
}

SubmitResult RenderCommandSender::post(std::function<void(GraphicsView&)> task) const
{
    if (!isOpen()) return SubmitResult::Closed;
    if (!task) return SubmitResult::InvalidArgument;
    return queue_->post(std::move(task));
}

bool RenderCommandSender::isOpen() const { return queue_ && queue_->isOpen(); }

namespace detail {
namespace {
void reportFailure(const char* context)
{
    try { throw; }
    catch (const std::exception& e) { std::cerr << context << ": " << e.what() << '\n'; }
    catch (...) { std::cerr << context << ": unknown exception\n"; }
}

class SubmittedFrame : public GraphicsItem {
public:
    SubmittedFrame() { setPickable(false); }
    void replace(std::shared_ptr<const Render3DItem> frame) { frame_ = std::move(frame); failed_ = false; }
protected:
    void renderScene(Renderer3D* r) override
    {
        if (failed_) return;
        try { r->renderRecorded(*frame_); }
        catch (...) { failed_ = true; reportFailure("Submitted drawing failed"); }
    }
private:
    std::shared_ptr<const Render3DItem> frame_;
    bool failed_ = false;
};
}

SubmitResult RenderCommandQueue::submit(std::string id, std::shared_ptr<const Render3DItem> frame)
{
    // Destroy replaced recordings outside the mutex.
    std::shared_ptr<const Render3DItem> retired;
    std::lock_guard<std::mutex> lock(mutex_);
    if (!open_) return SubmitResult::Closed;
    auto it = layers_.find(id);
    if (!frame && it == layers_.end()) return SubmitResult::Accepted;
    if (it == layers_.end() && layers_.size() >= RenderCommandSender::MaxLayers)
        return SubmitResult::QueueFull;
    const auto oldBytes = it != layers_.end() && it->second.frame ? it->second.frame->byteSize() : 0;
    const auto newBytes = frame ? frame->byteSize() : 0;
    if (newBytes > RenderCommandSender::MaxBufferedBytes - (bytes_ - oldBytes))
        return SubmitResult::TooLarge;
    if (it == layers_.end()) it = layers_.emplace(std::move(id), Layer{}).first;
    retired = std::move(it->second.frame);
    it->second.frame = std::move(frame);
    it->second.dirty = true;
    bytes_ = bytes_ - oldBytes + newBytes;
    return SubmitResult::Accepted;
}

SubmitResult RenderCommandQueue::post(std::function<void(GraphicsView&)> task)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!open_) return SubmitResult::Closed;
    if (tasks_.size() >= RenderCommandSender::MaxQueuedTasks) return SubmitResult::QueueFull;
    tasks_.push_back(std::move(task));
    return SubmitResult::Accepted;
}

bool RenderCommandQueue::isOpen() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return open_;
}

void RenderCommandQueue::close()
{
    std::deque<std::function<void(GraphicsView&)>> retiredTasks;
    std::map<std::string, Layer> retiredLayers;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        open_ = false;
        tasks_.swap(retiredTasks);
        layers_.swap(retiredLayers);
        bytes_ = 0;
    } // Captured objects may call back into the sender from their destructors.
}

void RenderCommandQueue::apply(GraphicsView& view)
{
    std::deque<std::function<void(GraphicsView&)>> tasks;
    std::map<std::string, std::shared_ptr<const Render3DItem>> frames;
    {
        // In a browser, never wait for a producer holding the mutex.
        std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
        if (!lock.owns_lock() || !open_) return;
        tasks.swap(tasks_);
        for (auto it = layers_.begin(); it != layers_.end();) {
            if (it->second.dirty) frames.emplace(it->first, it->second.frame);
            it->second.dirty = false;
            if (!it->second.frame) it = layers_.erase(it);
            else ++it;
        }
    }
    // New submissions from callbacks are deferred to the next frame.
    for (auto& task : tasks) {
        if (!isOpen()) return;
        try { task(view); }
        catch (...) { reportFailure("Posted view operation failed"); }
    }
    if (!isOpen()) return;
    for (auto& frame : frames) {
        auto it = active_.find(frame.first);
        if (!frame.second) {
            if (it != active_.end()) { view.removeGraphicsItem(it->second); active_.erase(it); }
        } else {
            if (it == active_.end()) {
                auto item = std::make_shared<SubmittedFrame>();
                item->setName(frame.first);
                it = active_.emplace(frame.first, item).first;
            }
            std::static_pointer_cast<SubmittedFrame>(it->second)->replace(std::move(frame.second));
            if (it->second->graphicsWindow() != &view) view.addGraphicsItem(it->second);
        }
    }
    if (!frames.empty() || !tasks.empty()) view.notifySceneChanged();
}

void RenderCommandQueue::release(GraphicsView& view)
{
    close();
    for (auto& item : active_) view.removeGraphicsItem(item.second);
    active_.clear();
}
} // namespace detail
} // namespace ptgl
