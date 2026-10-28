// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PANTERA_COMPONENTS_ADBLOCK_AD_BLOCK_SERVICE_H_
#define PANTERA_COMPONENTS_ADBLOCK_AD_BLOCK_SERVICE_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/circular_deque.h"
#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/task/sequenced_task_runner.h"
#include "base/threading/sequence_bound.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "components/prefs/pref_change_registrar.h"
#include "pantera/components/adblock/filter_list_catalog.h"
#include "pantera/components/adblock/rs/src/lib.rs.h"
#include "url/gurl.h"
#include "url/origin.h"

class PrefRegistrySimple;
class PrefService;

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace pantera {

class AdBlockEngines;

// Pantera's ad blocker. It keeps Brave's two adblock-rust engines loaded with
// the lists of Brave's catalog that are on (DECISION-15), and downloads those
// lists straight from their maintainers, never through Brave or Pantera
// (DECISION-6). One per browser process.
//
// Files in `storage_dir`: lists/ holds the downloaded lists, and
// engine-*.dat the engines built from them, so startup doesn't parse every
// list again.
class AdBlockService {
 public:
  // A downloaded list file to load into one of the engines.
  struct ListFile {
    base::FilePath path;
    uint8_t permission_mask = 0;
    bool default_engine = false;
    bool is_brave_list = false;
  };

  // How often list files are checked and failed downloads retried, as in
  // Brave.
  static constexpr base::TimeDelta kUpdateCheckInterval = base::Hours(1);
  // Refresh lists without an `! Expires` header weekly, and the others at
  // least every 14 days, as Brave does with list subscriptions.
  static constexpr base::TimeDelta kDefaultExpires = base::Days(7);
  static constexpr base::TimeDelta kMaxExpires = base::Days(14);
  // The first update check after startup. Lists never downloaded go first,
  // sooner.
  static constexpr base::TimeDelta kStartupCheckDelay = base::Minutes(1);
  static constexpr base::TimeDelta kFirstDownloadDelay = base::Seconds(10);

  // `resources_dir` holds the bundled list_catalog.json and resources.json.
  // `locale` is the browser's, to turn on the regional lists for it.
  AdBlockService(
      PrefService* local_state,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const base::FilePath& storage_dir,
      const base::FilePath& resources_dir,
      const std::string& locale);
  AdBlockService(const AdBlockService&) = delete;
  AdBlockService& operator=(const AdBlockService&) = delete;
  ~AdBlockService();

  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

  // Loads the catalog and the engines, then keeps the lists up to date.
  void Start();

  // Checks a request against the engines; `callback` runs on this sequence.
  // `request_type` is one of adblock-rust's, such as "script".
  void ShouldStartRequest(
      const GURL& url,
      const std::string& request_type,
      const url::Origin& request_initiator,
      const std::string& method,
      bool aggressive_blocking,
      base::OnceCallback<void(adblock::BlockerResult)> callback);

  // The catalog's lists that are on.
  std::vector<const FilterListCatalogEntry*> GetEnabledLists() const;

  // `callback` runs after each engine load with the number of list files in
  // the engines.
  void SetEnginesLoadedCallbackForTesting(
      base::RepeatingCallback<void(size_t)> callback);

 private:
  struct Download {
    GURL url;
    bool is_brave_list = false;
  };

  bool IsListEnabled(const FilterListCatalogEntry& entry) const;
  std::vector<ListFile> GetListFiles() const;
  base::FilePath GetListPath(const GURL& url) const;

  void OnBundledDataRead(std::pair<std::string, std::string> data);
  void LoadEngines();
  void OnEnginesLoaded(size_t list_count);
  void OnListSettingsChanged();
  void OnUpdatesSettingChanged();

  // Update checks: due lists are downloaded one at a time, and the engines
  // are rebuilt once the batch is done.
  void ScheduleUpdateCheck(base::TimeDelta delay);
  void CheckForUpdates();
  bool IsDownloadDue(const GURL& url, base::Time now) const;
  void StartNextDownload();
  void OnDownloaded(Download download, base::FilePath temp_path);
  void OnListStored(Download download, std::optional<int> expires_hours);
  void OnDownloadsFinished();

  SEQUENCE_CHECKER(sequence_checker_);

  const raw_ptr<PrefService> local_state_;
  const scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  const base::FilePath storage_dir_;
  const base::FilePath resources_dir_;
  const std::string locale_;
  const scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  base::SequenceBound<AdBlockEngines> engines_;
  std::vector<FilterListCatalogEntry> catalog_;
  bool started_ = false;

  base::OneShotTimer update_timer_;
  base::circular_deque<Download> download_queue_;
  std::unique_ptr<network::SimpleURLLoader> loader_;
  bool lists_changed_ = false;

  PrefChangeRegistrar pref_change_registrar_;
  base::RepeatingCallback<void(size_t)> engines_loaded_callback_for_testing_;

  base::WeakPtrFactory<AdBlockService> weak_factory_{this};
};

}  // namespace pantera

#endif  // PANTERA_COMPONENTS_ADBLOCK_AD_BLOCK_SERVICE_H_
