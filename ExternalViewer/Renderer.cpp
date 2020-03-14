#include "Renderer.h"
#include <d12util.h>
#include <memory>
#include <unordered_map>
#include <dxgi.h>
#include <imgui.h>
#include <plog/Log.h>
#include "ImGuiImplScreenState.h"
#include "ImGuiDX12.h"

#include <OrbitCamera.h>
#include "Gizmo.h"

#include "SceneLight.h"
#include "Scene.h"

using namespace d12u;

#include <shobjidl.h>
std::wstring OpenFileDialog(const std::wstring &folder)
{
    ComPtr<IFileOpenDialog> pFileOpen;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                                IID_PPV_ARGS(&pFileOpen))))
    {
        return L"";
    }

    COMDLG_FILTERSPEC fileTypes[] = {
        {L"gltf binary format", L"*.glb"},
        {L"all", L"*.*"},
    };
    if (FAILED(pFileOpen->SetFileTypes(_countof(fileTypes), fileTypes)))
    {
        return L"";
    }
    if (FAILED(pFileOpen->SetDefaultExtension(L".glb")))
    {
        return L"";
    }
    if (FAILED(pFileOpen->Show(NULL)))
    {
        return L"";
    }

    ComPtr<IShellItem> pItem;
    if (FAILED(pFileOpen->GetResult(&pItem)))
    {
        return L"";
    }

    PWSTR pszFilePath;
    if (FAILED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
    {
        return L"";
    }
    std::wstring result(pszFilePath);
    CoTaskMemFree(pszFilePath);

    // DWORD len = GetCurrentDirectoryW(0, NULL);
    // std::vector<wchar_t> dir(len);
    // GetCurrentDirectoryW((DWORD)dir.size(), dir.data());
    // if(dir.back()==0)
    // {
    //     dir.pop_back();
    // }
    // std::wcout << std::wstring(dir.begin(), dir.end()) << std::endl;

    return result;
}

static void ImGui_Impl_Win32_UpdateMouseCursor()
{
    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
    {
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return;
    }

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        ::SetCursor(NULL);
        return;
    }

    // Show OS mouse cursor
    LPTSTR win32_cursor = IDC_ARROW;
    switch (imgui_cursor)
    {
    case ImGuiMouseCursor_Arrow:
        win32_cursor = IDC_ARROW;
        break;
    case ImGuiMouseCursor_TextInput:
        win32_cursor = IDC_IBEAM;
        break;
    case ImGuiMouseCursor_ResizeAll:
        win32_cursor = IDC_SIZEALL;
        break;
    case ImGuiMouseCursor_ResizeEW:
        win32_cursor = IDC_SIZEWE;
        break;
    case ImGuiMouseCursor_ResizeNS:
        win32_cursor = IDC_SIZENS;
        break;
    case ImGuiMouseCursor_ResizeNESW:
        win32_cursor = IDC_SIZENESW;
        break;
    case ImGuiMouseCursor_ResizeNWSE:
        win32_cursor = IDC_SIZENWSE;
        break;
    case ImGuiMouseCursor_Hand:
        win32_cursor = IDC_HAND;
        break;
    case ImGuiMouseCursor_NotAllowed:
        win32_cursor = IDC_NO;
        break;
    }
    ::SetCursor(::LoadCursor(NULL, win32_cursor));
}

class Gui
{
    ImGuiDX12 m_dx12;

public:
    Gui(const ComPtr<ID3D12Device> &device, int bufferCount, HWND hwnd)
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // Setup Platform/Renderer bindings
        ImGui_Impl_ScreenState_Init();
        m_dx12.Initialize(device.Get(), bufferCount);

        // Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
        // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
        // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Read 'docs/FONTS.txt' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
        //io.Fonts->AddFontDefault();
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
        //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
        //IM_ASSERT(font != NULL);
    }

    ~Gui()
    {
        ImGui::DestroyContext();
    }

    void BeginFrame(const screenstate::ScreenState &state)
    {
        // Start the Dear ImGui frame
        ImGui_Impl_ScreenState_NewFrame(state);
        if (state.Has(screenstate::MouseButtonFlags::CursorUpdate))
        {
            ImGui_Impl_Win32_UpdateMouseCursor();
            // Update OS mouse cursor with the cursor requested by imgui
            // ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
            // if (g_LastMouseCursor != mouse_cursor)
            // {
            //     g_LastMouseCursor = mouse_cursor;
            //     UpdateMouseCursor();
            // }
        }

        ImGui::NewFrame();
    }

    void EndFrame(const ComPtr<ID3D12GraphicsCommandList> &commandList)
    {
        ImGui::Render();
        m_dx12.RenderDrawData(commandList.Get(), ImGui::GetDrawData());
    }
};

class Impl
{
    screenstate::ScreenState m_lastState = {};
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    std::unique_ptr<CommandQueue> m_queue;
    std::unique_ptr<SwapChain> m_rt;
    std::unique_ptr<RootSignature> m_rootSignature;
    std::unique_ptr<CommandList> m_commandlist;
    std::unique_ptr<SceneMapper> m_sceneMapper;

    std::unique_ptr<Gui> m_imgui;

    std::unique_ptr<hierarchy::Scene> m_scene;

    // scene
    std::unique_ptr<OrbitCamera> m_camera;
    std::unique_ptr<hierarchy::SceneLight> m_light;

    // gizmo
    Gizmo m_gizmo;

public:
    Impl(int maxModelCount)
        : m_queue(new CommandQueue),
          m_rt(new SwapChain(2)),
          m_commandlist(new CommandList),
          m_rootSignature(new RootSignature),
          m_sceneMapper(new SceneMapper),
          m_camera(new OrbitCamera),
          m_light(new hierarchy::SceneLight),
          m_scene(new hierarchy::Scene)
    {
        m_camera->zNear = 0.01f;
    }

    const std::unique_ptr<hierarchy::Scene> &Scene() const { return m_scene; }

    void Initialize(HWND hwnd)
    {
        ComPtr<IDXGIFactory4> factory;
        ThrowIfFailed(CreateDXGIFactory2(GetDxgiFactoryFlags(), IID_PPV_ARGS(&factory)));

        ComPtr<IDXGIAdapter1> hardwareAdapter = GetHardwareAdapter(factory.Get());
        ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)));

        m_queue->Initialize(m_device);
        m_rt->Initialize(factory, m_queue->Get(), hwnd);
        m_sceneMapper->Initialize(m_device);
        m_commandlist->InitializeDirect(m_device);
        m_rootSignature->Initialize(m_device);

        m_imgui.reset(new Gui(m_device, m_rt->BufferCount(), hwnd));
    }

    void OnFrame(HWND hwnd, const screenstate::ScreenState &state)
    {
        if (!m_device)
        {
            // first time
            Initialize(hwnd);

            m_rootSignature->GetNodeConstantsBuffer(m_gizmo.GetNodeID())->b1World = {
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1};
        }

        Update(hwnd, state);
        Draw(state);
    }

private:
    void Update(HWND hwnd, const screenstate::ScreenState &state)
    {
        m_sceneMapper->Update(m_device);

        // update
        if (m_lastState.Width != state.Width || m_lastState.Height != state.Height)
        {
            // recreate swapchain
            m_queue->SyncFence();
            m_rt->Resize(m_queue->Get(),
                         hwnd, state.Width, state.Height);
        }

        m_camera->Update(state);
        {
            auto buffer = m_rootSignature->GetSceneConstantsBuffer(0);
            buffer->b0Projection = fpalg::size_cast<DirectX::XMFLOAT4X4>(m_camera->state.projection);
            buffer->b0View = fpalg::size_cast<DirectX::XMFLOAT4X4>(m_camera->state.view);
            buffer->b0LightDir = m_light->LightDirection;
            buffer->b0LightColor = m_light->LightColor;
            buffer->b0CameraPosition = fpalg::size_cast<DirectX::XMFLOAT3>(m_camera->state.position);
            buffer->b0ScreenSizeFovY = {(float)state.Width, (float)state.Height, m_camera->state.fovYRadians};
            m_rootSignature->UploadSceneConstantsBuffer();
        }

        m_gizmo.Begin(state, m_camera->state);

        int nodeCount;
        auto nodes = m_scene->GetNodes(&nodeCount);
        for (int i = 0; i < nodeCount; ++i)
        {
            auto node = nodes[i];
            if (node)
            {
                m_rootSignature->GetNodeConstantsBuffer(node->ID())->b1World = node->TRS.Matrix();

                if (node->EnableGizmo())
                {
                    m_gizmo.Transform(i, node->TRS);
                }
            }
        }

        m_rootSignature->UploadNodeConstantsBuffer();
        m_lastState = state;

        // gizmo: upload
        {
            auto buffer = m_gizmo.End();

            auto drawable = m_sceneMapper->GetOrCreate(m_device, m_gizmo.GetMesh(), m_rootSignature.get());
            drawable->VertexBuffer()->MapCopyUnmap(buffer.pVertices, buffer.verticesBytes, buffer.vertexStride);
            drawable->IndexBuffer()->MapCopyUnmap(buffer.pIndices, buffer.indicesBytes, buffer.indexStride);
        }

        // shader udpate
        m_rootSignature->Update(m_device);
    }

    //
    // command
    //
    void Draw(const screenstate::ScreenState &state)
    {
        // new frame
        m_commandlist->Reset(nullptr);
        auto commandList = m_commandlist->Get();

        // clear
        float color[] = {
            0.2f,
            0.2f,
            0.3f,
            1.0f,
        };
        auto &rt = m_rt->Begin(commandList, color);

        // global settings
        m_rootSignature->Begin(commandList);

        // model
        int nodeCount;
        auto nodes = m_scene->GetNodes(&nodeCount);
        for (int i = 0; i < nodeCount; ++i)
        {
            auto node = nodes[i];
            if (node)
            {
                m_rootSignature->SetNodeDescriptorTable(commandList, node->ID());

                int meshCount;
                auto meshes = node->GetMeshes(&meshCount);
                for (int j = 0; j < meshCount; ++j)
                {
                    auto mesh = meshes[j];
                    if (mesh)
                    {
                        auto drawable = m_sceneMapper->GetOrCreate(m_device, mesh, m_rootSignature.get());
                        if (drawable && drawable->IsDrawable(m_commandlist.get()))
                        {
                            int offset = 0;
                            for (auto &submesh : mesh->submeshes)
                            {
                                auto material = m_rootSignature->GetOrCreate(m_device, submesh.material);

                                // texture setup
                                if (submesh.material->colorImage)
                                {
                                    auto [texture, textureSlot] = m_rootSignature->GetOrCreate(m_device, submesh.material->colorImage,
                                                                                               m_sceneMapper->GetUploader());
                                    if (texture)
                                    {
                                        if (texture->IsDrawable(m_commandlist.get(), 0))
                                        {
                                            m_rootSignature->SetTextureDescriptorTable(commandList, textureSlot);
                                        }
                                        else
                                        {
                                            // wait upload
                                            continue;
                                        }
                                    }
                                }

                                if (material->m_shader->Set(commandList))
                                {
                                    m_commandlist->Get()->DrawIndexedInstanced(submesh.draw_count, 1, offset, 0, 0);
                                }

                                offset += submesh.draw_count;
                            }
                        }
                    }
                }
            }
        }

        // gizmo: draw
        {
            m_rootSignature->SetNodeDescriptorTable(commandList, m_gizmo.GetNodeID());
            auto drawable = m_sceneMapper->GetOrCreate(m_device, m_gizmo.GetMesh(), m_rootSignature.get());
            if (drawable->IsDrawable(m_commandlist.get()))
            {
                int offset = 0;
                for (auto &submesh : m_gizmo.GetMesh()->submeshes)
                {
                    auto material = m_rootSignature->GetOrCreate(m_device, submesh.material);
                    if (material->m_shader->Set(commandList))
                    {
                        m_commandlist->Get()->DrawIndexedInstanced(submesh.draw_count, 1, offset, 0, 0);
                    }

                    offset += submesh.draw_count;
                }
            }
        }

        m_imgui->BeginFrame(state);

        ImGui::Begin("main");
        {
            if (ImGui::Button("open"))
            {
                auto path = OpenFileDialog(L"");
                m_scene->LoadFromPath(path);
                LOGI << "load: " << path;
            }
            ImGui::End();
        }

        m_imgui->EndFrame(commandList);

        // barrier
        m_rt->End(commandList, rt);

        // execute
        auto callbacks = m_commandlist->CloseAndGetCallbacks();
        m_queue->Execute(commandList);
        m_rt->Present();
        m_queue->SyncFence(callbacks);
    }
};

Renderer::Renderer(int maxModelCount)
    : m_impl(new Impl(maxModelCount))
{
}

Renderer::~Renderer()
{
    delete m_impl;
}

void Renderer::OnFrame(void *hwnd, const screenstate::ScreenState &state)
{
    m_impl->OnFrame((HWND)hwnd, state);
}

hierarchy::Scene *Renderer::GetScene()
{
    return m_impl->Scene().get();
}
