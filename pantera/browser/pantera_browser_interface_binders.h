// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PANTERA_BROWSER_PANTERA_BROWSER_INTERFACE_BINDERS_H_
#define PANTERA_BROWSER_PANTERA_BROWSER_INTERFACE_BINDERS_H_

#include "mojo/public/cpp/bindings/binder_map.h"

namespace content {
class RenderFrameHost;
}  // namespace content

namespace pantera {

// Adds the interfaces of Pantera's that frames can ask the browser for, such
// as their cosmetic filters. Chrome's PopulateChromeFrameBinders() calls this.
void PopulateFrameBinders(
    mojo::BinderMapWithContext<content::RenderFrameHost*>* map,
    content::RenderFrameHost* render_frame_host);

}  // namespace pantera

#endif  // PANTERA_BROWSER_PANTERA_BROWSER_INTERFACE_BINDERS_H_
