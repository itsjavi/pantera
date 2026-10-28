// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PANTERA_RENDERER_PANTERA_RENDER_FRAME_HOOKS_H_
#define PANTERA_RENDERER_PANTERA_RENDER_FRAME_HOOKS_H_

namespace content {
class RenderFrame;
}  // namespace content

namespace pantera {

// Adds Pantera's observers, such as cosmetic filtering, to a new frame.
void RenderFrameCreated(content::RenderFrame* render_frame);

// Runs Pantera's document-start scripts, before extensions run theirs, as
// brave-core does.
void RunScriptsAtDocumentStart(content::RenderFrame* render_frame);

}  // namespace pantera

#endif  // PANTERA_RENDERER_PANTERA_RENDER_FRAME_HOOKS_H_
