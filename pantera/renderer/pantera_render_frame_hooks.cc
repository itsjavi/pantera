// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pantera/renderer/pantera_render_frame_hooks.h"

#include "chrome/common/chrome_isolated_world_ids.h"
#include "pantera/components/cosmetic_filters/renderer/cosmetic_filters_js_render_frame_observer.h"

namespace pantera {

void RenderFrameCreated(content::RenderFrame* render_frame) {
  // Deletes itself with the frame.
  new CosmeticFiltersJsRenderFrameObserver(
      render_frame, ISOLATED_WORLD_ID_PANTERA_COSMETIC_FILTERS);
}

void RunScriptsAtDocumentStart(content::RenderFrame* render_frame) {
  if (auto* observer =
          CosmeticFiltersJsRenderFrameObserver::Get(render_frame)) {
    observer->RunScriptsAtDocumentStart();
  }
}

}  // namespace pantera
