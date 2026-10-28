// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pantera/components/adblock/ad_block_service.h"

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/prefs/testing_pref_service.h"
#include "pantera/components/adblock/pref_names.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace pantera {
namespace {

constexpr char kCatalog[] = R"([
  {
    "uuid": "default",
    "title": "Brave Default Adblock Filters",
    "default_enabled": true,
    "first_party_protections": true,
    "permission_mask": 1,
    "sources": [{"url": "https://lists.example/default.txt",
                 "title": "uBlock Origin Filters", "format": "Standard"}]
  },
  {
    "uuid": "extra",
    "title": "Cookie notice blocker",
    "default_enabled": true,
    "sources": [{"url": "https://lists.example/extra.txt",
                 "format": "Standard"}]
  },
  {
    "uuid": "FR",
    "title": "French website ad blocker",
    "langs": ["fr"],
    "sources": [{"url": "https://lists.example/fr.txt", "format": "Standard"}]
  },
  {
    "uuid": "off",
    "title": "Off by default",
    "sources": [{"url": "https://lists.example/off.txt", "format": "Standard"}]
  }
])";

constexpr char kDefaultListUrl[] = "https://lists.example/default.txt";
constexpr char kExtraListUrl[] = "https://lists.example/extra.txt";
constexpr char kDefaultList[] =
    "! Title: Default\n! Expires: 2 days\n"
    "||tracker.example^\n||news.example/ads/*\n";
constexpr char kExtraList[] = "! Title: Extra\n||news.example/promo/*\n";

class AdBlockServiceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    resources_dir_ = temp_dir_.GetPath().AppendASCII("resources");
    storage_dir_ = temp_dir_.GetPath().AppendASCII("storage");
    ASSERT_TRUE(base::CreateDirectory(resources_dir_));
    ASSERT_TRUE(base::WriteFile(resources_dir_.AppendASCII("list_catalog.json"),
                                kCatalog));
    ASSERT_TRUE(
        base::WriteFile(resources_dir_.AppendASCII("resources.json"), "[]"));
    AdBlockService::RegisterLocalStatePrefs(local_state_.registry());
  }

  std::unique_ptr<AdBlockService> CreateService(
      const std::string& locale = "en-US") {
    auto service = std::make_unique<AdBlockService>(
        &local_state_, test_url_loader_factory_.GetSafeWeakWrapper(),
        storage_dir_, resources_dir_, locale);
    service->SetEnginesLoadedCallbackForTesting(base::BindRepeating(
        &AdBlockServiceTest::OnEnginesLoaded, base::Unretained(this)));
    return service;
  }

  void OnEnginesLoaded(size_t list_count) {
    loads_.push_back(list_count);
    if (quit_) {
      std::move(quit_).Run();
    }
  }

  // Waits for the next engine load and returns its number of lists.
  size_t WaitForEngines() {
    if (loads_.size() == waited_) {
      base::RunLoop run_loop;
      quit_ = run_loop.QuitClosure();
      run_loop.Run();
    }
    return loads_[waited_++];
  }

  bool Blocks(AdBlockService& service,
              const std::string& url,
              const std::string& initiator) {
    base::test::TestFuture<adblock::BlockerResult> future;
    service.ShouldStartRequest(
        GURL(url), "script", url::Origin::Create(GURL(initiator)), "GET",
        /*aggressive_blocking=*/false, future.GetCallback());
    return future.Take().matched;
  }

  size_t CountListFiles() {
    size_t count = 0;
    base::FileEnumerator files(storage_dir_.AppendASCII("lists"), false,
                               base::FileEnumerator::FILES);
    for (base::FilePath path = files.Next(); !path.empty();
         path = files.Next()) {
      ++count;
    }
    return count;
  }

  const base::DictValue* DownloadState(const std::string& url) {
    return local_state_.GetDict(prefs::kAdBlockListDownloads).FindDict(url);
  }

  // Downloads both lists with a fresh service.
  std::unique_ptr<AdBlockService> StartWithDownloadedLists() {
    test_url_loader_factory_.AddResponse(kDefaultListUrl, kDefaultList);
    test_url_loader_factory_.AddResponse(kExtraListUrl, kExtraList);
    std::unique_ptr<AdBlockService> service = CreateService();
    service->Start();
    EXPECT_EQ(WaitForEngines(), 0u);
    EXPECT_EQ(WaitForEngines(), 2u);
    return service;
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::ScopedTempDir temp_dir_;
  base::FilePath resources_dir_;
  base::FilePath storage_dir_;
  TestingPrefServiceSimple local_state_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  std::vector<size_t> loads_;
  size_t waited_ = 0;
  base::OnceClosure quit_;
};

TEST_F(AdBlockServiceTest, DownloadsListsAndBlocks) {
  std::unique_ptr<AdBlockService> service = StartWithDownloadedLists();

  // Only the two lists that are on by default were downloaded.
  EXPECT_EQ(test_url_loader_factory_.total_requests(), 2u);
  EXPECT_EQ(CountListFiles(), 2u);
  EXPECT_TRUE(base::PathExists(storage_dir_.AppendASCII("engine-default.dat")));

  ASSERT_TRUE(DownloadState(kDefaultListUrl));
  EXPECT_EQ(DownloadState(kDefaultListUrl)->FindInt("expires_hours"), 48);
  EXPECT_TRUE(DownloadState(kDefaultListUrl)->Find("last_success"));
  EXPECT_EQ(DownloadState(kExtraListUrl)->FindInt("expires_hours"), 0);

  EXPECT_TRUE(
      Blocks(*service, "https://tracker.example/t.js", "https://news.example"));
  // Standard mode: the default list spares first-party requests, the other
  // list doesn't.
  EXPECT_FALSE(Blocks(*service, "https://news.example/ads/a.js",
                      "https://news.example"));
  EXPECT_TRUE(Blocks(*service, "https://news.example/promo/p.js",
                     "https://news.example"));
  EXPECT_FALSE(
      Blocks(*service, "https://cdn.example/app.js", "https://news.example"));
}

TEST_F(AdBlockServiceTest, UsesSavedEnginesAndRefreshesWhenDue) {
  StartWithDownloadedLists().reset();
  const size_t requests = test_url_loader_factory_.total_requests();

  std::unique_ptr<AdBlockService> service = CreateService();
  service->Start();
  EXPECT_EQ(WaitForEngines(), 2u);
  EXPECT_TRUE(
      Blocks(*service, "https://tracker.example/t.js", "https://news.example"));

  // Nothing is due yet.
  task_environment_.FastForwardBy(AdBlockService::kStartupCheckDelay +
                                  base::Hours(2));
  EXPECT_EQ(test_url_loader_factory_.total_requests(), requests);

  // The default list expires after 2 days, the other one after a week.
  test_url_loader_factory_.AddResponse(
      kDefaultListUrl, std::string(kDefaultList) + "||more.example^\n");
  task_environment_.FastForwardBy(base::Days(2));
  EXPECT_EQ(WaitForEngines(), 2u);
  EXPECT_EQ(test_url_loader_factory_.total_requests(), requests + 1);
  EXPECT_TRUE(
      Blocks(*service, "https://more.example/m.js", "https://news.example"));
}

TEST_F(AdBlockServiceTest, DownloadsNothingWithUpdatesOff) {
  local_state_.SetBoolean(prefs::kAdBlockListUpdatesEnabled, false);
  std::unique_ptr<AdBlockService> service = CreateService();
  service->Start();
  EXPECT_EQ(WaitForEngines(), 0u);

  task_environment_.FastForwardBy(base::Days(30));
  EXPECT_EQ(test_url_loader_factory_.total_requests(), 0u);
  EXPECT_EQ(test_url_loader_factory_.NumPending(), 0);
}

TEST_F(AdBlockServiceTest, KeepsTheOldListWhenADownloadIsBroken) {
  test_url_loader_factory_.AddResponse(kDefaultListUrl,
                                       "<!DOCTYPE html><p>Not found</p>");
  test_url_loader_factory_.AddResponse(kExtraListUrl, kExtraList);
  std::unique_ptr<AdBlockService> service = CreateService();
  service->Start();
  EXPECT_EQ(WaitForEngines(), 0u);
  EXPECT_EQ(WaitForEngines(), 1u);

  EXPECT_EQ(CountListFiles(), 1u);
  ASSERT_TRUE(DownloadState(kDefaultListUrl));
  EXPECT_FALSE(DownloadState(kDefaultListUrl)->Find("last_success"));
  EXPECT_TRUE(DownloadState(kDefaultListUrl)->Find("last_attempt"));
}

TEST_F(AdBlockServiceTest, ChoosesListsByLocaleAndSettings) {
  test_url_loader_factory_.AddResponse(kDefaultListUrl, kDefaultList);
  test_url_loader_factory_.AddResponse(kExtraListUrl, kExtraList);
  test_url_loader_factory_.AddResponse("https://lists.example/fr.txt",
                                       "||pub.example^\n");
  std::unique_ptr<AdBlockService> service = CreateService("fr-FR");
  service->Start();
  EXPECT_EQ(WaitForEngines(), 0u);
  EXPECT_EQ(WaitForEngines(), 3u);

  auto uuids = [&] {
    std::vector<std::string> result;
    for (const FilterListCatalogEntry* entry : service->GetEnabledLists()) {
      result.push_back(entry->uuid);
    }
    return result;
  };
  EXPECT_EQ(uuids(), (std::vector<std::string>{"default", "extra", "FR"}));
  EXPECT_TRUE(
      Blocks(*service, "https://pub.example/p.js", "https://news.example"));

  {
    ScopedDictPrefUpdate update(local_state_, prefs::kAdBlockListSettings);
    update->Set("FR", false);
    update->Set("off", true);
  }
  EXPECT_EQ(uuids(), (std::vector<std::string>{"default", "extra", "off"}));
  // The engines reload without the French list, before "off" downloads.
  EXPECT_EQ(WaitForEngines(), 2u);
  EXPECT_FALSE(
      Blocks(*service, "https://pub.example/p.js", "https://news.example"));
}

}  // namespace
}  // namespace pantera
