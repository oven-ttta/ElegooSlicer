#include "UserDataStorage.hpp"
#include <boost/log/trivial.hpp>
#include <boost/format.hpp>
#include <boost/nowide/fstream.hpp>

#if defined(_WIN32)
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "crypt32.lib")
#endif

#if defined(__APPLE__)
#include <Security/Security.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

#if !defined(_WIN32) && !defined(__APPLE__)
#include <cstring>
#endif

namespace Slic3r {

UserDataStorage::UserDataStorage(const std::string& filePath)
    : mFilePath(filePath) {}

#if defined(_WIN32)

bool UserDataStorage::encryptWindows(const std::string& plain, std::vector<uint8_t>& encrypted) {
    DATA_BLOB inBlob, outBlob;
    inBlob.pbData = (BYTE*)plain.data();
    inBlob.cbData = static_cast<DWORD>(plain.length());

    if (!CryptProtectData(&inBlob, nullptr, nullptr, nullptr, nullptr, 0, &outBlob)) {
        BOOST_LOG_TRIVIAL(error) << "failed to encrypt data with DPAPI, error: " << GetLastError();
        return false;
    }

    encrypted.assign(outBlob.pbData, outBlob.pbData + outBlob.cbData);
    LocalFree(outBlob.pbData);
    return true;
}

bool UserDataStorage::decryptWindows(const std::vector<uint8_t>& encrypted, std::string& plain) {
    DATA_BLOB inBlob, outBlob;
    inBlob.pbData = (BYTE*)encrypted.data();
    inBlob.cbData = static_cast<DWORD>(encrypted.size());

    if (!CryptUnprotectData(&inBlob, nullptr, nullptr, nullptr, nullptr, 0, &outBlob)) {
        BOOST_LOG_TRIVIAL(error) << "failed to decrypt data with DPAPI, error: " << GetLastError();
        return false;
    }

    plain.assign((char*)outBlob.pbData, outBlob.cbData);
    LocalFree(outBlob.pbData);
    return true;
}

#endif

#if defined(__APPLE__)

bool UserDataStorage::encryptMac(const std::string& plain, std::vector<uint8_t>& encrypted) {
    const char* service = "ElegooSlicerUserData";
    const char* account = "userdata";
    
    SecKeychainItemRef itemRef = nullptr;
    OSStatus status = SecKeychainFindGenericPassword(
        nullptr,
        static_cast<UInt32>(strlen(service)), service,
        static_cast<UInt32>(strlen(account)), account,
        nullptr, nullptr,
        &itemRef
    );

    if (status == errSecSuccess && itemRef) {
        status = SecKeychainItemModifyContent(itemRef, nullptr, 
            static_cast<UInt32>(plain.length()), plain.data());
        CFRelease(itemRef);
    } else {
        status = SecKeychainAddGenericPassword(
            nullptr,
            static_cast<UInt32>(strlen(service)), service,
            static_cast<UInt32>(strlen(account)), account,
            static_cast<UInt32>(plain.length()),
            plain.data(),
            nullptr
        );
    }

    if (status != errSecSuccess) {
        BOOST_LOG_TRIVIAL(error) << "failed to save data to Keychain, error: " << status;
        return false;
    }
    return true;
}

bool UserDataStorage::decryptMac(const std::vector<uint8_t>&, std::string& plain) {
    const char* service = "ElegooSlicerUserData";
    const char* account = "userdata";
    
    void* data = nullptr;
    UInt32 len = 0;
    OSStatus status = SecKeychainFindGenericPassword(
        nullptr,
        static_cast<UInt32>(strlen(service)), service,
        static_cast<UInt32>(strlen(account)), account,
        &len,
        &data,
        nullptr
    );

    if (status != errSecSuccess) {
        if (status != errSecItemNotFound) {
            BOOST_LOG_TRIVIAL(error) << "failed to load data from Keychain, error: " << status;
        }
        return false;
    }

    plain.assign((char*)data, len);
    SecKeychainItemFreeContent(nullptr, data);
    return true;
}

#endif

#if !defined(_WIN32) && !defined(__APPLE__)

static const unsigned int AES_KEY_LEN = 32;
static const unsigned int AES_IV_LEN = 16;
static const unsigned int AES_BLOCK_SIZE = 16;

bool UserDataStorage::encryptAES(const std::string& plain, std::vector<uint8_t>& encrypted) {
    uint8_t key[AES_KEY_LEN], iv[AES_IV_LEN];
    
    if (RAND_bytes(key, AES_KEY_LEN) != 1 || RAND_bytes(iv, AES_IV_LEN) != 1) {
        BOOST_LOG_TRIVIAL(error) << "failed to generate random bytes for AES encryption";
        return false;
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        BOOST_LOG_TRIVIAL(error) << "failed to create EVP cipher context";
        return false;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv) != 1) {
        BOOST_LOG_TRIVIAL(error) << "failed to initialize AES encryption";
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    encrypted.resize(plain.length() + AES_BLOCK_SIZE + AES_IV_LEN + AES_KEY_LEN);
    std::memcpy(encrypted.data(), iv, AES_IV_LEN);
    std::memcpy(encrypted.data() + AES_IV_LEN, key, AES_KEY_LEN);

    int len = 0;
    int ciphertext_len = 0;
    if (EVP_EncryptUpdate(ctx,
            encrypted.data() + AES_IV_LEN + AES_KEY_LEN,
            &len,
            (const unsigned char*)plain.data(),
            static_cast<int>(plain.length())) != 1) {
        BOOST_LOG_TRIVIAL(error) << "failed to encrypt data with AES";
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    ciphertext_len = len;

    if (EVP_EncryptFinal_ex(ctx,
            encrypted.data() + AES_IV_LEN + AES_KEY_LEN + len,
            &len) != 1) {
        BOOST_LOG_TRIVIAL(error) << "failed to finalize AES encryption";
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    ciphertext_len += len;

    encrypted.resize(AES_IV_LEN + AES_KEY_LEN + ciphertext_len);
    EVP_CIPHER_CTX_free(ctx);
    return true;
}

bool UserDataStorage::decryptAES(const std::vector<uint8_t>& encrypted, std::string& plain) {
    if (encrypted.size() < AES_IV_LEN + AES_KEY_LEN) {
        BOOST_LOG_TRIVIAL(error) << "encrypted data too small for AES decryption";
        return false;
    }

    const uint8_t* iv = encrypted.data();
    const uint8_t* key = encrypted.data() + AES_IV_LEN;
    const uint8_t* ciphertext = encrypted.data() + AES_IV_LEN + AES_KEY_LEN;
    size_t ciphertext_len = encrypted.size() - AES_IV_LEN - AES_KEY_LEN;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        BOOST_LOG_TRIVIAL(error) << "failed to create EVP cipher context for decryption";
        return false;
    }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv) != 1) {
        BOOST_LOG_TRIVIAL(error) << "failed to initialize AES decryption";
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    std::vector<uint8_t> plainBuf(ciphertext_len + AES_BLOCK_SIZE);
    int len = 0, plain_len = 0;
    
    if (EVP_DecryptUpdate(ctx, plainBuf.data(), &len, ciphertext, 
            static_cast<int>(ciphertext_len)) != 1) {
        BOOST_LOG_TRIVIAL(error) << "failed to decrypt data with AES";
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    plain_len = len;

    if (EVP_DecryptFinal_ex(ctx, plainBuf.data() + len, &len) != 1) {
        BOOST_LOG_TRIVIAL(error) << "failed to finalize AES decryption";
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    plain_len += len;

    plain.assign((char*)plainBuf.data(), plain_len);
    EVP_CIPHER_CTX_free(ctx);
    return true;
}

#endif

bool UserDataStorage::save(const std::string& jsonString) {
    std::vector<uint8_t> encrypted;

#if defined(_WIN32)
    if (!encryptWindows(jsonString, encrypted)) {
        BOOST_LOG_TRIVIAL(error) << "failed to encrypt user data on Windows";
        return false;
    }
#elif defined(__APPLE__)
    if (!encryptMac(jsonString, encrypted)) {
        BOOST_LOG_TRIVIAL(error) << "failed to encrypt user data on macOS";
        return false;
    }
    return true;
#else
    if (!encryptAES(jsonString, encrypted)) {
        BOOST_LOG_TRIVIAL(error) << "failed to encrypt user data on Linux";
        return false;
    }
#endif

#if !defined(__APPLE__)
    boost::nowide::ofstream f(mFilePath, std::ios::binary);
    if (!f.is_open()) {
        BOOST_LOG_TRIVIAL(error) << "failed to open file for writing: " << mFilePath;
        return false;
    }
    f.write((char*)encrypted.data(), encrypted.size());
    if (!f.good()) {
        BOOST_LOG_TRIVIAL(error) << "failed to write encrypted data to file: " << mFilePath;
        return false;
    }
    f.close();
#endif

    return true;
}

std::string UserDataStorage::load() {
#if defined(__APPLE__)
    std::string plain;
    if (!decryptMac({}, plain)) {
        return {};
    }
    return plain;
#endif

    boost::nowide::ifstream f(mFilePath, std::ios::binary);
    if (!f.is_open()) {
        return {};
    }

    f.seekg(0, std::ios::end);
    size_t fileSize = f.tellg();
    f.seekg(0, std::ios::beg);
    
    if (fileSize == 0) {
        f.close();
        return {};
    }

    std::vector<uint8_t> encrypted(fileSize);
    f.read(reinterpret_cast<char*>(encrypted.data()), fileSize);
    f.close();

    std::string plain;
#if defined(_WIN32)
    if (!decryptWindows(encrypted, plain)) {
        BOOST_LOG_TRIVIAL(error) << "failed to decrypt user data on Windows";
        return {};
    }
#else
    if (!decryptAES(encrypted, plain)) {
        BOOST_LOG_TRIVIAL(error) << "failed to decrypt user data on Linux";
        return {};
    }
#endif

    return plain;
}

} // namespace Slic3r

