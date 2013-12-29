# libDTShape

## What is it?

libDTShape loads and animates 3d animated shapes. It's an offshoot of the code from the Torque3D engine, simplified and abstracted so it can more easily be incorporated into other projects.

libDTShape supports both classic "compiled" dts models, and also collada models.

## What amazing features does it support?

To summarize:

* Rigid animation
* Skinned animation
* Animation triggers
* Ground frames
* Multiple animation threads & blending
* Imposters
* Detail levels
* Collision meshes

## What platforms does it support?

* Windows (x86, amd64)
* OS X (x86, amd64)
* Linux (x86, amd64, arm)

While libDTShape is mainly optimized for x86/x64, it can support any generic platform provided the platform headers are setup correctly.

## How do I use it?

An example application is located in the example folder. Currently since the code assumes nothing about your rendering library, you'll need to create your own TSMeshRenderer, TSMeshInstanceRenderData, TSMaterial, TSMaterialInstance and TSMaterialManager classes. 

If you want to do more advanced things such as collision detection, note that while interfaces are exposed to enumerate collision primitives and surfaces, as of yet there is no example of making use of this data.

## Why shoud I use it instead of "solution x"?

Good question. If you want a fully featured solution which you can insert into your indie game in 5 seconds, libdtshape might not be for you. On the other hand if you want something reasonably simple and non-assuming which can be built upon, maybe you should check out libdtshape.


*NOTE*: As libdtshape is currently in its early stages of development, the API is subject to change. Feel free to suggest ideas for improvement on the Issue Tracker.

