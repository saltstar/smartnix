
#include <cobalt-client/cpp/types-internal.h>
#include <lib/fidl/cpp/vector_view.h>

namespace cobalt_client {
namespace internal {

template <typename T> EventBuffer<T>::EventBuffer(EventBuffer&& other) {
    flushing_.store(other.flushing_.load());
    buffer_ = fbl::move(other.buffer_);
}

template <typename T> EventBuffer<T>::~EventBuffer() = default;

// Supported types for cobalt's metric types.
// Counter.
template class EventBuffer<uint32_t>;
// Histogram.
template class EventBuffer<fidl::VectorView<HistogramBucket>>;

} // namespace internal
} // namespace cobalt_client
