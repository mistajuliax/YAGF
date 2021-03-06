// Copyright (C) 2002-2012 Nikolaus Gebhardt
// Copyright (C) 2015 Vincent Lejeune
// Contains code from the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __I_SKINNED_MESH_H_INCLUDED__
#define __I_SKINNED_MESH_H_INCLUDED__

//#include <SceneNodes/IBoneSceneNode.h>
//#include <Core/IAnimatedMesh.h>
//#include "SSkinMeshBuffer.h"
#include <Core/BasicVertexLayout.h>
#include <Core/IMeshBuffer.h>
#include <Maths/quaternion.h>
#include <Maths/vector3d.h>
#include <Maths/matrix4.h>
#include <string>
#include <vector>
#include <algorithm>

namespace irr
{
  namespace video
  {
    //! Abstracted and easy to use fixed function/programmable pipeline material modes.
    enum E_MATERIAL_TYPE
    {
      //! Standard solid material.
      /** Only first texture is used, which is supposed to be the
      diffuse material. */
      EMT_SOLID = 0,

      //! Solid material with 2 texture layers.
      /** The second is blended onto the first using the alpha value
      of the vertex colors. This material is currently not implemented in OpenGL.
      */
      EMT_SOLID_2_LAYER,

      //! Material type with standard lightmap technique
      /** There should be 2 textures: The first texture layer is a
      diffuse map, the second is a light map. Dynamic light is
      ignored. */
      EMT_LIGHTMAP,

      //! Material type with lightmap technique like EMT_LIGHTMAP.
      /** But lightmap and diffuse texture are added instead of modulated. */
      EMT_LIGHTMAP_ADD,

      //! Material type with standard lightmap technique
      /** There should be 2 textures: The first texture layer is a
      diffuse map, the second is a light map. Dynamic light is
      ignored. The texture colors are effectively multiplied by 2
      for brightening. Like known in DirectX as D3DTOP_MODULATE2X. */
      EMT_LIGHTMAP_M2,

      //! Material type with standard lightmap technique
      /** There should be 2 textures: The first texture layer is a
      diffuse map, the second is a light map. Dynamic light is
      ignored. The texture colors are effectively multiplyied by 4
      for brightening. Like known in DirectX as D3DTOP_MODULATE4X. */
      EMT_LIGHTMAP_M4,

      //! Like EMT_LIGHTMAP, but also supports dynamic lighting.
      EMT_LIGHTMAP_LIGHTING,

      //! Like EMT_LIGHTMAP_M2, but also supports dynamic lighting.
      EMT_LIGHTMAP_LIGHTING_M2,

      //! Like EMT_LIGHTMAP_4, but also supports dynamic lighting.
      EMT_LIGHTMAP_LIGHTING_M4,

      //! Detail mapped material.
      /** The first texture is diffuse color map, the second is added
      to this and usually displayed with a bigger scale value so that
      it adds more detail. The detail map is added to the diffuse map
      using ADD_SIGNED, so that it is possible to add and substract
      color from the diffuse map. For example a value of
      (127,127,127) will not change the appearance of the diffuse map
      at all. Often used for terrain rendering. */
      EMT_DETAIL_MAP,

      //! Look like a reflection of the environment around it.
      /** To make this possible, a texture called 'sphere map' is
      used, which must be set as the first texture. */
      EMT_SPHERE_MAP,

      //! A reflecting material with an optional non reflecting texture layer.
      /** The reflection map should be set as first texture. */
      EMT_REFLECTION_2_LAYER,

      //! A transparent material.
      /** Only the first texture is used. The new color is calculated
      by simply adding the source color and the dest color. This
      means if for example a billboard using a texture with black
      background and a red circle on it is drawn with this material,
      the result is that only the red circle will be drawn a little
      bit transparent, and everything which was black is 100%
      transparent and not visible. This material type is useful for
      particle effects. */
      EMT_TRANSPARENT_ADD_COLOR,

      //! Makes the material transparent based on the texture alpha channel.
      /** The final color is blended together from the destination
      color and the texture color, using the alpha channel value as
      blend factor. Only first texture is used. If you are using
      this material with small textures, it is a good idea to load
      the texture in 32 bit mode
      (video::IVideoDriver::setTextureCreationFlag()). Also, an alpha
      ref is used, which can be manipulated using
      SMaterial::MaterialTypeParam. This value controls how sharp the
      edges become when going from a transparent to a solid spot on
      the texture. */
      EMT_TRANSPARENT_ALPHA_CHANNEL,

      //! Makes the material transparent based on the texture alpha channel.
      /** If the alpha channel value is greater than 127, a
      pixel is written to the target, otherwise not. This
      material does not use alpha blending and is a lot faster
      than EMT_TRANSPARENT_ALPHA_CHANNEL. It is ideal for drawing
      stuff like leafes of plants, because the borders are not
      blurry but sharp. Only first texture is used. If you are
      using this material with small textures and 3d object, it
      is a good idea to load the texture in 32 bit mode
      (video::IVideoDriver::setTextureCreationFlag()). */
      EMT_TRANSPARENT_ALPHA_CHANNEL_REF,

      //! Makes the material transparent based on the vertex alpha value.
      EMT_TRANSPARENT_VERTEX_ALPHA,

      //! A transparent reflecting material with an optional additional non reflecting texture layer.
      /** The reflection map should be set as first texture. The
      transparency depends on the alpha value in the vertex colors. A
      texture which will not reflect can be set as second texture.
      Please note that this material type is currently not 100%
      implemented in OpenGL. */
      EMT_TRANSPARENT_REFLECTION_2_LAYER,

      //! A solid normal map renderer.
      /** First texture is the color map, the second should be the
      normal map. Note that you should use this material only when
      drawing geometry consisting of vertices of type
      S3DVertexTangents (EVT_TANGENTS). You can convert any mesh into
      this format using IMeshManipulator::createMeshWithTangents()
      (See SpecialFX2 Tutorial). This shader runs on vertex shader
      1.1 and pixel shader 1.1 capable hardware and falls back to a
      fixed function lighted material if this hardware is not
      available. Only two lights are supported by this shader, if
      there are more, the nearest two are chosen. */
      EMT_NORMAL_MAP_SOLID,

      //! A transparent normal map renderer.
      /** First texture is the color map, the second should be the
      normal map. Note that you should use this material only when
      drawing geometry consisting of vertices of type
      S3DVertexTangents (EVT_TANGENTS). You can convert any mesh into
      this format using IMeshManipulator::createMeshWithTangents()
      (See SpecialFX2 Tutorial). This shader runs on vertex shader
      1.1 and pixel shader 1.1 capable hardware and falls back to a
      fixed function lighted material if this hardware is not
      available. Only two lights are supported by this shader, if
      there are more, the nearest two are chosen. */
      EMT_NORMAL_MAP_TRANSPARENT_ADD_COLOR,

      //! A transparent (based on the vertex alpha value) normal map renderer.
      /** First texture is the color map, the second should be the
      normal map. Note that you should use this material only when
      drawing geometry consisting of vertices of type
      S3DVertexTangents (EVT_TANGENTS). You can convert any mesh into
      this format using IMeshManipulator::createMeshWithTangents()
      (See SpecialFX2 Tutorial). This shader runs on vertex shader
      1.1 and pixel shader 1.1 capable hardware and falls back to a
      fixed function lighted material if this hardware is not
      available.  Only two lights are supported by this shader, if
      there are more, the nearest two are chosen. */
      EMT_NORMAL_MAP_TRANSPARENT_VERTEX_ALPHA,

      //! Just like EMT_NORMAL_MAP_SOLID, but uses parallax mapping.
      /** Looks a lot more realistic. This only works when the
      hardware supports at least vertex shader 1.1 and pixel shader
      1.4. First texture is the color map, the second should be the
      normal map. The normal map texture should contain the height
      value in the alpha component. The
      IVideoDriver::makeNormalMapTexture() method writes this value
      automatically when creating normal maps from a heightmap when
      using a 32 bit texture. The height scale of the material
      (affecting the bumpiness) is being controlled by the
      SMaterial::MaterialTypeParam member. If set to zero, the
      default value (0.02f) will be applied. Otherwise the value set
      in SMaterial::MaterialTypeParam is taken. This value depends on
      with which scale the texture is mapped on the material. Too
      high or low values of MaterialTypeParam can result in strange
      artifacts. */
      EMT_PARALLAX_MAP_SOLID,

      //! A material like EMT_PARALLAX_MAP_SOLID, but transparent.
      /** Using EMT_TRANSPARENT_ADD_COLOR as base material. */
      EMT_PARALLAX_MAP_TRANSPARENT_ADD_COLOR,

      //! A material like EMT_PARALLAX_MAP_SOLID, but transparent.
      /** Using EMT_TRANSPARENT_VERTEX_ALPHA as base material. */
      EMT_PARALLAX_MAP_TRANSPARENT_VERTEX_ALPHA,

      //! BlendFunc = source * sourceFactor + dest * destFactor ( E_BLEND_FUNC )
      /** Using only first texture. Generic blending method. */
      EMT_ONETEXTURE_BLEND,

      //! This value is not used. It only forces this enumeration to compile to 32 bit.
      EMT_FORCE_32BIT = 0x7fffffff
    };

    //! These flags allow to define the interpretation of vertex color when lighting is enabled
    /** Without lighting being enabled the vertex color is the only value defining the fragment color.
    Once lighting is enabled, the four values for diffuse, ambient, emissive, and specular take over.
    With these flags it is possible to define which lighting factor shall be defined by the vertex color
    instead of the lighting factor which is the same for all faces of that material.
    The default is to use vertex color for the diffuse value, another pretty common value is to use
    vertex color for both diffuse and ambient factor. */
    enum E_COLOR_MATERIAL
    {
      //! Don't use vertex color for lighting
      ECM_NONE = 0,
      //! Use vertex color for diffuse light, this is default
      ECM_DIFFUSE,
      //! Use vertex color for ambient light
      ECM_AMBIENT,
      //! Use vertex color for emissive light
      ECM_EMISSIVE,
      //! Use vertex color for specular light
      ECM_SPECULAR,
      //! Use vertex color for both diffuse and ambient light
      ECM_DIFFUSE_AND_AMBIENT
    };

    struct SMaterial
    {
      E_MATERIAL_TYPE MaterialType;
      bool ZWriteEnable;
      bool Lighting;
      SColor DiffuseColor;
      SColor AmbientColor;
      E_COLOR_MATERIAL ColorMaterial;
      bool GouraudShading;
      bool BackfaceCulling;
      float Shininess;
      std::string TextureNames[16];
    };
  }

  namespace scene
  {

    enum E_INTERPOLATION_MODE
    {
      // constant does use the current key-values without interpolation
      EIM_CONSTANT = 0,

      // linear interpolation
      EIM_LINEAR,

      //! count of all available interpolation modes
      EIM_COUNT
    };


    //! Interface for using some special functions of Skinned meshes
    class ISkinnedMesh
    {
    public:

      //! A vertex weight
      struct SWeight
      {
        //! Index of the mesh buffer
        unsigned short buffer_id; //I doubt 32bits is needed

                                  //! Index of the vertex
        unsigned short vertex_id; //Store global ID here

                                  //! Weight Strength/Percentage (0-1)
        float strength;

      private:
        //! Internal members used by CSkinnedMesh
        friend class ISkinnedMesh;
        bool *Moved;
        core::vector3df StaticPos;
        core::vector3df StaticNormal;
      };


      //! Animation keyframe which describes a new position
      struct SPositionKey
      {
        float frame;
        core::vector3df position;
      };

      //! Animation keyframe which describes a new scale
      struct SScaleKey
      {
        float frame;
        core::vector3df scale;
      };

      //! Animation keyframe which describes a new rotation
      struct SRotationKey
      {
        float frame;
        core::quaternion rotation;
      };

      //! Joints
      struct SJoint
      {
        SJoint() : UseAnimationFrom(0), GlobalSkinningSpace(false),
          positionHint(-1), scaleHint(-1), rotationHint(-1)
        {
        }

        //! The name of this joint
        std::string Name;

        //! Local matrix of this joint
        core::matrix4 LocalMatrix;

        //! List of child joints
        std::vector<SJoint*> Children;

        //! List of attached meshes
        std::vector<unsigned> AttachedMeshes;

        //! Animation keys causing translation change
        std::vector<SPositionKey> PositionKeys;

        //! Animation keys causing scale change
        std::vector<SScaleKey> ScaleKeys;

        //! Animation keys causing rotation change
        std::vector<SRotationKey> RotationKeys;

        //! Skin weights
        std::vector<SWeight> Weights;

        //! Unnecessary for loaders, will be overwritten on finalize
        core::matrix4 GlobalMatrix;
        core::matrix4 GlobalAnimatedMatrix;
        core::matrix4 LocalAnimatedMatrix;
        core::vector3df Animatedposition;
        core::vector3df Animatedscale;
        core::quaternion Animatedrotation;

        core::matrix4 GlobalInversedMatrix; //the x format pre-calculates this

      private:
        friend class ISkinnedMesh;
        SJoint *UseAnimationFrom;
        bool GlobalSkinningSpace;

        int positionHint;
        int scaleHint;
        int rotationHint;
      };

      struct WeightInfluence
      {
        int Index;
        float Weight;
      };
    private:
      std::vector<std::pair<SMeshBufferLightMap, video::SMaterial> > LocalBuffers;
      std::vector<SJoint *> AllJoints;
      std::vector<SJoint*> RootJoints;

      std::vector<std::pair<SMeshBufferLightMap, video::SMaterial> > *SkinningBuffers;
      std::vector<std::vector<std::vector<std::pair<size_t, float> > > > Weights;

      float FramesPerSecond;
      float AnimationFrames;
      bool HasAnimation;
      bool PreparedForSkinning;
      bool AnimateNormals;
      bool HardwareSkinning;
      float LastAnimatedFrame;
      bool SkinnedLastFrame;

      void getFrameData(float frame, SJoint *joint,
        core::vector3df &position, int &positionHint,
        core::vector3df &scale, int &scaleHint,
        core::quaternion &rotation, int &rotationHint)
      {
        int foundPositionIndex = -1;
        int foundScaleIndex = -1;
        int foundRotationIndex = -1;

        if (joint->UseAnimationFrom)
        {
          const std::vector<SPositionKey> &PositionKeys = joint->UseAnimationFrom->PositionKeys;
          const std::vector<SScaleKey> &ScaleKeys = joint->UseAnimationFrom->ScaleKeys;
          const std::vector<SRotationKey> &RotationKeys = joint->UseAnimationFrom->RotationKeys;

          if (PositionKeys.size())
          {
            foundPositionIndex = -1;

            //Test the Hints...
            if (positionHint >= 0 && (unsigned)positionHint < PositionKeys.size())
            {
              //check this hint
              if (positionHint > 0 && PositionKeys[positionHint].frame >= frame && PositionKeys[positionHint - 1].frame < frame)
                foundPositionIndex = positionHint;
              else if (positionHint + 1 < (int)PositionKeys.size())
              {
                //check the next index
                if (PositionKeys[positionHint + 1].frame >= frame &&
                  PositionKeys[positionHint + 0].frame < frame)
                {
                  positionHint++;
                  foundPositionIndex = positionHint;
                }
              }
            }

            //The hint test failed, do a full scan...
            if (foundPositionIndex == -1)
            {
              for (unsigned i = 0; i < PositionKeys.size(); ++i)
              {
                if (PositionKeys[i].frame >= frame) //Keys should to be sorted by frame
                {
                  foundPositionIndex = i;
                  positionHint = i;
                  break;
                }
              }
            }

            //Do interpolation...
            if (foundPositionIndex != -1)
            {
              if (//InterpolationMode == EIM_CONSTANT ||
                false || foundPositionIndex == 0)
              {
                position = PositionKeys[foundPositionIndex].position;
              }
              else if (true) //(InterpolationMode == EIM_LINEAR)
              {
                const SPositionKey& KeyA = PositionKeys[foundPositionIndex];
                const SPositionKey& KeyB = PositionKeys[foundPositionIndex - 1];

                const float fd1 = frame - KeyA.frame;
                const float fd2 = KeyB.frame - frame;
                position = ((KeyB.position - KeyA.position) / (fd1 + fd2))*fd1 + KeyA.position;
              }
            }
          }

          //------------------------------------------------------------

          if (ScaleKeys.size())
          {
            foundScaleIndex = -1;

            //Test the Hints...
            if (scaleHint >= 0 && (unsigned)scaleHint < ScaleKeys.size())
            {
              //check this hint
              if (scaleHint > 0 && ScaleKeys[scaleHint].frame >= frame && ScaleKeys[scaleHint - 1].frame < frame)
                foundScaleIndex = scaleHint;
              else if (scaleHint + 1 < (int)ScaleKeys.size())
              {
                //check the next index
                if (ScaleKeys[scaleHint + 1].frame >= frame &&
                  ScaleKeys[scaleHint + 0].frame < frame)
                {
                  scaleHint++;
                  foundScaleIndex = scaleHint;
                }
              }
            }

            //The hint test failed, do a full scan...
            if (foundScaleIndex == -1)
            {
              for (unsigned i = 0; i < ScaleKeys.size(); ++i)
              {
                if (ScaleKeys[i].frame >= frame) //Keys should to be sorted by frame
                {
                  foundScaleIndex = i;
                  scaleHint = i;
                  break;
                }
              }
            }

            //Do interpolation...
            if (foundScaleIndex != -1)
            {
              if (//InterpolationMode == EIM_CONSTANT ||
                false || foundScaleIndex == 0)
              {
                scale = ScaleKeys[foundScaleIndex].scale;
              }
              else if (true)//(InterpolationMode == EIM_LINEAR)
              {
                const SScaleKey& KeyA = ScaleKeys[foundScaleIndex];
                const SScaleKey& KeyB = ScaleKeys[foundScaleIndex - 1];

                const float fd1 = frame - KeyA.frame;
                const float fd2 = KeyB.frame - frame;
                scale = ((KeyB.scale - KeyA.scale) / (fd1 + fd2))*fd1 + KeyA.scale;
              }
            }
          }

          //-------------------------------------------------------------

          if (RotationKeys.size())
          {
            foundRotationIndex = -1;

            //Test the Hints...
            if (rotationHint >= 0 && (unsigned)rotationHint < RotationKeys.size())
            {
              //check this hint
              if (rotationHint > 0 && RotationKeys[rotationHint].frame >= frame && RotationKeys[rotationHint - 1].frame < frame)
                foundRotationIndex = rotationHint;
              else if (rotationHint + 1 < (int)RotationKeys.size())
              {
                //check the next index
                if (RotationKeys[rotationHint + 1].frame >= frame &&
                  RotationKeys[rotationHint + 0].frame < frame)
                {
                  rotationHint++;
                  foundRotationIndex = rotationHint;
                }
              }
            }


            //The hint test failed, do a full scan...
            if (foundRotationIndex == -1)
            {
              for (unsigned i = 0; i < RotationKeys.size(); ++i)
              {
                if (RotationKeys[i].frame >= frame) //Keys should be sorted by frame
                {
                  foundRotationIndex = i;
                  rotationHint = i;
                  break;
                }
              }
            }

            //Do interpolation...
            if (foundRotationIndex != -1)
            {
              if (//InterpolationMode == EIM_CONSTANT ||
                false || foundRotationIndex == 0)
              {
                rotation = RotationKeys[foundRotationIndex].rotation;
              }
              else if (true)//(InterpolationMode == EIM_LINEAR)
              {
                const SRotationKey& KeyA = RotationKeys[foundRotationIndex];
                const SRotationKey& KeyB = RotationKeys[foundRotationIndex - 1];

                const float fd1 = frame - KeyA.frame;
                const float fd2 = KeyB.frame - frame;
                const float t = fd1 / (fd1 + fd2);

                /*
                f32 t = 0;
                if (KeyA.frame!=KeyB.frame)
                t = (frame-KeyA.frame) / (KeyB.frame - KeyA.frame);
                */

                rotation.slerp(KeyA.rotation, KeyB.rotation, t);
              }
            }
          }
        }
      }

      void normalizeWeights()
      {
        std::vector<std::vector<float> > verticesTotalWeight;

        verticesTotalWeight.reserve(LocalBuffers.size());
        for (std::pair<SMeshBufferLightMap, video::SMaterial> LocalBuffer : LocalBuffers)
        {
          verticesTotalWeight.push_back(std::vector<float>());
          verticesTotalWeight.back().resize(LocalBuffer.first.getVertexCount());
        }

        for (unsigned i = 0; i < verticesTotalWeight.size(); ++i)
          for (unsigned j = 0; j < verticesTotalWeight[i].size(); ++j)
            verticesTotalWeight[i][j] = 0;

        for (SJoint *joint : AllJoints)
        {
          for (SWeight weight : joint->Weights)
          {
            if (weight.strength <= 0)//Check for invalid weights
            {
              //joint->Weights.erase(j);
              //--j;
            }
            else
              verticesTotalWeight[weight.buffer_id][weight.vertex_id] += weight.strength;
          }
        }

        for (SJoint *joint : AllJoints)
        {
          for (SWeight weight : joint->Weights)
          {
            float total = verticesTotalWeight[weight.buffer_id][weight.vertex_id];
            if (total != 0 && total != 1)
              weight.strength /= total;
          }
        }
      }


      void checkForAnimation()
      {
        HasAnimation = false;
        for (SJoint *joint : AllJoints)
        {
          if (joint->UseAnimationFrom)
          {
            if (!joint->UseAnimationFrom->PositionKeys.empty() ||
              !joint->UseAnimationFrom->ScaleKeys.empty() ||
              !joint->UseAnimationFrom->RotationKeys.empty())
            {
              HasAnimation = true;
              break;
            }
          }
        }

        //meshes with weights, are still counted as animated for ragdolls, etc
        if (!HasAnimation)
        {
          for (SJoint *joint : AllJoints)
          {
            if (!joint->Weights.empty())
            {
              HasAnimation = true;
              break;
            }
          }
        }

        if (HasAnimation)
        {
          //--- Find the length of the animation ---
          AnimationFrames = 0;
          for (SJoint *joint : AllJoints)
          {
            if (joint->UseAnimationFrom)
            {
              if (!joint->UseAnimationFrom->PositionKeys.empty())
                if (joint->UseAnimationFrom->PositionKeys.back().frame > AnimationFrames)
                  AnimationFrames = joint->UseAnimationFrom->PositionKeys.back().frame;

              if (!joint->UseAnimationFrom->ScaleKeys.empty())
                if (joint->UseAnimationFrom->ScaleKeys.back().frame > AnimationFrames)
                  AnimationFrames = joint->UseAnimationFrom->ScaleKeys.back().frame;

              if (!joint->UseAnimationFrom->RotationKeys.empty())
                if (joint->UseAnimationFrom->RotationKeys.back().frame > AnimationFrames)
                  AnimationFrames = joint->UseAnimationFrom->RotationKeys.back().frame;
            }
          }
        }

        if (HasAnimation && !PreparedForSkinning)
        {
          PreparedForSkinning = true;

          //check for bugs:
          for (SJoint *joint : AllJoints)
          {
            for (SWeight weight : joint->Weights)
            {
              const unsigned short buffer_id = weight.buffer_id;
              const unsigned vertex_id = weight.vertex_id;

              //check for invalid ids
              if (buffer_id >= LocalBuffers.size())
              {
                printf("Skinned Mesh: Weight buffer id too large");
                weight.buffer_id = weight.vertex_id = 0;
              }
              else if (vertex_id >= LocalBuffers[buffer_id].first.getVertexCount())
              {
                printf("Skinned Mesh: Weight vertex id too large");
                weight.buffer_id = weight.vertex_id = 0;
              }
            }
          }

          // normalize weights
          normalizeWeights();
        }
        SkinnedLastFrame = false;
      }

      void calculateGlobalMatrices(SJoint *joint, SJoint *parentJoint)
      {
        if (!joint && parentJoint) // bit of protection from endless loops
          return;

        //Go through the root bones
        if (!joint)
        {
          for (SJoint *rootjoint : RootJoints)
            calculateGlobalMatrices(rootjoint, 0);
          return;
        }

        if (!parentJoint)
          joint->GlobalMatrix = joint->LocalMatrix;
        else
          joint->GlobalMatrix = parentJoint->GlobalMatrix * joint->LocalMatrix;

        joint->LocalAnimatedMatrix = joint->LocalMatrix;
        joint->GlobalAnimatedMatrix = joint->GlobalMatrix;

        if (joint->GlobalInversedMatrix.isIdentity())//might be pre calculated
        {
          joint->GlobalInversedMatrix = joint->GlobalMatrix;
          joint->GlobalInversedMatrix.makeInverse(); // slow
        }

        for (SJoint *child : joint->Children)
          calculateGlobalMatrices(child, joint);
        SkinnedLastFrame = false;
      }

      void buildAllLocalAnimatedMatrices()
      {
        for (SJoint *joint : AllJoints)
        {
          //Could be faster:
          if (joint->UseAnimationFrom &&
            (joint->UseAnimationFrom->PositionKeys.size() ||
            joint->UseAnimationFrom->ScaleKeys.size() ||
            joint->UseAnimationFrom->RotationKeys.size()))
          {
            joint->GlobalSkinningSpace = false;

            // IRR_TEST_BROKEN_QUATERNION_USE: TODO - switched to getMatrix_transposed instead of getMatrix for downward compatibility. 
            //								   Not tested so far if this was correct or wrong before quaternion fix!
            joint->Animatedrotation.getMatrix_transposed(joint->LocalAnimatedMatrix);

            // --- joint->LocalAnimatedMatrix *= joint->Animatedrotation.getMatrix() ---
            float *m1 = joint->LocalAnimatedMatrix.pointer();
            core::vector3df &Pos = joint->Animatedposition;
            m1[0] += Pos.X * m1[3];
            m1[1] += Pos.Y * m1[3];
            m1[2] += Pos.Z * m1[3];
            m1[4] += Pos.X * m1[7];
            m1[5] += Pos.Y * m1[7];
            m1[6] += Pos.Z * m1[7];
            m1[8] += Pos.X * m1[11];
            m1[9] += Pos.Y * m1[11];
            m1[10] += Pos.Z * m1[11];
            m1[12] += Pos.X * m1[15];
            m1[13] += Pos.Y * m1[15];
            m1[14] += Pos.Z * m1[15];
            // -----------------------------------

            if (joint->ScaleKeys.size())
            {
              /*
              core::matrix4 scaleMatrix;
              scaleMatrix.setScale(joint->Animatedscale);
              joint->LocalAnimatedMatrix *= scaleMatrix;
              */

              // -------- joint->LocalAnimatedMatrix *= scaleMatrix -----------------
              core::matrix4& mat = joint->LocalAnimatedMatrix;
              mat[0] *= joint->Animatedscale.X;
              mat[1] *= joint->Animatedscale.X;
              mat[2] *= joint->Animatedscale.X;
              mat[3] *= joint->Animatedscale.X;
              mat[4] *= joint->Animatedscale.Y;
              mat[5] *= joint->Animatedscale.Y;
              mat[6] *= joint->Animatedscale.Y;
              mat[7] *= joint->Animatedscale.Y;
              mat[8] *= joint->Animatedscale.Z;
              mat[9] *= joint->Animatedscale.Z;
              mat[10] *= joint->Animatedscale.Z;
              mat[11] *= joint->Animatedscale.Z;
              // -----------------------------------
            }
          }
          else
            joint->LocalAnimatedMatrix = joint->LocalMatrix;
        }
        SkinnedLastFrame = false;
      }

      void buildAllGlobalAnimatedMatrices(SJoint *joint = nullptr, SJoint *parentJoint = nullptr)
      {
        if (!joint)
        {
          for (SJoint *rootjoint : RootJoints)
            buildAllGlobalAnimatedMatrices(rootjoint, 0);
          return;
        }
        else
        {
          // Find global matrix...
          if (!parentJoint || joint->GlobalSkinningSpace)
            joint->GlobalAnimatedMatrix = joint->LocalAnimatedMatrix;
          else
            joint->GlobalAnimatedMatrix = parentJoint->GlobalAnimatedMatrix * joint->LocalAnimatedMatrix;
        }

        for (SJoint *childjoint : joint->Children)
          buildAllGlobalAnimatedMatrices(childjoint, joint);
      }


      void skinJoint(SJoint *joint, SJoint *parentJoint, float strength)
      {
        if (!joint->Weights.empty())
        {
          //Find this joints pull on vertices...
          core::matrix4 jointVertexPull(core::matrix4::EM4CONST_NOTHING);
          jointVertexPull.setbyproduct(joint->GlobalAnimatedMatrix, joint->GlobalInversedMatrix);

          JointMatrixes.push_back(jointVertexPull);
        }

        //Skin all children
        for (SJoint *childjoint : joint->Children)
          skinJoint(childjoint, joint, strength);
      }

      void computeWeightInfluence(const SJoint *joint, size_t &index)
      {
        if (joint->Weights.size())
        {
          for (const SWeight &weight : joint->Weights)
            Weights[weight.buffer_id][weight.vertex_id].push_back(std::make_pair(index, weight.strength));
          index++;
        }

        for (const SJoint* childjoint : joint->Children)
          computeWeightInfluence(childjoint, index);
      }

      bool hasParent(const SJoint *jointA) const
      {
        for (SJoint *jointB : AllJoints)
          for (SJoint *jointBChild : jointB->Children)
            if (jointBChild == jointA)
              return true;
        return false;
      }

    public:
      std::vector<std::vector<WeightInfluence>> WeightBuffers;
      std::vector<core::matrix4> JointMatrixes;

      //! Gets the frame count of the animated mesh.
      /** \param fps Frames per second to play the animation with. If the amount is 0, it is not animated.
      The actual speed is set in the scene node the mesh is instantiated in.*/
      void setAnimationSpeed(float fps)
      {
        FramesPerSecond = fps;
      }

      //! Animates this mesh's joints based on frame input
      //! blend: {0-old position, 1-New position}
      void animateMesh(float frame, float blend)
      {
        if (!HasAnimation || LastAnimatedFrame == frame)
          return;

        LastAnimatedFrame = frame;
        SkinnedLastFrame = false;

        if (blend <= 0.f)
          return; //No need to animate

        for (SJoint *joint : AllJoints)
        {
          //The joints can be animated here with no input from their
          //parents, but for setAnimationMode extra checks are needed
          //to their parents

          const core::vector3df oldPosition = joint->Animatedposition;
          const core::vector3df oldScale = joint->Animatedscale;
          const core::quaternion oldRotation = joint->Animatedrotation;

          core::vector3df position = oldPosition;
          core::vector3df scale = oldScale;
          core::quaternion rotation = oldRotation;

          getFrameData(frame, joint,
            position, joint->positionHint,
            scale, joint->scaleHint,
            rotation, joint->rotationHint);

          if (blend == 1.0f)
          {
            //No blending needed
            joint->Animatedposition = position;
            joint->Animatedscale = scale;
            joint->Animatedrotation = rotation;
          }
          else
          {
            //Blend animation
            joint->Animatedposition = (1.f - blend) * oldPosition + blend * position;
            joint->Animatedscale = (1.f - blend) * oldScale + blend * scale;
            joint->Animatedrotation.slerp(oldRotation, rotation, blend);
          }
        }

        //Note:
        //LocalAnimatedMatrix needs to be built at some point, but this function may be called lots of times for
        //one render (to play two animations at the same time) LocalAnimatedMatrix only needs to be built once.
        //a call to buildAllLocalAnimatedMatrices is needed before skinning the mesh, and before the user gets the joints to move

        //----------------
        // Temp!
        buildAllLocalAnimatedMatrices();
      }

      //! Preforms a software skin on this mesh based of joint positions
      void skinMesh(float strength = 1.f)
      {
        if (!HasAnimation || SkinnedLastFrame)
          return;

        //----------------
        // This is marked as "Temp!".  A shiny dubloon to whomever can tell me why.
        buildAllGlobalAnimatedMatrices();
        //-----------------

        SkinnedLastFrame = true;
        JointMatrixes.clear();

          //rigid animation
          for (unsigned i = 0; i<AllJoints.size(); ++i)
          {
            for (unsigned j = 0; j<AllJoints[i]->AttachedMeshes.size(); ++j)
            {
//              SSkinMeshBuffer* Buffer = (*SkinningBuffers)[AllJoints[i]->AttachedMeshes[j]];
//              Buffer->Transformation = AllJoints[i]->GlobalAnimatedMatrix;
            }
          }

          //skin starting with the root joints
          for (SJoint *rootjoint : RootJoints)
            skinJoint(rootjoint, 0, strength);
        }

      //! converts the vertex type of all meshbuffers to tangents.
      /** E.g. used for bump mapping. */
//      virtual void convertMeshToTangents() = 0;

    public:
      ISkinnedMesh() : SkinningBuffers(0), AnimationFrames(0.f), FramesPerSecond(25.f),
        LastAnimatedFrame(-1), SkinnedLastFrame(false),
        HasAnimation(false), PreparedForSkinning(false),
        AnimateNormals(true), HardwareSkinning(false)
      {
#ifdef _DEBUG
        //setDebugName("CSkinnedMesh");
#endif

        SkinningBuffers = &LocalBuffers;
      }

      ~ISkinnedMesh()
      {
        for (SJoint *j : AllJoints)
          delete j;
      }

      //Interface for the mesh loaders (finalize should lock these functions, and they should have some prefix like loader_

      //these functions will use the needed arrays, set values, etc to help the loaders

      //! exposed for loaders: to add mesh buffers
      std::vector<std::pair<SMeshBufferLightMap, video::SMaterial> >& getMeshBuffers()
      {
        return LocalBuffers;
      }

      const std::vector<std::pair<SMeshBufferLightMap, video::SMaterial> >& getMeshBuffers() const
      {
        return LocalBuffers;
      }

      //! loaders should call this after populating the mesh
      void finalize()
      {
        // Make sure we recalc the next frame
        LastAnimatedFrame = -1;
        SkinnedLastFrame = false;

        if (!AllJoints.empty() || !RootJoints.empty())
        {
          // populate AllJoints or RootJoints, depending on which is empty
          if (RootJoints.empty())
          {
            for (SJoint *jointA : AllJoints)
              if (!hasParent(jointA))
                RootJoints.push_back(jointA);
          }
          else
            AllJoints = RootJoints;
        }

        for (SJoint *joint : AllJoints)
          joint->UseAnimationFrom = joint;

        //Todo: optimise keys here...
        checkForAnimation();

        if (HasAnimation)
        {
          //--- optimize and check keyframes ---
          for (SJoint *joint : AllJoints)
          {
            std::vector<SPositionKey> &PositionKeys = joint->PositionKeys;
            std::vector<SScaleKey> &ScaleKeys = joint->ScaleKeys;
            std::vector<SRotationKey> &RotationKeys = joint->RotationKeys;

            if (PositionKeys.size()>2)
            {
              for (unsigned j = 0; j<PositionKeys.size() - 2; ++j)
              {
                if (PositionKeys[j].position == PositionKeys[j + 1].position && PositionKeys[j + 1].position == PositionKeys[j + 2].position)
                {
                  //PositionKeys.erase(j + 1); //the middle key is unneeded
                  --j;
                }
              }
            }

            if (PositionKeys.size()>1)
            {
              for (unsigned j = 0; j<PositionKeys.size() - 1; ++j)
              {
                if (PositionKeys[j].frame >= PositionKeys[j + 1].frame) //bad frame, unneed and may cause problems
                {
                  //PositionKeys.erase(j + 1);
                  --j;
                }
              }
            }

            if (ScaleKeys.size()>2)
            {
              for (unsigned j = 0; j<ScaleKeys.size() - 2; ++j)
              {
                if (ScaleKeys[j].scale == ScaleKeys[j + 1].scale && ScaleKeys[j + 1].scale == ScaleKeys[j + 2].scale)
                {
                  //ScaleKeys.erase(j + 1); //the middle key is unneeded
                  --j;
                }
              }
            }

            if (ScaleKeys.size()>1)
            {
              for (unsigned j = 0; j<ScaleKeys.size() - 1; ++j)
              {
                if (ScaleKeys[j].frame >= ScaleKeys[j + 1].frame) //bad frame, unneed and may cause problems
                {
                  //ScaleKeys.erase(j + 1);
                  --j;
                }
              }
            }

            if (RotationKeys.size()>2)
            {
              for (unsigned j = 0; j < RotationKeys.size() - 2; ++j)
              {
                if (RotationKeys[j].rotation == RotationKeys[j + 1].rotation && RotationKeys[j + 1].rotation == RotationKeys[j + 2].rotation)
                {
                  //RotationKeys.erase(j + 1); //the middle key is unneeded
                  --j;
                }
              }
            }

            if (RotationKeys.size()>1)
            {
              for (unsigned j = 0; j<RotationKeys.size() - 1; ++j)
              {
                if (RotationKeys[j].frame >= RotationKeys[j + 1].frame) //bad frame, unneed and may cause problems
                {
                  //RotationKeys.erase(j + 1);
                  --j;
                }
              }
            }

            //Fill empty keyframe areas
            if (!PositionKeys.empty())
            {
              SPositionKey *Key;
              Key = &PositionKeys[0];//getFirst
              if (Key->frame != 0)
              {
                //PositionKeys.push_front(*Key);
                Key = &PositionKeys[0];//getFirst
                Key->frame = 0;
              }

              Key = &PositionKeys.back();
              if (Key->frame != AnimationFrames)
              {
                PositionKeys.push_back(*Key);
                Key = &PositionKeys.back();
                Key->frame = AnimationFrames;
              }
            }

            if (!ScaleKeys.empty())
            {
              SScaleKey *Key;
              Key = &ScaleKeys[0];//getFirst
              if (Key->frame != 0)
              {
                //ScaleKeys.push_front(*Key);
                Key = &ScaleKeys[0];//getFirst
                Key->frame = 0;
              }

              Key = &ScaleKeys.back();
              if (Key->frame != AnimationFrames)
              {
                ScaleKeys.push_back(*Key);
                Key = &ScaleKeys.back();
                Key->frame = AnimationFrames;
              }
            }

            if (!RotationKeys.empty())
            {
              SRotationKey *Key;
              Key = &RotationKeys[0];//getFirst
              if (Key->frame != 0)
              {
                //RotationKeys.push_front(*Key);
                Key = &RotationKeys[0];//getFirst
                Key->frame = 0;
              }

              Key = &RotationKeys.back();
              if (Key->frame != AnimationFrames)
              {
                RotationKeys.push_back(*Key);
                Key = &RotationKeys.back();
                Key->frame = AnimationFrames;
              }
            }
          }
        }

        //Needed for animation and skinning...
        calculateGlobalMatrices(0, 0);

        //rigid animation for non animated meshes
        for (unsigned i = 0; i<AllJoints.size(); ++i)
        {
          for (unsigned j = 0; j<AllJoints[i]->AttachedMeshes.size(); ++j)
          {
//            SSkinMeshBuffer* Buffer = (*SkinningBuffers)[AllJoints[i]->AttachedMeshes[j]];
//            Buffer->Transformation = AllJoints[i]->GlobalAnimatedMatrix;
          }
        }

        // Organise Weight by buffer
        for (const std::pair<irr::scene::SMeshBufferLightMap, video::SMaterial> &LocalBuffer : LocalBuffers)
        {

          Weights.push_back(std::vector<std::vector<std::pair<size_t, float> > >());
          Weights.back().resize(LocalBuffer.first.getVertexCount());
        }

        size_t idx = 0;
        for (const SJoint *rootjoint : RootJoints)
          computeWeightInfluence(rootjoint, idx);

        // Only keep the 4 most significant weight
        for (std::vector<std::vector<std::pair<size_t, float> > > &PerBufferWeights : Weights)
        {
          std::vector<WeightInfluence> BufferWeight;
          for (std::vector<std::pair<size_t, float> > &PerVertexWeights : PerBufferWeights)
          {
            std::sort(PerVertexWeights.begin(), PerVertexWeights.end(), [](const std::pair<size_t, float> &a, const std::pair<size_t, float> &b) {return a.second > b.second; });
            float current_weight = 0.;
            WeightInfluence WeightForVertex[4] = {};
            unsigned k;
            size_t bound = PerVertexWeights.size() > 4 ? 4 : PerVertexWeights.size();
            for (k = 0; k < bound; k++)
            {
              WeightForVertex[k].Index = (int)PerVertexWeights[k].first;
              WeightForVertex[k].Weight = PerVertexWeights[k].second;
              current_weight += PerVertexWeights[k].second;
            }
            for (; k < 4; k++)
              WeightForVertex[k].Index = -1;
            for (unsigned i = 0; i < 4; i++)
            {
              WeightForVertex[i].Weight /= current_weight;
              BufferWeight.push_back(WeightForVertex[i]);
            }
          }
          WeightBuffers.push_back(BufferWeight);
        }
      }

      //! Adds a new meshbuffer to the mesh, access it as last one
      std::pair<SMeshBufferLightMap, video::SMaterial>& addMeshBuffer()
      {
        LocalBuffers.push_back(std::make_pair(scene::SMeshBufferLightMap(), video::SMaterial()));
        return LocalBuffers.back();
      }

      //! Adds a new joint to the mesh, access it as last one
      SJoint* addJoint(SJoint *parent = 0)
      {
        AllJoints.push_back(new SJoint);
        if (parent)
          parent->Children.push_back(AllJoints.back());
        return AllJoints.back();
      }

      //! Adds a new weight to the mesh, access it as last one
      SWeight* addWeight(SJoint *joint)
      {
        joint->Weights.push_back(SWeight());
        return &joint->Weights.back();
      }

      //! Adds a new position key to the mesh, access it as last one
      SPositionKey* addPositionKey(SJoint *joint)
      {
        joint->PositionKeys.push_back(SPositionKey());
        return &joint->PositionKeys.back();
      }
      //! Adds a new scale key to the mesh, access it as last one
      SScaleKey* addScaleKey(SJoint *joint)
      {
        joint->ScaleKeys.push_back(SScaleKey());
        return &joint->ScaleKeys.back();
      }
      //! Adds a new rotation key to the mesh, access it as last one
      SRotationKey* addRotationKey(SJoint *joint)
      {
        joint->RotationKeys.push_back(SRotationKey());
        return &joint->RotationKeys.back();
      }

      //! Check if the mesh is non-animated
//      virtual bool isStatic() = 0;
    };

  } // end namespace scene
} // end namespace irr

#endif

