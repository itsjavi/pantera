/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Ported to Pantera from brave-core (brave/brave-core@2f490708,
 * components/cosmetic_filters/renderer/cosmetic_filters_js_handler.h).
 * Pantera's changes: the browser decides whether and how a frame is
 * filtered (DECISION-20); resources always load synchronously; the element
 * picker, de-AMP and the performance tracing are gone. */

#ifndef PANTERA_COMPONENTS_COSMETIC_FILTERS_RENDERER_COSMETIC_FILTERS_JS_HANDLER_H_
#define PANTERA_COMPONENTS_COSMETIC_FILTERS_RENDERER_COSMETIC_FILTERS_JS_HANDLER_H_

#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "pantera/components/cosmetic_filters/common/cosmetic_filters.mojom.h"
#include "url/gurl.h"
#include "v8/include/v8-forward.h"

namespace content {
class RenderFrame;
}  // namespace content

namespace pantera {

// CosmeticFiltersJSHandler class is responsible for JS execution inside a
// a given render_frame. It also does interactions with CosmeticFiltersResources
// class that lives in the main process.
class CosmeticFiltersJSHandler {
 public:
  CosmeticFiltersJSHandler(content::RenderFrame* render_frame,
                           const int32_t isolated_world_id);
  CosmeticFiltersJSHandler(const CosmeticFiltersJSHandler&) = delete;
  CosmeticFiltersJSHandler& operator=(const CosmeticFiltersJSHandler&) = delete;
  ~CosmeticFiltersJSHandler();

  // Adds the "cf_worker" JavaScript object and its functions to the current
  // render_frame_.
  void AddJavaScriptObjectToFrame(v8::Local<v8::Context> context);
  // Fetches an initial set of resources to inject into the page if cosmetic
  // filtering is enabled, and returns whether or not to proceed with cosmetic
  // filtering.
  bool ProcessURL(const GURL& url);
  void ApplyRules();

 private:
  void BindFunctionsToObject(v8::Isolate* isolate,
                             v8::Local<v8::Object> javascript_object);

  // Adds a function to the provided object.
  template <typename Sig>
  void BindFunctionToObject(v8::Isolate* isolate,
                            v8::Local<v8::Object> javascript_object,
                            const std::string& name,
                            const base::RepeatingCallback<Sig>& callback);
  bool EnsureConnected();
  void OnRemoteDisconnect();

  // Injects content_cosmetic bundle (if needed) and calls the entry point.
  void ExecuteObservingBundleEntryPoint();

  void CreateWorkerObject(v8::Isolate* isolate, v8::Local<v8::Context> context);

  // A function to be called from JS
  void HiddenClassIdSelectors(const std::string& input);

  void CSSRulesRoutine(const base::DictValue& resources_dict);
  void OnHiddenClassIdSelectors(base::DictValue result);
  bool OnIsFirstParty(const std::string& url_string);

  void InjectStylesheet(const std::string& stylesheet);

  bool generichide_ = false;

  raw_ptr<content::RenderFrame> render_frame_ = nullptr;
  mojo::Remote<mojom::CosmeticFiltersResources> cosmetic_filters_resources_;
  int32_t isolated_world_id_;
  bool enabled_1st_party_cf_;
  std::vector<std::string> exceptions_;
  GURL url_;
  std::optional<base::DictValue> resources_dict_;

  // True if the content_cosmetic.bundle.js has injected in the current frame.
  bool bundle_injected_ = false;

  base::WeakPtrFactory<CosmeticFiltersJSHandler> weak_ptr_factory_{this};
};

}  // namespace pantera

#endif  // PANTERA_COMPONENTS_COSMETIC_FILTERS_RENDERER_COSMETIC_FILTERS_JS_HANDLER_H_
