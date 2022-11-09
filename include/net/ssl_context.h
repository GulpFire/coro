#pragma once

#include <filesystem>
#include <mutex>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

namespace coro::net
{
class TCPClient;

enum class SSLFileType : int
{
    ASN1 = SSL_FILETYPE_ASN1,
    PEM = SSL_FILETYPE_PEM
};

class ssl_context
{
    public:
        ssl_context();
        ssl_context(std::filesystem::path certificate,
                SSLFileType certificate_type,
                std::filesystem::path private_key,
                ssl_file_type private_key_type);

        ~ssl_context();

    private:
        static uint64_t ssl_context_count_;
        static std::mutex ssl_context_mtx_;

        SSL_CTX* ssl_ctx_{nullptr};

        SSL_CTX* native_handle()
        {
            return ssl_ctx_;
        }

        const SSL_CTX* native_handle() const
        {
            return ssl_ctx_;
        }
};
};
