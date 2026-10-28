/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Ported to Pantera from brave-core (brave/brave-core@2f490708,
 * components/cosmetic_filters/renderer/
 * cosmetic_filters_js_render_frame_observer.cc).
 * Pantera's changes: resources always load synchronously, so a flag replaces
 * the one-shot event, and de-AMP is gone. */

#include "pantera/components/cosmetic_filters/renderer/cosmetic_filters_js_render_frame_observer.h"

#include <memory>
#include <optional>

#include "base/check_op.h"
#include "content/public/renderer/render_frame.h"
#include "pantera/components/cosmetic_filters/renderer/cosmetic_filters_js_handler.h"
#include "third_party/blink/public/platform/web_isolated_world_info.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "url/origin.h"

namespace pantera {

namespace {

constexpr char kSecurityOrigin[] = "chrome://cosmetic_filters";

void EnsureIsolatedWorldInitialized(int world_id) {
  static std::optional<int> last_used_world_id;
  if (last_used_world_id) {
    // Early return since the isolated world info. is already initialized.
    DCHECK_EQ(*last_used_world_id, world_id)
        << "EnsureIsolatedWorldInitialized should always be called with the "
           "same |world_id|";
    return;
  }

  last_used_world_id = world_id;

  // Set an empty CSP so that the main world's CSP is not used in the isolated
  // world.
  constexpr char kContentSecurityPolicy[] = "";

  blink::WebIsolatedWorldInfo info;
  info.security_origin =
      blink::WebSecurityOrigin::Create(GURL(kSecurityOrigin));
  info.content_security_policy =
      blink::WebString::FromUtf8(kContentSecurityPolicy);
  blink::SetIsolatedWorldInfo(world_id, info);
}

}  // namespace

CosmeticFiltersJsRenderFrameObserver::CosmeticFiltersJsRenderFrameObserver(
    content::RenderFrame* render_frame,
    const int32_t isolated_world_id)
    : RenderFrameObserver(render_frame),
      RenderFrameObserverTracker<CosmeticFiltersJsRenderFrameObserver>(
          render_frame),
      isolated_world_id_(isolated_world_id),
      native_javascript_handle_(
          std::make_unique<CosmeticFiltersJSHandler>(render_frame,
                                                     isolated_world_id)) {}

CosmeticFiltersJsRenderFrameObserver::~CosmeticFiltersJsRenderFrameObserver() =
    default;

void CosmeticFiltersJsRenderFrameObserver::DidStartNavigation(
    const GURL& url,
    std::optional<blink::WebNavigationType> navigation_type) {
  url_ = url;
}

void CosmeticFiltersJsRenderFrameObserver::ReadyToCommitNavigation(
    blink::WebDocumentLoader* document_loader) {
  ready_ = false;

  // There could be empty, invalid and "about:blank" URLs,
  // they should fallback to the main frame rules
  if (url_.is_empty() || !url_.is_valid() || url_.spec() == "about:blank") {
    url_ = url::Origin(render_frame()->GetWebFrame()->GetSecurityOrigin())
               .GetURL();
  }

  if (!url_.SchemeIsHTTPOrHTTPS()) {
    return;
  }

  ready_ = native_javascript_handle_->ProcessURL(url_);
}

void CosmeticFiltersJsRenderFrameObserver::RunScriptsAtDocumentStart() {
  if (ready_) {
    native_javascript_handle_->ApplyRules();
  }
}

void CosmeticFiltersJsRenderFrameObserver::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (world_id != isolated_world_id_) {
    return;
  }

  native_javascript_handle_->AddJavaScriptObjectToFrame(context);
}

void CosmeticFiltersJsRenderFrameObserver::DidCreateNewDocument() {
  EnsureIsolatedWorldInitialized(isolated_world_id_);
}

void CosmeticFiltersJsRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace pantera
