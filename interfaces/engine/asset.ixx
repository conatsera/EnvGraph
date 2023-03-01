module;
/*
#include <array>
#include <compare>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <ostream>
#include <random>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <concepts>
*/
#include <openssl/evp.h>
#include <openssl/x509.h>

#include <vips/vips8>

#if defined(_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <DirectXMath.h>

#endif
namespace fb
{
#include <flatbuffers/util.h>
#include "asset_bundle_generated.h"
} // namespace fb

export module asset;

import std.compat;

namespace EnvGraph
{
namespace Asset
{

    using namespace fb::EnvGraph::Asset::Buffer;

/*export AssetBundle GetRootAssetBundle() {

}*/

export template <uint64_t blockSize = 4096> bool VerifyAssetBundle(std::filesystem::path bundlePath)
{
    std::basic_ifstream<unsigned char> bundleFileInputStream(bundlePath);
    const std::filesystem::path bundleFileSigName = std::filesystem::path(bundlePath).append(".sig");

    std::ifstream bundleFileSigInputStream(bundleFileSigName);
    const auto bundleFileSize = std::filesystem::file_size(bundlePath);

    std::array<unsigned char, blockSize> bundleFileReadBuffer{};
    EVP_MD_CTX *digestCtx = nullptr;
    auto initResult = EVP_DigestInit_ex(digestCtx, EVP_sha256(), nullptr);
    if (initResult != 0)
    {
        std::cerr << "EVP_DigestInit_ex result code: " << initResult << std::endl;
        throw(-1);
    }
    for (auto i = 0; i < bundleFileSize / blockSize; i++)
    {
        bundleFileInputStream.read(bundleFileReadBuffer.data(), blockSize);
        EVP_DigestUpdate(digestCtx, bundleFileReadBuffer.data(), blockSize);
    }
    bundleFileInputStream.read(bundleFileReadBuffer.data(), blockSize);
    const std::streamsize lastSectorSize = bundleFileInputStream.gcount();
    EVP_DigestUpdate(digestCtx, bundleFileReadBuffer.data(), static_cast<std::size_t>(lastSectorSize));

    unsigned int bundleFileDigestSize = 0;
    std::array<unsigned char, EVP_MAX_MD_SIZE> bundleFileDigest{};
    EVP_DigestFinal_ex(digestCtx, bundleFileDigest.data(), &bundleFileDigestSize);

    auto validatedBundleFileDigest = std::array<unsigned char, EVP_MAX_MD_SIZE>();

    unsigned char *bundleFileSha512Digest = nullptr;

    return false;
}

struct Loader
{
};

export enum AssetError
{
    None = 0,
    NotFound,
    InvalidArgs,
    ImageSize,
    BuilderCreateError,
    BundleAlreadyBuilt
};

struct Bundle
{
    fb::flatbuffers::FlatBufferBuilder builder;
    std::filesystem::path bundleFilePath = std::filesystem::path("");

    void GetTextureNames(std::vector<std::string> &names) const
    {
        const auto textures = Buffer::GetBundle(builder.GetBufferPointer())->textures();
        names.resize(textures->size());
        std::transform(textures->begin(), textures->end(), names.begin(),
                       [](const auto &texture) -> std::string {
                return texture->name()->str();
            });
    }
};

constexpr const uint16_t kInvalidID = 0;
template <class C> struct ID
{
    ID<C>(uint16_t _value = kInvalidID) : value(_value)
    {
    }
    uint16_t value;

    operator uint16_t() const
    {
        return value;
    }
};

template<class C>
class std::hash<ID<C>> : public std::hash<uint16_t>
{
  public:
      using Base = std::hash<uint16_t>;

      using argument_type = uint16_t;
      using result_type = std::size_t;

      using Base::Base;
      using Base::operator();
};

export using LoaderID = ID<Loader>;

export using BundleID = ID<Bundle>;

constexpr const uint32_t kInvalidAssetID = 0;
template <class C> struct AssetID
{
      AssetID<C>(uint32_t _value = kInvalidAssetID) : value(_value)
      {
      }
      AssetID<C>(BundleID bundleId, uint16_t _value)
          : value(bundleId + (static_cast<uint32_t>(_value) >> 16))
      {
      }
      uint32_t value;

      BundleID GetBundleID() const
      {
        return value;
      }

      uint16_t GetTextureIndex() const
      {
        return value << 16;
      }

      operator uint32_t() const
      {
        return value;
      }
};

export using TextureID = AssetID<Buffer::Texture>;
export using MaterialID = AssetID<Buffer::Material>;
export using MeshID = AssetID<Buffer::Mesh>;

export class Manager
{
  public:
    Manager()
    {
    }
    ~Manager()
    {
    }

    void Init()
    {
        VIPS_INIT("");
        m_initialized = true;
    }

    constexpr bool IsValid() const
    {
        return m_initialized;
    }

    std::pair<AssetError, BundleID> NewBundle(std::string bundleName)
    {
        const BundleID newBundleID = GenerateID<BundleID>();


        const auto res = m_bundles.insert({newBundleID, std::make_unique<Bundle>()});

        if (!res.second)
            return {AssetError::BuilderCreateError, kInvalidID};

        //auto newbundle = CreateBundle(res.first->second)
        
        return {AssetError::None, res.first->first};
    }

    std::pair<AssetError, BundleID> NewBundleFromFile(std::filesystem::path bundlePath)
    {
        return {AssetError::None, kInvalidID};
    }

    std::pair<AssetError, TextureID> NewTextureFromFile(BundleID bundle, std::string textureName, std::filesystem::path inputImageFilePath)
    {
        return {AssetError::None, kInvalidID};
    }

    std::pair<AssetError, TextureID> NewTextureFromFiles(BundleID bundle, std::string textureName,
                                   std::vector<std::filesystem::path> &inputImageFilePaths)
    {
        using namespace vips;
        if (inputImageFilePaths.size() == 0)
            return {AssetError::InvalidArgs, kInvalidAssetID};

        auto builderIt = m_bundles.find(bundle);
        if (builderIt == m_bundles.end())
            return {AssetError::BundleAlreadyBuilt, kInvalidAssetID};

        auto &builder = builderIt->second->builder;

        

        uint32_t width = 0;
        uint32_t height = 0;
        
        

        std::vector<fb::flatbuffers::Offset<fb::flatbuffers::Vector<uint8_t>>> bitmapDataVector;
        bitmapDataVector.reserve(inputImageFilePaths.size());
        
        for (const auto& inputImageFilePath : inputImageFilePaths)
        {
            const VImage inImage = VImage::new_from_file(inputImageFilePath.generic_string().c_str());
            const uint32_t imageWidth = inImage.width();
            const uint32_t imageHeight = inImage.height();

            if (height == 0)
            {
                if (imageWidth == 0 || imageHeight == 0)
                {
                    return {AssetError::ImageSize, kInvalidID};
                }
                width = imageWidth;
                height = imageHeight;
            }
            else if (width != imageWidth || height != imageHeight)
            {
                return {AssetError::ImageSize, kInvalidID};
            }

            void *imageBuffer = nullptr;
            size_t imageSize = 0;

            inImage.write_to_buffer(".webp", (void**) &imageBuffer, &imageSize);

            bitmapDataVector.push_back(builder.CreateVector(reinterpret_cast<uint8_t *>(imageBuffer), imageSize));

            g_free(imageBuffer);
        }

        std::vector<fb::flatbuffers::Offset<Buffer::Bitmap>> bitmapVector;
        bitmapVector.reserve(inputImageFilePaths.size());

        for (const auto &bitmapData : bitmapDataVector)
        {
            bitmapVector.push_back(Buffer::CreateBitmap(builder, bitmapData));
        }
        const auto bitmaps = builder.CreateVector(bitmapVector);

        const auto textureNameString = builder.CreateString(textureName);
        const auto textureBuffer =
            Buffer::CreateTexture(builder, textureNameString, width, height, inputImageFilePaths.size(), bitmaps);

        const auto textureId = GenerateID<TextureID>();

        return {AssetError::None, textureId};
    }

  private:
    template<typename T>
    bool CheckIDUsed(T ID) const
    {
        if constexpr (std::is_same_v<BundleID, T>)
            return m_bundles.find(ID) != m_bundles.end();
        else
            return false;
    }

    template <class T> T GenerateID() const
    {
        static std::random_device rd;
        static std::default_random_engine randEng(rd());
        static std::uniform_int_distribution<decltype(T::value)> uniformDist(1, std::numeric_limits<decltype(T::value)>::max());

        const auto timeoutTimestamp = std::chrono::steady_clock::now() + std::chrono::seconds(1);
        while (true)
        {
            const T newID = uniformDist(randEng);
            if (CheckIDUsed(newID))
            {
                if (timeoutTimestamp.time_since_epoch().count() < std::chrono::steady_clock::now().time_since_epoch().count())
                    return kInvalidID;
                else
                    continue;
            }
            else
                return newID;
        }

    }

  private:
    bool m_initialized = false;
    //std::unordered_map<LoaderID, Loader> m_loaders;

    std::unordered_map<BundleID, std::unique_ptr<Bundle>> m_bundles;

    std::unordered_multimap<std::string, TextureID> m_textureNameIndex;
    std::unordered_multimap<std::string, MaterialID> m_materialNameIndex;
    std::unordered_multimap<std::string, MeshID> m_meshNameIndex;

};

static Manager sManager = Manager();

export Manager &GetManager()
{
    if (sManager.IsValid())
        return sManager;
    sManager.Init();
    return sManager;
};

} // namespace Asset
} // namespace EnvGraph
