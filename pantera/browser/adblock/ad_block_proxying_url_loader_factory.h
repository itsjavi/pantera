// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PANTERA_BROWSER_ADBLOCK_AD_BLOCK_PROXYING_URL_LOADER_FACTORY_H_
#define PANTERA_BROWSER_ADBLOCK_AD_BLOCK_PROXYING_URL_LOADER_FACTORY_H_

#include <stdint.h>

#include <optional>

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/global_routing_id.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "pantera/components/adblock/rs/src/lib.rs.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/self_deleting_url_loader_factory.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"

class HostContentSettingsMap;

namespace content {
class BrowserContext;
class RenderFrameHost;
}  // namespace content

namespace net {
class IsolationInfo;
}  // namespace net

namespace network {
class URLLoaderFactoryBuilder;
}  // namespace network

namespace pantera {

// Checks the requests of web pages and workers against the ad blocker before
// they reach the network (TASK-8). Blocked requests fail with
// ERR_BLOCKED_BY_CLIENT, and their element collapses, or get the bundled
// resource a `$redirect` rule names. Top-level page loads aren't checked, as
// in Brave.
//
// Shields settings come from the request's top-level site (DECISION-17).
class AdBlockProxyingURLLoaderFactory
    : public network::SelfDeletingURLLoaderFactory {
 public:
  // Installs the proxy into `factory_builder`, for the factory types that
  // carry web content.
  static void MaybeProxyRequest(
      content::BrowserContext* browser_context,
      content::RenderFrameHost* frame,
      content::ContentBrowserClient::URLLoaderFactoryType type,
      const url::Origin& request_initiator,
      const net::IsolationInfo& isolation_info,
      network::URLLoaderFactoryBuilder& factory_builder);

  AdBlockProxyingURLLoaderFactory(
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
      mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory,
      scoped_refptr<HostContentSettingsMap> settings_map,
      content::GlobalRenderFrameHostId frame_id,
      const url::Origin& request_initiator,
      std::optional<url::Origin> top_frame_origin,
      base::SelfDeletingPassKey pass_key);
  AdBlockProxyingURLLoaderFactory(const AdBlockProxyingURLLoaderFactory&) =
      delete;
  AdBlockProxyingURLLoaderFactory& operator=(
      const AdBlockProxyingURLLoaderFactory&) = delete;

  // network::mojom::URLLoaderFactory:
  void CreateLoaderAndStart(
      mojo::PendingReceiver<network::mojom::URLLoader> loader_receiver,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
      override;

 private:
  // A request waiting for the ad blocker's verdict.
  struct PendingRequest {
    PendingRequest();
    PendingRequest(PendingRequest&&);
    PendingRequest& operator=(PendingRequest&&);
    ~PendingRequest();

    mojo::PendingReceiver<network::mojom::URLLoader> loader_receiver;
    int32_t request_id = 0;
    uint32_t options = 0;
    network::ResourceRequest request;
    mojo::PendingRemote<network::mojom::URLLoaderClient> client;
    net::MutableNetworkTrafficAnnotationTag traffic_annotation;
  };

  ~AdBlockProxyingURLLoaderFactory() override;

  // The page whose shields settings apply to `request`.
  GURL GetTopFrameUrl(const network::ResourceRequest& request) const;
  void OnRequestChecked(PendingRequest pending, adblock::BlockerResult result);
  void Forward(PendingRequest pending);
  void OnTargetFactoryError();

  mojo::Remote<network::mojom::URLLoaderFactory> target_factory_;
  const scoped_refptr<HostContentSettingsMap> settings_map_;
  const content::GlobalRenderFrameHostId frame_id_;
  const url::Origin request_initiator_;
  const std::optional<url::Origin> top_frame_origin_;

  base::WeakPtrFactory<AdBlockProxyingURLLoaderFactory> weak_factory_{this};
};

}  // namespace pantera

#endif  // PANTERA_BROWSER_ADBLOCK_AD_BLOCK_PROXYING_URL_LOADER_FACTORY_H_
