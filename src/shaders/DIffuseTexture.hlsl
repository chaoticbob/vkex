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
  float3  PositionOS : PositionOS;
  float2  TexCoord   : TexCoord;
  float3  NormalOS   : NormalOS;
};

struct VSOutput {
  float4  PositionCS : SV_Position;
  float3  PositionWS : PositionWS;
  float2  TexCoord   : TexCoord;
  float3  NormalWS   : NormalWS;
};

ConstantBuffer<ViewConstantsData> ViewConstants : register(VKEX_SHADER_CONSTANTS_BASE_REGISTER);
Texture2D                         Tex0          : register(VKEX_SHADER_TEXTURE_BASE_REGISTER);
SamplerState                      Sampler0      : register(VKEX_SHADER_SAMPLER_BASE_REGISTER);

VSOutput vsmain(VSInput input)
{
  VSOutput output = (VSOutput)0;
  output.PositionCS = mul(ViewConstants.MVP, float4(input.PositionOS, 1));
  output.PositionWS = output.PositionCS.xyz;
  output.TexCoord = input.TexCoord;
  output.NormalWS = normalize(mul(ViewConstants.N, input.NormalOS));
  return output;

}

struct PSInput {
  float4  PositionCS : SV_Position;
  float3  PositionWS : PositionWS;
  float2  TexCoord   : TexCoord;
  float3  NormalWS   : NormalWS;
};

float4 psmain(PSInput input) : SV_Target
{
  float3 N = input.NormalWS;
  float3 L = normalize(ViewConstants.LP - input.PositionWS);
  float  D = max(0, dot(L, input.NormalWS));
  float3 color = Tex0.Sample(Sampler0, input.TexCoord).xyz;
  float4 final_color = float4(color * D, 1.0);
  return final_color;
}