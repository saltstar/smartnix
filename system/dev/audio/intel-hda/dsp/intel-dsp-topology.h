
#pragma once

namespace audio {
namespace intel_hda {

// Represents a pipeline backing an audio stream.
struct DspPipeline {
    uint8_t pl_source;
    uint8_t pl_sink;
};

}  // namespace intel_hda
}  // namespace audio
