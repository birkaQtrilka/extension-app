#include "TelegramClient.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <expected>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace {
    struct CurlDeleter {
        void operator()(CURL* curl) const {
            if (curl) {
                curl_easy_cleanup(curl);
            }
        }
    };
    using CurlHandle = std::unique_ptr<CURL, CurlDeleter>;
    struct CurlHeadersDeleter {
        void operator()(curl_slist* list) const {
            if (list) curl_slist_free_all(list);
        }
    };
    struct MimeDeleter {
        void operator()(curl_mime* mime) const {
            curl_mime_free(mime); // safe to call with nullptr
        }
    };
    using MimeHandle = std::unique_ptr<curl_mime, MimeDeleter>;

    size_t writeResponse(char* data, size_t size, size_t count, void* userData) {
        const auto bytes = size * count;
        static_cast<std::string*>(userData)->append(data, bytes);
        return bytes;
    }

    std::string buildUrl(std::string_view token, std::string_view method) {
        std::string url = "https://api.telegram.org/bot";
        url += token;
        url += '/';
        url += method;
        return url;
    }

    // Options shared by every request. Does not perform anything.
    void applyCommonOptions(CURL* curl, const std::string& url, std::string& responseBody) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeResponse);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

        // Explicitly require TLS certificate and hostname verification.
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    }

    // Runs the request and interprets the Telegram JSON response.
    std::expected<void, std::string> performRequest(CURL* curl, const std::string& responseBody) {
        const CURLcode code = curl_easy_perform(curl);
        if (code != CURLE_OK) {
            return std::unexpected(
                std::string("Telegram HTTP request failed: ") + curl_easy_strerror(code)
            );
        }

        long status = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

        try {
            const auto response = nlohmann::json::parse(responseBody);

            if (status < 200 || status >= 300 || !response.value("ok", false)) {
                return std::unexpected(
                    response.value("description", std::string("Telegram API request failed"))
                );
            }
        } catch (const nlohmann::json::exception& e) {
            return std::unexpected(std::string("Invalid Telegram API response: ") + e.what());
        }

        return {};
    }
}

TelegramClient::TelegramClient(std::string botToken, std::string chatId)
    : m_botToken(std::move(botToken)),
      m_chatId(std::move(chatId)) {
}

TelegramClient::~TelegramClient() = default;

std::expected<void, std::string>
TelegramClient::sendMessage(std::string_view text) const {
    CurlHandle curl(curl_easy_init());
    if (!curl) {
        return std::unexpected("Failed to initialize CURL handle");
    }

    const std::string payload = nlohmann::json{
            {"chat_id", m_chatId},
            {"text", text}
    }.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);

    const std::string url = buildUrl(m_botToken, "sendMessage");
    std::string responseBody;

    applyCommonOptions(curl.get(), url, responseBody);
    curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(
        curl.get(), CURLOPT_POSTFIELDSIZE_LARGE,
        static_cast<curl_off_t>(payload.size())
    );


    std::unique_ptr<curl_slist, CurlHeadersDeleter> headers(
        curl_slist_append(nullptr, "Content-Type: application/json")
    );
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());

    return performRequest(curl.get(), responseBody);
}

std::expected<void, std::string>
TelegramClient::sendDocument(const std::filesystem::path& file) const {
    CurlHandle curl(curl_easy_init());
    if (!curl) {
        return std::unexpected("Failed to initialize CURL handle");
    }

    const std::string url = buildUrl(m_botToken, "sendDocument");
    std::string responseBody;
    applyCommonOptions(curl.get(), url, responseBody);

    // The form must stay alive until curl_easy_perform has finished.
    MimeHandle form(curl_mime_init(curl.get()));
    if (!form) {
        return std::unexpected("Failed to create multipart form");
    }

    curl_mimepart* part = curl_mime_addpart(form.get());
    curl_mime_name(part, "chat_id");
    curl_mime_data(part, m_chatId.c_str(), CURL_ZERO_TERMINATED);

    part = curl_mime_addpart(form.get());
    curl_mime_name(part, "document");
    if (curl_mime_filedata(part, file.string().c_str()) != CURLE_OK) {
        return std::unexpected("Could not read file for upload: " + file.string());
    }

    curl_easy_setopt(curl.get(), CURLOPT_MIMEPOST, form.get());

    return performRequest(curl.get(), responseBody);
}