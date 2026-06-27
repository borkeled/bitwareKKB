#define NOMINMAX
#include "GpuMesh.h"

#include <Features/Chams/MemoryMeshes.h>

#include <d3dcompiler.h>
#include <Windows.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <unordered_map>
#include <vector>

#pragma comment(lib, "d3dcompiler.lib")

namespace mesh_gpu
{
namespace
{
    struct gpu_vertex final
    {
        float x, y, z;
    };

    struct frame_cb final
    {
        float view0[4];
        float view1[4];
        float view2[4];
        float view3[4];
        float shader_params[4];
        float shader_ext[4];
    };

    struct instance_data final
    {
        float rot0[4];
        float rot1[4];
        float rot2[4];
        float pos[4];
        float mesh_center[4];
        float mesh_scale[4];
        float color[4];
        float fx[4];
    };

    struct gpu_mesh_buffers final
    {
        ID3D11Buffer* vb = nullptr;
        ID3D11Buffer* ib = nullptr;
        UINT index_count = 0;
    };

    static ID3D11Device* g_device = nullptr;
    static ID3D11VertexShader* g_vs = nullptr;
    static ID3D11PixelShader* g_ps = nullptr;
    static ID3D11InputLayout* g_layout = nullptr;
    static ID3D11Buffer* g_frame_cb = nullptr;
    static ID3D11Buffer* g_instance_sb = nullptr;
    static ID3D11ShaderResourceView* g_instance_srv = nullptr;
    static ID3D11BlendState* g_blend = nullptr;
    static ID3D11BlendState* g_blend_glass = nullptr;
    static ID3D11RasterizerState* g_raster_solid = nullptr;
    static ID3D11RasterizerState* g_raster_outline = nullptr;
    static ID3D11DepthStencilState* g_depth_disabled = nullptr;
    static std::unordered_map<std::uint64_t, gpu_mesh_buffers> g_gpu_meshes;

    static constexpr UINT k_max_instances = 384;
    static constexpr UINT k_instance_stride = static_cast<UINT>(sizeof(instance_data));
    static constexpr std::uint64_t k_box_key_tag = 0xB0A7000000000000ull;

    static std::vector<instance_data> g_instance_scratch;
    static std::vector<draw_item> g_sort_scratch;

    static float g_time_seconds = 0.f;

    static void release_resources()
    {
        for (auto& [key, mesh] : g_gpu_meshes)
        {
            (void)key;
            if (mesh.vb) mesh.vb->Release();
            if (mesh.ib) mesh.ib->Release();
        }

        g_gpu_meshes.clear();

        if (g_instance_srv) { g_instance_srv->Release(); g_instance_srv = nullptr; }
        if (g_instance_sb) { g_instance_sb->Release(); g_instance_sb = nullptr; }
        if (g_depth_disabled) { g_depth_disabled->Release(); g_depth_disabled = nullptr; }
        if (g_raster_outline) { g_raster_outline->Release(); g_raster_outline = nullptr; }
        if (g_raster_solid) { g_raster_solid->Release(); g_raster_solid = nullptr; }
        if (g_blend_glass) { g_blend_glass->Release(); g_blend_glass = nullptr; }
        if (g_blend) { g_blend->Release(); g_blend = nullptr; }
        if (g_frame_cb) { g_frame_cb->Release(); g_frame_cb = nullptr; }
        if (g_layout) { g_layout->Release(); g_layout = nullptr; }
        if (g_ps) { g_ps->Release(); g_ps = nullptr; }
        if (g_vs) { g_vs->Release(); g_vs = nullptr; }
        g_device = nullptr;
    }

    static void decimate_indices(const mesh_chams::parsed_mesh& mesh, std::vector<std::uint32_t>& out_indices, int max_triangles)
    {
        const std::size_t max_indices = static_cast<std::size_t>((std::max)(1, max_triangles)) * 3;
        if (mesh.indices.size() <= max_indices)
        {
            out_indices = mesh.indices;
            return;
        }

        out_indices.clear();
        out_indices.reserve(max_indices);

        for (std::size_t i = 0; i + 2 < mesh.indices.size() && out_indices.size() + 3 <= max_indices; i += 3)
        {
            out_indices.push_back(mesh.indices[i]);
            out_indices.push_back(mesh.indices[i + 1]);
            out_indices.push_back(mesh.indices[i + 2]);
        }
    }

    static bool is_box_cache_key(std::uint64_t cache_key)
    {
        return (cache_key & 0xFFFF000000000000ull) == k_box_key_tag;
    }

    static bool ensure_gpu_mesh(ID3D11Device* device, std::uint64_t cache_key, const mesh_chams::parsed_mesh& mesh, int max_triangles)
    {
        auto it = g_gpu_meshes.find(cache_key);
        if (it != g_gpu_meshes.end())
            return true;

        if (mesh.vertices.empty() || mesh.indices.size() < 3)
            return false;

        std::vector<std::uint32_t> indices;
        if (is_box_cache_key(cache_key))
            indices = mesh.indices;
        else
            decimate_indices(mesh, indices, max_triangles);
        if (indices.size() < 3)
            return false;

        std::vector<gpu_vertex> vertices;
        vertices.reserve(mesh.vertices.size());
        for (const auto& vertex : mesh.vertices)
            vertices.push_back({ vertex.position.x, vertex.position.y, vertex.position.z });

        D3D11_BUFFER_DESC vb_desc{};
        vb_desc.Usage = D3D11_USAGE_IMMUTABLE;
        vb_desc.ByteWidth = static_cast<UINT>(vertices.size() * sizeof(gpu_vertex));
        vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vb_data{};
        vb_data.pSysMem = vertices.data();

        D3D11_BUFFER_DESC ib_desc{};
        ib_desc.Usage = D3D11_USAGE_IMMUTABLE;
        ib_desc.ByteWidth = static_cast<UINT>(indices.size() * sizeof(std::uint32_t));
        ib_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA ib_data{};
        ib_data.pSysMem = indices.data();

        gpu_mesh_buffers gpu_mesh{};
        HRESULT hr = device->CreateBuffer(&vb_desc, &vb_data, &gpu_mesh.vb);
        if (FAILED(hr)) return false;

        hr = device->CreateBuffer(&ib_desc, &ib_data, &gpu_mesh.ib);
        if (FAILED(hr))
        {
            gpu_mesh.vb->Release();
            return false;
        }

        gpu_mesh.index_count = static_cast<UINT>(indices.size());
        g_gpu_meshes.emplace(cache_key, gpu_mesh);
        return true;
    }

    static bool ensure_pipeline(ID3D11Device* device)
    {
        if (!device)
            return false;

        if (g_device == device && g_vs && g_ps && g_layout && g_frame_cb && g_instance_sb && g_instance_srv &&
            g_blend && g_blend_glass && g_raster_solid && g_raster_outline && g_depth_disabled)
        {
            return true;
        }

        if (g_device == device)
            release_resources();

        release_resources();
        g_device = device;

        static const char* vs_src =
            "cbuffer FrameCB : register(b0) {"
            " float4 view0; float4 view1; float4 view2; float4 view3;"
            " float4 shaderParams;"
            " float4 shaderExt;"
            "};"
            "struct InstanceData {"
            " float4 rot0; float4 rot1; float4 rot2; float4 objPos;"
            " float4 meshCenter; float4 meshScale; float4 meshColor;"
            " float4 fx;"
            "};"
            "StructuredBuffer<InstanceData> gInstances : register(t0);"
            "struct VSInput { float3 pos : POSITION; };"
            "struct VSOutput {"
            " float4 pos : SV_POSITION;"
            " float4 color : COLOR0;"
            " float3 worldPos : TEXCOORD0;"
            " float3 localDir : TEXCOORD1;"
            " float3 localPos : TEXCOORD2;"
            "};"
            "VSOutput main(VSInput input, uint instID : SV_InstanceID) {"
            " InstanceData obj = gInstances[instID];"
            " float inflate = shaderParams.z;"
            " float3 local = (input.pos - obj.meshCenter.xyz) * (obj.meshScale.xyz * inflate);"
            " float3 world;"
            " world.x = obj.objPos.x + dot(local, obj.rot0.xyz);"
            " world.y = obj.objPos.y + dot(local, obj.rot1.xyz);"
            " world.z = obj.objPos.z + dot(local, obj.rot2.xyz);"
            " VSOutput o;"
            " o.worldPos = world;"
            " o.localDir = normalize(local + 0.0001);"
            " o.localPos = local;"
            " o.pos.x = world.x * view0.x + world.y * view0.y + world.z * view0.z + view0.w;"
            " o.pos.y = world.x * view1.x + world.y * view1.y + world.z * view1.z + view1.w;"
            " o.pos.z = world.x * view2.x + world.y * view2.y + world.z * view2.z + view2.w;"
            " o.pos.w = world.x * view3.x + world.y * view3.y + world.z * view3.z + view3.w;"
            " o.color = obj.meshColor;"
            " o.color.w = shaderParams.w;"
            " return o;"
            "}";

        static const char* ps_src =
            "cbuffer FrameCB : register(b0) {"
            " float4 view0; float4 view1; float4 view2; float4 view3;"
            " float4 shaderParams;"
            " float4 shaderExt;"
            "};"
            "struct PSInput {"
            " float4 pos : SV_POSITION;"
            " float4 color : COLOR0;"
            " float3 worldPos : TEXCOORD0;"
            " float3 localDir : TEXCOORD1;"
            " float3 localPos : TEXCOORD2;"
            "};"
            "float3 hsv_to_rgb(float h, float s, float v) {"
            " float4 k = float4(1.0, 2.0/3.0, 1.0/3.0, 3.0);"
            " float3 p = abs(frac(h.xxx + k.xyz) * 6.0 - k.www);"
            " return v * lerp(k.xxx, saturate(p - k.xxx), s);"
            "}"
            "float hash(float2 p) {"
            " p = frac(p * float2(127.1, 311.7));"
            " p += dot(p, p.yx + 19.19);"
            " return frac((p.x + p.y) * p.x);"
            "}"
            "float noise2(float2 p) {"
            " float2 i = floor(p); float2 f = frac(p);"
            " float2 u = f*f*(3.0-2.0*f);"
            " return lerp(lerp(hash(i),hash(i+float2(1,0)),u.x),"
            "             lerp(hash(i+float2(0,1)),hash(i+float2(1,1)),u.x),u.y);"
            "}"
            "float fbm(float2 p) {"
            " float v=0.0,a=0.5;"
            " for(int i=0;i<4;i++){v+=a*noise2(p);p*=2.1;a*=0.5;}"
            " return v;"
            "}"
            "float3 specular_contrib(float3 col, float3 localDir, float specPow, float specInt) {"
            " float3 L = normalize(float3(0.6,1.0,0.4));"
            " float3 N = normalize(localDir);"
            " float3 H = normalize(L + float3(0,0,1));"
            " float spec = pow(saturate(dot(N,H)), specPow) * specInt;"
            " return col + spec;"
            "}"
            "float4 main(PSInput input) : SV_TARGET {"
            " float4 col = input.color;"
            " int mode = (int)(shaderParams.y + 0.5);"
            " float time = shaderParams.x;"
            " float baseA = shaderParams.w;"
            " float specPow = shaderExt.x;"
            " float specInt = shaderExt.y;"
            " float paramA = shaderExt.z;"
            " float paramB = shaderExt.w;"
            " float3 wp = input.worldPos;"
            " float3 ld = normalize(input.localDir + 0.0001);"
            " float3 lp = input.localPos;"
            " float seed = frac(wp.x*0.07 + wp.z*0.11);"

            " if(mode==0){"
            "  col.rgb = specular_contrib(col.rgb, ld, specPow, specInt);"
            "  col.a *= baseA; return col;"
            " }"

            " if(mode==1){"
            "  float rough = paramA;"
            "  float3 N = normalize(ld);"
            "  float3 L = normalize(float3(0.6,1.0,0.4));"
            "  float diff = saturate(dot(N,L))*0.6+0.4;"
            "  float3 H = normalize(L+float3(0,0,1));"
            "  float spec = pow(saturate(dot(N,H)), max(specPow,1.0)) * specInt;"
            "  float3 metal = col.rgb * diff + spec;"
            "  float env = 0.3+0.7*pow(saturate(1.0-abs(dot(N,float3(0,1,0)))),2.0);"
            "  metal = lerp(metal, col.rgb*1.4, env*(1.0-rough));"
            "  col.rgb = metal; col.a *= baseA; return col;"
            " }"

            " if(mode==2){"
            "  float amount = paramA;"
            "  float edgeW = paramB;"
            "  float n = fbm(wp.xz*1.5 + time*0.3);"
            "  if(n < amount) discard;"
            "  float edgeFac = saturate((n - amount) / max(edgeW,0.001));"
            "  float3 edgeCol = float3(1.0,0.4,0.0);"
            "  col.rgb = lerp(edgeCol, col.rgb, edgeFac);"
            "  col.a *= baseA; return col;"
            " }"

            " if(mode==3){"
            "  float3 bary = abs(ld);"
            "  float minB = min(bary.x, min(bary.y, bary.z));"
            "  float wire = 1.0 - smoothstep(0.0, 0.04, minB);"
            "  float glow = exp(-minB*30.0)*0.6;"
            "  col.rgb = col.rgb*wire + col.rgb*glow;"
            "  col.a = saturate(wire + glow) * baseA; return col;"
            " }"

            " if(mode==4){"
            "  float grey = dot(col.rgb, float3(0.299,0.587,0.114));"
            "  col.rgb = grey.xxx;"
            "  col.rgb = specular_contrib(col.rgb, ld, specPow, specInt);"
            "  col.a *= baseA; return col;"
            " }"

            " if(mode==5){"
            "  float cspd = paramB;"
            "  float cscl = paramA;"
            "  float2 uv = wp.xz * cscl * 0.5;"
            "  float c1 = noise2(uv + time*cspd);"
            "  float c2 = noise2(uv*1.7 - time*cspd*0.7);"
            "  float caustic = pow(saturate(c1*c2*2.0),1.5);"
            "  col.rgb = lerp(col.rgb, col.rgb*2.5+float3(0.2,0.4,0.8)*caustic, 0.7);"
            "  col.a *= baseA; return col;"
            " }"

            " if(mode==6){"
            "  float sharp = paramA;"
            "  float3 N = normalize(ld);"
            "  float3 R = reflect(float3(0,0,-1), N);"
            "  float env = pow(saturate(dot(R, float3(0.6,1.0,0.4))), sharp);"
            "  float3 chrome = lerp(col.rgb*0.3, float3(0.9,0.9,1.0), env);"
            "  chrome += pow(saturate(dot(N, normalize(float3(0.6,1.0,0.4)))), specPow)*specInt;"
            "  col.rgb = chrome; col.a *= baseA; return col;"
            " }"

            " if(mode==7){"
            "  float lspd = paramB;"
            "  float wave = paramA;"
            "  float2 uv = wp.xz*0.4 + float2(0, time*lspd);"
            "  float n = fbm(uv);"
            "  float ripple = sin(n*12.0 + time*lspd*2.0)*wave;"
            "  col.rgb = col.rgb*(0.7+ripple*0.5) + float3(0.0,0.1,0.3)*ripple;"
            "  col.a *= baseA*(0.7+ripple*0.3); return col;"
            " }"

            " if(mode==8){"
            "  float scanSpd = paramB;"
            "  float opacity = paramA;"
            "  float scan = frac(wp.y*0.3 - time*scanSpd);"
            "  float scanLine = step(0.95, scan);"
            "  float flicker = 0.85 + 0.15*sin(time*17.3 + seed*6.28);"
            "  col.rgb = col.rgb*flicker + float3(0.0,0.8,1.0)*scanLine*0.6;"
            "  col.a = opacity * baseA * flicker * (0.5 + scanLine*0.5); return col;"
            " }"

            " if(mode==9){"
            "  float gap = paramA;"
            "  float spd = paramB;"
            "  float slice = frac(lp.y*3.0 + time*spd);"
            "  if(slice < gap) discard;"
            "  col.a *= baseA; return col;"
            " }"

            " if(mode==10){"
            "  float hue = frac(seed + time*0.12 + length(wp)*0.03);"
            "  float3 rgb = hsv_to_rgb(hue, 0.85, 1.0);"
            "  col.rgb = lerp(col.rgb, rgb, 0.75);"
            "  col.rgb = specular_contrib(col.rgb, ld, specPow, specInt);"
            "  col.a *= baseA; return col;"
            " }"

            " if(mode==11){"
            "  float ndcZ = input.pos.z / max(input.pos.w, 0.001);"
            "  float fresnel = pow(saturate(1.0 - abs(ndcZ)), 2.4);"
            "  float edge = saturate(abs(ld.y) + abs(ld.x)*0.5);"
            "  col.rgb = col.rgb*(0.55+fresnel*0.9) + float3(0.08,0.12,0.18)*fresnel;"
            "  col.a = lerp(baseA*0.18, baseA*0.72, fresnel)*(0.7+edge*0.3); return col;"
            " }"

            " if(mode==12){"
            "  float lspd = paramA;"
            "  float lscl = paramB;"
            "  float2 uv = wp.xz*lscl*0.3 + float2(0, time*lspd*0.2);"
            "  float n = fbm(uv);"
            "  float3 lava1 = float3(1.0,0.2,0.0);"
            "  float3 lava2 = float3(1.0,0.8,0.0);"
            "  float3 dark  = float3(0.05,0.02,0.0);"
            "  float3 lc = lerp(dark, lerp(lava1,lava2,n), pow(n,0.5));"
            "  col.rgb = lerp(col.rgb, lc, 0.85);"
            "  col.a *= baseA; return col;"
            " }"

            " if(mode==13){"
            "  float gint = paramA;"
            "  float gspd = paramB;"
            "  float block = floor(wp.y*8.0 + time*gspd*3.0);"
            "  float rnd = hash(float2(block, floor(time*gspd)));"
            "  float shift = (rnd - 0.5)*gint*0.3;"
            "  col.rgb = col.rgb + float3(shift, -shift*0.5, shift*0.3);"
            "  float flicker = step(0.92, hash(float2(time*gspd*7.0, seed)));"
            "  col.rgb = lerp(col.rgb, 1.0-col.rgb, flicker*gint);"
            "  col.a *= baseA; return col;"
            " }"

            " if(mode==14){"
            "  float rough = paramA;"
            "  float n = fbm(wp.xz*2.0 + wp.y*1.5);"
            "  float3 iceBase = float3(0.7,0.85,1.0);"
            "  float3 iceDark = float3(0.3,0.5,0.8);"
            "  float3 ic = lerp(iceDark, iceBase, n);"
            "  float3 N = normalize(ld);"
            "  float3 H = normalize(float3(0.6,1.0,0.4)+float3(0,0,1));"
            "  float spec = pow(saturate(dot(N,H)), max(specPow,1.0))*specInt*(1.0-rough);"
            "  col.rgb = lerp(col.rgb, ic, 0.7) + spec;"
            "  col.a *= baseA*0.85; return col;"
            " }"

            " if(mode==15){"
            "  float pspd = paramA;"
            "  float pwid = paramB;"
            "  float pulse = 0.5+0.5*sin(time*pspd + seed*6.28);"
            "  float glow = exp(-abs(pulse-0.5)/max(pwid,0.01))*1.5;"
            "  col.rgb = col.rgb*(0.3+pulse*0.7) + col.rgb*glow*0.8;"
            "  col.a *= baseA*(0.45+pulse*0.55); return col;"
            " }"

            " if(mode==16){"
            "  float3 goldBase = float3(1.0,0.78,0.1);"
            "  float3 goldDark = float3(0.6,0.4,0.0);"
            "  float3 N = normalize(ld);"
            "  float3 L = normalize(float3(0.6,1.0,0.4));"
            "  float diff = saturate(dot(N,L))*0.7+0.3;"
            "  float3 H = normalize(L+float3(0,0,1));"
            "  float spec = pow(saturate(dot(N,H)), max(specPow,1.0))*specInt*1.5;"
            "  float3 gc = lerp(goldDark, goldBase, diff) + spec;"
            "  col.rgb = lerp(col.rgb, gc, 0.85);"
            "  col.a *= baseA; return col;"
            " }"

            " if(mode==17){"
            "  float density = paramA;"
            "  float depth = input.pos.z / max(input.pos.w, 0.001);"
            "  float fog = saturate(depth * density * 8.0);"
            "  float3 fogCol = float3(0.5,0.6,0.8);"
            "  col.rgb = lerp(col.rgb, fogCol, fog);"
            "  col.a *= baseA*(1.0-fog*0.5); return col;"
            " }"

            " if(mode==18){"
            "  float scl = paramA;"
            "  float2 uv = wp.xz*scl*0.8;"
            "  float n = noise2(uv);"
            "  float splat = step(0.55, n);"
            "  float3 splatCol = float3(0.8,0.1,0.0);"
            "  col.rgb = lerp(col.rgb, splatCol, splat*0.8);"
            "  col.a *= baseA; return col;"
            " }"

            " if(mode==19){"
            "  float rimPow = paramA;"
            "  float ndcZ = input.pos.z / max(input.pos.w, 0.001);"
            "  float fresnel = pow(saturate(1.0 - abs(ndcZ)), rimPow);"
            "  float edge = saturate(abs(ld.y) + abs(ld.x)*0.5);"
            "  col.rgb = col.rgb*(0.55+fresnel*0.9) + float3(0.08,0.12,0.18)*fresnel;"
            "  col.a = lerp(baseA*0.18, baseA*0.72, fresnel)*(0.7+edge*0.3); return col;"
            " }"

            " if(mode==20){"
            "  float wscl = paramA;"
            "  float rings = sin((length(wp.xz)*wscl*3.0) + fbm(wp.xz*wscl)*2.0)*0.5+0.5;"
            "  float3 woodLight = float3(0.76,0.52,0.22);"
            "  float3 woodDark  = float3(0.42,0.26,0.08);"
            "  col.rgb = lerp(woodDark, woodLight, rings);"
            "  col.rgb = specular_contrib(col.rgb, ld, specPow, specInt*0.3);"
            "  col.a *= baseA; return col;"
            " }"

            " if(mode==21){"
            "  float shine = paramA;"
            "  float3 N = normalize(ld);"
            "  float3 L = normalize(float3(0.6,1.0,0.4));"
            "  float diff = saturate(dot(N,L))*0.5+0.5;"
            "  float3 H = normalize(L+float3(0,0,1));"
            "  float spec = pow(saturate(dot(N,H)), shine)*specInt*1.2;"
            "  col.rgb = col.rgb*diff + spec;"
            "  col.a *= baseA; return col;"
            " }"

            " col.a *= baseA; return col;"
            "}";

        ID3DBlob* vs_blob = nullptr;
        ID3DBlob* ps_blob = nullptr;
        ID3DBlob* error_blob = nullptr;

        HRESULT hr = D3DCompile(vs_src, std::strlen(vs_src), nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0, &vs_blob, &error_blob);
        if (FAILED(hr))
        {
            if (error_blob) error_blob->Release();
            return false;
        }

        hr = D3DCompile(ps_src, std::strlen(ps_src), nullptr, nullptr, nullptr, "main", "ps_4_0", 0, 0, &ps_blob, &error_blob);
        if (FAILED(hr))
        {
            if (error_blob) error_blob->Release();
            if (vs_blob) vs_blob->Release();
            return false;
        }

        hr = device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &g_vs);
        if (FAILED(hr)) return false;

        hr = device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &g_ps);
        if (FAILED(hr)) return false;

        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        hr = device->CreateInputLayout(layout, 1, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &g_layout);
        vs_blob->Release();
        ps_blob->Release();
        if (FAILED(hr)) return false;

        D3D11_BUFFER_DESC cb_desc{};
        cb_desc.Usage = D3D11_USAGE_DYNAMIC;
        cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cb_desc.ByteWidth = (sizeof(frame_cb) + 15) & ~15u;
        hr = device->CreateBuffer(&cb_desc, nullptr, &g_frame_cb);
        if (FAILED(hr)) return false;

        D3D11_BUFFER_DESC sb_desc{};
        sb_desc.Usage = D3D11_USAGE_DYNAMIC;
        sb_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        sb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        sb_desc.ByteWidth = k_max_instances * k_instance_stride;
        sb_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        sb_desc.StructureByteStride = k_instance_stride;
        hr = device->CreateBuffer(&sb_desc, nullptr, &g_instance_sb);
        if (FAILED(hr)) return false;

        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer.FirstElement = 0;
        srv_desc.Buffer.NumElements = k_max_instances;
        hr = device->CreateShaderResourceView(g_instance_sb, &srv_desc, &g_instance_srv);
        if (FAILED(hr)) return false;

        D3D11_BLEND_DESC blend_desc{};
        blend_desc.RenderTarget[0].BlendEnable = TRUE;
        blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        hr = device->CreateBlendState(&blend_desc, &g_blend);
        if (FAILED(hr)) return false;

        blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        hr = device->CreateBlendState(&blend_desc, &g_blend_glass);
        if (FAILED(hr)) return false;

        D3D11_RASTERIZER_DESC solid_desc{};
        solid_desc.FillMode = D3D11_FILL_SOLID;
        solid_desc.CullMode = D3D11_CULL_NONE;
        solid_desc.DepthClipEnable = TRUE;
        hr = device->CreateRasterizerState(&solid_desc, &g_raster_solid);
        if (FAILED(hr)) return false;

        D3D11_RASTERIZER_DESC outline_desc = solid_desc;
        outline_desc.CullMode = D3D11_CULL_FRONT;
        hr = device->CreateRasterizerState(&outline_desc, &g_raster_outline);
        if (FAILED(hr)) return false;

        D3D11_DEPTH_STENCIL_DESC depth_desc{};
        depth_desc.DepthEnable = FALSE;
        hr = device->CreateDepthStencilState(&depth_desc, &g_depth_disabled);
        if (FAILED(hr)) return false;

        g_instance_scratch.reserve(k_max_instances);
        return true;
    }

    static bool upload_frame_cb(ID3D11DeviceContext* context, const float view[16], const render_params& params, float inflate)
    {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        HRESULT hr = context->Map(g_frame_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(hr)) return false;

        frame_cb cb{};
        std::memcpy(cb.view0, view + 0, sizeof(float) * 4);
        std::memcpy(cb.view1, view + 4, sizeof(float) * 4);
        std::memcpy(cb.view2, view + 8, sizeof(float) * 4);
        std::memcpy(cb.view3, view + 12, sizeof(float) * 4);
        cb.shader_params[0] = g_time_seconds;
        cb.shader_params[1] = static_cast<float>(params.shader_mode);
        cb.shader_params[2] = inflate;
        cb.shader_params[3] = params.fill_alpha;
        cb.shader_ext[0] = params.specular_power;
        cb.shader_ext[1] = params.specular_intensity;
        cb.shader_ext[2] = params.shader_param_a;
        cb.shader_ext[3] = params.shader_param_b;
        std::memcpy(mapped.pData, &cb, sizeof(cb));
        context->Unmap(g_frame_cb, 0);
        return true;
    }

    static bool upload_instances(ID3D11DeviceContext* context, const std::vector<instance_data>& instances)
    {
        if (instances.empty() || instances.size() > k_max_instances)
            return false;

        D3D11_MAPPED_SUBRESOURCE mapped{};
        HRESULT hr = context->Map(g_instance_sb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(hr)) return false;

        std::memcpy(mapped.pData, instances.data(), instances.size() * k_instance_stride);
        context->Unmap(g_instance_sb, 0);
        return true;
    }

    static instance_data make_instance(const draw_item& item, const float color[4])
    {
        instance_data inst{};
        inst.rot0[0] = item.rot[0]; inst.rot0[1] = item.rot[1]; inst.rot0[2] = item.rot[2];
        inst.rot1[0] = item.rot[3]; inst.rot1[1] = item.rot[4]; inst.rot1[2] = item.rot[5];
        inst.rot2[0] = item.rot[6]; inst.rot2[1] = item.rot[7]; inst.rot2[2] = item.rot[8];
        inst.pos[0] = item.pos.x; inst.pos[1] = item.pos.y; inst.pos[2] = item.pos.z; inst.pos[3] = 1.f;

        if (item.mesh && item.mesh->bounds.valid)
        {
            inst.mesh_center[0] = item.mesh->bounds.center.x;
            inst.mesh_center[1] = item.mesh->bounds.center.y;
            inst.mesh_center[2] = item.mesh->bounds.center.z;
            inst.mesh_scale[0] = item.mesh->bounds.size.x > 0.001f ? item.size.x / item.mesh->bounds.size.x : 1.f;
            inst.mesh_scale[1] = item.mesh->bounds.size.y > 0.001f ? item.size.y / item.mesh->bounds.size.y : 1.f;
            inst.mesh_scale[2] = item.mesh->bounds.size.z > 0.001f ? item.size.z / item.mesh->bounds.size.z : 1.f;
        }
        else
        {
            inst.mesh_scale[0] = item.size.x;
            inst.mesh_scale[1] = item.size.y;
            inst.mesh_scale[2] = item.size.z;
        }

        inst.color[0] = color[0];
        inst.color[1] = color[1];
        inst.color[2] = color[2];
        inst.color[3] = color[3];
        inst.fx[0] = item.part_seed;
        inst.fx[1] = item.live_mesh ? 1.f : 0.f;
        return inst;
    }

    static void setup_pipeline(
        ID3D11DeviceContext* context,
        float viewport_width,
        float viewport_height,
        float viewport_x,
        float viewport_y,
        ID3D11RasterizerState* raster,
        ID3D11BlendState* blend)
    {
        D3D11_VIEWPORT vp{};
        vp.TopLeftX = viewport_x;
        vp.TopLeftY = viewport_y;
        vp.Width = viewport_width;
        vp.Height = viewport_height;
        vp.MinDepth = 0.f;
        vp.MaxDepth = 1.f;
        context->RSSetViewports(1, &vp);
        context->IASetInputLayout(g_layout);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(g_vs, nullptr, 0);
        context->PSSetShader(g_ps, nullptr, 0);
        context->VSSetConstantBuffers(0, 1, &g_frame_cb);
        context->PSSetConstantBuffers(0, 1, &g_frame_cb);
        context->VSSetShaderResources(0, 1, &g_instance_srv);
        context->OMSetBlendState(blend, nullptr, 0xffffffff);
        context->RSSetState(raster);
        context->OMSetDepthStencilState(g_depth_disabled, 0);
    }

    static std::size_t draw_grouped_batches(
        ID3D11Device* device,
        ID3D11DeviceContext* context,
        const std::vector<draw_item>& items,
        const float color[4],
        float inflate,
        int max_triangles,
        bool occlusion_pass = false,
        bool want_occluded = false)
    {
        if (items.empty())
            return 0;

        std::unordered_map<std::uint64_t, std::vector<const draw_item*>> batches;
        batches.reserve(32);

        for (const draw_item& item : items)
        {
            if (!item.cache_key || !item.mesh)
                continue;
            if (occlusion_pass && item.occluded != want_occluded)
                continue;
            batches[item.cache_key].push_back(&item);
        }

        std::size_t drawn = 0;

        for (auto& [cache_key, group] : batches)
        {
            if (group.empty())
                continue;

            const mesh_chams::parsed_mesh& mesh = *group.front()->mesh;
            if (!ensure_gpu_mesh(device, cache_key, mesh, max_triangles))
                continue;

            auto mesh_it = g_gpu_meshes.find(cache_key);
            if (mesh_it == g_gpu_meshes.end())
                continue;

            g_instance_scratch.clear();
            g_instance_scratch.reserve(group.size());

            for (const draw_item* item : group)
            {
                if (g_instance_scratch.size() >= k_max_instances)
                    break;
                g_instance_scratch.push_back(make_instance(*item, color));
            }

            if (!upload_instances(context, g_instance_scratch))
                continue;

            UINT stride = sizeof(gpu_vertex);
            UINT offset = 0;
            context->IASetVertexBuffers(0, 1, &mesh_it->second.vb, &stride, &offset);
            context->IASetIndexBuffer(mesh_it->second.ib, DXGI_FORMAT_R32_UINT, 0);
            context->DrawIndexedInstanced(mesh_it->second.index_count, static_cast<UINT>(g_instance_scratch.size()), 0, 0, 0);
            drawn += g_instance_scratch.size();
        }

        return drawn;
    }
}

std::size_t render_draw_list(
    const std::vector<draw_item>& items,
    const float view[16],
    float viewport_width,
    float viewport_height,
    float viewport_x,
    float viewport_y,
    ID3D11Device* device,
    ID3D11DeviceContext* context,
    const render_params& params)
{
    if (!device || !context || items.empty())
        return 0;

    if (!ensure_pipeline(device))
        return 0;

    static auto start_time = std::chrono::steady_clock::now();
    const auto now = std::chrono::steady_clock::now();
    g_time_seconds = std::chrono::duration<float>(now - start_time).count() * params.shader_speed;

    std::size_t drawn = 0;

    auto draw_pass = [&](const float fill_col[4], const float outline_col[4], bool want_occluded)
    {
        if (params.outline)
        {
            upload_frame_cb(context, view, params, params.outline_scale);
            setup_pipeline(context, viewport_width, viewport_height, viewport_x, viewport_y, g_raster_outline, g_blend);
            drawn += draw_grouped_batches(
                device, context, items, outline_col, params.outline_scale, params.max_triangles,
                params.occlusion_enabled, want_occluded);
        }

        if (params.fill_enabled)
        {
            upload_frame_cb(context, view, params, 1.f);
            setup_pipeline(context, viewport_width, viewport_height, viewport_x, viewport_y, g_raster_solid, g_blend);
            drawn += draw_grouped_batches(
                device, context, items, fill_col, 1.f, params.max_triangles,
                params.occlusion_enabled, want_occluded);
        }
    };

    if (params.occlusion_enabled)
    {
        draw_pass(params.visible_color, params.outline_color, false);
        draw_pass(params.occluded_color, params.outline_color, true);
    }
    else
    {
        draw_pass(params.fill_color, params.outline_color, false);
    }

    return drawn;
}

void shutdown()
{
    release_resources();
}

} // namespace mesh_gpu
