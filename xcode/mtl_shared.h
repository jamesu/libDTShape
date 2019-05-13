//
//  metal_shared.h
//  DTSTest
//
//  Created by James Urquhart on 02/05/2019.
//  Copyright © 2019 James Urquhart. All rights reserved.
//

#ifndef metal_shared_h
#define metal_shared_h

#include <simd/simd.h>

enum VertexBufferIds
{
   TS_PUSH_VBO=0,
   TS_VERT_VBO=1,
   TS_BONE_VBO=2
};

enum SamplerIds
{
   TS_DIFFUSE_SAMPLER=0
};

typedef struct
{
   matrix_float4x4 worldMatrixProjection;
   matrix_float4x4 worldMatrix;
   vector_float3 lightPos;
   vector_float3 lightColor;
} PushConstants;



typedef struct
{
   int inBoneOffset;
   int inStride;
   int outStride;
} SkinParams;

typedef struct
{
   vector_uchar4 aBlendIndices0;
   vector_float4 aBlendWeights0;
} SS2;

#endif /* metal_shared_h */
