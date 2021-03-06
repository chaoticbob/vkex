/*
 Copyright 2018 Google Inc.
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#include "Common.h"

struct VSInput {
  float3  PositionOS  : PositionOS;
  float3  NormalOS    : NormalOS;
  float3  VertexColor : VertexColor;
};

struct VSOutput {
  float4  PositionCS  : SV_Position;
  float3  PositionWS  : PositionWS;
  float3  NormalWS    : NormalWS;
  float3  VertexColor : VertexColor;
};

ConstantBuffer<ViewConstantsData> ViewConstants : register(VKEX_SHADER_CONSTANTS_BASE_REGISTER);

VSOutput vsmain(VSInput input)
{ 
  VSOutput output = (VSOutput)0;
  output.PositionCS = mul(ViewConstants.MVP, float4(input.PositionOS, 1));
  output.VertexColor = input.VertexColor;
  output.NormalWS = mul(ViewConstants.N, input.NormalOS);
  return output;
}

struct PSInput {
  float4  PositionCS  : SV_Position;
  float3  PositionWS  : PositionWS;
  float3  NormalWS    : NormalWS;
  float3  VertexColor : VertexColor;
};

float4 psmain(PSInput input) : SV_Target
{
  float3 N = input.NormalWS;
  float3 L = normalize(ViewConstants.LP - input.PositionWS);
  float  D = max(0, dot(L, input.NormalWS));
  float3 color = input.VertexColor;
  float4 final_color = float4(color * D, 1.0);
  return final_color;
}