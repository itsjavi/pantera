// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pantera/components/adblock/ad_block_service.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/containers/to_vector.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/json/values_util.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "crypto/hash.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "pantera/components/adblock/ad_block_engine_wrapper.h"
#include "pantera/components/adblock/filter_list_preprocessor.h"
#include "pantera/components/adblock/pref_names.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/fetch_api.mojom-shared.h"

namespace pantera {

namespace {

constexpr base::FilePath::CharType kCatalogFileName[] =
    FILE_PATH_LITERAL("list_catalog.json");
constexpr base::FilePath::CharType kResourcesFileName[] =
    FILE_PATH_LITERAL("resources.json");
constexpr base::FilePath::CharType kListsDirName[] = FILE_PATH_LITERAL("lists");
constexpr base::FilePath::CharType kDefaultEngineFileName[] =
    FILE_PATH_LITERAL("engine-default.dat");
constexpr base::FilePath::CharType kAdditionalEngineFileName[] =
    FILE_PATH_LITERAL("engine-additional.dat");
// Describes the list files the engine files were built from.
constexpr base::FilePath::CharType kEngineKeyFileName[] =
    FILE_PATH_LITERAL("engine.key");

// Larger than any list in the catalog by far.
constexpr int64_t kMaxListSize = 32 * 1024 * 1024;
constexpr base::TimeDelta kDownloadTimeout = base::Minutes(2);

constexpr char kLastAttemptKey[] = "last_attempt";
constexpr char kLastSuccessKey[] = "last_success";
constexpr char kExpiresHoursKey[] = "expires_hours";

constexpr net::NetworkTrafficAnnotationTag kFilterListTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("pantera_adblock_filter_lists", R"(
        semantics {
          sender: "Pantera ad blocker"
          description:
            "Downloads the filter lists Pantera's ad blocker uses, such as "
            "EasyList and uBlock Origin's lists, straight from their "
            "maintainers."
          trigger:
            "Shortly after startup for lists never downloaded, then whenever a "
            "list is due: when its `Expires` header says, weekly without one, "
            "and at least every 14 days."
          data: "None. The request has no cookies or credentials."
          destination: OTHER
          destination_other: "The servers of each list's maintainer."
        }
        policy {
          cookies_allowed: NO
          setting:
            "The pantera.adblock.list_updates_enabled preference turns list "
            "downloads off."
          policy_exception_justification: "Not implemented."
        })");

std::string ReadCatalog(const base::FilePath& resources_dir) {
  std::string catalog;
  if (!base::ReadFileToString(resources_dir.Append(kCatalogFileName),
                              &catalog)) {
    LOG(ERROR) << "Ad block: no filter list catalog in "
               << resources_dir.value();
  }
  return catalog;
}

// Checks a downloaded list and moves it into place. Returns its `! Expires`
// in hours (0 when it has none), or nullopt if it isn't a usable list.
std::optional<int> StoreList(const base::FilePath& temp_path,
                             const base::FilePath& list_path,
                             bool is_brave_list) {
  std::string contents;
  const bool read = base::ReadFileToString(temp_path, &contents);
  const std::string_view start = base::TrimWhitespaceASCII(
      std::string_view(contents).substr(0, 512), base::TRIM_LEADING);
  // An empty body or an HTML error page isn't a list, and neither is a list
  // whose `!#if` blocks don't balance.
  if (!read || start.empty() || start.starts_with('<') ||
      !PreprocessFilterList(contents, is_brave_list)) {
    base::DeleteFile(temp_path);
    return std::nullopt;
  }

  const adblock::FilterListMetadata metadata = adblock::read_list_metadata(
      std::vector<uint8_t>(contents.begin(), contents.end()));

  if (!base::CreateDirectory(list_path.DirName()) ||
      !base::ReplaceFile(temp_path, list_path, nullptr)) {
    base::DeleteFile(temp_path);
    return std::nullopt;
  }
  return metadata.expires_hours.has_value ? metadata.expires_hours.value : 0;
}

}  // namespace

// Owns the engines on their own sequence, and builds them from list files or
// from the serialized engines saved after the last build.
class AdBlockEngines {
 public:
  AdBlockEngines(const base::FilePath& storage_dir,
                 const base::FilePath& resources_dir)
      : storage_dir_(storage_dir),
        resources_dir_(resources_dir),
        wrapper_(AdBlockEngineWrapper::Create()),
        resources_(adblock::new_empty_resource_storage()) {
    // adblock-rust asks Chromium's public suffix list for registrable
    // domains. This can be set once per process.
    [[maybe_unused]] static const bool kResolverSet =
        adblock::set_domain_resolver();
  }

  // Loads the bundled resources and the engines saved last time, without
  // waiting for the catalog. The service calls this first, so requests made
  // at startup, which queue behind it on this sequence, are already checked.
  void LoadSaved() {
    std::string resources_json;
    if (base::ReadFileToString(resources_dir_.Append(kResourcesFileName),
                               &resources_json)) {
      resources_ = adblock::new_resource_storage(resources_json);
      wrapper_->UseResources(*resources_);
    } else {
      LOG(ERROR) << "Ad block: no resources.json in "
                 << resources_dir_.value();
    }

    std::string saved_key;
    if (base::ReadFileToString(storage_dir_.Append(kEngineKeyFileName),
                               &saved_key) &&
        LoadSavedEngines()) {
      loaded_key_ = std::move(saved_key);
      VLOG(1) << "Ad block: loaded saved engines";
    }
  }

  // Makes the engines match `files`: nothing to do when they already do,
  // the saved engines when they were built from the same files, a rebuild
  // otherwise. Returns the number of list files in the engines.
  size_t Load(const std::vector<AdBlockService::ListFile>& files) {
    std::vector<const AdBlockService::ListFile*> present;
    std::string key = "pantera-adblock-engine 1\n";
    for (const AdBlockService::ListFile& file : files) {
      base::File::Info info;
      if (!base::GetFileInfo(file.path, &info)) {
        continue;
      }
      present.push_back(&file);
      base::StrAppend(&key,
                      {file.path.BaseName().AsUTF8Unsafe(), " ",
                       base::NumberToString(info.size), " ",
                       base::NumberToString(
                           info.last_modified.InMillisecondsSinceUnixEpoch()),
                       " ", base::NumberToString(file.permission_mask), " ",
                       file.default_engine ? "default" : "additional", "\n"});
    }

    if (key == loaded_key_) {
      VLOG(1) << "Ad block: engines are up to date with " << present.size()
              << " lists";
      return present.size();
    }

    const base::FilePath key_path = storage_dir_.Append(kEngineKeyFileName);
    const base::FilePath default_path =
        storage_dir_.Append(kDefaultEngineFileName);
    const base::FilePath additional_path =
        storage_dir_.Append(kAdditionalEngineFileName);

    std::string saved_key;
    if (base::ReadFileToString(key_path, &saved_key) && saved_key == key &&
        LoadSavedEngines()) {
      loaded_key_ = std::move(key);
      VLOG(1) << "Ad block: loaded saved engines with " << present.size()
              << " lists";
      return present.size();
    }

    rust::Box<adblock::FilterSet> default_set =
        adblock::new_filter_set(/*debug=*/false);
    rust::Box<adblock::FilterSet> additional_set =
        adblock::new_filter_set(/*debug=*/false);
    size_t loaded = 0;
    for (const AdBlockService::ListFile* file : present) {
      std::string contents;
      if (!base::ReadFileToString(file->path, &contents)) {
        continue;
      }
      std::optional<std::string> list =
          PreprocessFilterList(contents, file->is_brave_list);
      if (!list) {
        continue;
      }
      rust::Box<adblock::FilterSet>& set =
          file->default_engine ? default_set : additional_set;
      adblock::AddFilterListResult result =
          set->add_filter_list_with_permissions(
              std::vector<uint8_t>(list->begin(), list->end()),
              file->permission_mask);
      if (result.result_kind == adblock::ResultKind::Success) {
        ++loaded;
      }
    }

    wrapper_->Load(true, std::move(default_set), *resources_);
    wrapper_->Load(false, std::move(additional_set), *resources_);
    loaded_key_ = key;
    VLOG(1) << "Ad block: built engines from " << loaded << " lists";

    if (loaded == 0) {
      base::DeleteFile(key_path);
      base::DeleteFile(default_path);
      base::DeleteFile(additional_path);
      return 0;
    }
    // The key goes last, so an interrupted write leaves no key that matches.
    if (!base::CreateDirectory(storage_dir_) ||
        !base::WriteFile(default_path, wrapper_->Serialize(true)) ||
        !base::WriteFile(additional_path, wrapper_->Serialize(false)) ||
        !base::WriteFile(key_path, key)) {
      base::DeleteFile(key_path);
    }
    return loaded;
  }

  adblock::BlockerResult ShouldStartRequest(const GURL& url,
                                            const std::string& request_type,
                                            const url::Origin& initiator,
                                            const std::string& method,
                                            bool aggressive_blocking) {
    return wrapper_->ShouldStartRequest(url, request_type, initiator, method,
                                        aggressive_blocking,
                                        /*previously_matched_rule=*/false,
                                        /*previously_matched_exception=*/false,
                                        /*previously_matched_important=*/false);
  }

 private:
  bool LoadSavedEngines() {
    std::optional<std::vector<uint8_t>> default_engine =
        base::ReadFileToBytes(storage_dir_.Append(kDefaultEngineFileName));
    std::optional<std::vector<uint8_t>> additional_engine =
        base::ReadFileToBytes(storage_dir_.Append(kAdditionalEngineFileName));
    return default_engine && additional_engine &&
           wrapper_->LoadSerialized(true, *default_engine, *resources_) &&
           wrapper_->LoadSerialized(false, *additional_engine, *resources_);
  }

  const base::FilePath storage_dir_;
  const base::FilePath resources_dir_;
  const std::unique_ptr<AdBlockEngineWrapper> wrapper_;
  rust::Box<adblock::ResourceStorage> resources_;
  // The key of the list files the engines hold now.
  std::string loaded_key_;
};

AdBlockService::AdBlockService(
    PrefService* local_state,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const base::FilePath& storage_dir,
    const base::FilePath& resources_dir,
    const std::string& locale)
    : local_state_(local_state),
      url_loader_factory_(std::move(url_loader_factory)),
      storage_dir_(storage_dir),
      resources_dir_(resources_dir),
      locale_(locale),
      file_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})),
      engines_(base::ThreadPool::CreateSequencedTaskRunner(
                   {base::MayBlock(), base::TaskPriority::USER_BLOCKING,
                    base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN}),
               storage_dir,
               resources_dir) {}

AdBlockService::~AdBlockService() = default;

// static
void AdBlockService::RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kAdBlockListUpdatesEnabled, true);
  registry->RegisterDictionaryPref(prefs::kAdBlockListSettings);
  registry->RegisterDictionaryPref(prefs::kAdBlockListDownloads);
}

void AdBlockService::Start() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(!started_);
  started_ = true;
  engines_.AsyncCall(&AdBlockEngines::LoadSaved);
  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&ReadCatalog, resources_dir_),
      base::BindOnce(&AdBlockService::OnCatalogRead,
                     weak_factory_.GetWeakPtr()));
}

void AdBlockService::ShouldStartRequest(
    const GURL& url,
    const std::string& request_type,
    const url::Origin& request_initiator,
    const std::string& method,
    bool aggressive_blocking,
    base::OnceCallback<void(adblock::BlockerResult)> callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  engines_.AsyncCall(&AdBlockEngines::ShouldStartRequest)
      .WithArgs(url, request_type, request_initiator, method,
                aggressive_blocking)
      .Then(std::move(callback));
}

std::vector<const FilterListCatalogEntry*> AdBlockService::GetEnabledLists()
    const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::vector<const FilterListCatalogEntry*> lists;
  for (const FilterListCatalogEntry& entry : catalog_) {
    if (IsListEnabled(entry)) {
      lists.push_back(&entry);
    }
  }
  return lists;
}

void AdBlockService::SetEnginesLoadedCallbackForTesting(
    base::RepeatingCallback<void(size_t)> callback) {
  engines_loaded_callback_for_testing_ = std::move(callback);
}

bool AdBlockService::IsListEnabled(const FilterListCatalogEntry& entry) const {
  if (!entry.SupportsMacOS()) {
    return false;
  }
  const base::DictValue& settings =
      local_state_->GetDict(prefs::kAdBlockListSettings);
  if (std::optional<bool> enabled = settings.FindBool(entry.uuid)) {
    return *enabled;
  }
  return entry.default_enabled || entry.MatchesLocale(locale_);
}

std::vector<AdBlockService::ListFile> AdBlockService::GetListFiles() const {
  std::vector<ListFile> files;
  std::set<GURL> urls;
  for (const FilterListCatalogEntry* entry : GetEnabledLists()) {
    for (const FilterListSource& source : entry->sources) {
      if (!urls.insert(source.url).second) {
        continue;
      }
      files.push_back({GetListPath(source.url), entry->permission_mask,
                       entry->first_party_protections, source.IsBraveList()});
    }
  }
  return files;
}

base::FilePath AdBlockService::GetListPath(const GURL& url) const {
  const auto hash = crypto::hash::Sha256(url.spec());
  return storage_dir_.Append(kListsDirName)
      .AppendASCII(base::StrCat(
          {base::HexEncodeLower(base::span(hash).first<16>()), ".txt"}));
}

void AdBlockService::OnCatalogRead(std::string catalog_json) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  catalog_ = ParseFilterListCatalog(catalog_json);
  VLOG(1) << "Ad block: " << catalog_.size() << " lists in the catalog, "
          << GetEnabledLists().size() << " on";
  LoadEngines();

  pref_change_registrar_.Init(local_state_);
  pref_change_registrar_.Add(
      prefs::kAdBlockListSettings,
      base::BindRepeating(&AdBlockService::OnListSettingsChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      prefs::kAdBlockListUpdatesEnabled,
      base::BindRepeating(&AdBlockService::OnUpdatesSettingChanged,
                          base::Unretained(this)));

  const base::DictValue& downloads =
      local_state_->GetDict(prefs::kAdBlockListDownloads);
  const bool missing_lists = std::ranges::any_of(
      GetEnabledLists(), [&](const FilterListCatalogEntry* entry) {
        return std::ranges::any_of(
            entry->sources, [&](const FilterListSource& source) {
              return !downloads.FindDict(source.url.spec());
            });
      });
  ScheduleUpdateCheck(missing_lists ? kFirstDownloadDelay : kStartupCheckDelay);
}

void AdBlockService::LoadEngines() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  engines_.AsyncCall(&AdBlockEngines::Load)
      .WithArgs(GetListFiles())
      .Then(base::BindOnce(&AdBlockService::OnEnginesLoaded,
                           weak_factory_.GetWeakPtr()));
}

void AdBlockService::OnEnginesLoaded(size_t list_count) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (engines_loaded_callback_for_testing_) {
    engines_loaded_callback_for_testing_.Run(list_count);
  }
}

void AdBlockService::OnListSettingsChanged() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  LoadEngines();
  // Lists just turned on may never have been downloaded.
  if (!loader_ && download_queue_.empty()) {
    ScheduleUpdateCheck(base::TimeDelta());
  }
}

void AdBlockService::OnUpdatesSettingChanged() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (local_state_->GetBoolean(prefs::kAdBlockListUpdatesEnabled)) {
    ScheduleUpdateCheck(base::TimeDelta());
    return;
  }
  // Stop right away, including a download in progress.
  update_timer_.Stop();
  download_queue_.clear();
  loader_.reset();
  if (lists_changed_) {
    OnDownloadsFinished();
  }
}

void AdBlockService::ScheduleUpdateCheck(base::TimeDelta delay) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!local_state_->GetBoolean(prefs::kAdBlockListUpdatesEnabled)) {
    update_timer_.Stop();
    return;
  }
  update_timer_.Start(
      FROM_HERE, delay,
      base::BindOnce(&AdBlockService::CheckForUpdates, base::Unretained(this)));
}

void AdBlockService::CheckForUpdates() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!local_state_->GetBoolean(prefs::kAdBlockListUpdatesEnabled) || loader_ ||
      !download_queue_.empty()) {
    return;
  }
  const base::Time now = base::Time::Now();
  std::set<GURL> queued;
  for (const FilterListCatalogEntry* entry : GetEnabledLists()) {
    for (const FilterListSource& source : entry->sources) {
      if (IsDownloadDue(source.url, now) && queued.insert(source.url).second) {
        download_queue_.push_back({source.url, source.IsBraveList()});
      }
    }
  }
  VLOG(1) << "Ad block: " << download_queue_.size() << " lists to download";
  StartNextDownload();
}

bool AdBlockService::IsDownloadDue(const GURL& url, base::Time now) const {
  const base::DictValue* state =
      local_state_->GetDict(prefs::kAdBlockListDownloads).FindDict(url.spec());
  if (!state) {
    return true;
  }
  const std::optional<base::Time> last_attempt =
      base::ValueToTime(state->Find(kLastAttemptKey));
  if (last_attempt && now - *last_attempt < kUpdateCheckInterval) {
    return false;
  }
  const std::optional<base::Time> last_success =
      base::ValueToTime(state->Find(kLastSuccessKey));
  if (!last_success) {
    return true;
  }
  const int expires_hours = state->FindInt(kExpiresHoursKey).value_or(0);
  const base::TimeDelta expires =
      expires_hours > 0 ? std::min(base::Hours(expires_hours), kMaxExpires)
                        : kDefaultExpires;
  return now - *last_success >= expires;
}

void AdBlockService::StartNextDownload() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (download_queue_.empty()) {
    OnDownloadsFinished();
    return;
  }
  Download download = std::move(download_queue_.front());
  download_queue_.pop_front();

  {
    ScopedDictPrefUpdate update(*local_state_, prefs::kAdBlockListDownloads);
    update->EnsureDict(download.url.spec())
        ->Set(kLastAttemptKey, base::TimeToValue(base::Time::Now()));
  }

  auto request = std::make_unique<network::ResourceRequest>();
  request->url = download.url;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->load_flags = net::LOAD_DISABLE_CACHE;
  loader_ = network::SimpleURLLoader::Create(std::move(request),
                                             kFilterListTrafficAnnotation);
  loader_->SetRetryOptions(1,
                           network::SimpleURLLoader::RETRY_ON_NETWORK_CHANGE);
  loader_->SetTimeoutDuration(kDownloadTimeout);
  loader_->DownloadToTempFile(
      url_loader_factory_.get(),
      base::BindOnce(&AdBlockService::OnDownloaded, base::Unretained(this),
                     std::move(download)),
      kMaxListSize);
}

void AdBlockService::OnDownloaded(Download download, base::FilePath temp_path) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  loader_.reset();
  if (temp_path.empty()) {
    VLOG(1) << "Ad block: failed to download " << download.url;
    StartNextDownload();
    return;
  }
  const base::FilePath list_path = GetListPath(download.url);
  const bool is_brave_list = download.is_brave_list;
  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&StoreList, temp_path, list_path, is_brave_list),
      base::BindOnce(&AdBlockService::OnListStored, weak_factory_.GetWeakPtr(),
                     std::move(download)));
}

void AdBlockService::OnListStored(Download download,
                                  std::optional<int> expires_hours) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!expires_hours) {
    VLOG(1) << "Ad block: " << download.url << " isn't a usable list";
  } else {
    VLOG(1) << "Ad block: downloaded " << download.url;
    ScopedDictPrefUpdate update(*local_state_, prefs::kAdBlockListDownloads);
    base::DictValue* state = update->EnsureDict(download.url.spec());
    state->Set(kLastSuccessKey, base::TimeToValue(base::Time::Now()));
    state->Set(kExpiresHoursKey, *expires_hours);
    lists_changed_ = true;
  }
  // Updates may have been turned off meanwhile.
  if (local_state_->GetBoolean(prefs::kAdBlockListUpdatesEnabled)) {
    StartNextDownload();
  } else {
    OnDownloadsFinished();
  }
}

void AdBlockService::OnDownloadsFinished() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (lists_changed_) {
    lists_changed_ = false;
    LoadEngines();
  }
  ScheduleUpdateCheck(kUpdateCheckInterval);
}

}  // namespace pantera
