/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Ported to Pantera from brave-core (brave/brave-core@2f490708,
 * components/cosmetic_filters/renderer/cosmetic_filters_js_handler.cc).
 * Pantera's changes: the browser decides whether and how a frame is
 * filtered (DECISION-20); resources always load synchronously; a new page
 * starts without the previous one's exceptions; Brave's feature parameters
 * are fixed at their defaults; the element picker, de-AMP and the
 * performance tracing and histograms are gone. */

#include "pantera/components/cosmetic_filters/renderer/cosmetic_filters_js_handler.h"

#include <optional>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/containers/fixed_flat_set.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/values.h"
#include "content/public/renderer/render_frame.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "pantera/components/cosmetic_filters/common/scriptlet_constants.h"
#include "pantera/components/cosmetic_filters/resources/grit/cosmetic_filters_resources.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "third_party/blink/public/platform/browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/web/web_css_origin.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/origin.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-function.h"
#include "v8/include/v8-isolate.h"
#include "v8/include/v8-object.h"
#include "v8/include/v8-primitive.h"

namespace {

constexpr auto kVettedSearchEngines = base::MakeFixedFlatSet<std::string_view>(
    {"duckduckgo", "qwant", "bing", "startpage", "google", "yandex", "ecosia",
     "brave"});

// Entry point to content_cosmetic.ts script.
constexpr char kObservingScriptletEntryPoint[] =
    "window.content_cosmetic.tryScheduleQueuePump()";

constexpr char kScriptletInitScript[] =
    R"((function() {
          let text =
              `(function() {\n%s\n` + %s + '})()';
          let script;
          try {
            script = document.createElement('script');
            const textNode = document.createTextNode(text);
            script.appendChild(textNode);
            (document.head || document.documentElement).appendChild(script);
          } catch (ex) {
            /* Unused catch */
          }
          if (script) {
            if (script.parentNode) {
              script.parentNode.removeChild(script);
            }
            script.textContent = '';
          }
        })();)";

constexpr char kPreInitScript[] =
    R"((function() {
          if (window.content_cosmetic == undefined) {
            window.content_cosmetic = {};
          }
          %s
        })();)";

constexpr char kCosmeticFilteringInitScript[] =
    R"({
        const CC = window.content_cosmetic
        if (CC.hide1pContent === undefined)
          CC.hide1pContent = %s;
        if (CC.generichide === undefined)
          CC.generichide = %s;
        if (CC.firstSelectorsPollingDelayMs === undefined)
          CC.firstSelectorsPollingDelayMs = %s;
        if (CC.switchToSelectorsPollingThreshold === undefined)
          CC.switchToSelectorsPollingThreshold = %s;
        if (CC.fetchNewClassIdRulesThrottlingMs === undefined)
          CC.fetchNewClassIdRulesThrottlingMs = %s;
       })";

constexpr char kHideSelectorsInjectScript[] =
    R"((function() {
          let nextIndex =
              window.content_cosmetic.cosmeticStyleSheet.rules.length;
          const selectors = %s;
          selectors.forEach(selector => {
            if ((typeof selector === 'string') &&
                (window.content_cosmetic.hide1pContent ||
                !window.content_cosmetic.allSelectorsToRules.has(selector))) {
              let rule = selector + '{display:none !important;}';
              try {
                window.content_cosmetic.cosmeticStyleSheet.insertRule(
                  `${rule}`, nextIndex);
                if (!window.content_cosmetic.hide1pContent) {
                  window.content_cosmetic.allSelectorsToRules.set(
                    selector, nextIndex);
                  window.content_cosmetic.firstRunQueue.add(selector);
                }
                nextIndex++;
              } catch (e) {
                console.warn('Brave Shields ignored an invalid CSS injection: ' + rule)
              }
            }
          });
          if (!document.adoptedStyleSheets.includes(
              window.content_cosmetic.cosmeticStyleSheet)) {
            document.adoptedStyleSheets =
              [window.content_cosmetic.cosmeticStyleSheet,
                ...document.adoptedStyleSheets];
          };
        })();)";

// Brave's defaults for its kCosmeticFilteringJsPerformance feature
// parameters.
constexpr char kSubFrameFirstSelectorsPollingDelayMs[] = "1000";
constexpr char kSwitchToSelectorsPollingThreshold[] = "1000";
constexpr char kFetchNewClassIdRulesThrottlingMs[] = "100";

std::string LoadDataResource(const int id) {
  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  if (resource_bundle.IsGzipped(id)) {
    return resource_bundle.LoadDataResourceString(id);
  }

  return std::string(resource_bundle.GetRawDataResource(id));
}

bool IsVettedSearchEngine(const GURL& url) {
  std::string domain_and_registry =
      net::registry_controlled_domains::GetDomainAndRegistry(
          url, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  std::optional<std::string_view> registry =
      net::registry_controlled_domains::GetRegistry(
          url, net::registry_controlled_domains::EXCLUDE_UNKNOWN_REGISTRIES,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  if (registry && domain_and_registry.length() > registry->length() + 1) {
    std::string_view host =
        std::string_view(domain_and_registry)
            .substr(0, domain_and_registry.length() - registry->length() - 1);
    if (kVettedSearchEngines.contains(host)) {
      return true;
    }
  }

  return false;
}

}  // namespace

namespace pantera {

CosmeticFiltersJSHandler::CosmeticFiltersJSHandler(
    content::RenderFrame* render_frame,
    const int32_t isolated_world_id)
    : render_frame_(render_frame),
      isolated_world_id_(isolated_world_id),
      enabled_1st_party_cf_(false) {
  EnsureConnected();
}

CosmeticFiltersJSHandler::~CosmeticFiltersJSHandler() = default;

void CosmeticFiltersJSHandler::HiddenClassIdSelectors(
    const std::string& input) {
  if (!EnsureConnected()) {
    return;
  }

  cosmetic_filters_resources_->HiddenClassIdSelectors(
      input, exceptions_,
      base::BindOnce(&CosmeticFiltersJSHandler::OnHiddenClassIdSelectors,
                     base::Unretained(this)));
}

bool CosmeticFiltersJSHandler::OnIsFirstParty(const std::string& url_string) {
  const auto url = url_.Resolve(url_string);
  if (!url.is_valid()) {
    return false;
  }

  return net::registry_controlled_domains::SameDomainOrHost(
      url, url_, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

void CosmeticFiltersJSHandler::AddJavaScriptObjectToFrame(
    v8::Local<v8::Context> context) {
  CHECK(render_frame_);
  v8::Isolate* isolate =
      render_frame_->GetWebFrame()->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope(isolate);
  if (context.IsEmpty()) {
    return;
  }

  v8::Context::Scope context_scope(context);

  CreateWorkerObject(isolate, context);
  bundle_injected_ = false;
}

// Stylesheets injected this way will be able to override `!important` styles
// from in-page styles, but cannot be reverted.
// `WebDocument::RemoveInsertedStyleSheet` works, but using a single stylesheet
// per rule has a significant performance impact and should be avoided.
void CosmeticFiltersJSHandler::InjectStylesheet(const std::string& stylesheet) {
  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();

  blink::WebStyleSheetKey* style_sheet_key = nullptr;
  blink::WebString stylesheet_webstring =
      blink::WebString::FromUtf8(stylesheet);
  web_frame->GetDocument().InsertStyleSheet(
      stylesheet_webstring, style_sheet_key, blink::WebCssOrigin::kUser);
}

void CosmeticFiltersJSHandler::CreateWorkerObject(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context) {
  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Object> cosmetic_filters_obj;
  v8::Local<v8::Value> cosmetic_filters_value;
  if (!global->Get(context, gin::StringToV8(isolate, "cf_worker"))
           .ToLocal(&cosmetic_filters_value) ||
      !cosmetic_filters_value->IsObject()) {
    cosmetic_filters_obj = v8::Object::New(isolate);
    global
        ->Set(context, gin::StringToSymbol(isolate, "cf_worker"),
              cosmetic_filters_obj)
        .Check();
    BindFunctionsToObject(isolate, cosmetic_filters_obj);
  }
}

void CosmeticFiltersJSHandler::BindFunctionsToObject(
    v8::Isolate* isolate,
    v8::Local<v8::Object> javascript_object) {
  BindFunctionToObject(
      isolate, javascript_object, "hiddenClassIdSelectors",
      base::BindRepeating(&CosmeticFiltersJSHandler::HiddenClassIdSelectors,
                          base::Unretained(this)));
  BindFunctionToObject(
      isolate, javascript_object, "isFirstPartyUrl",
      base::BindRepeating(&CosmeticFiltersJSHandler::OnIsFirstParty,
                          base::Unretained(this)));
  BindFunctionToObject(
      isolate, javascript_object, "injectStylesheet",
      base::BindRepeating(&CosmeticFiltersJSHandler::InjectStylesheet,
                          base::Unretained(this)));
}

template <typename Sig>
void CosmeticFiltersJSHandler::BindFunctionToObject(
    v8::Isolate* isolate,
    v8::Local<v8::Object> javascript_object,
    const std::string& name,
    const base::RepeatingCallback<Sig>& callback) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  // Get the isolate associated with this object.
  javascript_object
      ->Set(context, gin::StringToSymbol(isolate, name),
            gin::CreateFunctionTemplate(isolate, callback)
                ->GetFunction(context)
                .ToLocalChecked())
      .Check();
}

bool CosmeticFiltersJSHandler::EnsureConnected() {
  if (!cosmetic_filters_resources_.is_bound()) {
    render_frame_->GetBrowserInterfaceBroker().GetInterface(
        cosmetic_filters_resources_.BindNewPipeAndPassReceiver());
    cosmetic_filters_resources_.set_disconnect_handler(
        base::BindOnce(&CosmeticFiltersJSHandler::OnRemoteDisconnect,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  return cosmetic_filters_resources_.is_bound();
}

void CosmeticFiltersJSHandler::OnRemoteDisconnect() {
  cosmetic_filters_resources_.reset();
  EnsureConnected();
}

bool CosmeticFiltersJSHandler::ProcessURL(const GURL& url) {
  resources_dict_ = std::nullopt;
  url_ = url;
  enabled_1st_party_cf_ = false;
  exceptions_.clear();

  // Trivially, don't make exceptions for malformed URLs.
  if (!EnsureConnected() || url_.is_empty() || !url_.is_valid()) {
    return false;
  }

  // The top-level frame's site has the shields settings. The browser decides
  // whether to filter and whether to hide first-party content (DECISION-20).
  GURL top_frame_url = url_;
  bool cross_origin_to_top_frame = false;
  if (!render_frame_->IsMainFrame()) {
    const url::Origin top_origin(
        render_frame_->GetWebFrame()->Top()->GetSecurityOrigin());
    top_frame_url = top_origin.GetURL();
    cross_origin_to_top_frame =
        !top_origin.IsSameOriginWith(url::Origin::Create(url_));
  }

  std::optional<base::DictValue> result;
  if (!cosmetic_filters_resources_->UrlCosmeticResources(
          url_, top_frame_url, cross_origin_to_top_frame, &result) ||
      !result) {
    return false;
  }
  enabled_1st_party_cf_ = result->FindBool("hide_first_party").value_or(false);
  resources_dict_ = std::move(result);
  return true;
}

void CosmeticFiltersJSHandler::ApplyRules() {
  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
  if (!resources_dict_ || web_frame->IsProvisional()) {
    return;
  }

  std::string scriptlet_script;
  base::Value* injected_script = resources_dict_->Find("injected_script");

  if (injected_script &&
      base::JSONWriter::Write(*injected_script, &scriptlet_script)) {
    const std::string scriptlet_globals = GetScriptletGlobalsScript(
        /*is_de_amp_enabled=*/false, /*can_debug=*/false);
    scriptlet_script = absl::StrFormat(kScriptletInitScript, scriptlet_globals,
                                       scriptlet_script);
  }
  if (!scriptlet_script.empty()) {
    web_frame->ExecuteScriptInIsolatedWorld(
        isolated_world_id_,
        blink::WebScriptSource(blink::WebString::FromUtf8(scriptlet_script)),
        blink::BackForwardCacheAware::kAllow);
  }

  // Working on css rules
  generichide_ = resources_dict_->FindBool("generichide").value_or(false);
  std::string cosmetic_filtering_init_script = absl::StrFormat(
      kCosmeticFilteringInitScript, enabled_1st_party_cf_ ? "true" : "false",
      generichide_ ? "true" : "false",
      render_frame_->IsMainFrame() ? "undefined"
                                   : kSubFrameFirstSelectorsPollingDelayMs,
      kSwitchToSelectorsPollingThreshold, kFetchNewClassIdRulesThrottlingMs);
  std::string pre_init_script =
      absl::StrFormat(kPreInitScript, cosmetic_filtering_init_script);

  web_frame->ExecuteScriptInIsolatedWorld(
      isolated_world_id_,
      blink::WebScriptSource(blink::WebString::FromUtf8(pre_init_script)),
      blink::BackForwardCacheAware::kAllow);

  const std::string* procedural_actions_script =
      resources_dict_->FindString("procedural_actions_script");
  if (procedural_actions_script) {
    v8::Isolate* isolate = web_frame->GetAgentGroupScheduler()->Isolate();
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Value> v8_stylesheet =
        web_frame->ExecuteScriptInIsolatedWorldAndReturnValue(
            isolated_world_id_,
            blink::WebScriptSource(
                blink::WebString::FromUtf8(*procedural_actions_script)),
            blink::BackForwardCacheAware::kAllow);
    if (!v8_stylesheet.IsEmpty() && v8_stylesheet->IsString()) {
      v8::Local<v8::String> v8_str = v8_stylesheet.As<v8::String>();
      int length = v8_str->Utf8LengthV2(isolate);
      if (length > 0) {
        std::string str;
        gin::Converter<std::string>::FromV8(isolate, v8_str, &str);

        InjectStylesheet(str);
      }
    }
  }

  ExecuteObservingBundleEntryPoint();

  CSSRulesRoutine(*resources_dict_);
}

void CosmeticFiltersJSHandler::CSSRulesRoutine(
    const base::DictValue& resources_dict) {
  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
  const auto* cf_exceptions_list = resources_dict.FindList("exceptions");
  if (cf_exceptions_list) {
    for (const auto& item : *cf_exceptions_list) {
      if (item.is_string()) {
        exceptions_.push_back(item.GetString());
      }
    }
  }
  // If its a vetted engine AND we're not in aggressive mode, don't apply
  // cosmetic filtering from the default engine.
  const auto* hide_selectors_list =
      (IsVettedSearchEngine(url_) && !enabled_1st_party_cf_)
          ? nullptr
          : resources_dict.FindList("hide_selectors");

  std::string stylesheet = "";

  if (hide_selectors_list && !hide_selectors_list->empty()) {
    // treat `hide_selectors` the same as `force_hide_selectors` if aggressive
    // mode is enabled.
    if (enabled_1st_party_cf_) {
      for (auto& selector : *hide_selectors_list) {
        if (selector.is_string()) {
          stylesheet += selector.GetString() + "{display:none !important}";
        }
      }
    } else {
      std::string json_selectors;
      base::JSONWriter::Write(*hide_selectors_list, &json_selectors);
      if (json_selectors.empty()) {
        json_selectors = "[]";
      }
      // Building a script for stylesheet modifications
      std::string new_selectors_script =
          absl::StrFormat(kHideSelectorsInjectScript, json_selectors);
      web_frame->ExecuteScriptInIsolatedWorld(
          isolated_world_id_,
          blink::WebScriptSource(
              blink::WebString::FromUtf8(new_selectors_script)),
          blink::BackForwardCacheAware::kAllow);
    }
  }

  const auto* force_hide_selectors_list =
      resources_dict.FindList("force_hide_selectors");
  if (force_hide_selectors_list) {
    for (auto& selector : *force_hide_selectors_list) {
      if (selector.is_string()) {
        stylesheet += selector.GetString() + "{display:none !important}";
      }
    }
  }

  if (!stylesheet.empty()) {
    InjectStylesheet(stylesheet);
  }

  if (!enabled_1st_party_cf_) {
    ExecuteObservingBundleEntryPoint();
  }
}

void CosmeticFiltersJSHandler::OnHiddenClassIdSelectors(
    base::DictValue result) {
  if (generichide_) {
    return;
  }

  base::ListValue* hide_selectors = result.FindList("hide_selectors");
  if (!hide_selectors) {
    return;
  }

  base::ListValue* force_hide_selectors =
      result.FindList("force_hide_selectors");

  if (force_hide_selectors && force_hide_selectors->size() != 0) {
    std::string stylesheet = "";
    for (auto& selector : *force_hide_selectors) {
      if (selector.is_string()) {
        stylesheet += selector.GetString() + "{display:none !important}";
      }
    }
    InjectStylesheet(stylesheet);
  }

  // If its a vetted engine AND we're not in aggressive
  // mode, don't check elements from the default engine (in hide_selectors).
  if (!enabled_1st_party_cf_ && IsVettedSearchEngine(url_)) {
    return;
  }

  if (enabled_1st_party_cf_) {
    std::string stylesheet = "";
    for (auto& selector : *hide_selectors) {
      if (selector.is_string()) {
        stylesheet += selector.GetString() + "{display:none !important}";
      }
    }
    InjectStylesheet(stylesheet);
  } else {
    blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
    std::string json_selectors;
    if (!base::JSONWriter::Write(*hide_selectors, &json_selectors) ||
        json_selectors.empty()) {
      json_selectors = "[]";
    }
    // Building a script for stylesheet modifications
    std::string new_selectors_script =
        absl::StrFormat(kHideSelectorsInjectScript, json_selectors);
    if (hide_selectors->size() != 0) {
      web_frame->ExecuteScriptInIsolatedWorld(
          isolated_world_id_,
          blink::WebScriptSource(
              blink::WebString::FromUtf8(new_selectors_script)),
          blink::BackForwardCacheAware::kAllow);
    }

    if (!enabled_1st_party_cf_) {
      ExecuteObservingBundleEntryPoint();
    }
  }
}

void CosmeticFiltersJSHandler::ExecuteObservingBundleEntryPoint() {
  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
  CHECK(web_frame);

  if (!bundle_injected_) {
    static base::NoDestructor<std::string> s_observing_script(
        LoadDataResource(IDR_PANTERA_COSMETIC_FILTERS_CONTENT_COSMETIC_JS));
    bundle_injected_ = true;

    web_frame->ExecuteScriptInIsolatedWorld(
        isolated_world_id_,
        blink::WebScriptSource(blink::WebString::FromUtf8(*s_observing_script)),
        blink::BackForwardCacheAware::kAllow);

    // kObservingScriptletEntryPoint was called by `s_observing_script`.
    return;
  }

  web_frame->ExecuteScriptInIsolatedWorld(
      isolated_world_id_,
      blink::WebScriptSource(
          blink::WebString::FromUtf8(kObservingScriptletEntryPoint)),
      blink::BackForwardCacheAware::kAllow);
}

}  // namespace pantera
