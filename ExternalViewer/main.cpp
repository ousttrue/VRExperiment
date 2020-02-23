#include "Win32Window.h"
#include "Renderer.h"
#include "VR.h"
#include "Scene.h"
#include <d3d12.h>

static std::shared_ptr<hierarchy::SceneMesh> CreateGrid()
{
    // /** A single vertex in a render model */
    // struct RenderModel_Vertex_t
    // {
    // 	HmdVector3_t vPosition;		// position in meters in device space
    // 	HmdVector3_t vNormal;
    // 	float rfTextureCoord[2];
    // };
    vr::RenderModel_Vertex_t vertices[] = {
        {{-1, 0, -1}, {0, 0, 1}, {0, 1}},
        {{1, 0, -1}, {0, 0, 1}, {1, 1}},
        {{1, 0, 1}, {0, 0, 1}, {1, 0}},
        {{-1, 0, 1}, {0, 0, 1}, {0, 0}},
    };
    uint16_t indices[] = {
        0, 1, 2, //
        2, 3, 0, //
    };
    auto model = hierarchy::SceneMesh::Create();
    model->SetVertices(vertices);
    model->SetIndices(indices);
    return model;
}

const auto CLASS_NAME = L"ExternalViewerClass";
const auto WINDOW_NAME = L"ExternalViewer";

int main(int argc, char **argv)
{
    screenstate::Win32Window window(CLASS_NAME);

    auto hwnd = window.Create(WINDOW_NAME);
    if (!hwnd)
    {
        return 1;
    }

    window.Show();

    {
        Renderer renderer(65);
        VR vr;

        if (!vr.Connect())
        {
            return 2;
        }

        auto scene = renderer.GetScene();
        for (int i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i)
        {
            scene->AddEmpty();
        }

        auto grid = CreateGrid();
        scene->AddModel(grid);

        if (argc > 1)
        {
            scene->LoadFromPath(argv[1]);
        }

        screenstate::ScreenState state;
        while (window.Update(&state))
        {
            vr.OnFrame(scene);
            renderer.OnFrame(hwnd, state);
        }
    }

    return 0;
}
