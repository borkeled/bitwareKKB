#include "FakeStrings.h"
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <Auth/skStr.h>

char** FakeStringss::pool = nullptr;
size_t FakeStringss::poolCount = 0;

static int Rand(int min, int max)
{
    return min + (rand() % (max - min + 1));
}

static void FillRandomASCII(char* buf, size_t len)
{
    for (size_t i = 0; i < len; ++i)
    {
        int r = Rand(0, 2);
        if (r == 0) buf[i] = 'A' + Rand(0, 25);
        else if (r == 1) buf[i] = 'a' + Rand(0, 25);
        else buf[i] = '0' + Rand(0, 9);
    }
    buf[len] = '\0';
}

void FakeStringss::Generate(size_t count)
{
    if (pool) return; 

    srand((unsigned)time(nullptr));

    pool = new char* [count];
    poolCount = count;

    for (size_t i = 0; i < count; ++i)
    {
        size_t len = Rand(8, 64);

        pool[i] = new char[len + 1];
        FillRandomASCII(pool[i], len);

        if (Rand(0, 10) == 0)
        {
            const std::string FakeDecoyStrings[] = {
                std::string(skCrypt("TextureCache_Init")),
                std::string(skCrypt("ShaderCompile_Complete")),
                std::string(skCrypt("FrameBuffer_Alloc")),
                std::string(skCrypt("VertexBuffer_Update")),
                std::string(skCrypt("RenderTarget_Create")),
                std::string(skCrypt("DepthStencil_Init")),
                std::string(skCrypt("PipelineState_Create")),
                std::string(skCrypt("DescriptorHeap_Alloc")),
                std::string(skCrypt("CommandList_Execute")),
                std::string(skCrypt("FenceSignal_Wait")),
                std::string(skCrypt("SwapChain_Present")),
                std::string(skCrypt("ResourceUpload_Complete")),
                std::string(skCrypt("ComputeShader_Dispatch")),
                std::string(skCrypt("AsyncCompute_Start")),
                std::string(skCrypt("CopyQueue_Submit")),
                std::string(skCrypt("MeshletCull_Update")),
                std::string(skCrypt("IBLProbe_Generate")),
                std::string(skCrypt("ShadowMap_Update")),
                std::string(skCrypt("SSAO_ComputePass")),
                std::string(skCrypt("BloomBlur_Apply")),
                std::string(skCrypt("ToneMapping_Apply")),
                std::string(skCrypt("PostProcess_Execute")),
                std::string(skCrypt("ParticleSystem_Update")),
                std::string(skCrypt("PhysicsBroadphase_Update")),
                std::string(skCrypt("AnimationBlend_Compute")),
                std::string(skCrypt("SkinnedMesh_Upload")),
                std::string(skCrypt("AudioMixer_Process")),
                std::string(skCrypt("NetworkSync_Tick")),
                std::string(skCrypt("NavMesh_Generate")),
                std::string(skCrypt("AIPathFind_Request")),
                std::string(skCrypt("EntitySystem_Update")),
                std::string(skCrypt("ECSChunk_Alloc")),
                std::string(skCrypt("JobSystem_Wait")),
                std::string(skCrypt("AssetStream_Load")),
                std::string(skCrypt("BundleDecompress_Finish")),
                std::string(skCrypt("TextureStream_Mip")),
                std::string(skCrypt("AudioSource_Play")),
                std::string(skCrypt("MIDIEvent_Process")),
                std::string(skCrypt("InputDevice_Poll")),
                std::string(skCrypt("GamepadRumble_Apply")),
            };
            const std::string& b = FakeDecoyStrings[Rand(0, (int)(sizeof(FakeDecoyStrings) / sizeof(FakeDecoyStrings[0])) - 1)];
            strncpy_s(pool[i], len + 1, b.c_str(), _TRUNCATE);
            pool[i][len] = '\0';
        }
    }
}


void FakeStringss::Clear()
{
    if (!pool) return;

    for (size_t i = 0; i < poolCount; ++i)
        delete[] pool[i];

    delete[] pool;
    pool = nullptr;
    poolCount = 0;
}
