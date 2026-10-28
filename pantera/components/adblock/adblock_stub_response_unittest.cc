// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pantera/components/adblock/adblock_stub_response.h"

#include <string>

#include "net/http/http_response_headers.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace pantera {
namespace {

TEST(AdBlockStubResponseTest, UsesTheRedirectResource) {
  network::ResourceRequest request;
  auto head = network::mojom::URLResponseHead::New();
  std::string body;
  // "(function() {})();" in base64.
  MakeStubResponse(
      "data:application/javascript;base64,KGZ1bmN0aW9uKCkge30pKCk7", request,
      &head, &body);
  EXPECT_EQ(body, "(function() {})();");
  EXPECT_EQ(head->mime_type, "application/javascript");
  EXPECT_EQ(head->headers->response_code(), 200);
  EXPECT_EQ(head->headers->GetNormalizedHeader("Access-Control-Allow-Origin"),
            "*");
}

TEST(AdBlockStubResponseTest, AnswersImagesWithATransparentPixel) {
  network::ResourceRequest request;
  request.headers.SetHeader("Accept", "image/png,image/*;q=0.8");
  auto head = network::mojom::URLResponseHead::New();
  std::string body;
  MakeStubResponse(std::nullopt, request, &head, &body);
  EXPECT_EQ(head->mime_type, "image/png");
  ASSERT_GE(body.size(), 8u);
  EXPECT_EQ(body.substr(1, 3), "PNG");
}

TEST(AdBlockStubResponseTest, DefaultsToAnEmptyPage) {
  network::ResourceRequest request;
  auto head = network::mojom::URLResponseHead::New();
  std::string body;
  MakeStubResponse(std::nullopt, request, &head, &body);
  EXPECT_EQ(head->mime_type, "text/html");
  EXPECT_TRUE(body.empty());
}

}  // namespace
}  // namespace pantera
