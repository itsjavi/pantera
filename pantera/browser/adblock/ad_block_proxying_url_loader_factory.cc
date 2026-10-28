// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pantera/browser/adblock/ad_block_proxying_url_loader_factory.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/byte_size.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/self_deleting.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "extensions/common/constants.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/data_pipe_producer.h"
#include "mojo/public/cpp/system/string_data_source.h"
#include "net/base/isolation_info.h"
#include "net/base/net_errors.h"
#include "pantera/browser/adblock/ad_block_request_type.h"
#include "pantera/browser/adblock/ad_block_tab_helper.h"
#include "pantera/browser/pantera_browser_main_extra_parts.h"
#include "pantera/components/adblock/ad_block_service.h"
#include "pantera/components/adblock/adblock_stub_response.h"
#include "pantera/components/shields/shields_settings.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/cpp/url_loader_factory_builder.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"

namespace pantera {

namespace {

using URLLoaderFactoryType =
    content::ContentBrowserClient::URLLoaderFactoryType;

// Answers a request with the resource a `$redirect` rule names, and keeps the
// request's pipes open until the body is written. Deletes itself.
class StubResponse {
 public:
  static void Send(
      mojo::PendingReceiver<network::mojom::URLLoader> loader_receiver,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const network::ResourceRequest& request,
      const std::string& data_url) {
    auto head = network::mojom::URLResponseHead::New();
    std::string body;
    MakeStubResponse(data_url, request, &head, &body);

    mojo::Remote<network::mojom::URLLoaderClient> client_remote(
        std::move(client));
    mojo::ScopedDataPipeProducerHandle producer;
    mojo::ScopedDataPipeConsumerHandle consumer;
    if (mojo::CreateDataPipe(nullptr, producer, consumer) != MOJO_RESULT_OK) {
      client_remote->OnComplete(
          network::URLLoaderCompletionStatus(net::ERR_INSUFFICIENT_RESOURCES));
      return;
    }
    client_remote->OnReceiveResponse(std::move(head), std::move(consumer),
                                     std::nullopt);
    auto* response =
        new StubResponse(std::move(loader_receiver), std::move(client_remote),
                         std::move(body), std::move(producer));
    response->Write();
  }

 private:
  StubResponse(mojo::PendingReceiver<network::mojom::URLLoader> loader_receiver,
               mojo::Remote<network::mojom::URLLoaderClient> client,
               std::string body,
               mojo::ScopedDataPipeProducerHandle producer)
      : loader_receiver_(std::move(loader_receiver)),
        client_(std::move(client)),
        body_(std::move(body)),
        producer_(
            std::make_unique<mojo::DataPipeProducer>(std::move(producer))) {}

  void Write() {
    producer_->Write(
        std::make_unique<mojo::StringDataSource>(
            body_, mojo::StringDataSource::AsyncWritingMode::
                       STRING_STAYS_VALID_UNTIL_COMPLETION),
        base::BindOnce(&StubResponse::OnWritten, base::Unretained(this)));
  }

  void OnWritten(MojoResult result) {
    if (result == MOJO_RESULT_OK) {
      network::URLLoaderCompletionStatus status(net::OK);
      status.encoded_data_length = base::ByteSize(body_.size());
      status.encoded_body_length = base::ByteSize(body_.size());
      status.decoded_body_length = base::ByteSize(body_.size());
      client_->OnComplete(status);
    }
    delete this;
  }

  // Held, not bound: the renderer keeps its end until the response is done.
  mojo::PendingReceiver<network::mojom::URLLoader> loader_receiver_;
  mojo::Remote<network::mojom::URLLoaderClient> client_;
  std::string body_;
  std::unique_ptr<mojo::DataPipeProducer> producer_;
};

bool ShouldProxyFactory(URLLoaderFactoryType type) {
  switch (type) {
    case URLLoaderFactoryType::kNavigation:
    case URLLoaderFactoryType::kDocumentSubResource:
    case URLLoaderFactoryType::kWorkerMainResource:
    case URLLoaderFactoryType::kWorkerSubResource:
    case URLLoaderFactoryType::kServiceWorkerScript:
    case URLLoaderFactoryType::kServiceWorkerSubResource:
    case URLLoaderFactoryType::kPrefetch:
    case URLLoaderFactoryType::kEarlyHints:
      return true;
    case URLLoaderFactoryType::kDownload:
    case URLLoaderFactoryType::kDevTools:
      return false;
  }
}

}  // namespace

AdBlockProxyingURLLoaderFactory::PendingRequest::PendingRequest() = default;
AdBlockProxyingURLLoaderFactory::PendingRequest::PendingRequest(
    PendingRequest&&) = default;
AdBlockProxyingURLLoaderFactory::PendingRequest&
AdBlockProxyingURLLoaderFactory::PendingRequest::operator=(PendingRequest&&) =
    default;
AdBlockProxyingURLLoaderFactory::PendingRequest::~PendingRequest() = default;

// static
void AdBlockProxyingURLLoaderFactory::MaybeProxyRequest(
    content::BrowserContext* browser_context,
    content::RenderFrameHost* frame,
    URLLoaderFactoryType type,
    const url::Origin& request_initiator,
    const net::IsolationInfo& isolation_info,
    network::URLLoaderFactoryBuilder& factory_builder) {
  if (!browser_context || !ShouldProxyFactory(type)) {
    return;
  }
  HostContentSettingsMap* settings_map =
      HostContentSettingsMapFactory::GetForProfile(
          Profile::FromBrowserContext(browser_context));
  if (!settings_map) {
    return;
  }
  auto [receiver, remote] = factory_builder.Append();
  base::MakeSelfDeleting<AdBlockProxyingURLLoaderFactory>(
      std::move(receiver), std::move(remote),
      base::WrapRefCounted(settings_map),
      frame ? frame->GetGlobalId() : content::GlobalRenderFrameHostId(),
      request_initiator, isolation_info.top_frame_origin());
}

AdBlockProxyingURLLoaderFactory::AdBlockProxyingURLLoaderFactory(
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
    mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory,
    scoped_refptr<HostContentSettingsMap> settings_map,
    content::GlobalRenderFrameHostId frame_id,
    const url::Origin& request_initiator,
    std::optional<url::Origin> top_frame_origin,
    base::SelfDeletingPassKey pass_key)
    : network::SelfDeletingURLLoaderFactory(std::move(receiver), pass_key),
      target_factory_(std::move(target_factory)),
      settings_map_(std::move(settings_map)),
      frame_id_(frame_id),
      request_initiator_(request_initiator),
      top_frame_origin_(std::move(top_frame_origin)) {
  target_factory_.set_disconnect_handler(
      base::BindOnce(&AdBlockProxyingURLLoaderFactory::OnTargetFactoryError,
                     base::Unretained(this)));
}

AdBlockProxyingURLLoaderFactory::~AdBlockProxyingURLLoaderFactory() = default;

void AdBlockProxyingURLLoaderFactory::CreateLoaderAndStart(
    mojo::PendingReceiver<network::mojom::URLLoader> loader_receiver,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  PendingRequest pending;
  pending.loader_receiver = std::move(loader_receiver);
  pending.request_id = request_id;
  pending.options = options;
  pending.request = request;
  pending.client = std::move(client);
  pending.traffic_annotation = traffic_annotation;

  const auto resource_type =
      static_cast<blink::mojom::ResourceType>(request.resource_type);
  const url::Origin initiator =
      request.request_initiator.value_or(request_initiator_);
  AdBlockService* ad_block_service = GetAdBlockService();

  // As in Brave: only web requests, never top-level pages, and nothing an
  // extension asks for.
  if (!ad_block_service || !request.url.SchemeIsHTTPOrHTTPS() ||
      resource_type == blink::mojom::ResourceType::kMainFrame ||
      initiator.scheme() == extensions::kExtensionScheme) {
    Forward(std::move(pending));
    return;
  }

  const GURL top_frame_url = GetTopFrameUrl(request);
  if (!AreShieldsUp(*settings_map_, top_frame_url)) {
    Forward(std::move(pending));
    return;
  }
  const bool aggressive = IsAggressiveBlocking(*settings_map_, top_frame_url) ||
                          IsForcedAggressive(initiator);

  const GURL url = request.url;
  const std::string method = request.method;
  ad_block_service->ShouldStartRequest(
      url, std::string(GetAdBlockRequestType(resource_type)), initiator, method,
      aggressive,
      base::BindOnce(&AdBlockProxyingURLLoaderFactory::OnRequestChecked,
                     weak_factory_.GetWeakPtr(), std::move(pending)));
}

GURL AdBlockProxyingURLLoaderFactory::GetTopFrameUrl(
    const network::ResourceRequest& request) const {
  // Navigations carry their own isolation info; the other requests use their
  // factory's, then the tab's page, then the initiator (workers).
  if (request.trusted_params &&
      request.trusted_params->isolation_info.top_frame_origin()) {
    return request.trusted_params->isolation_info.top_frame_origin()->GetURL();
  }
  if (top_frame_origin_ && !top_frame_origin_->opaque()) {
    return top_frame_origin_->GetURL();
  }
  if (content::RenderFrameHost* frame =
          content::RenderFrameHost::FromID(frame_id_)) {
    return frame->GetOutermostMainFrame()->GetLastCommittedURL();
  }
  return request.request_initiator.value_or(request_initiator_).GetURL();
}

void AdBlockProxyingURLLoaderFactory::OnRequestChecked(
    PendingRequest pending,
    adblock::BlockerResult result) {
  // Brave's verdict: an `$important` match, or a match without an exception.
  const bool blocked =
      result.important || (result.matched && !result.has_exception);
  if (!blocked) {
    Forward(std::move(pending));
    return;
  }

  VLOG(1) << "Ad block: blocked " << pending.request.url;
  if (content::RenderFrameHost* frame =
          content::RenderFrameHost::FromID(frame_id_)) {
    AdBlockTabHelper::RecordBlockedRequest(
        content::WebContents::FromRenderFrameHost(frame), pending.request.url);
  }

  if (result.redirect.has_value && !result.redirect.value.empty()) {
    StubResponse::Send(std::move(pending.loader_receiver),
                       std::move(pending.client), pending.request,
                       std::string(result.redirect.value));
    return;
  }

  network::URLLoaderCompletionStatus status(net::ERR_BLOCKED_BY_CLIENT);
  // Images, frames and objects that were blocked take no space on the page.
  status.should_collapse_initiator = true;
  mojo::Remote<network::mojom::URLLoaderClient>(std::move(pending.client))
      ->OnComplete(status);
}

void AdBlockProxyingURLLoaderFactory::Forward(PendingRequest pending) {
  target_factory_->CreateLoaderAndStart(
      std::move(pending.loader_receiver), pending.request_id, pending.options,
      pending.request, std::move(pending.client), pending.traffic_annotation);
}

void AdBlockProxyingURLLoaderFactory::OnTargetFactoryError() {
  DisconnectReceiversAndDestroy();
}

}  // namespace pantera
