#include "Application.h"
#include "VR.h"
#include "Gui.h"
#include "Gizmo.h"
#include <OrbitCamera.h>
#include "Renderer.h"
#include <hierarchy.h>
#include <functional>

#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#define YAP_ENABLE
#include <YAP.h>

///
/// logger setup
///
namespace plog
{
class MyFormatter
{
public:
    static util::nstring header()
    {
        return util::nstring();
    }

    static util::nstring format(const Record &r)
    {
        tm t;
        util::localtime_s(&t, &r.getTime().time);

        util::nostringstream ss;
        ss
            << std::setw(2) << t.tm_hour << ':' << std::setw(2) << t.tm_min << '.' << std::setw(2) << t.tm_sec
            << '[' << severityToString(r.getSeverity()) << ']'
            << r.getFunc() << '(' << r.getLine() << ") "
            << r.getMessage()

            << "\n"; // Produce a simple string with a log message.

        return ss.str();
    }
};

template <class Formatter>          // Typically a formatter is passed as a template parameter.
class MyAppender : public IAppender // All appenders MUST inherit IAppender interface.
{
    using OnWrite = std::function<void(const char *)>;
    OnWrite m_onWrite;

public:
    void write(const Record &record) override // This is a method from IAppender that MUST be implemented.
    {
        util::nstring str = Formatter::format(record); // Use the formatter to get a string from a record.
        if (m_onWrite)
        {
            auto utf8 = UTF8Converter::convert(str);
            m_onWrite(utf8.c_str());
        }
    }

    void onWrite(const OnWrite &callback)
    {
        m_onWrite = callback;
    }
};

} // namespace plog

static std::shared_ptr<hierarchy::SceneMesh> CreateGrid()
{
    struct GridVertex
    {
        std::array<float, 2> position;
        std::array<float, 2> uv;
    };
    GridVertex vertices[] = {
        {{-1, 1}, {0, 0}},
        {{-1, -1}, {0, 1}},
        {{1, -1}, {1, 1}},
        {{1, 1}, {1, 0}},
    };
    uint16_t indices[] = {
        0, 1, 2, //
        2, 3, 0, //
    };
    auto mesh = hierarchy::SceneMesh::Create();
    mesh->vertices = hierarchy::VertexBuffer::CreateStatic(
        hierarchy::Semantics::Vertex,
        sizeof(vertices[0]), vertices, sizeof(vertices));
    mesh->indices = hierarchy::VertexBuffer::CreateStatic(
        hierarchy::Semantics::Index,
        2, indices, sizeof(indices));
    {
        auto material = hierarchy::SceneMaterial::Create();
        material->shader = hierarchy::ShaderManager::Instance().get("grid");
        mesh->submeshes.push_back({.draw_count = _countof(indices),
                                   .material = material});
    }
    return mesh;
}

class View
{
    OrbitCamera m_camera;
    Gizmo m_gizmo;
    hierarchy::SceneNodePtr m_selected;

public:
    float clearColor[4] = {
        0.2f,
        0.2f,
        0.3f,
        1.0f};

    View()
    {
        m_camera.zNear = 0.01f;
    }

    const OrbitCamera *Camera() const
    {
        return &m_camera;
    }

    int GizmoNodeID() const
    {
        return m_gizmo.GetNodeID();
    }

    hierarchy::SceneMeshPtr GizmoMesh() const
    {
        return m_gizmo.GetMesh();
    }

    gizmesh::GizmoSystem::Buffer GizmoBuffer()
    {
        return m_gizmo.End();
    }

    void Update3DView(const screenstate::ScreenState &viewState, size_t texture, const hierarchy::SceneNodePtr &selected)
    {
        //
        // update camera
        //
        if (selected != m_selected)
        {
            if (selected)
            {
                m_camera.gaze = -selected->World().translation;
            }
            else
            {
                // m_camera->gaze = {0, 0, 0};
            }

            m_selected = selected;
        }
        m_camera.Update(viewState);

        //
        // update gizmo
        //
        m_gizmo.Begin(viewState, m_camera.state);
        if (selected)
        {
            // if (selected->EnableGizmo())
            {
                auto parent = selected->Parent();
                m_gizmo.Transform(selected->ID(),
                                  selected->Local(),
                                  parent ? parent->World() : falg::Transform{});
            }
        }
    }
};

class ApplicationImpl
{
    plog::ColorConsoleAppender<plog::MyFormatter> m_consoleAppender;
    plog::MyAppender<plog::MyFormatter> m_imGuiAppender;

    VR m_vr;
    hierarchy::Scene m_scene;

    Renderer m_renderer;

    Gui m_imgui;
    View m_view;

    bool m_initialized = false;

public:
    ApplicationImpl(int argc, char **argv)
        : m_renderer(256)
    {
        m_imGuiAppender.onWrite(std::bind(&Gui::Log, &m_imgui, std::placeholders::_1));
        plog::init(plog::debug, &m_consoleAppender).addAppender(&m_imGuiAppender);

        auto path = std::filesystem::current_path();
        if (argc > 1)
        {
            path = argv[1];
        }
        hierarchy::ShaderManager::Instance().watch(path);

        if (argc > 2)
        {
            auto node = hierarchy::SceneGltf::LoadFromPath(argv[2]);
            if (node)
            {
                m_scene.AddRootNode(node);
                LOGI << "load: " << argv[2];
            }
            else
            {
                LOGW << "fail to load: " << argv[2];
            }
        }

        auto node = hierarchy::SceneNode::Create("grid");
        node->Mesh(CreateGrid());
        m_scene.AddRootNode(node);

        if (m_vr.Connect())
        {
            LOGI << "vr.Connect";
        }
        else
        {
            LOGW << "fail to vr.Connect";
        }
    }

    void OnFrame(void *hwnd, const screenstate::ScreenState &state)
    {
        if (!m_initialized)
        {
            m_renderer.Initialize(hwnd);
            m_initialized = true;
        }

        {
            YAP::ScopedSection(VR);
            m_vr.OnFrame(&m_scene);
        }

        // imgui
        m_imgui.BeginFrame(state);
        m_imgui.Update(&m_scene, m_view.clearColor);

        // view
        auto viewTextureID = m_renderer.ViewTextureID();
        screenstate::ScreenState viewState;
        bool isShowView = m_imgui.View(state, viewTextureID, &viewState);
        if (isShowView)
        {
            m_view.Update3DView(viewState, viewTextureID, m_imgui.Selected());
        }

        // d3d
        if (isShowView)
        {
            auto buffer = m_view.GizmoBuffer();
            m_renderer.UpdateViewResource(viewState, m_view.Camera(), m_view.GizmoMesh(), buffer);
        }
        YAP::ScopedSection(Render);
        m_renderer.OnFrame(hwnd, state, &m_scene,
                           m_view.GizmoNodeID(), m_view.GizmoMesh());
    }
};

Application::Application(int argc, char **argv)
    : m_impl(new ApplicationImpl(argc, argv))
{
}

Application::~Application()
{
    delete m_impl;
}

void Application::OnFrame(void *hwnd, const screenstate::ScreenState &state)
{
    m_impl->OnFrame(hwnd, state);
}
