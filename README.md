# Imposter 插件 说明文档

v1.0 | for Unreal Engine 4+
malosgao(高锴庚)
2021/11/8

---

[TOC]



## 插件简介

![image-20211105165558852](README.assets\image-20211105165558852.png)

“Imposter” 可以理解为场景中一个原始模型的“替身”。不同于传统的 LOD 和基于合并多边形的 HLOD，该插件所使用的 Imposter 基于光场渲染，在运行时只通过一个朝向摄像机的平面（Billboard），可以显示出与原始模型别无二致的替身物体，并且效果优于低面数的 LOD 模型。

Imposter 技术可以用来生成一个原始模型的 LOD 模型，也可以生成代表多个原始物体的 HLOD 模型。插件提供一套工作流，美术可以指定场景中的某些或全部物体，插件既可以一键生成出选中物体的 Imposter，其大小和位置与原始物体完全相同。Imposter 对象将会被保存到硬盘上（蓝图、纹理、材质实例、关卡），插件同时会创建 LOD 子关卡，供 World Composition 等支持 LOD 关卡的开放世界功能模块所使用。



## 插件特性

* **插件由 https://github.com/ictusbrucks/ImpostorBaker 改良而来。**渲染算法遵循原始插件，使用 C++ 重构原始插件，重写材质，并对渲染质量、性能和工作流加以优化。
* 相比基于 Mesh 减面的 LOD（用于HLOD），Imposter 能提供更好的效果和性能。同时由于纹理可以流式加载而Mesh不能，Imposter 比传统方法更省内存，适合用在移动平台和开放世界类型游戏中。
* 插件能够在不侵入场景的情况下，捕获并生成 Imposter 物体到新的子关卡中，位置和大小均与原物体一致，并将资产文件存入硬盘。该子关卡能够直接被用作 LOD 子关卡，供 World Composition 系统使用。
* 插件提供完善的工作流支持，可以一键识别并生成整个景观关卡的 Imposter 物体并保存，简化美术操作繁琐程度。
* 插件能够捕获并渲染常用的材质属性，包括 BaseColor, Depth, Normal, Opacity, Roughness, Metallic, Specular, AO，并且可以被 Imposter 还原。
* Imposter 同时能正常渲染深度，能够和 Mesh 或 Imposter 产生正常的遮挡关系。
* 支持延迟渲染和前向渲染，兼容移动平台渲染器（ES3）。
* **完善的代码注释（英文）。**



## 使用流程

2. 导入插件。
3. 在插件目录的 `Blueprints` 下，找到 `BP_ImposterManagerActor` 并拖入到待生成的场景中。
4. 参照“插件参数”一章配置好参数。刚刚拖入场景中的蓝图已经包含一套默认的捕获参数，可以直接拿来使用。
4. 确保当前虚幻引擎工作在延迟渲染的渲染路径中（仅需要在捕获时使用延迟渲染）。
5. 点击 *A. Add Scene Actors* 按钮，将当前场景中满足条件的 Static Mesh Actor 添加到列表中，或找到 `Captured Actors ` 参数手动指定需要捕获的 Actors。
6. 点击 *B Init Core*，随后点击 *C Capture* 并等待（很短的）一段时间，此时插件已经将数据捕获到内存中。
7. 点击 *D Generate* ，此时插件将把 Imposter 蓝图类、相关纹理、材质蓝图和子关卡写入到硬盘中，并且将 Imposter Actor 以及其包含的子关卡添加进场景结构中。
8. 同时，也可以通过点击 *E Preview* 创建一份预览的 Imposter Actor 到场景中，供临时查看效果所使用。此时插件不会向硬盘写入任何信息，预览使用的 Imposter Actor 也不会被保存到地图中。



## 插件参数

### 捕获部分

为了保证低耦合性，因此捕获部分将不会硬编码任何和渲染和捕获材质有关的参数。当参数完全配置完毕后，该蓝图类实例即和材质产生耦合，可以被一起使用。捕获部分所对应的参数均来自于 `BP_ImposterManagerActor` 蓝图类中。

#### 通用配置

下列参数均被包含在结构体 `FImposterGeneratorSettings` 中。后期版本将会允许序列化该类型到硬盘中，作为固定的配置信息资产，并可直接设置在插件面板中。

![image-20211108163609671](README.assets\image-20211108163609671.png)

* **Imposter Type**：Imposter 的种类，其中包含：

  * Upper Hemi：上半球 Imposter。由于多数建筑、树木等物件将会被摆放在地上，因此为了节省纹理，地下的半球可以不捕获；
  * Full：整个球面捕获的 Imposter。

* **Capture Material Map**：欲生成的纹理类型，为一个 Map 数据结构。字段可以选择为：

  ```c++
  // These types can be captured by setting capture component's capture source, and it might be faster since GBuffer will not be composed.
  // post-processing material will not be used (ignored if assigned) here.
  SceneColor,
  // SCS_SceneColorHDR
  BaseColor,
  // SCS_BaseColor
  WorldNormal,
  // SCS_WorldNormal
  SceneDepth,
  // SCS_SceneDepth
  
  // These types should be used with a post-processing material to extract a particular RT in GBuffer. SCS_FinalColorLDR will be used to enable custom post-processing material.
  // There is no way to reuse GBuffer without tinkering engine code, so for each type, whole scene will be rendered entirely. Use aggregate capture type if you can.
  Depth01,
  Specular,
  Metallic,
  Opacity,
  Roughness,
  AO,
  Emissive,
  
  // Useful aggregate types below. Use these to avoid custom channel packing and speed will be faster. These type will always use a custom post-processing material.
  BaseColorAlpha,
  // RGB: Base Color, A: Opacity
  NormalDepth,
  // RGB: Normal, A: Linear Depth
  MetallicRoughnessSpecularAO,
  // R: Metallic, G: Roughness, B: Specular, A: AO
  
  // Custom type.
  // Actually, all types above can be considered as a named custom type since logics are the same.
  Custom1,
  Custom2,
  Custom3,
  Custom4,
  Custom5,
  Custom6,
  ```

  对于字段为 `SceneColor`，`BaseColor`，`WorldNormal`，`SceneDepth` 参数来说，其可以直接使用 SceneCaptureComponent 得到的结果，因此不需要指定这些字段所对应的值，留空即可；对于剩下的字段来说，需要指定一个材质，否则捕获将会被跳过。

  当前插件预制了对于常用的 `BaseColorAlpha`，`NormalDepth`，`MetallicRoughnessSpecularAO` 的捕获材质。其为一个后处理材质，将待捕获的通道从 GBuffer 中取出并输出到对应分量中。用户也可以手动按照上述思路制作其余字段的捕获材质来拓展插件功能。

  ![image-20211108170514549](README.assets\image-20211108170514549.png)

* **Additional Capture Blit**：（可选）捕获的后处理。一些捕获后的纹理可能需要附加的后处理 Pass 才能呈现最好的效果。即捕获流程为 `Capture Pass -> Additional Blit Pass #1 -> Additional Blit Pass #2...`

  * Capture Type 对应着上面所列举出来的捕获类型。这些类型也应该同时存在于上述 Capture Material Map 中来构成完整的捕获流程，不存在的类型将会被忽略（当然也可以选择都放在这里，用来支持所有 Capture Material Map 可能出现的捕获类型）；

  * Blit Material：后处理材质。同样，此处内置了两个常用的后处理材质：

    * `DilationRGB+DFAlpha`：对 RGB 通道做 UV 膨胀，对 Alpha 通道生成 Distance Field。
    * `DilationRGBA`：对 RGBA 通道均做 UV 膨胀。

    UV 膨胀可以用于优化 Mipmap 的生成，对 Opacity Mask 生成 Distance Field 有助于让不同角度更好地进行混合。

* **Imposter Material**：真正用来渲染 Imposter 的材质。由于捕获部分与材质解耦，因此此处选用的渲染材质类型必须和其余参数搭配使用。

  内置的材质类型均为 `M_Impostor_SimpleOffset` 的材质实例，可以通过调整开关参数实现：

  * `SimpleOffset_Full`：整个球面的 Imposter。
  * `SimpleOffset_Hemi`：半球 Imposter。
  * `SimpleOffset_Hemi_MRSA`：半球 Imposter。支持输出 PBR 属性（Metallic, Roughness, Specular, AO）。
  * `SimpleOffset_Hemi_MRSA_NoPDO`：半球 Imposter。支持输出 PBR 属性。不输出 PDO。
  * `SimpleOffset_Hemi_NoPDO`：半球 Imposter。不输出 PDO。
  * `SimpleOffset_Hemi_NoPDO_NoParallax`：半球 Imposter。不输出 PDO。不进行二次深度矫正。

  具体开关参数含义可以参考下面“内置 Imposter 渲染材质”一章。

* **Imposter Material Grid Size Param**：渲染 Imposter 的材质中，决定 Octahedron 栅格大小的参数名称。

  对于上述内置材质 `M_Impostor_SimpleOffset` 来说，该名称为 `Num Frames`。

* **Imposter Material Size Param**：渲染 Imposter 的材质中，决定物体大小，即物体球体包围盒半径（`CaptureBounds.SphereRadius`）的参数名称。

  对于上述内置材质 `M_Impostor_SimpleOffset` 来说，该名称为 `Default Mesh Size`。

![image-20211108180710560](README.assets\image-20211108180710560.png)

* **Imposter Material Texture Binding Map**：渲染 Imposter 的材质中纹理参数所对应的名称。
* **Unit Quad**：Imposter 所使用的 Billboard 几何体（Static Mesh）。可以参考内置 Mesh `SubdividedPlane` 。
* **Capture MPC**：捕获材质所使用的 Material Property Collection（MPC）引用。插件通过该 MPC 将捕获参数传递给捕获材质。对于内置的捕获材质，内置的 `MPC_Capture` 已经够用。
* **Working Path**：插件工作目录。所有 Imposter 相关内容均会被生成于工作目录之内，便于管理。

#### 需要在每次生成时酌情调整的配置

下列参数在每次生成时可能均会更改，因此不纳入配置资产文件：

![image-20211108180744755](README.assets\image-20211108180744755.png)

* **Imposter Name**：此次生成的 Imposter 名称。需要不和之前的命名重复。

  每个 Imposter 在保存资产时均会保存在工作目录内，以该名称命名的文件夹下。

* **Captured Actors**：此次需要捕获的 Actor 列表。该列表可以手动指定，也可以通过 *A Add Scene Actors* 按钮自动添加。

#### 功能按钮

![image-20211108181820233](README.assets\image-20211108181820233.png)

其中 Actions 分类中的五个按钮在使用流程一章均有提及，此处不再赘述。

* ***Debug Capture Bounds***：在编辑器视口中绘制捕获物体对应的 AABB 和球包围盒。需要在执行到 *B Init Core* 之后使用。
* ***Debug Capture Directions***：在编辑器视口中绘制全部捕获方向的 Debug 信息。需要在执行到 *B Init Core* 之后使用。

### 内置 Imposter 渲染材质

材质位于插件目录下 `Material/Imposter/M_Impostor_SimpleOffset.uasset`

对于其材质实例来说，有下列可调节参数（以 `M_Impostor_SimpleOffset_Hemi_MRSA_Inst` 为例）：

![image-20211108183130883](README.assets\image-20211108183130883.png)

* **Full Sphere**：是否渲染为整个球面的 Imposter。该选项需要和上述捕获部分的配置保持一致。
* **Use MSRA**：是否采样并输出 PBR 属性（Metallic, Roughness, Specular, AO）。关闭可以减少一张纹理使用，但效果会变差。
* **Use Parallax**：是否开启额外的深度矫正。关闭深度矫正用来获得更高的性能，但效果会变差。
* **Use PDO**：是否输出 Pixel Depth Offset，用来写入正确的物体深度以正确渲染遮挡关系。部分机型可能不兼容 PDO。关闭 PDO 用来获得更高的性能。
* **Atlas Mip Bias**：采样纹理图集时的 Mip 偏移（作用于 `TexCoord0`）。调高此值来使用更高 Mipmap 渲染，可能会节省纹理内存。
* **Default Mesh Size**：物体大小，即物体球体包围盒半径（`CaptureBounds.SphereRadius`）。该值由插件自动设置。
* **Mip Level For Parallax**：深度矫正时使用的 Mipmaps 层级。调高层级可能会获得更高的性能。
* **Num Frames**：Octahedron 栅格大小。该值由插件自动设置。
* **Opacity Mask Offset**：输出 Opacity Mask 时的偏移值。材质使用 0.5 来截断 Opacity Mask。使用负值可以缩小物体轮廓，可能能够避免一些特定的渲染异常现象。
* **BaseColorAlpha，MetallicRoughnessSpecularAO，NormalDepth**：和捕获部分所对应，为材质所需要的三张纹理。如果 Use MSRA 未开启，MetallicRoughnessSpecularAO 将不可用，即所需纹理降至两张。



## 目录结构

### ImposterGenerator Content

* Blueprints
  * *BP_ImposterManagerActor*：当前的插件交互面板。拖入场景中，Details 面板即为用户的交互面板。
* Material
  * Capture
    * Material Functions：目前均为 Additional Capture Blit 所使用的材质方法。
    * *M_Blit* 系列：上述 Additional Capture Blit 所使用的材质。
    * *M_Capture* 系列：上述 Capture Material Map 所使用的材质。
  * Imposter
    * *M_Impostor_SimpleOffset*：预设的 Imposter 渲染材质。
    * MaterialFunctions：预设的 Imposter 渲染材质所使用到的材质方法。
    * MaterialInstances：几种常用的 Imposter 渲染材质实例。
* Mesh
  * *SubdividedPlane*：当前所使用的 Billboard 面片。细分一次用来保证计算得到的顶点信息插值正确。
* MPC
  * *MPC_Capture*：当前插件默认的，用来给捕获材质传递信息所使用的 Material Property Collection 资源。

### ImposterGenerator C++ Classes

* `ImposterActor`：承载 Imposter 的 Actor，带有一个 `StaticMeshComponent`，将会在游戏中使用。其包含少量代码用来设置其自身属性，例如是否投射阴影（默认为否，因为 Imposter 当前不支持阴影）。
* `ImposterCaptureActor`：用来在场景中捕获物体的 Actor，带有一个 `SceneCaptureComponent2D` 用来捕获场景到 Render Target。被 `ImposterCore` 管理，承载 `ImposterCore` 与场景交互的逻辑，会在多次捕获之间复用。不会被保存到地图中。
* `ImposterCore`：Imposter 核心模块，为一个 `UObject`。
* `ImposterManagerActor`：用户和插件的交互 Actor，通过操作该 Actor 属性栏的参数和按钮来完成全部工作。该 Actor 会在工作时初始化并使用 `ImposterCore` 实例。后续有可能会使用 Slate 制作插件的交互面板，此时该 Actor 可能会被废弃。

