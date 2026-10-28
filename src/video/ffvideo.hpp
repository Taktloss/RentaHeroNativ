// SPDX-License-Identifier: GPL-3.0-or-later
// VideoDecoder über FFmpeg (Cinepak, Indeo 3/5), bildweise ohne Uhr (docs/VIDEO.md).
#pragma once

#include <memory>
#include <string>

#include "core/bytes.hpp"
#include "engine/aviimage.hpp"

namespace rah {

// AVI aus dem Speicher öffnen; nullptr bei Fehler
std::unique_ptr<VideoDecoder> openFfmpegVideo(Bytes data, std::string* error);

}  // namespace rah
