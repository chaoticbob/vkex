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
};

struct VSOutput {
  float4  PositionCS : SV_Position;
  float2  TexCoord   : TexCoord;
};

ConstantBuffer<ViewConstantsData> ViewConstants : register(VKEX_SHADER_CONSTANTS_BASE_REGISTER);
Texture2D                         Tex0          : register(VKEX_SHADER_TEXTURE_BASE_REGISTER);
SamplerState                      Sampler0      : register(VKEX_SHADER_SAMPLER_BASE_REGISTER);

VSOutput vsmain(VSInput input)
{
  VSOutput output = (VSOutput)0;
  output.PositionCS = mul(ViewConstants.MVP, float4(input.PositionOS, 1));
  output.TexCoord = input.TexCoord;
  return output;

}

struct PSInput {
  float4  PositionCS : SV_Position;
  float2  TexCoord   : TexCoord;
};


float4 psmain(PSInput input) : SV_Target
{
  float4 color = Tex0.Sample(Sampler0, input.TexCoord);
  return color;
}