import asset;

#include "gtest/gtest.h"

#include <vector>
#include <memory>
#include <filesystem>

#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

namespace
{

using namespace EnvGraph;
using namespace EnvGraph::Asset;

TEST(AssetTest, InitAssetManager)
{
    auto &assetManager = Asset::GetManager();

    ASSERT_EQ(assetManager.IsValid(), true);
}

TEST(AssetTest, NewBundle)
{
    auto &assetManager = Asset::GetManager();

    ASSERT_EQ(assetManager.IsValid(), true);
}

class AssetBundlerTest : public ::testing::Test {
public:
    void SetUp()
    {
#ifdef _WINDOWS
        _set_error_mode(_OUT_TO_MSGBOX);
#endif
        auto &assetManager = Asset::GetManager();
        ASSERT_EQ(assetManager.IsValid(), true);
        auto newBundleResult = assetManager.NewBundle("TestBundle");
        ASSERT_EQ(newBundleResult.first, AssetError::None);
        m_bundle = newBundleResult.second;
    }

    void TearDown()
    {
    
    }

protected:
    BundleID m_bundle;
};


TEST_F(AssetBundlerTest, NewTextureFromFile)
{
    auto &assetManager = Asset::GetManager();

    std::filesystem::path testImagePath("./1_webp_ll.webp");

    assetManager.NewTextureFromFile(m_bundle, "testImageTexture", testImagePath);
}

TEST_F(AssetBundlerTest, NewTextureFromFiles)
{
    auto &assetManager = Asset::GetManager();

    std::vector<std::filesystem::path> testImagePaths{std::filesystem::path("./1_webp_ll.webp"),
                                                      std::filesystem::path("./1_webp_ll.png")};
    assetManager.NewTextureFromFiles(m_bundle, "testImageTexture", testImagePaths);
}

}