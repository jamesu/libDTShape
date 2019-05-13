//
//  metalShaders.metal
//  DTSTest
//
//  Created by James Urquhart on 30/04/2019.
//  Copyright © 2019 James Urquhart. All rights reserved.
//

#include <metal_stdlib>
using namespace metal;
#include <simd/simd.h>
#include "mtl_shared.h"

typedef struct
{
   float4 gl_Position [[position]];
   float4 vColor0;
   float2 vTexCoord0;
} TSFragVert;

struct TSVertex
{
   float3 aPosition  [[attribute(0)]];
   float2 aTexCoord0 [[attribute(1)]];
   float3 aNormal    [[attribute(2)]];
   float4 aColor     [[attribute(3)]];
};

struct TSVertexSkinned
{
   float3 aPosition  [[attribute(0)]];
   float2 aTexCoord0 [[attribute(1)]];
   float3 aNormal    [[attribute(2)]];
   float4 aColor     [[attribute(3)]];
   uchar4 aBlendIndices0 [[attribute(4)]];
   float4 aBlendWeights0 [[attribute(5)]];
};

struct TSVertexDualSkinned
{
   float3 aPosition  [[attribute(0)]];
   float2 aTexCoord0 [[attribute(1)]];
   float3 aNormal    [[attribute(2)]];
   float4 aColor     [[attribute(3)]];
   uchar4 aBlendIndices0 [[attribute(4)]];
   uchar4 aBlendIndices1 [[attribute(5)]];
   float4 aBlendWeights0 [[attribute(6)]];
   float4 aBlendWeights1 [[attribute(7)]];
};

typedef struct
{
   uchar4 indices;
   float4 weights;
} TSBlendVertRecord;



matrix_float3x3 rotationOnly(matrix_float4x4 in)
{
   return matrix_float3x3(in[0].xyz, in[1].xyz, in[2].xyz);
}
vertex TSFragVert standard_vert(TSVertex in [[stage_in]],
                                     constant PushConstants &push_constants [[ buffer(TS_PUSH_VBO) ]])
{
   TSFragVert vert;
   
   float3 normal, lightDir;
   float4 diffuse;
   float NdotL;
   
   matrix_float3x3 rotM = rotationOnly(push_constants.worldMatrix);
   normal = normalize(rotM * in.aNormal);
   lightDir = normalize(float3(push_constants.lightPos));
   
   NdotL = max(dot(normal, lightDir), 0.0);
   diffuse = float4(push_constants.lightColor, 1.0);
   
   vert.gl_Position = push_constants.worldMatrixProjection * float4(in.aPosition, 1);
   vert.vTexCoord0 = in.aTexCoord0;
   vert.vColor0 = NdotL * diffuse;
   vert.vColor0.a = 1.0;
   
   return vert;
}

vertex TSFragVert skinned_vert(TSVertexSkinned in [[stage_in]],
                                   constant PushConstants &push_constants [[ buffer(TS_PUSH_VBO) ]],
                                   constant matrix_float4x4* boneTransforms [[buffer(TS_BONE_VBO)]])
{
   TSFragVert vert;
   
   float3 normal, lightDir;
   float4 diffuse;
   float NdotL;
   
   // Start skinning
   float3 inPosition;
   float3 inNormal;
   float3 posePos = float3(0.0);
   float3 poseNormal = float3(0.0);
   for (int i=0; i<4; i++) {
      matrix_float4x4 mat = boneTransforms[in.aBlendIndices0[i]];
      matrix_float3x3 m33 = rotationOnly(mat);
      posePos += (mat * float4(in.aPosition, 1)).xyz * in.aBlendWeights0[i];
      poseNormal += (m33 * in.aNormal) * in.aBlendWeights0[i];
   }
   inPosition = posePos;
   inNormal = normalize(poseNormal);
   // End skinning
   
   matrix_float3x3 rotM = rotationOnly(push_constants.worldMatrix);
   normal = normalize(rotM * inNormal);
   lightDir = normalize(float3(push_constants.lightPos));
   
   NdotL = max(dot(normal, lightDir), 0.0);
   diffuse = float4(push_constants.lightColor, 1.0);
   
   vert.gl_Position = push_constants.worldMatrixProjection * float4(inPosition, 1);
   vert.vTexCoord0 = in.aTexCoord0;
   vert.vColor0 = NdotL * diffuse;
   vert.vColor0.a = 1.0;
   
   return vert;
}

vertex TSFragVert dual_skinned_vert(TSVertexDualSkinned in [[stage_in]],
                               constant PushConstants &push_constants [[ TS_PUSH_VBO ]],
                               constant matrix_float4x4* boneTransforms [[ TS_BONE_VBO ]])
{
   TSFragVert vert;
   
   float3 normal, lightDir;
   float4 diffuse;
   float NdotL;
   
   // Start skinning
   float3 inPosition;
   float3 inNormal;
   float3 posePos = float3(0.0);
   float3 poseNormal = float3(0.0);
   for (int i=0; i<4; i++) {
      matrix_float4x4 mat = boneTransforms[int(in.aBlendIndices0[i])];
      matrix_float3x3 m33 = rotationOnly(mat);
      posePos += (mat * float4(in.aPosition, 1)).xyz * in.aBlendWeights0[i];
      poseNormal += (m33 * in.aNormal) * in.aBlendWeights0[i];
   }
   for (int i=0; i<4; i++) {
      matrix_float4x4 mat = boneTransforms[int(in.aBlendIndices1[i])];
      matrix_float3x3 m33 = rotationOnly(mat);
      posePos += (mat * float4(in.aPosition, 1)).xyz * in.aBlendWeights1[i];
      poseNormal += (m33 * in.aNormal) * in.aBlendWeights1[i];
   }
   inPosition = posePos;
   inNormal = normalize(poseNormal);
   // End skinning
   
   normal = normalize(rotationOnly(push_constants.worldMatrix) * inNormal);
   lightDir = normalize(float3(push_constants.lightPos));
   NdotL = max(dot(normal, lightDir), 0.0);
   diffuse = float4(push_constants.lightColor, 1.0);
   
   vert.gl_Position = push_constants.worldMatrixProjection * float4(inPosition, 1);
   vert.vTexCoord0 = in.aTexCoord0;
   vert.vColor0 = NdotL * diffuse;
   vert.vColor0.a = 1.0;
   
   return vert;
}

fragment float4 standard_fragment(TSFragVert in [[stage_in]],
                               constant PushConstants &push_constants [[ TS_PUSH_VBO ]],
                               texture2d<half, access::sample> tex [[ texture(0) ]],
                               sampler texSampler0 [[sampler(0)]]
                               )
{
   float4 gl_FragColor = (float4)tex.sample(texSampler0, in.vTexCoord0);
   gl_FragColor.r = gl_FragColor.r * in.vColor0.r * in.vColor0.a;
   gl_FragColor.g = gl_FragColor.g * in.vColor0.g * in.vColor0.a;
   gl_FragColor.b = gl_FragColor.b * in.vColor0.b * in.vColor0.a;
   return gl_FragColor;
}

struct SkinStruct1
{
   uchar4 aBlendIndices0;
   float4 aBlendWeights0;
};

// This does the same thing as the vert stage shader
// TODO: figure out why this fails miserably on nvidia
kernel void skin_verts_cs(constant SkinParams& params [[buffer(0)]],
                          const device char* modelVerts [[buffer(1)]], // verts should ideally be aligned to 4
                          device char* outVerts [[buffer(2)]],  // verts should ideally be aligned to 4
                          const device matrix_float4x4* boneTransforms [[buffer(3)]],
                             uint2 gid         [[thread_position_in_grid]])
{
   uint vertIdx = gid.x;
   float3 inPos = *((const device float3*)(&modelVerts[vertIdx * params.inStride]));
   float3 inNorm = *((const device float3*)(&modelVerts[(vertIdx * params.inStride) + (4*4)]));
   uchar4 skinIndex = *((const device uchar4 *)(&modelVerts[params.inBoneOffset + (vertIdx * params.inStride)]));
   float4 skinWeight = *((const device float4 *)(&modelVerts[params.inBoneOffset + 4 + (vertIdx * params.inStride)]));
   
   float3 posePos = float3(0.0);
   float3 poseNormal = float3(0.0);
   for (int i=0; i<4; i++) {
      float weight = (skinWeight[i]);
      uint index = skinIndex[i];
      matrix_float4x4 mat = boneTransforms[index];
      matrix_float3x3 m33 = rotationOnly(mat);
      posePos += (mat * float4(inPos, 1)).xyz * weight;
      poseNormal += (m33 * inNorm) * weight;
   }
   
   //posePos = inPos;
   //poseNormal = inNorm;
   
   device float* outVert = (device float*)&outVerts[vertIdx * params.outStride];
   device float* outNormal = &outVert[4];
   
   {
      *outVert++ = (posePos.x);
      *outVert++ = (posePos.y);
      *outVert++ = (posePos.z);
      *outNormal++ = poseNormal.x;
      *outNormal++ = poseNormal.y;
      *outNormal++ = poseNormal.z;
   }
}

// TODO: skinning shader which uses better packed source data

