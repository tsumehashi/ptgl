#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace ptgl::remote {
using Id = std::uint32_t;
using Bytes = std::vector<std::uint8_t>;
using Vec3 = std::array<double, 3>;
using Quaternion = std::array<double, 4>; // x, y, z, w
enum class PublishResult { Accepted, QueueFull, TooLarge, InvalidArgument, Closed };
enum class ViewKind : std::uint8_t { scene3D = 1, plot = 2 };
struct ViewDefinition { Id id = 1; ViewKind kind = ViewKind::scene3D; std::string title = "Remote"; };
struct ListenOptions { std::string address = "127.0.0.1"; std::uint16_t port = 9002; };
struct Limits {
    static constexpr std::size_t maxPayload = 4 * 1024 * 1024;
    static constexpr std::size_t maxQueued = 16 * 1024 * 1024;
    static constexpr std::size_t maxState = 64 * 1024 * 1024;
    static constexpr std::size_t maxInFlight = 8 * 1024 * 1024;
    static constexpr std::size_t maxViews = 8, maxLayers = 128, maxWidgets = 256;
    static constexpr std::size_t maxClients = 8, maxEvents = 1024, maxPoints = 100000;
};
struct GuiEvent {
    std::uint64_t peerId = 0, eventId = 0, revision = 0;
    Id viewId = 0, widgetId = 0;
    double value = 0;
    std::string text;
};
struct Statistics {
    std::uint64_t received = 0, sent = 0, droppedSamples = 0, rejected = 0;
    std::size_t clients = 0, queuedBytes = 0;
};
} // namespace ptgl::remote
