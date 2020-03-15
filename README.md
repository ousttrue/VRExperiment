# MainMonitor
VR desktop experiment

## desktop_dupl(OpenVR dashboard overlay)

* [x] replace CopyResource to Quad Render 
* [x] mouse cursor
* [ ] reference tracker position

## MainMonitor(Unity OpenVR App)

TODO

## ExternalViewer(OpenVR tracker viewer)

* [x] windows position restore
* shader/material
    * [ ] light/lambert
    * [x] Grid
    * [ ] glTF pbr
    * [ ] alpha blending
    * [ ] reflection Constants Semantics
* imgui
    * [x] frame rate
    * [x] clear color
    * [x] scene tree
    * [ ] docking
    * [x] consume wheel, click event.
    * [x] select node
* gizmo
    * [x] VertexColor
    * [ ] Hover
    * [ ] scene hierarchy(has parent)
* glTF
    * [x] node
    * [ ] CPU skinning
    * [ ] GPU skinning
    * [ ] animation
* scene
    * [ ] light node
    * [ ] manage node id(d3d12 slot)
    * [ ] manage material id(d3d12 slot)

## hlsl memo

VSInput vs;
PSInput ps;
constants
    scene: sProjection
    node: nModel
    material: mDiffuse
