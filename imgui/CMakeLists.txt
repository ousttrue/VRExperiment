set(TARGET_NAME imgui)
set(IMGUI_DIR ${EXTERNAL_DIR}/imgui)
add_library(${TARGET_NAME}
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    # ${IMGUI_DIR}/examples/imgui_impl_win32.cpp
    # ${IMGUI_DIR}/examples/imgui_impl_dx12.cpp
    )
target_include_directories(${TARGET_NAME} PUBLIC
    ${IMGUI_DIR}
    )
